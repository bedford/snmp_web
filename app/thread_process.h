#ifndef _THREAD_PROCESS_H_
#define _THREAD_PROCESS_H_

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

void thread_terminate(thread_t *thiz);
void thread_destroy(thread_t *thiz);
void thread_join(thread_t *thiz);
int thread_start(thread_t *thiz, void *arg);

#ifdef __cplusplus
}
#endif

#endif
