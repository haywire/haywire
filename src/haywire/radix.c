/*
 * a proper radix tree implementation.
 * works on strings of bytes.
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "radix.h"

#ifndef LSB_FIRST
static inline int count_bits(char *k1, char *k2, int count)
{
    int mask = 128;
    while (~(*k1 ^ *k2) & mask && --count) {
        mask >>= 1;
        if (0 == mask) {
            mask = 128;
            k1 += 1;
            k2 += 1;
        }
    }
    return count;
}
#else
static __inline int count_bits(char *k1, char *k2, int count)
{
    int mask = 1;
    while (~(*k1 ^ *k2) & mask && --count) {
        mask <<= 1;
        if (256 == mask) {
            mask = 1;
            k1 += 1;
            k2 += 1;
        }
    }
    return count;
}
#endif

static int count_common_bits(char *k1, char *k2, int max)
{
    int count = max;
    // XXX SIMD-ify?
    while (*k1 == *k2 && count >= sizeof(int) * 8) {
        int *i1 = (int*)k1, *i2 = (int*)k2;
        if (*i1 == *i2) {
            k1 += sizeof(int);
            k2 += sizeof(int);
            count -= sizeof(int) * 8;
        } else break;
    }
    while (*k1 == *k2 && count >= 8) {
        k1++;
        k2++;
        count -= 8;
    }

    return max - count_bits(k1, k2, count);
}

#ifndef LSB_FIRST
static inline int shift(int i)
{
    return 128 >> (i & 7);
}
#else
static __inline int shift(int i)
{
    return 1 << (i & 7);
}
#endif

static __inline int get_bit_at(char *k, int i)
{
    int bytes = i >> 3;
    int mask = shift(i);
    k += bytes;
    return *k & mask;
}

static __inline int rdx_min(int a, int b)
{
    return a > b ? b : a;
}

static int insert_leaf(rxt_node *newleaf, rxt_node *sibling, rxt_node *parent)
{
    int idx, bit, max_len;
    rxt_node *inner;


    max_len = rdx_min(newleaf->pos, sibling->pos);
    idx  = count_common_bits(newleaf->key, sibling->key, max_len);
    bit = get_bit_at(newleaf->key, idx);

    if (!parent) {
        // insert at the root, so rotate things like so:
/*
           /\    to     /\
          1  2         /\ 3
                      1  2                   */

        parent = sibling;
        inner = (rxt_node *)malloc(sizeof(rxt_node));
        if (!inner) return -1;
        inner->color = 0;
        inner->value = NULL;
        inner->parent = parent;
        inner->left = parent->left;
        inner->right = parent->right;
        inner->key = parent->key;
        inner->pos = parent->pos;
        parent->pos = idx;
        parent->left->parent = inner;
        parent->right->parent = inner;
        newleaf->parent = parent;
        if (bit) {
            parent->right = newleaf;
            parent->left = inner;
        } else {
            parent->right = inner;
            parent->left = newleaf;
        }
        return 0;
    }

    if (idx < parent->pos) {
        // use the parent as a sibling
        return insert_leaf(newleaf, parent, parent->parent);
    } else {
        // otherwise, add newleaf as a child of inner

        // Check for duplicates.
        // FIXME feels hackish; do this properly.
        if (newleaf->pos == sibling->pos &&
            !strncmp(newleaf->key, sibling->key, newleaf->pos)) {
            free(newleaf);
            return -1;
        }

        inner = (rxt_node *)malloc(sizeof(rxt_node));
        if (!inner) free(inner);
        inner->color = 0;
        inner->value = NULL;
        inner->parent = parent;
        inner->pos = idx;
        inner->key = sibling->key;
        newleaf->parent = inner;
        sibling->parent = inner;

        if (bit) {
            inner->right = newleaf;
            inner->left = sibling;
        } else {
            inner->right = sibling;
            inner->left = newleaf;
        }

        // now find out which branch of parent to assign inner
        if (parent->left == sibling)
            parent->left = inner;
        else if (parent->right == sibling)
            parent->right = inner;
        else {
            fprintf(stderr, "inappropriate child %s/%s found in parent when inserting leaf %s (expected %s)\n", parent->left->key, parent->right->key, newleaf->key, sibling->key);
            return -1;
        }
    }
    return 0;
}

// color: 1 for leaf, 0 for inner
static int insert_internal(rxt_node *newleaf, rxt_node *n)
{
    // FIRST: check for common bits
    rxt_node *left = n->left, *right = n->right;
    int bits   = count_common_bits(newleaf->key, left->key,
                                   rdx_min(newleaf->pos, left->pos));
    int bits2  = count_common_bits(newleaf->key, right->key,
                                   rdx_min(newleaf->pos, right->pos));

    if (rdx_min(bits, bits2) < n->pos) {
        if (bits >= bits2)
            return insert_leaf(newleaf, n->left, n);
        return insert_leaf(newleaf, n->right, n);
    }

    if (bits >= bits2) {
         if (left->color)
            return insert_leaf(newleaf, n->left, n);
        return insert_internal(newleaf, left);
    } else {
        if (right->color)
            return insert_leaf(newleaf, n->right, n);
        return insert_internal(newleaf, right);
    }

    return -1; // this should never happen
}

static __inline int keylen(char *str)
{
    int len = strlen(str);
    if (len < 0 || len >= RADIXTREE_KEYSIZE)
        fprintf(stderr, "Warning: rxt key (%d) exceeds limit (%d)\n",
                len, RADIXTREE_KEYSIZE);
    return 8 * (len + 1) - 1;
}

int rxt_put(char *key, void *value, rxt_node *n)
{
#define NEWLEAF(nl, k, v) \
    nl = (rxt_node *)malloc(sizeof(rxt_node)); \
    if (!nl) return -1; \
    strncpy(nl->keycache, k, RADIXTREE_KEYSIZE); \
    nl->keycache[RADIXTREE_KEYSIZE-1] = '\0'; \
    nl->key = nl->keycache; \
    nl->pos = keylen(k); \
    nl->value = v; \
    nl->color = 1; \
    nl->parent = n; \
    nl->left = NULL; \
    nl->right = NULL

    rxt_node *newleaf;
    NEWLEAF(newleaf, key, value);

    // this special case takes care of the first two entries
    if (!(n->left || n->right)) {
        rxt_node *sib;
        int bits;
        // create root
        if (!n->value) {
            // attach root
            n->color = 2;
            n->value = newleaf;
            return 0;
        }
        // else convert root to inner and attach leaves
        sib = (rxt_node *)n->value;

        // count bits in common
        bits = count_common_bits(key, sib->key,
                    rdx_min(newleaf->pos, sib->pos));


        if (get_bit_at(key, bits)) {
            n->right = newleaf;
            n->left = sib;
        } else {
            n->right = sib;
            n->left = newleaf;
        }
        n->value = NULL;
        n->key = sib->key;
        n->pos = bits;
        n->color = 0;
        return 0;
    }

    newleaf->parent = NULL; // null for now

    return insert_internal(newleaf, n);

#undef NEWLEAF
}

static rxt_node* get_internal(char *key, rxt_node *root)
{
    if (!root) return NULL;

    if (root->color) {
        if (2 == root->color) root = (rxt_node *)root->value;
        if (!strncmp(key, root->key, root->pos))
            return root;
        return NULL;
    }

    if (get_bit_at(key, root->pos))
        return get_internal(key, root->right);
    return get_internal(key, root->left);
}

static void reset_key(char *key, char *newkey, rxt_node *n)
{
    // This should only be propagated up inner nodes.
    // Right now the algorithm guarantees this, but it is unchecked.

    if (key == n->key) {
        n->key = newkey;
        if (n->left && n->right)
            n->pos = count_common_bits(n->left->key, n->right->key,
                                   rdx_min(n->left->pos, n->right->pos));

        if (n->parent)
            reset_key(key, newkey, n->parent);
    }
}

static void *delete_internal(rxt_node *n, rxt_node *sibling)
{
    rxt_node *parent = n->parent;
    void *v = n->value;

    // TODO ascii art
    if (sibling->color) {
        parent->value = sibling;
        parent->left = NULL;
        parent->right = NULL;
        parent->color = 2;
        parent->pos = 0;
    } else {
        parent->left = sibling->left;
        parent->right = sibling->right;
        parent->pos = sibling->pos;
        sibling->left->parent = parent;
        sibling->right->parent = parent;
        free(sibling);
    }

    reset_key(n->key, sibling->key, parent);
    free(n);
    return v;
}

void* rxt_delete(char *key, rxt_node *root)
{
    rxt_node *parent, *grandparent;
    rxt_node *n = get_internal(key, root);
    void *v;
    char *newkey = NULL;
    if (!n) return NULL; // nonexistent

    v = n->value;

    // remove both the node and the parent inner node
    // XXX TODO FIXME Still somewhat broken. Figure out.
    parent = n->parent;
    grandparent = parent->parent;

    if (!grandparent) {
        if (parent->left == n) {
            return delete_internal(n, parent->right);
        } else if (parent->right == n) {
            return delete_internal(n, parent->left);
        } else if (parent->value == n) {
            parent->value = NULL;
            parent->color = 0;
            parent->pos = 0;
        } else
            printf("something very wrong when removing w/o gp!\n");

        free(n);
        return v;
    }

    // properly move around pointers and shit
    // TODO ascii art
    if (grandparent->left == n->parent) {
        newkey = grandparent->right->key; // not sure if this is correct
        if (parent->left == n) {
            grandparent->left = parent->right;
            parent->right->parent = grandparent;
        } else if (parent->right == n) {
            grandparent->left = parent->left;
            parent->left->parent = grandparent;
        } else
            printf("something very wrong: removing grandparent->left\n");
    } else if (grandparent->right == n->parent) {
        newkey = grandparent->left->key;
        if (parent->left == n ) {
            grandparent->right = parent->right;
            parent->right->parent = grandparent;
        } else if (parent->right == n) {
            grandparent->right = parent->left;
            parent->left->parent = grandparent;
        } else
            printf("something very wrong: removing grandparent->right\n");
    } else
        printf("something very wrong: grandparent does not possess child\n");

    reset_key(n->key, newkey, grandparent);
    parent->left = NULL;
    parent->right = NULL;
    free(parent);
    free(n); // we don't dynamically allocate node

    return v;
}

void rxt_free(rxt_node *root)
{
    if (!root) return;
    rxt_free(root->left);
    rxt_free(root->right);
    if (root->value) free(root->value);
    root->left = NULL;
    root->right = NULL;
    root->value = NULL;
    free(root);
}

void* rxt_get(char *key, rxt_node *root)
{
    rxt_node *n = get_internal(key, root);
    if (!n) return NULL;
    return n->value;
}

rxt_node *rxt_init()
{
    rxt_node *root = (rxt_node *)malloc(sizeof(rxt_node));
    if (!root) return NULL;
    memset(root, 0, sizeof(rxt_node));
    return root;
}