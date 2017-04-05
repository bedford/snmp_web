#ifndef _DATA_WRITE_THREAD_H
#define _DATA_WRITE_THREAD_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	thread_t	*self;
	void 		*data_db_handle;
	void 		*rb_handle;
	void 		*mpool_handle;
} data_write_thread_param_t;

thread_t *data_write_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
