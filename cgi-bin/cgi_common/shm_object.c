#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#include "shm_object.h"

#pragma pack(1)

typedef struct {
    int32_t size;
    int32_t shm_key;
    void    *memory;
} priv_info_t;

#pragma pack()

static void *__get_shm(key_t key, size_t size, int flag)
{
        int shm_id = shmget(key, size, flag);
        if (shm_id < 0) {
                return NULL;
        }

        void *p = shmat(shm_id, NULL, 0);
        if (p == (void *)-1) {
                return NULL;
        }

        return p;
}

static int get_shm(key_t key, size_t size, void **addr)
{
        if ((*addr = __get_shm(key, size, 0666)) != NULL) {
                return 0;
        }

        if ((*addr = __get_shm(key, size, 0666 | IPC_CREAT)) != NULL) {
                return 1;
        }

        return -1;
}

static int shm_object_get(shm_object_t *thiz, void *unit)
{
    if (!thiz || !unit) {
        return -1;
    }

    priv_info_t *priv = (priv_info_t *)thiz->priv;

    /* 读数据  */
    memcpy(unit, priv->memory, priv->size);

    return 0;
}

static int shm_object_put(shm_object_t *thiz, void *unit)
{
    if (!thiz || !unit) {
        return -2;
    }

    priv_info_t *priv = (priv_info_t *)thiz->priv;

    /* 写数据  */
    memcpy(priv->memory, unit, priv->size);

    return 0;
}

static void shm_object_destroy(shm_object_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
        if (priv->memory) {
            if (priv->shm_key) {
                shmdt(priv->memory);
                priv->memory = NULL;
            }
        }

        memset(thiz, 0, sizeof(priv_info_t) + sizeof(shm_object_t));
        free(thiz);
        thiz = NULL;
    }
}

shm_object_t *shm_object_create(key_t shm_key, unsigned int size)
{
    if (!size || !shm_key) {
        return NULL;
    }

    shm_object_t *thiz = NULL;
    thiz = (shm_object_t *)calloc(1, sizeof(shm_object_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
        thiz->shm_put = shm_object_put;
        thiz->shm_get = shm_object_get;
        thiz->shm_destroy = shm_object_destroy;

        priv_info_t *priv = (priv_info_t *)thiz->priv;

        do {
            void *memory = NULL;
            bool old_shm = false;

            int ret = get_shm(shm_key, size, &memory);
            if (ret < 0) {
                break;
            } else if (ret == 0) {
                old_shm = true;
            }

            priv->memory    = memory;
            priv->shm_key   = shm_key;
            priv->size      = size;

            /* 如果是新内存则进行初始化 */
            if (!old_shm) {
                memset(memory, 0, size);
            }
        } while(0);
    }

    return thiz;
}
