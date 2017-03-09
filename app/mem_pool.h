#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mpool mem_pool_t;

typedef void *(*_mem_pool_alloc)(mem_pool_t *thiz);
typedef int (*_mem_pool_free)(mem_pool_t *thiz, void *buffer);
typedef void (*_mem_pool_destroy)(mem_pool_t *thiz);
typedef void (*_mem_pool_dump)(mem_pool_t *thiz);

struct _mpool
{
        _mem_pool_alloc         mpool_alloc;
        _mem_pool_free          mpool_free;
        _mem_pool_destroy       mpool_destroy;
        _mem_pool_dump          mpool_dump;

        char priv[1];
};

mem_pool_t *mem_pool_create(unsigned int buffer_size, unsigned int buffer_count);

#ifdef __cplusplus
}
#endif

#endif
