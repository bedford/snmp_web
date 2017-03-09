#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "thread_process.h"

void thread_terminate(thread_t *thiz)
{
        thiz->thread_status = 0;
}

int thread_start(thread_t *thiz, void *arg)
{
        int ret = -1;
        if ((thiz != NULL)
                && (thiz->thread_routine != NULL)) {
                printf("thread name : %s\n", thiz->thread_name);
                int ret = pthread_create(&(thiz->thread_ID),
                                         NULL,
                                         thiz->thread_routine,
                                         arg);
                if (ret == 0) {
                        thiz->thread_status = 1;
                        printf("create thread %s success, thread id %lx\n",
                                thiz->thread_name, thiz->thread_ID);
                        ret = 0;
                } else {
                        printf("create thread %s failed\n", thiz->thread_name);
                }
        }

        return ret;
}

void thread_join(thread_t *thiz)
{
        if (thiz->thread_status) {
                thiz->thread_status = 0;
        }

        if (thiz->thread_ID > 0) {
                pthread_join(thiz->thread_ID, NULL);
        }
}

void thread_destroy(thread_t *thiz)
{
        if (thiz != NULL) {
                memset(thiz, 0, sizeof(thread_t));
                free(thiz);
                thiz = NULL;
        }
}
