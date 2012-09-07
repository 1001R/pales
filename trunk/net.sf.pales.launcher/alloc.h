#ifndef ALLOC_H
#define ALLOC_H

#include <stdlib.h>

typedef struct pool *pool_t;

pool_t pool_create(pool_t parent);
void *pool_malloc(pool_t pool, size_t len);
void pool_free(pool_t pool);

#endif
