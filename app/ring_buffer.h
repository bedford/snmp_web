#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

struct _ring_buffer;
typedef struct _ring_buffer ring_buffer_t;

typedef int (*_ring_buffer_pop)(ring_buffer_t *thiz, void **data);
typedef int (*_ring_buffer_push)(ring_buffer_t *thiz, void *data);
typedef void (*_ring_buffer_destroy)(ring_buffer_t *thiz);

struct _ring_buffer
{
        _ring_buffer_pop        pop;
        _ring_buffer_push       push;
        _ring_buffer_destroy    destroy;

        char priv[1];
};

ring_buffer_t *ring_buffer_create(int size);

#ifdef __cplusplus
}
#endif

#endif
