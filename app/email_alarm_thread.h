#ifndef _EMAIL_ALARM_THREAD_H
#define _EMAIL_ALARM_THREAD_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	thread_t	*self;
	void 		*sys_db_handle;
	void 		*pref_handle;

	void 		*rb_handle;
	void 		*mpool_handle;

	void 		*email_rb_handle;
	void 		*alarm_pool_handle;
} email_alarm_thread_param_t;

thread_t *email_alarm_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
