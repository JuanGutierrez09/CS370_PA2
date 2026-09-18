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
 * black. */
static struct rb_node NIL_NODE = {
    .color = BLACK, .left = &NIL_NODE, .right = &NIL_NODE, .parent = &NIL_NODE
};
#define NIL (&NIL_NODE)

/* ---- static helpers ---- */

static struct rb_node *bst_search(const rbtree_t *t, const char *key,
                                   struct rb_node **out_parent);
static char *copy_key(const char *key);
static struct rb_node *node_alloc(void);
static void node_destroy(rbtree_t *t, struct rb_node *n);
static void free_subtree(rbtree_t *t, struct rb_node *n);
static void inorder_walk(struct rb_node *n,
                          void (*fn)(const char *key, void *value, void *ctx),
                          void *ctx);
static int is_red(const struct rb_node *n);
static void rotate_left(rbtree_t *t, struct rb_node *x);
static void rotate_right(rbtree_t *t, struct rb_node *x);
static void insert_fixup(rbtree_t *t, struct rb_node *z);

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
    struct rb_node *parent = NIL;
    struct rb_node *existing = bst_search(t, key, &parent);

    if (existing != NIL) {
        if (t->value_free != NULL) {
            t->value_free(existing->value);
        }
        existing->value = value;
        return 0;
    }

    struct rb_node *z = node_alloc();
    if (z == NULL) {
        return -1;
    }

    z->key = copy_key(key);
    if (z->key == NULL) {
        goto cleanup_node;
    }
    z->value = value;
    z->parent = parent;

    if (parent == NIL) {
        t->root = z;
    } else if (strcmp(key, parent->key) < 0) {
        parent->left = z;
    } else {
        parent->right = z;
    }

    t->size++;
    insert_fixup(t, z);

    return 0;

cleanup_node:
    free(z);
    return -1;
}

void *rb_find(const rbtree_t *t, const char *key)
{
    struct rb_node *n = bst_search(t, key, NULL);
    return (n != NIL) ? n->value : NULL;
}

int rb_delete(rbtree_t *t, const char *key)
{
    (void)t;
    (void)key;
    /* TODO: bst_search() for the node; splice it out via a replace_child()
     * helper (using a subtree-minimum helper for the two-children case);
     * delete_fixup() if the spliced-out node was black; free its key/value
     * and itself; size--. */
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
    inorder_walk(t->root, fn, ctx);
}

int rb_validate(const rbtree_t *t)
{
    (void)t;
    /* TODO: recursive check of BST ordering, no red-red violations, equal
     * black-height on both children, root-is-black, and a node count that
     * matches t->size. */
    return -1;
}

void rb_destroy(rbtree_t *t)
{
    if (t == NULL) {
        return;
    }
    free_subtree(t, t->root);
    free(t);
}

/* ---- helper bodies ---- */


/* Precondition: x->right != NIL (rotate_left needs a real right child to
 * pivot up). */
static void rotate_left(rbtree_t *t, struct rb_node *x)
{
    struct rb_node *y = x->right;
    x->right = y->left;

    if (y->left != NIL) {
        y->left->parent = x;
    }
    
    y->parent = x->parent;

    if (x->parent==NIL) {
        t->root = y;
    }

    else if (x == x->parent->left) {
        x->parent->left = y;
    }
    else {x->parent->right = y;
    }

    y->left=x;
    x->parent = y;
}

/* Precondition: x->left != NIL (rotate_right needs a real left child to
 * pivot up). */
static void rotate_right(rbtree_t *t, struct rb_node *x)
{
    struct rb_node *y = x->left;
    x->left = y->right;

    if (y->right != NIL) {
        y->right->parent = x;
    }

    y->parent = x->parent;

    if (x->parent == NIL) {
        t->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}

static int is_red(const struct rb_node *n)
{
    return n->color == RED;
}

static void insert_fixup(rbtree_t *t, struct rb_node *z)
{
    /* invariant: z is red; each iteration either resolves the red-red
     * conflict (uncle red -> recolor and move z to the grandparent) or
     * fixes it with rotations and exits, so the loop terminates once z's
     * parent is black (root's parent NIL is always black). */
    while (is_red(z->parent)) {
        if (z->parent == z->parent->parent->left) {
            struct rb_node *y = z->parent->parent->right; /* uncle */
            if (is_red(y)) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotate_left(t, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_right(t, z->parent->parent);
            }
        } else {
            struct rb_node *y = z->parent->parent->left; /* uncle */
            if (is_red(y)) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(t, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_left(t, z->parent->parent);
            }
        }
    }
    t->root->color = BLACK;
}



static void inorder_walk(
    struct rb_node *n,
    void (*fn)(const char *key, void *value, void *ctx),
    void *ctx)
{
    if (n == NIL) {
        return;
    }

    inorder_walk(n->left, fn, ctx);
    fn(n->key, n->value, ctx);
    inorder_walk(n->right, fn, ctx);
}


static struct rb_node *bst_search(const rbtree_t *t, const char *key,
                                   struct rb_node **out_parent)
{
    struct rb_node *parent = NIL;
    struct rb_node *n = t->root;

    /* invariant: n is always a strict descendant of parent (or NIL, while
     * still at the root), so each iteration strictly descends the tree
     * and the loop terminates at a leaf. */
    while (n != NIL) {
        int cmp = strcmp(key, n->key);
        if (cmp == 0) {
            if (out_parent != NULL) {
                *out_parent = n->parent;
            }
            return n;
        }
        parent = n;
        n = (cmp < 0) ? n->left : n->right;
    }

    if (out_parent != NULL) {
        *out_parent = parent;
    }
    return NIL;
}

static char *copy_key(const char *key)
{
    size_t len = strlen(key) + 1;
    char *copy = malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, key, len);
    return copy;
}

static struct rb_node *node_alloc(void)
{
    struct rb_node *n = malloc(sizeof *n);
    if (n == NULL) {
        return NULL;
    }

    n->key = NULL;
    n->value = NULL;
    n->color = RED;
    n->left = NIL;
    n->right = NIL;
    n->parent = NIL;

    return n;
}

static void node_destroy(rbtree_t *t, struct rb_node *n)
{
    free(n->key);
    if (t->value_free != NULL) {
        t->value_free(n->value);
    }
    free(n);
}

static void free_subtree(rbtree_t *t, struct rb_node *n)
{
    /* invariant: each recursive call operates on a strictly smaller
     * subtree (one fewer node between n and the nearest NIL leaf), so
     * recursion terminates at NIL; depth is bounded by tree height. */
    if (n == NIL) {
        return;
    }
    free_subtree(t, n->left);
    free_subtree(t, n->right);
    node_destroy(t, n);
}
