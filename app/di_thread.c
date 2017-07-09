#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "protocol_interfaces.h"

#include "thread_process.h"
#include "di_thread.h"
#include "debug.h"
#include "db_access.h"
#include "preference.h"

#include "drv_gpio.h"
#include "types.h"
#include "ring_buffer.h"
#include "mem_pool.h"

#include "common_type.h"
#include "shm_object.h"
#include "semaphore.h"

#define MAX_DI_NUMBER	(4)
#define MAX_PARAM_LEN	(128)
#define MIN_PARAM_LEN	(32)

enum {
	NONE		= 0,
	SMS_ALARM 	= 1,
	EMAIL_ALARM	= 2,
	BOTH		= 3
};

typedef struct {
	char			di_name[MIN_PARAM_LEN];		/* DI名称 */
	char			di_desc[MIN_PARAM_LEN];
	char			device_name[MIN_PARAM_LEN];
	char			low_desc[MIN_PARAM_LEN]; 	/* 低电平描述 */
	char			high_desc[MIN_PARAM_LEN]; 	/* 高电平描述 */
	unsigned int	id;							/* DI编号 0, 1, 2, 3*/
	unsigned int	enable;						/* 是否使能 */
	unsigned int	alarm_level;				/* 报警电平 */
	unsigned int	alarm_method;				/* 报警方式 */
	unsigned int	alarm_status;				/* 状态 */
	unsigned int	last_status;				/* 上一个状态 */
} di_param_t;

typedef struct {
	db_access_t		*sys_db_handle;
	ring_buffer_t	*rb_handle;
	mem_pool_t		*mpool_handle;
	preference_t	*pref_handle;

	ring_buffer_t	*sms_rb_handle;
	ring_buffer_t	*email_rb_handle;
	mem_pool_t		*alarm_pool_handle;

	di_realdata_t	*di_realdata;

	int				sem_id;
	shm_object_t	*shm_handle;

	struct timeval	last_record_time;
	di_param_t		di_param[4];
} priv_info_t;

static void alarm_data_record(priv_info_t *priv, int index, unsigned char value)
{
	msg_t *msg = NULL;
	di_param_t *param = &(priv->di_param[index]);
	msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
	if (msg == NULL) {
		printf("memory pool is empty\n");
		return;
	}

	struct timeval now_time;
	struct tm *tm = NULL;
	gettimeofday(&now_time, NULL);
    tm = localtime(&(now_time.tv_sec));

	char alarm_desc[128] = {0};
	if (value == param->alarm_level) {
		sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]DI:%s%s告警",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec, param->device_name,
			(value == 1) ? "高电平" : "低电平");
		priv->di_realdata->data[index].alarm_type = 1;
		param->alarm_status = 1;
	} else {
		sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]DI:%s%s告警解除",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec, param->device_name,
			(value == 1) ? "低电平" : "高电平");
		priv->di_realdata->data[index].alarm_type = 0;
		param->alarm_status = 0;
	}

    sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
		param_name, param_desc, param_type, analog_value, unit, enum_value, enum_en_desc, enum_cn_desc, alarm_desc) \
		VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s', '%s')", "alarm_record",
		LOCAL_DI, "DI", param->di_desc, param->id, param->di_name, param->device_name,
		PARAM_TYPE_ENUM, 0.0, "", value, (value == 1) ? "high" : "low",
		(value == 1) ? param->high_desc : param->low_desc, alarm_desc);

	if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
		printf("ring buffer is full\n");
		priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
	}
	msg = NULL;

	alarm_msg_t *alarm_msg = (alarm_msg_t *)priv->alarm_pool_handle->mpool_alloc(priv->alarm_pool_handle);
	if (alarm_msg == NULL) {
		printf("memory pool is empty\n");
		return;
	}

	alarm_msg_t *tmp_alarm_msg = (alarm_msg_t *)priv->alarm_pool_handle->mpool_alloc(priv->alarm_pool_handle);
	if (tmp_alarm_msg == NULL) {
		printf("memory pool is empty\n");
		priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
		return;
	}

    if (value == param->alarm_level) {
		alarm_msg->alarm_type = ALARM_RAISE;
	} else {
		alarm_msg->alarm_type = ALARM_DISCARD;
	}
	alarm_msg->protocol_id = LOCAL_DI;
	strcpy(alarm_msg->protocol_name, "DI");
	strcpy(alarm_msg->protocol_desc, "干接点输入");
	alarm_msg->param_id = param->id;
	strcpy(alarm_msg->param_name, param->di_name);
	strcpy(alarm_msg->param_desc, param->device_name);
	strcpy(alarm_msg->param_unit, "");
	alarm_msg->param_type = PARAM_TYPE_ENUM;
	alarm_msg->param_value = 0.0;
	alarm_msg->enum_value = value;
	strcpy(alarm_msg->enum_desc, (value == 1) ? param->high_desc : param->low_desc);
	strcpy(alarm_msg->alarm_desc, alarm_desc);
	memcpy(tmp_alarm_msg, alarm_msg, sizeof(alarm_msg_t));

	if (priv->sms_rb_handle->push(priv->sms_rb_handle, (void *)alarm_msg)) {
		printf("sms ring buffer is full\n");
		priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
	}
	alarm_msg = NULL;

	if (priv->email_rb_handle->push(priv->email_rb_handle, (void *)tmp_alarm_msg)) {
		printf("email ring buffer is full\n");
		priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)tmp_alarm_msg);
	}
	tmp_alarm_msg = NULL;
}

static void history_data_record(priv_info_t *priv, int index, unsigned char value)
{
	msg_t *msg = NULL;
	di_param_t *param = &(priv->di_param[index]);
	msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
	if (msg == NULL) {
		printf("memory pool is empty\n");
		return;
	}

	char tmp[32] = {0};
	sprintf(tmp, "di%d", index);
	sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
		param_name, param_desc, param_type, analog_value, unit, enum_value, enum_en_desc, enum_cn_desc) \
		VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s')", "data_record",
		LOCAL_DI, "DI", param->di_desc, param->id, param->di_name, param->device_name,
		PARAM_TYPE_ENUM, 0.0, "", value, tmp, (value == 1) ? param->high_desc : param->low_desc);

	if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
		printf("ring buffer is full\n");
		priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
	}
	msg = NULL;
}

static void update_di_param(priv_info_t *priv)
{
	char sql[256] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s order by id", "di_cfg");
	memset(&query_result, 0, sizeof(query_result_t));
	priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);

	di_param_t *param = NULL;
	int i = 0;
	if (query_result.row > 0) {
		for (i = 0; i < 4; i++) {
			param = &(priv->di_param[i]);
			param->id = atoi(query_result.result[(i + 1) * query_result.column]);
			strcpy(param->di_name, query_result.result[(i + 1) * query_result.column + 1]);
			strcpy(param->di_desc, query_result.result[(i + 1) * query_result.column + 2]);
			strcpy(param->device_name, query_result.result[(i + 1) * query_result.column + 3]);
			strcpy(param->low_desc, query_result.result[(i + 1) * query_result.column + 4]);
			strcpy(param->high_desc, query_result.result[(i + 1) * query_result.column + 5]);
			param->alarm_level = atoi(query_result.result[(i + 1) * query_result.column + 6]);
			param->enable = atoi(query_result.result[(i + 1) * query_result.column + 7]);
			param->alarm_method = atoi(query_result.result[(i + 1) * query_result.column + 8]);
			printf("device_name %s, low_desc %s, high_desc %s, alarm_level %d, alarm_method %d, enable %d\n",
			param->device_name, param->low_desc, param->high_desc, param->alarm_level,
			param->alarm_method, param->enable);
		}
	}

	priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);

	param = NULL;
}

static void update_di_realdata(priv_info_t *priv, element_data_t *di_data, int index, unsigned char value)
{
	di_param_t *param = &(priv->di_param[index]);

	di_data->protocol_id = LOCAL_DI;
	strcpy(di_data->protocol_name, "DI");
	strcpy(di_data->protocol_desc, "干接点输入");
	di_data->param_id = param->id;
	strcpy(di_data->param_name, param->di_name);
	strcpy(di_data->param_desc, param->device_name);
	strcpy(di_data->param_unit, "");
	di_data->param_type = PARAM_TYPE_ENUM;
	di_data->analog_value = 0.0;
	di_data->enum_value = value;
	strcpy(di_data->enum_cn_desc, (value == 1) ? param->high_desc : param->low_desc);
	strcpy(di_data->enum_en_desc, "");
	if (param->enable) {
		if (value == param->alarm_level) {
			di_data->alarm_type = 1;
		} else {
			di_data->alarm_type = 0;
		}
	} else {
		di_data->alarm_type = 0;
	}
}

static void *di_process(void *arg)
{
	di_thread_param_t *thread_param = (di_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->sys_db_handle = (db_access_t *)thread_param->sys_db_handle;
	priv->rb_handle = (ring_buffer_t *)thread_param->rb_handle;
	priv->mpool_handle = (mem_pool_t *)thread_param->mpool_handle;
	priv->pref_handle = (preference_t *)thread_param->pref_handle;

	priv->sms_rb_handle = (ring_buffer_t *)thread_param->sms_rb_handle;
	priv->email_rb_handle = (ring_buffer_t *)thread_param->email_rb_handle;
	priv->alarm_pool_handle = (mem_pool_t *)thread_param->alarm_pool_handle;

	char sql[256] = {0};
	query_result_t query_result;

	char buf[256] = {0};
	int update_di_param_flag = 0;
	update_di_param(priv);

	int index = 0;
	di_param_t *param = NULL;
	unsigned char value = 0;
	for (index = 0; index < MAX_DI_NUMBER; index++) {
		param = &(priv->di_param[index]);
		drv_gpio_open(index);
		drv_gpio_read(index, &value);
		if (param->enable) {
			history_data_record(priv, index, value);
			param->last_status = value;

			if (value == param->alarm_level) {
				alarm_data_record(priv, index, value);
			}
		}
	}
	gettimeofday(&(priv->last_record_time), NULL);

	priv->shm_handle = shm_object_create(DI_SHM_KEY, sizeof(di_realdata_t));
	int ret = 0;
	if (priv->shm_handle == NULL) {
		ret = -1;
	}

	struct timeval current_time;
	int timeout_record_flag = 0;
	element_data_t *di_data = NULL;
	while (thiz->thread_status) {
		if (update_di_param_flag) {
			update_di_param(priv);
		}

		timeout_record_flag = 0;
		gettimeofday(&current_time, NULL);
		if ((current_time.tv_sec - priv->last_record_time.tv_sec) > (60 * 30)) {
			priv->last_record_time = current_time;
			timeout_record_flag = 1;
		}

		for (index = 0; index < MAX_DI_NUMBER; index++) {
			param = &(priv->di_param[index]);
			drv_gpio_read(index, &value);
			di_data = &(priv->di_realdata->data[index]);
			if (param->enable) {
				if ((value == param->alarm_level) && (param->alarm_status == 0)) {
					alarm_data_record(priv, index, value);
				}

				if ((value != param->alarm_level) && (param->alarm_status == 1)) {
					alarm_data_record(priv, index, value);
				}
				//printf("line %d, func %s, now %d\n", __LINE__, __func__, value);
			}
			update_di_realdata(priv, di_data, index, value);

			if (param->enable) {
				if (timeout_record_flag) {
					history_data_record(priv, index, value);
				} else {
					if (param->last_status != value) {
						history_data_record(priv, index, value);
					}
				}
			}
			param->last_status = value;
		}
		priv->di_realdata->cnt = 4;

		semaphore_p(priv->sem_id);
		priv->shm_handle->shm_put(priv->shm_handle, (void *)(priv->di_realdata));
		semaphore_v(priv->sem_id);

		if (update_di_param_flag) {
			update_di_param_flag = 0;
			priv->pref_handle->set_di_alarm_flag(priv->pref_handle, 0);
		} else {
			update_di_param_flag = priv->pref_handle->get_di_alarm_flag(priv->pref_handle);
		}
		sleep(1);
	}

	for (index = 0; index < MAX_DI_NUMBER; index++) {
		drv_gpio_close(index);
	}

	priv->shm_handle->shm_destroy(priv->shm_handle);

	return (void *)0;
}

static void di_thread_destroy(thread_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
		if (priv->di_realdata) {
			memset(priv->di_realdata, 0, sizeof(di_realdata_t));
			priv->di_realdata = NULL;
		}

        memset(thiz, 0, sizeof(thread_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

thread_t *di_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t) + sizeof(priv_info_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= di_process;

		strcpy(thiz->thread_name, "di_process");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= di_thread_destroy;

		priv_info_t *priv = (priv_info_t *)thiz->priv;
		priv->di_realdata = calloc(1, sizeof(di_realdata_t));
		if (priv->di_realdata == NULL) {
			di_thread_destroy(thiz);
			thiz = NULL;
		}
		priv->sem_id = semaphore_create(DI_KEY);
	}

	return thiz;
}
