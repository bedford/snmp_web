#ifndef _THREAD_H_
#define _THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

struct _thread;
typedef struct _thread thread_t;

typedef void *(*_routine)(void *arg);

typedef int (*_thread_start)(thread_t *thiz, void *arg);
typedef void (*_thread_join)(thread_t *thiz);
typedef void (*_thread_destroy)(thread_t *thiz);
typedef void (*_thread_terminate)(thread_t *thiz);

struct _thread
{
        char                    thread_name[32];
        int                     thread_status;
        _routine                thread_routine;
        pthread_t               thread_ID;

        _thread_join            join;
        _thread_start           start;
        _thread_terminate       terminate;
        _thread_destroy         destroy;

        char                    priv[1];
};

#ifdef __cplusplus
}
#endif

#endif
