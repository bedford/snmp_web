#ifndef _SMS_ALARM_THREAD_H
#define _SMS_ALARM_THREAD_H

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

	void 		*sms_rb_handle;
	void 		*alarm_pool_handle;
} sms_alarm_thread_param_t;

thread_t *sms_alarm_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
