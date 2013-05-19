#include <stdlib.h>
#include "haywire.h"
#include "trie/radix.h"

keyvalue_enumerator* keyvalue_pair_enumerator_init(http_request *request)
{
    keyvalue_enumerator* enumerator = rxt_enumerator_init((rxt_node *)request->headers);
    return enumerator;
}

keyvalue_pair* keyvalue_pair_next_pair(keyvalue_enumerator *enumerator)
{
    keyvalue_pair* pair = malloc(sizeof(keyvalue_pair));
    rxt_node* node = rxt_next_node(enumerator);
    if (node != NULL)
    {
        pair->key = node->key;
        pair->value = node->value;
        return pair;
    }
    else
    {
        return NULL;
    }
}

void free_keyvalue_pair_enumerator(keyvalue_enumerator* enumerator)
{
    /* free(enumerator->previousState); */
}
