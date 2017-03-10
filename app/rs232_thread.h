#ifndef _RS232_THREAD_H
#define _RS232_THREAD_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	thread_t	*self;
	void 		*sys_db_handle;
	void 		*rb_handle;
	void 		*mpool_handle;
	int			init_flag;
} rs232_thread_param_t;

thread_t *rs232_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif