#ifndef _SHM_OBJECT_H_
#define _SHM_OBJECT_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _shm_object shm_object_t;

typedef int (*_shm_object_put)(shm_object_t *thiz, void *object);
typedef int (*_shm_object_get)(shm_object_t *thiz, void *object);
typedef void (*_shm_object_destroy)(shm_object_t *thiz);

struct _shm_object
{
    _shm_object_put     shm_put;
    _shm_object_get     shm_get;
    _shm_object_destroy shm_destroy;

    char priv[1];
};

shm_object_t *shm_object_create(key_t shm_key, unsigned int size);

# ifdef __cplusplus
}
# endif

#endif