#ifndef _DI_THREAD_H
#define _DI_THREAD_H

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
	void 		*pref_handle;

	void 		*sms_rb_handle;
	void 		*email_rb_handle;
	void 		*alarm_pool_handle;

	int 		*alarm_cnt;
	int			init_flag;
} di_thread_param_t;

thread_t *di_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
