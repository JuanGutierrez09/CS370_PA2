/* White-box tests for insert_fixup. The public API (include/rbtree.h,
 * the graded contract) intentionally exposes no way to inspect node color
 * or tree shape, so this translation unit #includes rbtree.c directly to
 * reach struct rb_node, NIL, and enum color, and asserts on the three
 * classic red-black fixup cases by name:
 *   1. uncle red      -> recolor, no rotation
 *   2. triangle        -> rotate around the parent, then the grandparent
 *   3. line (straight) -> a single rotation around the grandparent
 * Each expected tree shape below was hand-derived by tracing the CLRS
 * RB-INSERT-FIXUP algorithm for the exact key sequence inserted. */
#include "../src/rbtree.c"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)

static struct rb_node *find_node(rbtree_t *t, const char *key)
{
    return bst_search(t, key, NULL);
}

/* Builds a standalone node (key copy owned like a normal insert would),
 * bypassing rb_insert so tests can wire up an exact tree shape by hand and
 * exercise rotate_left/rotate_right directly, independent of insert_fixup. */
static struct rb_node *make_node(const char *key)
{
    struct rb_node *n = node_alloc();
    n->key = copy_key(key);
    return n;
}

/* rotate_left on the root, full 5-node subtree:
 *     x            y
 *    / \          / \
 *   a   y   ->   x   c
 *      / \      / \
 *     b   c    a   b
 * Covers the x->parent == NIL branch (root reassignment) and checks every
 * pointer the rotation touches, including the reparented middle child b. */
static void test_rotate_left_full_subtree_and_root(void)
{
    printf("test_rotate_left_full_subtree_and_root\n");
    rbtree_t *t = rb_create(NULL);

    struct rb_node *x = make_node("x");
    struct rb_node *a = make_node("a");
    struct rb_node *y = make_node("y");
    struct rb_node *b = make_node("b");
    struct rb_node *c = make_node("c");

    t->root = x;
    x->parent = NIL;
    x->left = a;  a->parent = x;
    x->right = y; y->parent = x;
    y->left = b;  b->parent = y;
    y->right = c; c->parent = y;

    rotate_left(t, x);

    CHECK(t->root == y, "y should become the new root");
    CHECK(y->parent == NIL, "new root's parent must be NIL");
    CHECK(y->left == x, "x should become y's left child");
    CHECK(x->parent == y, "x's parent should be updated to y");
    CHECK(x->left == a, "x's left child (a) must be untouched");
    CHECK(a->parent == x, "a's parent should remain x");
    CHECK(x->right == b, "x's right child should become y's old left child (b)");
    CHECK(b->parent == x, "b's parent should be updated to x");
    CHECK(y->right == c, "y's right child (c) must be untouched");
    CHECK(c->parent == y, "c's parent should remain y");

    rb_destroy(t);
}

/* rotate_left where x is its parent's LEFT child -- covers the
 * `x == x->parent->left` branch, and confirms the parent's unrelated
 * right child (w) is left alone. */
static void test_rotate_left_as_left_child_updates_parent_slot(void)
{
    printf("test_rotate_left_as_left_child_updates_parent_slot\n");
    rbtree_t *t = rb_create(NULL);

    struct rb_node *p = make_node("p");
    struct rb_node *w = make_node("w");
    struct rb_node *x = make_node("x");
    struct rb_node *a = make_node("a");
    struct rb_node *y = make_node("y");
    struct rb_node *b = make_node("b");

    t->root = p;
    p->parent = NIL;
    p->left = x;  x->parent = p;
    p->right = w; w->parent = p;
    x->left = a;  a->parent = x;
    x->right = y; y->parent = x;
    y->left = b;  b->parent = y;

    rotate_left(t, x);

    CHECK(t->root == p, "rotating a non-root node must not change the root");
    CHECK(p->left == y, "p's left slot must now point to y");
    CHECK(y->parent == p, "y's parent should be updated to p");
    CHECK(p->right == w, "p's untouched right child (w) must be unaffected");
    CHECK(y->left == x, "x should become y's left child");
    CHECK(x->parent == y, "x's parent should be updated to y");
    CHECK(x->right == b, "x's right child should become y's old left child (b)");
    CHECK(b->parent == x, "b's parent should be updated to x");

    rb_destroy(t);
}

/* rotate_left where x is its parent's RIGHT child -- covers the `else`
 * branch. Neither test_triangle_case_rotates_twice nor
 * test_line_case_rotates_once ever drives insert_fixup into calling
 * rotate_left on a right-child node, so this branch was previously
 * untested. */
static void test_rotate_left_as_right_child_updates_parent_slot(void)
{
    printf("test_rotate_left_as_right_child_updates_parent_slot\n");
    rbtree_t *t = rb_create(NULL);

    struct rb_node *p = make_node("p");
    struct rb_node *w = make_node("w");
    struct rb_node *x = make_node("x");
    struct rb_node *a = make_node("a");
    struct rb_node *y = make_node("y");
    struct rb_node *b = make_node("b");

    t->root = p;
    p->parent = NIL;
    p->left = w;  w->parent = p;
    p->right = x; x->parent = p;
    x->left = a;  a->parent = x;
    x->right = y; y->parent = x;
    y->left = b;  b->parent = y;

    rotate_left(t, x);

    CHECK(t->root == p, "rotating a non-root node must not change the root");
    CHECK(p->right == y, "p's right slot must now point to y");
    CHECK(y->parent == p, "y's parent should be updated to p");
    CHECK(p->left == w, "p's untouched left child (w) must be unaffected");
    CHECK(y->left == x, "x should become y's left child");
    CHECK(x->parent == y, "x's parent should be updated to y");
    CHECK(x->right == b, "x's right child should become y's old left child (b)");
    CHECK(b->parent == x, "b's parent should be updated to x");

    rb_destroy(t);
}

/* rotate_right on the root, full 5-node subtree (mirror of the
 * rotate_left root test):
 *       x          y
 *      / \        / \
 *     y   c  ->  a   x
 *    / \            / \
 *   a   b          b   c */
static void test_rotate_right_full_subtree_and_root(void)
{
    printf("test_rotate_right_full_subtree_and_root\n");
    rbtree_t *t = rb_create(NULL);

    struct rb_node *x = make_node("x");
    struct rb_node *y = make_node("y");
    struct rb_node *a = make_node("a");
    struct rb_node *b = make_node("b");
    struct rb_node *c = make_node("c");

    t->root = x;
    x->parent = NIL;
    x->left = y;  y->parent = x;
    x->right = c; c->parent = x;
    y->left = a;  a->parent = y;
    y->right = b; b->parent = y;

    rotate_right(t, x);

    CHECK(t->root == y, "y should become the new root");
    CHECK(y->parent == NIL, "new root's parent must be NIL");
    CHECK(y->right == x, "x should become y's right child");
    CHECK(x->parent == y, "x's parent should be updated to y");
    CHECK(x->right == c, "x's right child (c) must be untouched");
    CHECK(c->parent == x, "c's parent should remain x");
    CHECK(x->left == b, "x's left child should become y's old right child (b)");
    CHECK(b->parent == x, "b's parent should be updated to x");
    CHECK(y->left == a, "y's left child (a) must be untouched");
    CHECK(a->parent == y, "a's parent should remain y");

    rb_destroy(t);
}

/* rotate_right where x is its parent's RIGHT child -- covers the
 * `x == x->parent->right` branch (rotate_right checks this one first,
 * mirrored from rotate_left's check order). */
static void test_rotate_right_as_right_child_updates_parent_slot(void)
{
    printf("test_rotate_right_as_right_child_updates_parent_slot\n");
    rbtree_t *t = rb_create(NULL);

    struct rb_node *p = make_node("p");
    struct rb_node *w = make_node("w");
    struct rb_node *x = make_node("x");
    struct rb_node *y = make_node("y");
    struct rb_node *a = make_node("a");
    struct rb_node *b = make_node("b");

    t->root = p;
    p->parent = NIL;
    p->left = w;  w->parent = p;
    p->right = x; x->parent = p;
    x->left = y;  y->parent = x;
    y->left = a;  a->parent = y;
    y->right = b; b->parent = y;

    rotate_right(t, x);

    CHECK(t->root == p, "rotating a non-root node must not change the root");
    CHECK(p->right == y, "p's right slot must now point to y");
    CHECK(y->parent == p, "y's parent should be updated to p");
    CHECK(p->left == w, "p's untouched left child (w) must be unaffected");
    CHECK(y->right == x, "x should become y's right child");
    CHECK(x->parent == y, "x's parent should be updated to y");
    CHECK(x->left == b, "x's left child should become y's old right child (b)");
    CHECK(b->parent == x, "b's parent should be updated to x");

    rb_destroy(t);
}

/* rotate_right where x is its parent's LEFT child -- covers the `else`
 * branch. Symmetric gap to the rotate_left right-child case above: neither
 * fixup test ever drives insert_fixup into calling rotate_right on a
 * left-child node. */
static void test_rotate_right_as_left_child_updates_parent_slot(void)
{
    printf("test_rotate_right_as_left_child_updates_parent_slot\n");
    rbtree_t *t = rb_create(NULL);

    struct rb_node *p = make_node("p");
    struct rb_node *w = make_node("w");
    struct rb_node *x = make_node("x");
    struct rb_node *y = make_node("y");
    struct rb_node *a = make_node("a");
    struct rb_node *b = make_node("b");

    t->root = p;
    p->parent = NIL;
    p->left = x;  x->parent = p;
    p->right = w; w->parent = p;
    x->left = y;  y->parent = x;
    y->left = a;  a->parent = y;
    y->right = b; b->parent = y;

    rotate_right(t, x);

    CHECK(t->root == p, "rotating a non-root node must not change the root");
    CHECK(p->left == y, "p's left slot must now point to y");
    CHECK(y->parent == p, "y's parent should be updated to p");
    CHECK(p->right == w, "p's untouched right child (w) must be unaffected");
    CHECK(y->right == x, "x should become y's right child");
    CHECK(x->parent == y, "x's parent should be updated to y");
    CHECK(x->left == b, "x's left child should become y's old right child (b)");
    CHECK(b->parent == x, "b's parent should be updated to x");

    rb_destroy(t);
}

/* m(black) gets red children d and t; inserting b under d creates a
 * red-red conflict (b, d) with a red uncle (t) -> parent and uncle turn
 * black, grandparent (m, the root) turns red then back to black by the
 * end-of-function root fixup. No rotation, no shape change. */
static void test_uncle_red_recolors(void)
{
    printf("test_uncle_red_recolors\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    rb_insert(t, "t", &v);
    rb_insert(t, "b", &v);

    struct rb_node *root = t->root;
    struct rb_node *d = find_node(t, "d");
    struct rb_node *tt = find_node(t, "t");
    struct rb_node *b = find_node(t, "b");

    CHECK(strcmp(root->key, "m") == 0, "recolor case must not change the root key");
    CHECK(root->color == BLACK, "root must stay black");
    CHECK(d->color == BLACK, "parent must be recolored black");
    CHECK(tt->color == BLACK, "uncle must be recolored black");
    CHECK(b->color == RED, "newly inserted node stays red");
    CHECK(d->parent == root, "recolor must not change d's position");
    CHECK(tt->parent == root, "recolor must not change t's position");
    CHECK(b->parent == d, "recolor must not change b's position");

    rb_destroy(t);
}

/* m(black) gets a red left child d (no uncle, uncle is NIL/black).
 * Inserting f (d < f < m) makes f a red right child of red left child d:
 * a triangle. fixup rotates left around d first, then right around m,
 * leaving f as the new (black) subtree root with d and m as red children. */
static void test_triangle_case_rotates_twice(void)
{
    printf("test_triangle_case_rotates_twice\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    rb_insert(t, "f", &v);

    struct rb_node *root = t->root;

    CHECK(strcmp(root->key, "f") == 0, "triangle fixup should promote f to subtree root");
    CHECK(root->color == BLACK, "post-fixup root must be black");
    CHECK(strcmp(root->left->key, "d") == 0, "d should end up as f's left child");
    CHECK(strcmp(root->right->key, "m") == 0, "m should end up as f's right child");
    CHECK(root->left->color == RED, "d stays red after rotation");
    CHECK(root->right->color == RED, "m becomes red after rotation");
    CHECK(root->left->parent == root, "d's parent pointer must be fixed up to f");
    CHECK(root->right->parent == root, "m's parent pointer must be fixed up to f");

    rb_destroy(t);
}

/* m(black) gets a red left child d (uncle NIL/black). Inserting b (b < d)
 * makes b a red left child of red left child d: a straight line. fixup
 * does a single right rotation around m, leaving d as the new (black)
 * subtree root with b and m as red children. */
static void test_line_case_rotates_once(void)
{
    printf("test_line_case_rotates_once\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    rb_insert(t, "b", &v);

    struct rb_node *root = t->root;

    CHECK(strcmp(root->key, "d") == 0, "line fixup should promote d to subtree root");
    CHECK(root->color == BLACK, "post-fixup root must be black");
    CHECK(strcmp(root->left->key, "b") == 0, "b should end up as d's left child");
    CHECK(strcmp(root->right->key, "m") == 0, "m should end up as d's right child");
    CHECK(root->left->color == RED, "b stays red after rotation");
    CHECK(root->right->color == RED, "m becomes red after rotation");
    CHECK(root->left->parent == root, "b's parent pointer must be fixed up to d");
    CHECK(root->right->parent == root, "m's parent pointer must be fixed up to d");

    rb_destroy(t);
}

/* ---- rb_validate: positive cases ---- */

static void test_validate_empty_tree_is_valid(void)
{
    printf("test_validate_empty_tree_is_valid\n");
    rbtree_t *t = rb_create(NULL);

    CHECK(rb_validate(t) == 0, "a freshly created empty tree must validate");

    rb_destroy(t);
}

static void test_validate_single_node_tree_is_valid(void)
{
    printf("test_validate_single_node_tree_is_valid\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    CHECK(rb_validate(t) == 0, "a single-node tree (black root) must validate");

    rb_destroy(t);
}

/* Inserts a larger, unsorted key set and checks rb_validate after every
 * single insert, not just at the end -- this exercises whatever mix of
 * recolors and rotations each insert happens to trigger, organically, as
 * a general regression net for insert_fixup rather than one hand-picked
 * shape. */
static void test_validate_holds_after_every_insert(void)
{
    printf("test_validate_holds_after_every_insert\n");
    rbtree_t *t = rb_create(NULL);
    const char *keys[] = { "m", "d", "t", "b", "f", "p", "z", "a", "c", "e" };
    size_t n = sizeof(keys) / sizeof(keys[0]);
    int v = 0;

    for (size_t i = 0; i < n; i++) {
        rb_insert(t, keys[i], &v);
        CHECK(rb_validate(t) == 0, "tree must remain a valid red-black tree after every insert");
    }

    rb_destroy(t);
}

/* ---- rb_validate: negative cases ----
 * Each test builds a small valid tree via rb_insert (sanity-checked with
 * rb_validate == 0), then corrupts exactly one invariant by hand and
 * checks that rb_validate rejects it. Corruptions are chosen to leave the
 * other invariants intact where possible, so each test isolates the one
 * check it names. */

/* d is m's LEFT child but its key is mutated to "z" (> "m"): a pure BST
 * ordering violation. Colors/shape are untouched, so this isolates
 * check_bst_order specifically. */
static void test_validate_detects_bst_order_violation(void)
{
    printf("test_validate_detects_bst_order_violation\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    rb_insert(t, "t", &v);
    CHECK(rb_validate(t) == 0, "sanity: tree must be valid before corrupting it");

    struct rb_node *d = find_node(t, "d");
    free(d->key);
    d->key = copy_key("z"); /* "z" > "m", but still m's left child */

    CHECK(rb_validate(t) != 0, "rb_validate must reject a left child whose key exceeds its ancestor");

    rb_destroy(t);
}

/* d is already red (single red child of black root m). Manually attaching
 * a second red node (e) as d's right child creates a real red-red
 * conflict. t->size is bumped to match so the node-count check stays
 * consistent, isolating the red-red violation. */
static void test_validate_detects_red_red_violation(void)
{
    printf("test_validate_detects_red_red_violation\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    CHECK(rb_validate(t) == 0, "sanity: tree must be valid before corrupting it");

    struct rb_node *d = find_node(t, "d");
    CHECK(is_red(d), "sanity: d must already be red for this to be a real red-red conflict");

    struct rb_node *e = make_node("e"); /* "e" > "d": fits as d's right child in order */
    e->parent = d;
    e->color = RED;
    d->right = e;
    t->size++;

    CHECK(rb_validate(t) != 0, "rb_validate must reject a red node with a red child");

    rb_destroy(t);
}

/* d starts red (m's only child, right child stays NIL). Forcing d black
 * makes the left subtree's black-height 1 while the right (NIL) subtree's
 * stays 0 -- a pure black-height mismatch, with order/red-red/count all
 * untouched. */
static void test_validate_detects_black_height_mismatch(void)
{
    printf("test_validate_detects_black_height_mismatch\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    CHECK(rb_validate(t) == 0, "sanity: tree must be valid before corrupting it");

    struct rb_node *d = find_node(t, "d");
    d->color = BLACK;

    CHECK(rb_validate(t) != 0, "rb_validate must reject unequal black-heights between subtrees");

    rb_destroy(t);
}

static void test_validate_detects_red_root(void)
{
    printf("test_validate_detects_red_root\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    CHECK(rb_validate(t) == 0, "sanity: tree must be valid before corrupting it");

    t->root->color = RED;

    CHECK(rb_validate(t) != 0, "rb_validate must reject a red root");

    rb_destroy(t);
}

/* Shape/order/colors are all fine; only the bookkeeping t->size is wrong. */
static void test_validate_detects_size_mismatch(void)
{
    printf("test_validate_detects_size_mismatch\n");
    rbtree_t *t = rb_create(NULL);
    int v = 0;

    rb_insert(t, "m", &v);
    rb_insert(t, "d", &v);
    rb_insert(t, "t", &v);
    CHECK(rb_validate(t) == 0, "sanity: tree must be valid before corrupting it");

    t->size = 99;

    CHECK(rb_validate(t) != 0, "rb_validate must reject a size that doesn't match the actual node count");

    rb_destroy(t);
}

/* root stays NIL (truly empty) but size claims otherwise. */
static void test_validate_detects_inconsistent_empty_tree(void)
{
    printf("test_validate_detects_inconsistent_empty_tree\n");
    rbtree_t *t = rb_create(NULL);

    t->size = 3;

    CHECK(rb_validate(t) != 0, "rb_validate must reject an empty root paired with a nonzero size");

    rb_destroy(t);
}

int main(void)
{
    test_rotate_left_full_subtree_and_root();
    test_rotate_left_as_left_child_updates_parent_slot();
    test_rotate_left_as_right_child_updates_parent_slot();
    test_rotate_right_full_subtree_and_root();
    test_rotate_right_as_right_child_updates_parent_slot();
    test_rotate_right_as_left_child_updates_parent_slot();
    test_uncle_red_recolors();
    test_triangle_case_rotates_twice();
    test_line_case_rotates_once();
    test_validate_empty_tree_is_valid();
    test_validate_single_node_tree_is_valid();
    test_validate_holds_after_every_insert();
    test_validate_detects_bst_order_violation();
    test_validate_detects_red_red_violation();
    test_validate_detects_black_height_mismatch();
    test_validate_detects_red_root();
    test_validate_detects_size_mismatch();
    test_validate_detects_inconsistent_empty_tree();

    if (failures > 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("all internal fixup tests passed\n");
    return 0;
}
