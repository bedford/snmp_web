#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "thread_process.h"
#include "data_write_thread.h"
#include "db_access.h"

#include "types.h"
#include "mem_pool.h"
#include "ring_buffer.h"

static void clear_ring_buffer(db_access_t	*data_db_handle,
							  ring_buffer_t *rb_handle,
							  mem_pool_t 	*mpool_handle)
{
	char error_msg[512] = {0};
	msg_t *msg = NULL;
	while (1) {
		if (rb_handle->pop(rb_handle, (void **)&msg) == 0) {
			memset(error_msg, 0, sizeof(error_msg));
			data_db_handle->action(data_db_handle, msg->buf, error_msg);
			mpool_handle->mpool_free(mpool_handle, (void *)msg);
			msg = NULL;
		} else {
			break;
		}
	}
}

static void *data_in_db_process(void *arg)
{
	data_write_thread_param_t *thread_param = (data_write_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	db_access_t *data_db_handle = (db_access_t *)thread_param->data_db_handle;
	ring_buffer_t *rb_handle = (ring_buffer_t *)thread_param->rb_handle;
	mem_pool_t	*mpool_handle = (mem_pool_t *)thread_param->mpool_handle;

	msg_t *msg = NULL;
	char error_msg[512] = {0};
	while (thiz->thread_status) {
		if (rb_handle->pop(rb_handle, (void **)&msg)) {
			usleep(50000);
			continue;
		}

		memset(error_msg, 0, sizeof(error_msg));
		data_db_handle->action(data_db_handle, msg->buf, error_msg);

		mpool_handle->mpool_free(mpool_handle, (void *)msg);
		msg = NULL;
	}

	clear_ring_buffer(data_db_handle, rb_handle, mpool_handle);
	data_db_handle = NULL;
	rb_handle = NULL;
	mpool_handle = NULL;
	thiz = NULL;
	thread_param = NULL;

	return (void *)0;
}

thread_t *data_write_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= data_in_db_process;

		strcpy(thiz->thread_name, "data_write_process");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= thread_destroy;
	}

	return thiz;
}
