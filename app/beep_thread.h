#ifndef _BEEP_THREAD_H
#define _BEEP_THREAD_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  @brief    蜂鸣器线程参数对象

  包含对象本身的指针及配置文件对象指针
 */
typedef struct
{
	thread_t	*self;
	void 		*pref_handle;
} beep_thread_param_t;

/**
  @brief    创建蜂鸣器线程对象
  @return   线程对象指针
 */
thread_t *beep_thread_create(void);

#ifdef __cplusplus
}
#endif

#endif
