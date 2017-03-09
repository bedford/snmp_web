#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "dll.h"
#include "mem_pool.h"

#define MAX_MEMORY_POOL_NUM  36
#define MAX_MEMORY_POOL_SIZE (1024 * 1024 * 10)

static const int MPOOL_BUFFER_ALIGN_BIT = 3;
static const int MPOOL_NODE_ALIGN_BIT   = 2;

typedef struct
{
        dll_node_t      node;
        unsigned int    addr;
        unsigned int    in_use;
} mpool_node_t;

typedef struct
{
        unsigned int    start_addr;
        unsigned int    end_addr;
        unsigned int    buffer_size;
        unsigned int    buffer_count;

        dll_t           free_buffer;
        mpool_node_t    *p_node;
        pthread_mutex_t mutex;

} priv_info;

static void *pool_mem_alloc(unsigned int alignment_in_bits, unsigned int size)
{
        unsigned int size_required = size + (1 << alignment_in_bits) - 1;
        unsigned int mem = (unsigned int)calloc(1, size_required);
        mem &= ~((1 << alignment_in_bits) - 1);

        if ((mem & ((1 << alignment_in_bits) - 1)) != 0) {
                free((void *)mem);
                mem = (unsigned int)NULL;
        }

        return (void *)mem;
}

static int mpool_init(priv_info *priv,
                      unsigned int buffer_size,
                      unsigned int buffer_count)
{
        priv->start_addr =
                (unsigned int)pool_mem_alloc(MPOOL_BUFFER_ALIGN_BIT,
                                                buffer_size * buffer_count);
        if (0 == priv->start_addr) {
                return -1;
        }

        dll_init(&priv->free_buffer);

        mpool_node_t *p_node =
                (mpool_node_t *)pool_mem_alloc(MPOOL_NODE_ALIGN_BIT,
                                        sizeof(mpool_node_t) * buffer_count);
        if (0 == p_node) {
                return -1;
        }

        priv->p_node = p_node;

        unsigned int index;
        unsigned int buffer_start_addr = priv->start_addr;
        for (index = 0; index < buffer_count; index++) {
                p_node->addr = buffer_start_addr;
                p_node->in_use = 0;
                dll_push_tail(&priv->free_buffer, &p_node->node);
                p_node++;
                buffer_start_addr += buffer_size;
        }

        priv->buffer_size = buffer_size;
        priv->buffer_count = buffer_count;
        priv->end_addr = priv->start_addr + buffer_size * buffer_count;

        return 0;
}

static void *mpool_alloc(mem_pool_t *thiz)
{
        void *ret = 0;
        if (thiz != NULL) {
                priv_info *priv = (priv_info *)thiz->priv;
                mpool_node_t *p_node = 0;
                pthread_mutex_lock(&priv->mutex);
                p_node = (mpool_node_t *)dll_pop_head(&priv->free_buffer);
                pthread_mutex_unlock(&priv->mutex);

                if (0 != p_node) {
                        p_node->in_use = 1;
                        ret = (void *)p_node->addr;
                        priv = 0;
                        p_node = 0;
                }
        }

        return ret;
}

static int mpool_free(mem_pool_t *thiz, void *p_buf)
{
        unsigned int free_addr = (unsigned int)p_buf;
        priv_info *priv = (priv_info *)thiz->priv;

        if ((free_addr < priv->start_addr)
                        || (free_addr > priv->end_addr)) {
                return -1;
        }
        free_addr -= priv->start_addr;

        unsigned int index;
        if ((free_addr % priv->buffer_size) == 0) {
                index = free_addr / priv->buffer_size;
        } else {
                return -1;
        }

        if (priv->p_node[index].addr != (unsigned int)p_buf) {
                return -1;
        }

        pthread_mutex_lock(&priv->mutex);
        if (priv->p_node[index].in_use != 1) {
                pthread_mutex_unlock(&priv->mutex);
                priv = NULL;
                return -1;
        }

        priv->p_node[index].in_use = 0;
        dll_push_tail(&priv->free_buffer, &priv->p_node[index].node);
        pthread_mutex_unlock(&priv->mutex);
        priv = NULL;

        return 0;
}

static void mpool_dump(mem_pool_t *thiz)
{
        if (thiz != NULL) {
                priv_info *priv = (priv_info *)thiz->priv;
                printf ("\n");
                printf ("Summary\n");
                printf ("-------\n");
                pthread_mutex_lock(&priv->mutex);
                printf ("     Buffer Size: %u\n", priv->buffer_size);
                printf ("  Used Buffer(s): %u\n", priv->buffer_count - dll_size(&priv->free_buffer));
                printf ("   Total Buffers: %u\n", priv->buffer_count);
                pthread_mutex_unlock(&priv->mutex);
                printf ("\n");
                priv = NULL;
        }
}

static void mpool_release(mem_pool_t *thiz)
{
        if (thiz != NULL) {
                priv_info *priv = (priv_info *)thiz->priv;
                if (priv->p_node) {
                        memset(priv->p_node, 0, sizeof(mpool_node_t) * priv->buffer_count);
                        free(priv->p_node);
                        priv->p_node = 0;
                }

                if (priv->start_addr) {
                        memset((void *)(priv->start_addr), 0,
                                priv->buffer_size * priv->buffer_count);
                        free((void *)(priv->start_addr));
                        priv->start_addr = 0;
                        priv->end_addr = 0;
                }
                priv = NULL;
        }
}

static void mpool_destroy(mem_pool_t *thiz)
{
        if (thiz != NULL) {
                priv_info *priv = (priv_info *)thiz->priv;
                mpool_release(thiz);

                if (&priv->mutex) {
                        pthread_mutex_destroy(&priv->mutex);
                }

                memset(thiz, 0, sizeof(mem_pool_t) + sizeof(priv_info));
                free(thiz);
                thiz = NULL;
        }
}

mem_pool_t *mem_pool_create(unsigned int buffer_size, unsigned int buffer_count)
{
        if ((buffer_size < sizeof(unsigned int))
                || (buffer_size > MAX_MEMORY_POOL_SIZE)
                || (buffer_count == 0)
                || (buffer_count > MAX_MEMORY_POOL_NUM)) {
                return NULL;
        }

        if (0 != (buffer_size & ((1 << MPOOL_BUFFER_ALIGN_BIT) - 1))) {
                return NULL;
        }

        mem_pool_t *thiz = NULL;
        thiz = (mem_pool_t *)calloc(1, sizeof(mem_pool_t) + sizeof(priv_info));
        if (thiz != NULL) {
                thiz->mpool_alloc       = mpool_alloc;
                thiz->mpool_free        = mpool_free;
                thiz->mpool_destroy     = mpool_destroy;
                thiz->mpool_dump        = mpool_dump;

                priv_info *priv = (priv_info *)thiz->priv;
                if (mpool_init(priv, buffer_size, buffer_count) < 0) {
                        mpool_destroy(thiz);
                        thiz = NULL;
                } else {
                        pthread_mutex_init(&priv->mutex, NULL);
                }
        }

        return thiz;
}
