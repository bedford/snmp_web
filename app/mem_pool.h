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

/**
 * @brief   声明内存池类
 */
struct _mpool
{
        _mem_pool_alloc         mpool_alloc;
        _mem_pool_free          mpool_free;
        _mem_pool_destroy       mpool_destroy;
        _mem_pool_dump          mpool_dump;

        char priv[1];
};

/**
 * @brief   mem_pool_create 内存池对象创建
 * @param   buffer_size     单个内存大小
 * @param   buffer_count    内存块数量
 * @return
 */
mem_pool_t *mem_pool_create(unsigned int buffer_size, unsigned int buffer_count);

#ifdef __cplusplus
}
#endif

#endif
