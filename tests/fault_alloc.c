#include "fault_alloc.h"

#include <stdlib.h>

/* M0: plain forwarding only. The fault injector (fault_alloc_arm,
 * fault_alloc_disarm, fault_alloc_total) is implemented in Mutation 1. */

void *rb_malloc(size_t n)
{
    return malloc(n);
}

void rb_free(void *p)
{
    free(p);
}
