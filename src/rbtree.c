#include "rbtree.h"
#include "../tests/fault_alloc.h"

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
static void replace_child(rbtree_t *t, struct rb_node *u, struct rb_node *v);
static struct rb_node *tree_minimum(struct rb_node *n);
static void delete_fixup(rbtree_t *t, struct rb_node *x, struct rb_node *x_parent);
static int check_bst_order(const struct rb_node *n, const char **prev);
static int check_rb_shape(const struct rb_node *n, size_t *count);

/* ---- public API (include/rbtree.h is the contract; do not edit it) ---- */

rbtree_t *rb_create(rb_value_free_fn value_free)
{
    rbtree_t *t = rb_malloc(sizeof *t);
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
    rb_free(z);
    return -1;
}

void *rb_find(const rbtree_t *t, const char *key)
{
    struct rb_node *n = bst_search(t, key, NULL);
    return (n != NIL) ? n->value : NULL;
}

int rb_delete(rbtree_t *t, const char *key)
{
    struct rb_node *z = bst_search(t, key, NULL);
    if (z == NIL) {
        return -1;
    }

    struct rb_node *y = z;
    enum color y_original_color = y->color;
    struct rb_node *x;
    struct rb_node *x_parent;

    if (z->left == NIL) {
        x = z->right;
        x_parent = z->parent;
        replace_child(t, z, z->right);
    } else if (z->right == NIL) {
        x = z->left;
        x_parent = z->parent;
        replace_child(t, z, z->left);
    } else {
        y = tree_minimum(z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent == z) {
            x_parent = y;
        } else {
            x_parent = y->parent;
            replace_child(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        replace_child(t, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == BLACK) {
        delete_fixup(t, x, x_parent);
    }

    node_destroy(t, z);
    t->size--;

    return 0;
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
    if (t->root == NIL) {
        return (t->size == 0) ? 0 : -1;
    }
    if (t->root->color != BLACK) {
        return -1;
    }
    
    const char *prev = NULL;
    if (check_bst_order(t->root, &prev) != 0) {
        return -1;
    }

    size_t count = 0;
    if (check_rb_shape(t->root, &count) < 0) {
        return -1;
    }
    return (count == t->size) ? 0 : -1;
}

void rb_destroy(rbtree_t *t)
{
    if (t == NULL) {
        return;
    }
    free_subtree(t, t->root);
    rb_free(t);
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

/* CLRS RB-TRANSPLANT: rewires u's parent to point at v in u's old slot.
 * v's parent is only written when v is a real node, preserving the
 * invariant (upheld throughout this file) that NIL's fields are never
 * mutated. */
static void replace_child(rbtree_t *t, struct rb_node *u, struct rb_node *v)
{
    if (u->parent == NIL) {
        t->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }

    if (v != NIL) {
        v->parent = u->parent;
    }
}

/* invariant: n is always a node of the subtree whose minimum is sought;
 * each iteration strictly descends left, so it terminates at the
 * leftmost real node. */
static struct rb_node *tree_minimum(struct rb_node *n)
{
    while (n->left != NIL) {
        n = n->left;
    }
    return n;
}

/* CLRS RB-DELETE-FIXUP, parameterized on x's parent explicitly (x_parent)
 * instead of reading x->parent, since x may be NIL and this file never
 * mutates NIL's fields (see replace_child above). x_parent is only ever
 * dereferenced here when x is a real node too far from the root for the
 * loop to have already exited, so it is always a genuine tree node at
 * every point it's read.
 *
 * invariant: each iteration either clears the "extra black" deficiency at
 * x outright (recolor+rotate, cases 1/3/4 below) or pushes it one level
 * up the tree (case 2, x becomes x_parent), so the loop terminates once x
 * turns red or reaches the root. */
static void delete_fixup(rbtree_t *t, struct rb_node *x, struct rb_node *x_parent)
{
    while (x != t->root && !is_red(x)) {
        if (x == x_parent->left) {
            struct rb_node *w = x_parent->right; /* sibling; never NIL here */
            if (is_red(w)) {
                w->color = BLACK;
                x_parent->color = RED;
                rotate_left(t, x_parent);
                w = x_parent->right;
            }
            if (!is_red(w->left) && !is_red(w->right)) {
                w->color = RED;
                x = x_parent;
                x_parent = x->parent;
            } else {
                if (!is_red(w->right)) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotate_right(t, w);
                    w = x_parent->right;
                }
                w->color = x_parent->color;
                x_parent->color = BLACK;
                w->right->color = BLACK;
                rotate_left(t, x_parent);
                x = t->root;
            }
        } else {
            struct rb_node *w = x_parent->left; /* sibling; never NIL here */
            if (is_red(w)) {
                w->color = BLACK;
                x_parent->color = RED;
                rotate_right(t, x_parent);
                w = x_parent->left;
            }
            if (!is_red(w->left) && !is_red(w->right)) {
                w->color = RED;
                x = x_parent;
                x_parent = x->parent;
            } else {
                if (!is_red(w->left)) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotate_left(t, w);
                    w = x_parent->left;
                }
                w->color = x_parent->color;
                x_parent->color = BLACK;
                w->left->color = BLACK;
                rotate_right(t, x_parent);
                x = t->root;
            }
        }
    }
    if (x != NIL) {
        x->color = BLACK;
    }
}

/* In-order walk enforcing strictly increasing keys. *prev starts NULL and
 * is updated after every visited node; invariant: on return, *prev holds
 * the largest key seen so far in this walk, so each new node is compared
 * against the correct predecessor regardless of recursion depth. Returns
 * 0 if order holds throughout, nonzero on the first violation found. */
static int check_bst_order(const struct rb_node *n, const char **prev)
{
    if (n == NIL) {
        return 0;
    }
    if (check_bst_order(n->left, prev) != 0) {
        return -1;
    }
    if (*prev != NULL && strcmp(n->key, *prev) <= 0) {
        return -1;
    }
    *prev = n->key;
    return check_bst_order(n->right, prev);
}

/* Post-order walk checking, in one pass, that no red node has a red child
 * and that both children of every node have equal black-height; *count is
 * incremented once per real (non-NIL) node visited. Returns this
 * subtree's black-height, or -1 if a violation was found anywhere in it --
 * the -1 sentinel short-circuits back up through the recursion without
 * further checks once a violation is found. */
static int check_rb_shape(const struct rb_node *n, size_t *count)
{
    if (n == NIL) {
        return 0;
    }
    int left_bh = check_rb_shape(n->left, count);
    if (left_bh < 0) {
        return -1;
    }
    int right_bh = check_rb_shape(n->right, count);
    if (right_bh < 0) {
        return -1;
    }
    if (left_bh != right_bh) {
        return -1;
    }
    if (n->color == RED && (n->left->color != BLACK || n->right->color != BLACK)) {
        return -1;
    }
    (*count)++;
    return left_bh + (n->color == BLACK ? 1 : 0);
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
    char *copy = rb_malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, key, len);
    return copy;
}

static struct rb_node *node_alloc(void)
{
    struct rb_node *n = rb_malloc(sizeof *n);
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
    rb_free(n->key);
    if (t->value_free != NULL) {
        t->value_free(n->value);
    }
    rb_free(n);
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
