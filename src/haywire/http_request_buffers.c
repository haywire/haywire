#include <stdbool.h>
#include <errno.h>
#include "haywire.h"
#include "http_request_buffers.h"

#define DEFAULT_BUFFER_SHRINKSIZE 65536

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

KHASH_MAP_INIT_INT64(pointer_hashmap, void*)

typedef struct {
    size_t max_size;
    size_t size;
    size_t mark;
    size_t used;
    size_t used_before;
    void* current;
    khash_t(pointer_hashmap)* offsets;
    bool offsets_active;
} http_request_buffer;

void http_request_buffer_consume(hw_request_buffer* buf, size_t consumed) {
    http_request_buffer* buffer = (http_request_buffer*) buf;
    buffer->used_before = buffer->used;
    buffer->used += consumed;
}

void http_request_buffer_mark(hw_request_buffer* buf) {
    http_request_buffer* buffer = (http_request_buffer*) buf;
    /* unfortunately, the parser doesn't tell us where the request ends exactly,
     * so the only thing we can be sure is that it ends in the current buffer chunk, so anything before it can
     * effectively be swept, so we're placing the mark at that point now. */
    buffer->mark = buffer->used_before;
}

void http_request_buffer_sweep(hw_request_buffer* buf) {
    http_request_buffer* buffer = (http_request_buffer*) buf;
    void* pointer;
    int offset;
    int used = buffer->used - buffer->mark;

    if (buffer->mark > 0) {
        bool offsets_active = false;

        if (buffer->used > 0) {
            /* Move data beyond the mark to the beginning of the buffer.
             * While we should avoid memory copies, this is relatively infrequent and will only really copy a
             * significant amount of data if requests are pipelined. Otherwise, we'll just be copying the last chunk
             * that was read back to the beginning of the buffer. */
            memcpy(buffer->current, (unsigned char *)buffer->current + buffer->mark, used);
        }

        if (buffer->size > DEFAULT_BUFFER_SHRINKSIZE && buffer->used < DEFAULT_BUFFER_SHRINKSIZE) {
            /* Shrink buffer */
            buffer->size = DEFAULT_BUFFER_SHRINKSIZE;
            buffer->current = realloc(buffer->current, buffer->size);
            if (!buffer->current) {
                errno = ENOMEM;
                buffer->size = 0;
            }
        }

        /* Update offsets */
        if (buffer->used) {
            kh_foreach(buffer->offsets, pointer, offset, {
                khiter_t offset_key = kh_get(pointer_hashmap, buffer->offsets, pointer);

                if (offset <= buffer->mark) {
                    /* Delete offsets that pointed to bytes before the mark */
                    kh_del(pointer_hashmap, buffer->offsets, offset_key);
                } else {
                    /* There's at least one offset beyond the mark, so the offsets are active and should be considered
                     * when locating pointers. We need to shift the offset back by the width of the mark */
                    offsets_active = true;
                    kh_value(buffer->offsets, offset_key) = offset - buffer->mark;
                }
            });
        } else {
            /* the buffer is now empty, so we don't need to keep offsets */
            kh_clear(pointer_hashmap, buffer->offsets);
        }

        buffer->mark = 0;
        buffer->used = used;
        buffer->offsets_active = offsets_active;
    }
}

hw_request_buffer* http_request_buffer_init(size_t max_size) {
    http_request_buffer* buffer = malloc(sizeof(http_request_buffer));
    buffer->max_size = max_size;
    buffer->size = 0;
    buffer->used = 0;
    buffer->mark = 0;
    buffer->used_before = 0;
    buffer->current = NULL;
    buffer->offsets = kh_init(pointer_hashmap);
    buffer->offsets_active = false;
    return buffer;
}

void http_request_buffer_chunk(hw_request_buffer* buf, hw_request_buffer_chunk* chunk) {
    http_request_buffer *buffer = (http_request_buffer *) buf;
    chunk->size = buffer->size ? buffer->size - buffer->used : 0;
    chunk->buffer = (unsigned char *)buffer->current + buffer->used;
}

bool http_request_buffer_alloc(hw_request_buffer* buf, size_t requested_size) {
    http_request_buffer* buffer = (http_request_buffer*) buf;
    bool ret = true;
    void* previous = NULL;

    size_t requested_size_capped = MIN(buffer->max_size, requested_size);

    if (!buffer->current) {
        buffer->current = malloc(requested_size_capped);
        if (!buffer->current) {
            buffer->size = 0;
            errno = ENOMEM;
            ret = false;
        } else {
            buffer->size = requested_size_capped;
        }
    } else if (buffer->used * 2 < buffer->size) {
        /* ignoring allocation size unless we're above 50% usage */
    } else if (buffer->size + requested_size_capped <= buffer->max_size) {
        /* time to reallocate memory and re-point anything using the buffer */
        previous = buffer->current;

        buffer->current = realloc(buffer->current, buffer->size + requested_size_capped);
        buffer->size += requested_size_capped;

        if (!buffer->current) {
            buffer->size = 0;
            errno = ENOMEM;
            ret = false;
        } else if (buffer->current != previous) {
            buffer->offsets_active = true;
        }
    } else {
        /* maximum request size exceeded */
        errno = ERANGE;
        buffer->size = 0;
        ret = false;
    }

    return ret;
}

void http_request_buffer_print(hw_request_buffer* buf) {
    http_request_buffer* buffer = (http_request_buffer*) buf;

    printf("Buffer: current=%u; size=%u; used=%u\n", buffer->current, buffer->size, buffer->used);
    printf("    0\t");
    for (int i = 0; i < buffer->used; i++) {
        if (((char*) buffer->current)[i] == '\n') {
            printf("\\n");
        } else if (((char*) buffer->current)[i] == '\r') {
            printf("\\r");
        } else {
            printf("%c", ((char*) buffer->current)[i]);
        }

        if ((i + 1) % 10 == 0) {
            printf("\n%5d\t", (i + 1) / 10);
        } else {
            printf("\t");
        }
    }
    printf("\n");

    void* pointer;
    int offset;
    kh_foreach(buffer->offsets, pointer, offset, {
        printf("\tPointer %u -> offset=%u\n", pointer, offset);
    });
    printf("----\n");
}

void http_request_buffer_pin(hw_request_buffer* buf, void* key, void* pointer) {
    http_request_buffer* buffer = (http_request_buffer*) buf;

    khiter_t offset_key = kh_get(pointer_hashmap, buffer->offsets, key);

    int offset = (unsigned char *)pointer - buffer->current;
    int ret;

    int is_missing = (offset_key == kh_end(buffer->offsets));
    if (is_missing) {
        offset_key = kh_put(pointer_hashmap, buffer->offsets, key, &ret);
    } 

    kh_value(buffer->offsets, offset_key) = offset;
}

void http_request_buffer_reassign_pin(hw_request_buffer* buf, void* old_key, void* new_key) {
    http_request_buffer* buffer = (http_request_buffer*) buf;

    khiter_t old_offset_key = kh_get(pointer_hashmap, buffer->offsets, old_key);

    int offset;
    int ret;

    int is_missing = (old_offset_key == kh_end(buffer->offsets));
    if (!is_missing) {
        offset = kh_val(buffer->offsets, old_offset_key);

        khiter_t new_offset_key = kh_put(pointer_hashmap, buffer->offsets, new_key, &ret);
        kh_value(buffer->offsets, new_offset_key) = offset;
        old_offset_key = kh_get(pointer_hashmap, buffer->offsets, old_key);
        kh_del(pointer_hashmap, buffer->offsets, old_offset_key);
    }
}

void* http_request_buffer_locate(hw_request_buffer* buf, void* key, void* default_pointer) {
    http_request_buffer* buffer = (http_request_buffer*) buf;
    void* location = default_pointer;
    khiter_t offset_key = kh_get(pointer_hashmap, buffer->offsets, key);

    int offset, is_missing;

    if (buffer->offsets_active) {
        is_missing = (offset_key == kh_end(buffer->offsets));
        if (!is_missing) {
            offset = kh_value(buffer->offsets, offset_key);
            location = (unsigned char *)buffer->current + offset;
        }
    }

    return location;
}

void http_request_buffer_destroy(hw_request_buffer* buf) {
    http_request_buffer* buffer = (http_request_buffer*) buf;
    kh_destroy(pointer_hashmap, buffer->offsets);
    free(buffer->current);
    free(buffer);
}
