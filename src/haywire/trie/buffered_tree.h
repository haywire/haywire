#ifndef WHEAT_BUFFERED_TREE
#define WHEAT_BUFFERED_TREE

#include <stdint.h>

// Normally, L2 Cache line size is 128/256 bytes. 16 containers can
// occupy 128 bytes
#define BFTREE_DEFAULT_CONTAINER       16
#define BFTREE_CONTAINER_THRESHOLD     16
#define BFTREE_PAYLOAD_THRESHOLD       100

#define BF_OK                          0
#define BF_WRONG                       (-1)

struct bftree;
typedef int (*key_compare_func)(const void *key1, const void *key2);

enum payload_type {
    Put,
    Del,
};

struct payload {
    uint8_t *key;
    uint8_t *val;
    struct payload *next;
    enum payload_type type;
};

struct bftree_opts {
    void *(*key_dup)(const void *key);
    void *(*val_dup)(const void *obj);
    key_compare_func key_compare;
    void (*key_destructor)(void *key);
    void (*val_destructor)(void *obj);
};


struct bftree *bftree_create(struct bftree_opts *opts);
void bftree_free(struct bftree *tree);
int bftree_put(struct bftree *tree, void *key, void *val);
void *bftree_get(struct bftree *tree, void *key);
int bftree_del(struct bftree *tree, void *key);
// Ineffective count implementation!!!! Don't call it if big tree
int bftree_count(struct bftree *tree);

// ================== Iterator Area ========================
// Iterator isn't effective as insert or delete operator, it may
// slower than normal dictionary implementation.
// If modify payload when iterate buffer tree, you should call
// bftree_iter_set_val and bftree_iter_set_del to avoid unexpected
// things
struct bftree_iterator *bftree_get_iterator(struct bftree *tree);
struct payload *bftree_next(struct bftree_iterator *iter);
void bftree_free_iterator(struct bftree_iterator *iter);

#define payload_getkey(payload) ((payload)->key);
#define payload_getval(payload) ((payload)->val)

static __inline void bftree_iter_set_del(struct bftree_iterator *iter,
                                       struct payload *payload)
{
    payload->type = Del;
}

static __inline void bftree_iter_set_val(struct bftree_iterator *iter,
                                       struct payload *payload,
                                       void *val)
{
    payload->val = val;
}

#endif