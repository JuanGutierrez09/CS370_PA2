#include "rbtree.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 64

/* Randomized smoke test: repeatedly insert into a small pool of keys and,
 * after every insert, cross-check every pool key against an oracle array
 * that tracks what the last insert into that key should have stored.
 * Purpose: current code has no balancing yet, so this exercises rb_insert/
 * rb_find/rb_size under many random attach orders and overwrite patterns,
 * beyond what the small hand-picked unit tests in test_rbtree.c cover. */
int main(int argc, char **argv)
{
    long iterations = (argc > 1) ? strtol(argv[1], NULL, 10) : 1000;
    srand(12345); /* fixed seed: failures are reproducible */

    rbtree_t *t = rb_create(NULL);
    if (t == NULL) {
        fprintf(stderr, "rb_create failed\n");
        return 1;
    }

    long oracle[POOL_SIZE];
    for (int i = 0; i < POOL_SIZE; i++) {
        oracle[i] = -1; /* -1 means "key%d" has never been inserted */
    }

    /* invariant: each iteration inserts into exactly one pool slot, so at
     * most one oracle entry changes per iteration, and the full-pool
     * re-check below must hold after every single insert. */
    for (long iter = 0; iter < iterations; iter++) {
        int slot = rand() % POOL_SIZE;
        char key[16];
        snprintf(key, sizeof key, "key%d", slot);

        long value = iter; /* distinct per insert so overwrites are observable */
        /* encode as value+1: value 0 must not collide with the NULL
         * pointer rb_find returns for an absent key */
        if (rb_insert(t, key, (void *)(uintptr_t)(value + 1)) != 0) {
            fprintf(stderr, "rb_insert unexpectedly failed at iteration %ld\n", iter);
            rb_destroy(t);
            return 1;
        }
        oracle[slot] = value;

        for (int i = 0; i < POOL_SIZE; i++) {
            char check_key[16];
            snprintf(check_key, sizeof check_key, "key%d", i);
            void *found = rb_find(t, check_key);
            long expected = oracle[i];
            long actual = (found == NULL) ? -1 : (long)(uintptr_t)found - 1;
            if (actual != expected) {
                fprintf(stderr, "mismatch at key%d: expected %ld got %ld (iteration %ld)\n",
                        i, expected, actual, iter);
                rb_destroy(t);
                return 1;
            }
        }
    }

    long expected_size = 0;
    for (int i = 0; i < POOL_SIZE; i++) {
        if (oracle[i] != -1) {
            expected_size++;
        }
    }
    if (rb_size(t) != (size_t)expected_size) {
        fprintf(stderr, "size mismatch: expected %ld got %zu\n", expected_size, rb_size(t));
        rb_destroy(t);
        return 1;
    }

    rb_destroy(t);
    printf("fuzz: %ld iterations OK\n", iterations);
    return 0;
}
