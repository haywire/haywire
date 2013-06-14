#ifndef BFTREE_SET_H
#define BFTREE_SET_H

#include "buffered_tree.h"

// bftmap ownership:
// `key` will be copied and store. User should free `key` buffer passed in

// key is char*
struct bftree *bftset_create();
void bftset_free(struct bftree *tree);
int bftset_put(struct bftree *tree, char *key, size_t key_len);
void *bftset_get(struct bftree *tree, char *key, size_t key_len);
int bftset_del(struct bftree *tree, char *key, size_t key_len);

#endif