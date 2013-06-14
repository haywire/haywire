#ifndef BUFTREE_MAP_H
#define BUFTREE_MAP_H

#include <stddef.h>

#include "buffered_tree.h"

// bftmap ownership:
// `key` will be copied and store. User should free `key` buffer passed in
// `val` is ignored whatever it is

// key is char* and val is char* too.
struct bftree *bftmap_create();
void bftmap_free(struct bftree *tree);
int bftmap_put(struct bftree *tree, char *key, size_t key_len, char *val);
void *bftmap_get(struct bftree *tree, char *key, size_t key_len);
int bftmap_del(struct bftree *tree, char *key, size_t key_len);

#endif