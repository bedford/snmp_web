#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "thread_process.h"
#include "beep_thread.h"

#include "types.h"
#include "preference.h"

#include "common_type.h"
#include "drv_gpio.h"
#include "shm_object.h"
#include "semaphore.h"

/**
  @brief    蜂鸣器线程处理函数
  @param    beep_thread_param_t 线程参数对象指针
  @return

  通过读取共享内存中RS232串口数据、RS485串口数据和DI输入数据，
  如存在报警的状态，同时使能蜂鸣器报警的情况下，打开蜂鸣器报警
 */
static void *beep_process(void *arg)
{
	beep_thread_param_t *thread_param = (beep_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;
	preference_t *pref_handle = (preference_t *)thread_param->pref_handle;

	shm_object_t *rs232_shm_handle;
	shm_object_t *rs485_shm_handle;
	shm_object_t *di_shm_handle;

	int di_sem_id = semaphore_create(DI_KEY);
	int rs232_sem_id = semaphore_create(RS232_KEY);
	int rs485_sem_id = semaphore_create(RS485_KEY);

	uart_realdata_t *uart_realdata = NULL;
	int ret = 0;
	do {
		rs232_shm_handle = shm_object_create(RS232_SHM_KEY, sizeof(uart_realdata_t));
		if (rs232_shm_handle == NULL) {
			printf("create RS232 share memory queue failed\n");
			ret = -1;
			break;
		}

		rs485_shm_handle = shm_object_create(RS485_SHM_KEY, sizeof(uart_realdata_t));
		if (rs485_shm_handle == NULL) {
			printf("create RS485 share memory queue failed\n");
			ret = -1;
			break;
		}

		uart_realdata = calloc(1, sizeof(uart_realdata_t));
		if (uart_realdata == NULL) {
			printf("create uart realdata memory failed\n");
			ret = -1;
			break;
		}
	} while(0);

	di_realdata_t *di_realdata = NULL;
	do {
		di_shm_handle = shm_object_create(DI_SHM_KEY, sizeof(di_realdata_t));
		if (di_shm_handle == NULL) {
			printf("create DI share memory queue failed\n");
			ret = -1;
			break;
		}

		di_realdata = calloc(1, sizeof(di_realdata_t));
		if (di_realdata == NULL) {
			ret = -1;
			break;
		}
	} while(0);

	do_param_t do_param;
	int alarm_flag = 0;
	drv_gpio_open(DIGITAL_OUT_0);
	unsigned char last_value = 0;
	unsigned char value = 0;
	drv_gpio_write(DIGITAL_OUT_0, value);
	int i = 0;
	int index = 0;
	while (thiz->thread_status) {
		alarm_flag = 0;

		semaphore_p(di_sem_id);
		ret = di_shm_handle->shm_get(di_shm_handle, (void *)di_realdata);
		semaphore_v(di_sem_id);
		if (ret == 0) {
			for (i = 0; i < di_realdata->cnt; i++) {
                if (di_realdata->data[i].alarm_type == 1) {
                    alarm_flag = 1;
                }
			}
		}

		semaphore_p(rs232_sem_id);
		ret = rs232_shm_handle->shm_get(rs232_shm_handle, (void *)uart_realdata);
		semaphore_v(rs232_sem_id);
		if (ret == 0) {
			for (index = 0; index < uart_realdata->protocol_cnt; index++) {
				for (i = 0; i < uart_realdata->realdata[index].cnt; i++) {
					if (uart_realdata->realdata[index].data[i].alarm_type != 0) {
						alarm_flag = 1;
					}
				}
			}
        }

		semaphore_p(rs485_sem_id);
		ret = rs485_shm_handle->shm_get(rs485_shm_handle, (void *)uart_realdata);
		semaphore_v(rs485_sem_id);
		if (ret == 0) {
			for (index = 0; index < uart_realdata->protocol_cnt; index++) {
				for (i = 0; i < uart_realdata->realdata[index].cnt; i++) {
					if (uart_realdata->realdata[index].data[i].alarm_type != 0) {
						alarm_flag = 1;
					}
				}
			}
        }

		do_param = pref_handle->get_do_param(pref_handle);
		if (do_param.beep_enable) {
            if (last_value == 0) {
                if (alarm_flag) {
                    value = 1;
                    drv_gpio_write(DIGITAL_OUT_0, value);
                    last_value = value;
                }
            } else {
                if (alarm_flag == 0) {
				    value = 0;
				    drv_gpio_write(DIGITAL_OUT_0, value);
                    last_value = value;
                }
			}
		} else {
            if (last_value) {
			    value = 0;
			    drv_gpio_write(DIGITAL_OUT_0, value);
                last_value = value;
            }
		}
        sleep(2);
	}
	drv_gpio_close(DIGITAL_OUT_0);

	pref_handle = NULL;
	thiz = NULL;
	thread_param = NULL;

	return (void *)0;
}

/**
  @brief    创建蜂鸣器线程对象
  @return   线程对象指针
 */
thread_t *beep_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= beep_process;

		strcpy(thiz->thread_name, "beep_process");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= thread_destroy;
	}

	return thiz;
}
