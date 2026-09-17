#include "rbtree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)

/* rb_create() must hand back a live, empty tree -- the baseline every
 * other test builds on. */
static void test_create_returns_empty_tree(void)
{
    printf("test_create_returns_empty_tree\n");
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL, "rb_create returned NULL");
    CHECK(rb_size(t) == 0, "fresh tree should have size 0");
    rb_destroy(t);
}

/* The most basic contract: a value inserted under a key must be
 * retrievable under that same key. */
static void test_insert_then_find_roundtrip(void)
{
    printf("test_insert_then_find_roundtrip\n");
    rbtree_t *t = rb_create(NULL);
    int value = 42;

    CHECK(rb_insert(t, "alpha", &value) == 0, "insert should succeed");
    CHECK(rb_find(t, "alpha") == &value, "find should return the inserted value");
    CHECK(rb_size(t) == 1, "size should be 1 after one insert");

    rb_destroy(t);
}

/* rb_find must report absence with NULL rather than crashing or returning
 * a stale pointer, both on an empty tree and once other keys exist. */
static void test_find_absent_key_returns_null(void)
{
    printf("test_find_absent_key_returns_null\n");
    rbtree_t *t = rb_create(NULL);
    int value = 1;

    CHECK(rb_find(t, "missing") == NULL, "find on empty tree should return NULL");

    rb_insert(t, "present", &value);
    CHECK(rb_find(t, "missing") == NULL, "find for a never-inserted key should return NULL");

    rb_destroy(t);
}

/* Exercises the BST attach logic in both directions (smaller keys go
 * left, larger keys go right) by inserting keys around "m" and confirming
 * every key -- not just the root -- is independently findable. */
static void test_insert_builds_correct_bst_shape(void)
{
    printf("test_insert_builds_correct_bst_shape\n");
    rbtree_t *t = rb_create(NULL);
    const char *keys[] = { "m", "d", "t", "b", "f", "p", "z" };
    size_t n = sizeof(keys) / sizeof(keys[0]);
    int values[7];

    for (size_t i = 0; i < n; i++) {
        values[i] = (int)i;
        CHECK(rb_insert(t, keys[i], &values[i]) == 0, "insert should succeed");
    }
    CHECK(rb_size(t) == n, "size should equal number of distinct keys inserted");

    for (size_t i = 0; i < n; i++) {
        CHECK(rb_find(t, keys[i]) == &values[i], "each key should map back to its own value");
    }

    rb_destroy(t);
}

/* Header says overwriting an existing key must free the old value via
 * value_free -- verify it's invoked exactly once, with the OLD pointer. */
static int freed_count = 0;
static void *last_freed = NULL;

static void spy_value_free(void *value)
{
    freed_count++;
    last_freed = value;
}

static void test_insert_duplicate_key_overwrites_and_frees_old_value(void)
{
    printf("test_insert_duplicate_key_overwrites_and_frees_old_value\n");
    rbtree_t *t = rb_create(spy_value_free);
    int old_value = 1;
    int new_value = 2;

    freed_count = 0;
    last_freed = NULL;

    CHECK(rb_insert(t, "key", &old_value) == 0, "first insert should succeed");
    CHECK(rb_insert(t, "key", &new_value) == 0, "overwrite insert should succeed");

    CHECK(rb_size(t) == 1, "overwriting a key must not grow the tree");
    CHECK(rb_find(t, "key") == &new_value, "find should return the newest value");
    CHECK(freed_count == 1, "value_free should be invoked exactly once");
    CHECK(last_freed == &old_value, "value_free should be called with the OLD value");

    rb_destroy(t);
}

/* Header says rb_insert copies the key (tree owns the copy). Mutate the
 * caller's buffer after inserting and confirm lookup is unaffected,
 * proving the tree didn't just alias the caller's pointer. */
static void test_insert_copies_key(void)
{
    printf("test_insert_copies_key\n");
    rbtree_t *t = rb_create(NULL);
    char buf[16];
    int value = 7;

    strcpy(buf, "mutable");
    CHECK(rb_insert(t, buf, &value) == 0, "insert should succeed");

    memset(buf, 'x', sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    CHECK(rb_find(t, "mutable") == &value, "lookup by the original key text must still work");

    rb_destroy(t);
}

/* rb_size must track the number of distinct keys: +1 per new key, and
 * never change on its own. */
static void test_size_reflects_distinct_key_count(void)
{
    printf("test_size_reflects_distinct_key_count\n");
    rbtree_t *t = rb_create(NULL);
    int value = 0;

    CHECK(rb_size(t) == 0, "empty tree should report size 0");
    rb_insert(t, "one", &value);
    CHECK(rb_size(t) == 1, "size should be 1 after first insert");
    rb_insert(t, "two", &value);
    CHECK(rb_size(t) == 2, "size should be 2 after second distinct insert");

    rb_destroy(t);
}

/* Header explicitly documents rb_destroy as NULL-safe: calling it on NULL
 * must be a no-op, not a crash. Reaching the next line is the assertion. */
static void test_destroy_null_is_noop(void)
{
    printf("test_destroy_null_is_noop\n");
    rb_destroy(NULL);
}

int main(void)
{
    test_create_returns_empty_tree();
    test_insert_then_find_roundtrip();
    test_find_absent_key_returns_null();
    test_insert_builds_correct_bst_shape();
    test_insert_duplicate_key_overwrites_and_frees_old_value();
    test_insert_copies_key();
    test_size_reflects_distinct_key_count();
    test_destroy_null_is_noop();

    if (failures > 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("all tests passed\n");
    return 0;
}
