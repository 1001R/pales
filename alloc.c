#include "alloc.h"
#include <string.h>

#define DEFAULT_CAPACITY 8

typedef struct chunk {
	void *chunk;
	struct chunk *next;
} chunk_t;


struct pool {
	struct pool *parent;
	struct pool *first_child;
	struct pool *prev_sibling;
	struct pool *next_sibling;
	void **chunks;
	int clen;
	int ccap;
};

pool_t pool_create(pool_t parent)
{
	pool_t new_pool = (pool_t) malloc(sizeof(struct pool));
	if (new_pool == NULL) {
		return NULL;
	}
	memset(new_pool, 0, sizeof(*new_pool));
	new_pool->parent = parent;
	new_pool->chunks = malloc(sizeof(void*) * DEFAULT_CAPACITY);
	if (new_pool->chunks == NULL) {
		free(new_pool);
		return NULL;
	}
	new_pool->ccap = DEFAULT_CAPACITY;
	if (parent != NULL) {
		if (parent->first_child == NULL) {
			parent->first_child = new_pool;
		}
		else {
			pool_t p = parent->first_child;
			while (p->next_sibling != NULL) {
				p = p->next_sibling;
			}
			p->next_sibling = new_pool;
			new_pool->prev_sibling = p;
		}
	}
	return new_pool;
}

void *pool_malloc(pool_t pool, size_t len) {
	void *chunk;

	if (pool->clen == pool->ccap) {
		void **p;
		pool->ccap *= 2;
		p = realloc(pool->chunks, pool->ccap);
		if (p == NULL) {
			return NULL;
		}
		else {
			pool->chunks = p;
		}
	}
	chunk = malloc(len);
	if (chunk == NULL) {
		return NULL;
	}
	pool->chunks[pool->clen] = chunk;
	pool->clen++;
	return chunk;
}

static void pool_free_r(pool_t pool) {
	for (pool_t child = pool->first_child; child != NULL; child = child->next_sibling) {
		pool_free_r(child);
	}
	for (int i = 0; i < pool->clen; i++) {
		free(pool->chunks[i]);
	}
}

void pool_free(pool_t pool) {
	if (pool == pool->parent->first_child) {
		pool->parent->first_child = pool->next_sibling;
	}
	if (pool->prev_sibling != NULL) {
		pool->prev_sibling->next_sibling = pool->next_sibling;
	}
	if (pool->next_sibling != NULL) {
		pool->next_sibling->prev_sibling = pool->prev_sibling;
	}
}

