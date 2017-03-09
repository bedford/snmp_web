#ifndef _RS485_THREAD_H
#define _RS485_THREAD_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	thread_t	*self;
	void 		*sys_db_handle;
} rs485_thread_param_t;

thread_t *rs485_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
