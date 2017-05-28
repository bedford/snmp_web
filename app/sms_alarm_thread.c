#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "thread_process.h"
#include "sms_alarm_thread.h"
#include "db_access.h"
#include "preference.h"

#include "types.h"
#include "mem_pool.h"
#include "ring_buffer.h"

#include "modem.h"

#define MAX_QUEUE_NUMBER	(16)
#define MAX_SMS_USER_NUM	(10)

typedef struct {
	char	name[32];
	char	phone_num[16];
} sms_contact_t;

typedef struct {
	int				sms_contact_cnt;
	sms_contact_t	sms_user_array[MAX_SMS_USER_NUM];
	modem_t			*modem;

	int				send_times;
	int				send_interval;

	preference_t	*pref_handle;
	db_access_t 	*sys_db_handle;

	alarm_msg_t		*alarm_queue[MAX_QUEUE_NUMBER];
	struct timeval	last_send_timing[MAX_QUEUE_NUMBER];
	mem_pool_t		*local_mpool_handle;

	ring_buffer_t 	*rb_handle;
	mem_pool_t		*mpool_handle;

	ring_buffer_t 	*sms_rb_handle;
	mem_pool_t 		*alarm_pool_handle;
} priv_info_t;

static void clear_ring_buffer(ring_buffer_t *rb_handle,
							  mem_pool_t 	*mpool_handle)
{
	char error_msg[512] = {0};
	alarm_msg_t *msg = NULL;
	while (1) {
		if (rb_handle->pop(rb_handle, (void **)&msg) == 0) {
			memset(error_msg, 0, sizeof(error_msg));
			mpool_handle->mpool_free(mpool_handle, (void *)msg);
			msg = NULL;
		} else {
			break;
		}
	}
}

static void update_queue(priv_info_t *priv, alarm_msg_t *msg)
{
	int i = 0;
	for (i = 0; i < MAX_QUEUE_NUMBER; i++) {
		if (priv->alarm_queue[i] == NULL) {
			priv->alarm_queue[i] = msg;
			priv->last_send_timing[i].tv_sec = 0;
			priv->last_send_timing[i].tv_usec = 0;
			break;
		}
	}
}

static void update_alarm_table(priv_info_t *priv, alarm_msg_t *alarm_msg)
{
	int i = 0;
	alarm_msg_t *msg = NULL;
	for (i = 0; i < MAX_QUEUE_NUMBER; i++) {
		msg = priv->alarm_queue[i];
		if (msg == NULL) {
			continue;
		}

		if ((msg->protocol_id == alarm_msg->protocol_id)
			&& (msg->param_id == alarm_msg->param_id)) {
			memcpy(msg, alarm_msg, sizeof(alarm_msg_t));
            if (alarm_msg->alarm_type == ALARM_DISCARD) {
			    msg->send_times = 1;
            }
			priv->last_send_timing[i].tv_sec = 0;
			priv->last_send_timing[i].tv_usec = 0;
			break;
		}
	}

	if (i == MAX_QUEUE_NUMBER) {
		msg = (alarm_msg_t *)priv->local_mpool_handle->mpool_alloc(priv->local_mpool_handle);
		if (msg == NULL) {
			printf("function %s, line %d, memory pool is empty\n", __func__, __LINE__);
		} else {
			memcpy(msg, alarm_msg, sizeof(alarm_msg_t));
            if (alarm_msg->alarm_type == ALARM_DISCARD) {
			    msg->send_times = 1;
            } else {
			    msg->send_times = priv->send_times;
            }

			update_queue(priv, msg);
			msg = NULL;
		}
	}
}

static void update_sms_contact(priv_info_t *priv)
{
	priv->sms_contact_cnt = 0;

	char error_msg[512] = {0};
	char sql[512] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s order by id", "phone_user");
	memset(&query_result, 0, sizeof(query_result_t));
	priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);

	if (query_result.row > 0) {
		int i = 0;
		for (i = 1; i < (query_result.row + 1); i++) {
			strcpy(priv->sms_user_array[priv->sms_contact_cnt].name,
				query_result.result[i * query_result.column + 1]);
			strcpy(priv->sms_user_array[priv->sms_contact_cnt].phone_num,
				query_result.result[i * query_result.column + 2]);
			priv->sms_contact_cnt++;
		}
	}
	priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);
}

static void send_to_contact(priv_info_t *priv, alarm_msg_t *alarm_msg)
{
	char sca_code[32] = {0};
	if (priv->sms_contact_cnt <= 0) {
		return;
	}

	int ret = -1;
	do {
		if (priv->modem->connected(priv->modem)) {
			printf("conntected modem failed\n");
			break;
		}

		if (priv->modem->get_sca(priv->modem, sca_code)) {
			printf("get sca failed\n");
			break;
		}

		if (priv->modem->set_mode(priv->modem, SMS_MODE_PDU)) {
			printf("set pdu mode failed\n");
			break;
		}
		ret = 0;
	} while(0);

	int i = 0;
	msg_t *msg = NULL;
	char sms_content[256] = {0};
	sprintf(sms_content, "%s, %s", alarm_msg->protocol_name, alarm_msg->alarm_desc);
	for (i = 0; i < priv->sms_contact_cnt; i++) {
		msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
		if (msg == NULL) {
			printf("memory pool is empty\n");
			continue;
		}

		if (ret == 0) {
			if (priv->modem->send_sms(priv->modem,
									priv->sms_user_array[i].phone_num,
									sms_content) == 0) {
		        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
					param_name, param_desc, param_type, analog_value, enum_value, enum_desc, name, phone, send_status, sms_content) \
					VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, %d, '%s', '%s', '%s', %d, '%s')", "sms_record",
		                alarm_msg->protocol_id, alarm_msg->protocol_name, alarm_msg->protocol_desc,
		                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
						alarm_msg->param_value, alarm_msg->enum_value, alarm_msg->enum_desc,
						priv->sms_user_array[i].name, priv->sms_user_array[i].phone_num,
						0, sms_content);
			} else {
		        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
					param_name, param_desc, param_type, analog_value, enum_value, enum_desc, name, phone, send_status, sms_content) \
					VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, %d, '%s', '%s', '%s', %d, '%s')", "sms_record",
		                alarm_msg->protocol_id, alarm_msg->protocol_name, alarm_msg->protocol_desc,
		                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
						alarm_msg->param_value, alarm_msg->enum_value, alarm_msg->enum_desc,
						priv->sms_user_array[i].name, priv->sms_user_array[i].phone_num,
						1, sms_content);
			}
		} else {
	        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
				param_name, param_desc, param_type, analog_value, enum_value, enum_desc, name, phone, send_status, sms_content) \
				VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, %d, '%s', '%s', '%s', %d, '%s')", "sms_record",
	                alarm_msg->protocol_id, alarm_msg->protocol_name, alarm_msg->protocol_desc,
	                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
					alarm_msg->param_value, alarm_msg->enum_value, alarm_msg->enum_desc,
					priv->sms_user_array[i].name, priv->sms_user_array[i].phone_num,
					1, sms_content);
		}

		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;
	}
}

static void send_alarm_sms(priv_info_t *priv)
{
    struct timeval current_time;
	gettimeofday(&current_time, NULL);

	int i = 0;
	alarm_msg_t *msg = NULL;
	for (i = 0; i < MAX_QUEUE_NUMBER; i++) {
		msg = priv->alarm_queue[i];
		if (msg == NULL) {
			continue;
		}

		if ((priv->last_send_timing[i].tv_sec == 0) && (priv->last_send_timing[i].tv_usec == 0)) {
			send_to_contact(priv, msg);
			priv->last_send_timing[i] = current_time;
			msg->send_times--;
		} else if ((current_time.tv_sec - priv->last_send_timing[i].tv_sec) > (priv->send_interval * 5 * 60)) {
			send_to_contact(priv, msg);
			priv->last_send_timing[i] = current_time;
			msg->send_times--;
		}

		if (msg->send_times <= 0) {
			priv->local_mpool_handle->mpool_free(priv->local_mpool_handle, (void *)msg);
			msg = NULL;
			priv->alarm_queue[i] = NULL;
		}
	}
}

static void *sms_alarm_process(void *arg)
{
	sms_alarm_thread_param_t *thread_param = (sms_alarm_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->sys_db_handle	= (db_access_t *)thread_param->sys_db_handle;
	priv->pref_handle = (preference_t *)thread_param->pref_handle;

	priv->rb_handle = (ring_buffer_t *)thread_param->rb_handle;
	priv->mpool_handle = (mem_pool_t *)thread_param->mpool_handle;

	priv->sms_rb_handle = (ring_buffer_t *)thread_param->sms_rb_handle;
	priv->alarm_pool_handle = (mem_pool_t *)thread_param->alarm_pool_handle;

	update_sms_contact(priv);
	priv->modem = modem_create();

	alarm_msg_t *alarm_msg = NULL;
	while (thiz->thread_status) {
		priv->send_times = priv->pref_handle->get_send_sms_times(priv->pref_handle);
		priv->send_interval = priv->pref_handle->get_send_sms_interval(priv->pref_handle);

		if (priv->sms_rb_handle->pop(priv->sms_rb_handle, (void **)&alarm_msg) == 0) {
			update_alarm_table(priv, alarm_msg);
			priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
			alarm_msg = NULL;
		}

		if (priv->pref_handle->get_sms_contact_flag(priv->pref_handle)) {
			priv->pref_handle->set_sms_contact_flag(priv->pref_handle, 0);
			update_sms_contact(priv);
		}

		send_alarm_sms(priv);
		alarm_msg = NULL;
	}

	clear_ring_buffer(priv->rb_handle, priv->mpool_handle);

	priv->modem->destroy(priv->modem);
	priv->modem = NULL;

	priv = NULL;
	thiz = NULL;
	thread_param = NULL;

	return (void *)0;
}

thread_t *sms_alarm_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t) + sizeof(priv_info_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= sms_alarm_process;

		strcpy(thiz->thread_name, "sms_alarm");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= thread_destroy;

		priv_info_t *priv = (priv_info_t *)thiz->priv;
		priv->local_mpool_handle = mem_pool_create(sizeof(alarm_msg_t), MAX_QUEUE_NUMBER);
		if (priv->local_mpool_handle == NULL) {
			printf("create local mpool hanlde faile\n");
		}
	}

	return thiz;
}
