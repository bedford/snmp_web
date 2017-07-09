#ifndef _BEEP_THREAD_H
#define _BEEP_THREAD_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	thread_t	*self;
	void 		*pref_handle;
} beep_thread_param_t;

thread_t *beep_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
