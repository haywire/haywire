#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "buffered_tree.h"

struct container {
    struct payload *payload_first;
    uint32_t payload_size;
    struct node *child;
};

struct node {
    struct node *parent;
    struct container **containers;
    uint32_t container_count;
    uint32_t container_size;
};

struct bftree {
    struct node *root;
    struct bftree_opts *opts;
    uint32_t height;
    int is_migrated;
    uint32_t del_payload_count;
    uint32_t put_payload_count;
};

struct bftree_iterator {
    struct bftree *tree;
    struct payload *next;
    int closed;
};

static struct container *container_insert(struct bftree *tree, struct node *node,
                                          uint32_t container_idx, struct payload *new_payload);
void bftree_node_print(struct node *node);
static void validate_containers(struct node *node, key_compare_func compare);

struct payload *payload_create(void *key, void *val, enum payload_type type)
{
    struct payload *payload;
    
    payload = malloc(sizeof(*payload));
    payload->key = key;
    payload->val = val;
    payload->next = NULL;
    payload->type = type;
    
    return payload;
}

void payload_free(struct bftree *tree, struct payload *payload, int nofree)
{
    if (payload->key && tree->opts->key_destructor)
        tree->opts->key_destructor(payload->key);
    if (payload->val && tree->opts->val_destructor && !nofree)
        tree->opts->val_destructor(payload->val);
    if (payload->type == Put)
        tree->put_payload_count--;
    else
        tree->del_payload_count--;
    
    free(payload);
}

void payload_replace(struct bftree *tree, struct payload *older, struct payload *newer)
{
    void *temp;
    temp = older->val;
    older->val = newer->val;
    newer->val = temp;
    older->type = newer->type;
    payload_free(tree, newer, 0);
}

struct container *container_create()
{
    struct container *container;
    
    container = malloc(sizeof(*container));
    container->payload_first = NULL;
    container->payload_size = 0;
    container->child = NULL;
    
    return container;
}

void container_free(struct bftree *tree, struct container *container)
{
    struct payload *curr, *next;
    
    curr = container->payload_first;
    while (curr) {
        next = curr->next;
        payload_free(tree, curr, 0);
        curr = next;
    }
    
    free(container);
}

void insert_after_container(struct node *node, struct container *container,
                            uint32_t container_idx)
{
    if (node->container_size == node->container_count) {
        node->containers = realloc(node->containers, sizeof(void*)*node->container_count*2);
        node->container_count *= 2;
    }
    
    if (node->container_size == 0) {
        node->containers[node->container_size++] = container;
    } else {
        memmove(&node->containers[container_idx+2], &node->containers[container_idx+1],
                (node->container_size-container_idx-1)*sizeof(void*));
        node->containers[container_idx+1] = container;
        node->container_size++;
    }
}

struct node *node_create(struct node *parent_node)
{
    struct node *node;
    
    node = malloc(sizeof(*node));
    node->parent = parent_node;
    node->containers = malloc(sizeof(struct container*)*BFTREE_DEFAULT_CONTAINER);
    node->container_count = BFTREE_DEFAULT_CONTAINER;
    node->container_size = 0;
    
    return node;
}

void node_free(struct bftree *tree, struct node *node)
{
    int i;
    struct container *container;
    
    for (i = 0; i < node->container_size; i++) {
        container = node->containers[i];
        container_free(tree, container);
    }
    free(node->containers);
    free(node);
}

void bftree_free_node(struct bftree *tree, struct node *node)
{
    int i;
    struct container *container;
    
    for (i = 0; i < node->container_size; i++) {
        container = node->containers[i];
        if (container->child)
            bftree_free_node(tree, container->child);
    }
    node_free(tree, node);
}

static uint32_t find_container(key_compare_func compare, struct node *node,
                               void *key, uint32_t start_container)
{
    int left, right, middle, result, compared;
    struct container **containers;
    
    left = start_container;
    right = node->container_size - 1;
    containers = node->containers;
    compared = 0;
    while (left <= right) {
        middle = (left + right) / 2;
        compared = compare(key, containers[middle]->payload_first->key);
        if (compared < 0) {
            right = middle - 1;
        } else if (compared > 0) {
            left = middle + 1;
        } else {
            right = middle;
            break ;
        }
    }
    if (compared > 0)
        result = left - 1;
    else if (compared < 0)
        result = right;
    else
        result = right;
    
    if (result == 0)
        return 0;
    return right - 1;
}

static struct container *remove_container(struct node *node, uint32_t idx)
{
    struct container *removed;
    
    removed = node->containers[idx];
    memmove(&node->containers[idx], &node->containers[idx+1],
            (node->container_size-idx-1)*sizeof(void*));
    node->container_size--;
    return removed;
}

static struct payload *get_payload(key_compare_func compare, struct payload *payload_start,
                                   void *key, int *is_equal)
{
    struct payload *curr_payload, *prev_payload;
    int compared;
    
    prev_payload = NULL;
    curr_payload = payload_start;
    *is_equal = 0;
    while (curr_payload) {
        compared = compare(key, curr_payload->key);
        if (compared <= 0) {
            if (compared == 0) {
                *is_equal = 1;
                return curr_payload;
            }
            return prev_payload;
        }
        prev_payload = curr_payload;
        curr_payload = curr_payload->next;
    }
    
    return prev_payload;
}

static void push_to_child(struct bftree *tree, struct node *node,
                          struct container *container)
{
    struct payload *curr_payload, *next_payload;
    uint32_t child_container, push_count;
    key_compare_func compare;
    
    compare = tree->opts->key_compare;
    curr_payload = container->payload_first->next;
    child_container = 0;
    
    push_count = container->payload_size / 2;
    container->payload_size -= push_count;
    while (push_count--) {
        next_payload = curr_payload->next;
        container->payload_first->next = next_payload;
        child_container = find_container(compare, container->child,
                                         curr_payload->key, child_container);
        container_insert(tree, container->child, child_container, curr_payload);
        curr_payload = next_payload;
    }
}

static void order_container_payload(struct bftree *tree, struct node *node,
                                    uint32_t migrated_idx, uint32_t import_idx)
{
    struct payload *separator, *curr;
    key_compare_func compare;
    int is_equal;
    struct container *left, *right;
    
    left = node->containers[migrated_idx];
    right = node->containers[import_idx];
    compare = tree->opts->key_compare;
    separator = get_payload(compare, left->payload_first,
                            right->payload_first->key, &is_equal);
    
    if (is_equal) {
        // TODO need optimize
        struct payload *prev;
        prev = left->payload_first;
        while (prev->next != separator) {
            prev = prev->next;
        }
        payload_replace(tree, right->payload_first, separator);
        separator = prev;
        left->payload_size--;
    }
    
    if (separator) {
        curr = separator->next;
        separator->next = NULL;
        tree->is_migrated = 1;
        while (curr) {
            left->payload_size--;
            container_insert(tree, node, import_idx, curr);
            curr = curr->next;
        }
        tree->is_migrated = 0;
    }
}

static void try_split_node(struct bftree *tree, struct node *node)
{
    uint32_t middle_container_idx, parent_container_idx;
    int i;
    struct node *new_node, *new_root;
    struct container *container, *new_node_first_container;
    
    if (node->container_size < BFTREE_CONTAINER_THRESHOLD) {
        // the number of container in this node is full
        return ;
    }
    
    middle_container_idx = node->container_size / 2;
    new_node = node_create(node->parent);
    new_node_first_container = node->containers[middle_container_idx];
    new_node_first_container->child = new_node;
    for (i = middle_container_idx+1; i < node->container_size; ++i) {
        container = node->containers[i];
        insert_after_container(new_node, container, i-middle_container_idx-2);
    }
    node->container_size -= (i-middle_container_idx);
    
    if (node == tree->root) {
        // produce new root
        new_root = node_create(NULL);
        tree->root = new_root;
        tree->height++;
        node->parent = new_root;
        new_node->parent = new_root;
        container = remove_container(node, 0);
        container->child = node;
        insert_after_container(new_root, container, 0);
        insert_after_container(new_root, new_node_first_container, 0);
    } else {
        parent_container_idx = find_container(tree->opts->key_compare,
                                              node->parent, new_node_first_container->payload_first->key, 0);
        insert_after_container(node->parent, new_node_first_container, parent_container_idx);
        order_container_payload(tree, node->parent, parent_container_idx, parent_container_idx+1);
        try_split_node(tree, node->parent);
    }
}

static void split_container(struct bftree *tree, struct node *node,
                            uint32_t container_idx)
{
    uint32_t half_count, i;
    struct container *new_container, *target;
    struct payload *payload;
    
    new_container = container_create();
    insert_after_container(node, new_container, container_idx);
    
    target = node->containers[container_idx];
    half_count = target->payload_size / 2;
    payload = target->payload_first;
    for (i = 0; i < half_count - 1; ++i)
        payload = payload->next;
    new_container->payload_first = payload->next;
    payload->next = NULL;
    new_container->payload_size = target->payload_size - half_count;
    target->payload_size = half_count;
    
    try_split_node(tree, node);
}

static struct container *container_insert(struct bftree *tree, struct node *node,
                                          uint32_t container_idx, struct payload *new_payload)
{
    struct payload *curr_payload;
    struct container *target;
    int is_equal;
    key_compare_func compare;
    
    compare = tree->opts->key_compare;
    if (container_idx >= node->container_size) {
        target = container_create();
        insert_after_container(node, target, 0);
    } else {
        target = node->containers[container_idx];
    }
    
    curr_payload = get_payload(tree->opts->key_compare, target->payload_first,
                               new_payload->key, &is_equal);
    
    if (is_equal) {
        // exist same key, swap value of payload
        payload_replace(tree, curr_payload, new_payload);
    } else {
        if (curr_payload) {
            // not at the header of payload list
            new_payload->next = curr_payload->next;
            curr_payload->next = new_payload;
        } else {
            // at the header of payload list
            new_payload->next = target->payload_first;
            target->payload_first = new_payload;
        }
        target->payload_size++;
    }
    
    if (target->payload_size > BFTREE_PAYLOAD_THRESHOLD && tree->is_migrated) {
        if (target->child)
            push_to_child(tree, node, target);
        else
            split_container(tree, node, container_idx);
    }
    
    return target;
}

static struct payload *container_get(struct bftree *tree, struct node *node,
                                     uint32_t container_idx, void *key)
{
    struct payload *curr_payload;
    struct container *container;
    int (*compare)(const void *, const void *);
    int is_equal;
    
    if (container_idx >= node->container_size)
        return NULL;
    
    compare = tree->opts->key_compare;
    container = node->containers[container_idx];
    curr_payload = get_payload(compare, container->payload_first, key, &is_equal);
    
    if (is_equal) {
        if (curr_payload->type == Put)
            return curr_payload;
        return NULL;
    }
    if (container->child) {
        container_idx = find_container(compare, container->child, key, 0);
        return container_get(tree, container->child, container_idx, key);
    }
    return NULL;
}


// ================================================================
// ========================== Public API ==========================
// ================================================================

struct bftree *bftree_create(struct bftree_opts *opts)
{
    struct node *root;
    struct bftree *tree;
    
    tree = malloc(sizeof(*tree));
    root = node_create(NULL);
    tree->root = root;
    tree->height = 1;
    tree->opts = opts;
    tree->is_migrated = 0;
    tree->del_payload_count = tree->put_payload_count = 0;
    assert(opts->key_destructor && opts->val_destructor);
    
    return tree;
}

void bftree_free(struct bftree *tree)
{
    bftree_free_node(tree, tree->root);
    free(tree);
}

int bftree_put(struct bftree *tree, void *key, void *val)
{
    struct payload *new_payload;
    uint32_t idx;
    
    if (!tree || !key)
        return BF_WRONG;
    
    new_payload = payload_create(key, val, Put);
    idx = find_container(tree->opts->key_compare, tree->root, new_payload->key, 0);
    container_insert(tree, tree->root, idx, new_payload);
    
    return BF_OK;
}

void *bftree_get(struct bftree *tree, void *key)
{
    uint32_t idx;
    struct payload *r;
    
    if (!tree || !key)
        return NULL;
    
    idx = find_container(tree->opts->key_compare, tree->root, key, 0);
    r = container_get(tree, tree->root, idx, key);
    
    if (r) {
        return r->val;
    }
    return NULL;
}

int bftree_del(struct bftree *tree, void *key)
{
    uint32_t idx;
    struct payload *new_payload;
    
    if (!tree || !key)
        return BF_WRONG;
    
    new_payload = payload_create(key, NULL, Del);
    idx = find_container(tree->opts->key_compare, tree->root, new_payload->key, 0);
    container_insert(tree, tree->root, idx, new_payload);
    
    return BF_OK;
}

struct bftree_iterator *bftree_get_iterator(struct bftree *tree)
{
    struct bftree_iterator *iter;
    
    iter = malloc(sizeof(*iter));
    iter->tree = tree;
    iter->next = NULL;
    iter->closed = 0;
    
    return iter;
}

struct payload *bftree_next(struct bftree_iterator *iter)
{
    struct bftree *tree;
    struct container *container;
    struct node *node;
    struct payload *curr, *next, *min;
    uint32_t idx;
    key_compare_func key_compare;
    int is_equal;
    
    if (iter->closed)
        return NULL;
    
    tree = iter->tree;
    key_compare = tree->opts->key_compare;
    if (!iter->next) {
        if (tree->root->container_size == 0)
            return NULL;
        iter->next = tree->root->containers[0]->payload_first;
    }
    
    curr = iter->next;
    min = NULL;
    node = tree->root;
    do {
        idx = find_container(key_compare, node, curr->key, 0);
        container = node->containers[idx];
        next = get_payload(key_compare, container->payload_first,
                           curr->key, &is_equal);
        if (!next)
            next = container->payload_first;
        else
            next = next->next;
        if (next) {
            if (!min) {
                min = next;
            } else if (key_compare(next->key, min->key) < 0) {
                min = next;
            }
        }
        node = container->child;
    } while(node);
    
    iter->next = min;
    if (!min)
        iter->closed = 1;
    
    return curr;
}

void bftree_free_iterator(struct bftree_iterator *iter)
{
    free(iter);
}

int bftree_count(struct bftree *tree)
{
    struct bftree_iterator *iter;
    int count;
    
    count = 0;
    
    iter = bftree_get_iterator(tree);
    while (bftree_next(iter) != NULL)
        count++;
    bftree_free_iterator(iter);
    
    return count;
}

// ================================================================
// ========================== Debug Area ==========================
// ================================================================

static void validate_containers(struct node *node, key_compare_func compare)
{
    int i;
    struct payload *curr, *prev;
    
    for (i = 0; i < node->container_size; i++) {
        prev = node->containers[i]->payload_first;
        curr = prev->next;
        while (curr) {
            assert(compare(prev->key, curr->key) < 0);
            prev = curr;
            curr = curr->next;
        }
        if (i == 0)
            continue;
        assert(compare(node->containers[i-1]->payload_first->key,
                       node->containers[i]->payload_first->key) < 0);
    }
}

void bftree_node_print(struct node *node)
{
    int i;
    struct payload *payload;
    
    for (i = 0; i < node->container_size; ++i) {
        printf("container%d %d %s\t", i, node->containers[i]->payload_size,
               node->containers[i]->payload_first->key);
        payload = node->containers[i]->payload_first;
        while (payload) {
            printf("%s => %s ", payload->key, payload->val);
            payload = payload->next;
        }
        printf("\n");
    }
    printf("\n");
}

// ================================================================
// ========================== Map Type Area =======================
// ================================================================

typedef char *wstr;

struct wstrhd {
    size_t len;
    char buf[];
};

static __inline wstr wstr_newlen(const void *init, size_t init_len)
{
    struct wstrhd *sh;
    sh = malloc(sizeof(struct wstrhd)+init_len+1);
    if (sh == NULL) {
        return NULL;
    }
    if (init) {
        memcpy(sh->buf, init, init_len);
        sh->len = init_len;
    } else {
        sh->len = 0;
    }
    sh->buf[sh->len] = '\0';
    return (wstr)(sh->buf);
}

static __inline void wstr_free(wstr s)
{
    if (s == NULL) {
        return ;
    }
    free(s - sizeof(struct wstrhd));
}

static __inline size_t wstrlen(const wstr s)
{
    struct wstrhd *hd = (struct wstrhd *)(s - sizeof(struct wstrhd));
    return hd->len;
}

static __inline int wstr_keycompare(const void *key1, const void *key2)
{
    size_t l1,l2;
    
    l1 = wstrlen((wstr)key1);
    l2 = wstrlen((wstr)key2);
    if (l1 != l2) return l1 < l2 ? -1 : 1;
    return memcmp(key1, key2, l1);
}

static struct bftree_opts map_opt = {
    NULL,
    NULL,
    wstr_keycompare,
    (void (*)(void*))wstr_free,
    free,
};

struct bftree *bftmap_create()
{
    return bftree_create(&map_opt);
}

void bftmap_free(struct bftree *tree)
{
    bftree_free(tree);
}

int bftmap_put(struct bftree *tree, char *key, size_t key_len, void *val)
{
    wstr s;
    if (!key || !key_len)
        return BF_WRONG;
    
    s = wstr_newlen(key, key_len);
    return bftree_put(tree, s, val);
}

void *bftmap_get(struct bftree *tree, char *key, size_t key_len)
{
    void *r;
    wstr s;

    if (!key || !key_len)
        return NULL;
    
    s = wstr_newlen(key, key_len);
    r = bftree_get(tree, s);
    wstr_free(s);
    return r;
}

int bftmap_del(struct bftree *tree, char *key, size_t key_len)
{
    wstr s;

    if (!key || !key_len)
        return BF_WRONG;
    
    s = wstr_newlen(key, key_len);
    return bftree_del(tree, s);
}

// ================================================================
// ========================== Set Type Area =======================
// ================================================================

struct bftree *bftset_create()
{
    return bftree_create(&map_opt);
}

void bftset_free(struct bftree *tree)
{
    bftree_free(tree);
}

int bftset_put(struct bftree *tree, char *key, size_t key_len)
{
    wstr s;

    if (!key || !key_len)
        return BF_WRONG;
    
    s = wstr_newlen(key, key_len);
    return bftree_put(tree, s, NULL);
}

void *bftset_get(struct bftree *tree, char *key, size_t key_len)
{
    void *r;
    wstr s;

    if (!key || !key_len)
        return NULL;
    
    s = wstr_newlen(key, key_len);
    r = bftree_get(tree, s);
    wstr_free(s);
    return r;
}

int bftset_del(struct bftree *tree, char *key, size_t key_len)
{
    wstr s;

    if (!key || !key_len)
        return BF_WRONG;
    
    s = wstr_newlen(key, key_len);
    return bftree_del(tree, s);
}