#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "thread_process.h"

/**
 * @brief   thread_terminate    结束线程
 * @param   thiz
 */
void thread_terminate(thread_t *thiz)
{
        thiz->thread_status = 0;
}

/**
 * @brief   thread_start    启动线程 
 * @param   thiz
 * @param   arg
 * @return
 */
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

/**
 * @brief   thread_join 等待线程结束 
 * @param   thiz
 */
void thread_join(thread_t *thiz)
{
        if (thiz->thread_status) {
                thiz->thread_status = 0;
        }

        if (thiz->thread_ID > 0) {
                pthread_join(thiz->thread_ID, NULL);
        }
}

/**
 * @brief   thread_destroy  销毁线程对象 
 * @param   thiz
 */
void thread_destroy(thread_t *thiz)
{
        if (thiz != NULL) {
                memset(thiz, 0, sizeof(thread_t));
                free(thiz);
                thiz = NULL;
        }
}
