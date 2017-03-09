#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ring_buffer.h"

typedef struct
{
        void *data;
} element_t;

typedef struct
{
        int             size;
        int             read_cursor;
        int             write_cursor;
        element_t       *element;
        pthread_mutex_t mutex;

} priv_info;

int ring_buffer_push(ring_buffer_t *thiz, void *data)
{
        int ret = -1;
        if ((thiz != NULL) && (data != NULL)) {
                priv_info *priv = (priv_info *)thiz->priv;
                pthread_mutex_lock(&priv->mutex);
                int write_cursor = (priv->write_cursor + 1) % priv->size;

                if (write_cursor != priv->read_cursor) {
                        priv->element[priv->write_cursor].data  = data;
                        priv->write_cursor                      = write_cursor;
                        ret = 0;
                }
                pthread_mutex_unlock(&priv->mutex);
        }

        return ret;
}

int ring_buffer_pop(ring_buffer_t *thiz, void **data)
{
        int ret = -1;
        if (thiz != NULL) {
                priv_info *priv = (priv_info *)thiz->priv;
                pthread_mutex_lock(&priv->mutex);
                if (priv->read_cursor != priv->write_cursor) {
                        *data = priv->element[priv->read_cursor].data;
                        priv->read_cursor =
                                (priv->read_cursor + 1) % priv->size;

                        ret = 0;
                }
                pthread_mutex_unlock(&priv->mutex);
        }

        return ret;
}

void ring_buffer_destroy(ring_buffer_t *thiz)
{
        if (thiz != NULL) {
                priv_info *priv = (priv_info *)thiz->priv;
                if (&priv->mutex) {
                        pthread_mutex_destroy(&priv->mutex);
                }

                memset(thiz, 0, sizeof(ring_buffer_t)
                                + sizeof(priv_info)
                                + priv->size * sizeof(element_t));
                free(thiz);
                thiz = NULL;
        }
}

ring_buffer_t *ring_buffer_create(int size)
{
        ring_buffer_t *thiz = NULL;
        thiz = (ring_buffer_t *)calloc(1, sizeof(ring_buffer_t)
                                        + sizeof(priv_info)
                                        + (size + 1) * sizeof(element_t));
        if (thiz != NULL) {
                thiz->pop       = ring_buffer_pop;
                thiz->push      = ring_buffer_push;
                thiz->destroy   = ring_buffer_destroy;

                priv_info *priv = (priv_info *)thiz->priv;
                priv->element   = (element_t *)((char *)thiz
                                                + sizeof(ring_buffer_t)
                                                + sizeof(priv_info));

                priv->size              = size + 1;
                priv->read_cursor       = 0;
                priv->write_cursor      = 0;
				pthread_mutex_init(&priv->mutex, NULL);
        }

        return thiz;
}
