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

int main(void)
{
    test_uncle_red_recolors();
    test_triangle_case_rotates_twice();
    test_line_case_rotates_once();

    if (failures > 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("all internal fixup tests passed\n");
    return 0;
}
