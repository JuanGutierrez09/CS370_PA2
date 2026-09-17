#include "rbtree.h"

#include <stdlib.h>
#include <string.h>

enum color {
    RED,
    BLACK
};

struct rb_node {
    char *key;
    void *value;

    enum color color;

    struct rb_node *left;
    struct rb_node *right;
    struct rb_node *parent;
};

struct rbtree {
    struct rb_node *root;
    size_t size;
    rb_value_free_fn value_free;
};

/* Shared sentinel used as every leaf and as the root's parent; always
 * black. Its left/right/parent fields are otherwise unused, except that
 * delete_fixup() temporarily sets NIL_NODE.parent so the fixup loop can
 * climb from a NIL position (the standard CLRS technique; safe only
 * because there is a single global sentinel and no concurrent use). */
static struct rb_node NIL_NODE = { .color = BLACK };
#define NIL (&NIL_NODE)

/* ---- static helpers (declared now; bodies land with each vertical slice) ---- */

static bool is_red(const struct rb_node *n);

static void replace_child(rbtree_t *t, struct rb_node *parent,
                           struct rb_node *old_child, struct rb_node *new_child);
static void rotate_left(rbtree_t *t, struct rb_node *x);
static void rotate_right(rbtree_t *t, struct rb_node *x);
static struct rb_node *sibling_of(const struct rb_node *parent,
                                   const struct rb_node *child);

static struct rb_node *bst_search(const rbtree_t *t, const char *key,
                                   struct rb_node **out_parent);

static char *copy_key(const char *key);
static struct rb_node *node_alloc(void);
static void insert_fixup(rbtree_t *t, struct rb_node *z);

static struct rb_node *subtree_min(struct rb_node *n);
static void delete_fixup(rbtree_t *t, struct rb_node *x);

static void node_destroy(rbtree_t *t, struct rb_node *n);
static void free_subtree(rbtree_t *t, struct rb_node *n);

static void inorder_walk(const struct rb_node *n,
                          void (*fn)(const char *key, void *value, void *ctx),
                          void *ctx);

static int validate_subtree(const struct rb_node *n, const char *lo, const char *hi,
                             size_t *count, int *black_height);

/* ---- public API (include/rbtree.h is the contract; do not edit it) ---- */

rbtree_t *rb_create(rb_value_free_fn value_free)
{
    rbtree_t *t = malloc(sizeof *t);
    if (t == NULL) {
        return NULL;
    }

    t->root = NIL;
    t->size = 0;
    t->value_free = value_free;

    return t;
}

int rb_insert(rbtree_t *t, const char *key, void *value)
{
    (void)t;
    (void)key;
    (void)value;
    /* TODO: bst_search() for the attach point / an existing node; on a
     * miss, node_alloc() + copy_key() with goto-cleanup on key-copy
     * failure (2-allocation path per CLAUDE.md); attach under the found
     * parent; insert_fixup(); size++. */
    return -1;
}

void *rb_find(const rbtree_t *t, const char *key)
{
    (void)t;
    (void)key;
    /* TODO: struct rb_node *n = bst_search(t, key, NULL);
     * return n != NIL ? n->value : NULL; */
    return NULL;
}

int rb_delete(rbtree_t *t, const char *key)
{
    (void)t;
    (void)key;
    /* TODO: bst_search() for the node; splice it out via replace_child()
     * (using subtree_min() for the two-children case); delete_fixup() if
     * the spliced-out node was black; node_destroy(); size--. */
    return -1;
}

size_t rb_size(const rbtree_t *t)
{
    return t->size;
}

void rb_foreach(const rbtree_t *t,
                void (*fn)(const char *key, void *value, void *ctx),
                void *ctx)
{
    if (t == NULL || fn == NULL) {
        return;
    }
    (void)ctx;
    /* TODO: inorder_walk(t->root, fn, ctx); */
}

int rb_validate(const rbtree_t *t)
{
    (void)t;
    /* TODO: size_t count = 0; int black_height = 0;
     * validate_subtree(t->root, NULL, NULL, &count, &black_height), plus
     * a root-is-black check and count == t->size. */
    return -1;
}

void rb_destroy(rbtree_t *t)
{
    if (t == NULL) {
        return;
    }
    /* TODO: free_subtree(t, t->root); (no leak yet: the tree can't hold
     * nodes until rb_insert() is implemented). */
    free(t);
}
