#ifndef RADIXTREE
#define RADIXTREE

#define RADIXTREE_KEYSIZE 128

typedef struct rxt_node {
    int color;
    char *key;
    void *value;
    int pos; // bit index of the key to compare at (critical position)
    char keycache[RADIXTREE_KEYSIZE];
    int level; // tree level; for debug only
    int parent_id; //for debug only
    struct rxt_node *parent;
    struct rxt_node *left;
    struct rxt_node *right;
}rxt_node;

typedef struct rxt_result {
    int match;
    rxt_node *node;
    void *data;
}rxt_result;

typedef int (*rxt_compare_method)(char *key, rxt_node *root);

int rxt_put(char*, void *, rxt_node*);
void* rxt_get(char*, rxt_node*);
void* rxt_get_custom(char*, rxt_node*, rxt_compare_method compare_method);
void* rxt_delete(char*, rxt_node*);
void rxt_free(rxt_node *);
rxt_node *rxt_init();

#endif // RADIXTREE
