#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "thread_process.h"
#include "email_alarm_thread.h"
#include "db_access.h"
#include "preference.h"

#include "types.h"
#include "mem_pool.h"
#include "ring_buffer.h"

#include "email.h"

typedef struct {
	char	name[32];
	char	email_addr[32];
} email_contact_t;

typedef struct {
	int				email_contact_cnt;
	email_contact_t	email_user_array[10];

	int				send_times;
	int				send_interval;

	preference_t	*pref_handle;
	db_access_t 	*email_alarm_db_handle;
	db_access_t 	*sys_db_handle;

	ring_buffer_t 	*rb_handle;
	mem_pool_t		*mpool_handle;

	ring_buffer_t 	*email_rb_handle;
	mem_pool_t 		*alarm_pool_handle;

	email_t 		*email_handle;
} priv_info_t;

static void clear_ring_buffer(db_access_t	*email_alarm_db_handle,
							  ring_buffer_t *rb_handle,
							  mem_pool_t 	*mpool_handle)
{
	char error_msg[512] = {0};
	alarm_msg_t *msg = NULL;
	while (1) {
		if (rb_handle->pop(rb_handle, (void **)&msg) == 0) {
			memset(error_msg, 0, sizeof(error_msg));
			//email_alarm_db_handle->action(email_alarm_db_handle, msg->buf, error_msg);
			mpool_handle->mpool_free(mpool_handle, (void *)msg);
			msg = NULL;
		} else {
			break;
		}
	}
}

static void update_alarm_table(priv_info_t *priv, alarm_msg_t *alarm_msg)
{
	char error_msg[512] = {0};
	char sql[512] = {0};

	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d AND param_id=%d order by id",
			"alarm_record", alarm_msg->protocol_id, alarm_msg->param_id);
	memset(&query_result, 0, sizeof(query_result_t));
	priv->email_alarm_db_handle->query(priv->email_alarm_db_handle, sql, &query_result);

	if (query_result.row > 0) {
		memset(sql, 0, sizeof(sql));
        sprintf(sql, "DELETE FROM %s WHERE id=%d", "alarm_record",
			atoi(query_result.result[query_result.column]));
		priv->email_alarm_db_handle->action(priv->email_alarm_db_handle, sql, error_msg);
	}
	priv->email_alarm_db_handle->free_table(priv->email_alarm_db_handle, query_result.result);

	memset(sql, 0, sizeof(sql));
	if (alarm_msg->alarm_type == ALARM_DISCARD) {
        sprintf(sql, "INSERT INTO %s (sent_time, protocol_id, protocol_name, param_id, param_name, param_desc, \
			param_type, analog_value, unit, enum_value, enum_desc, alarm_desc, alarm_type, send_cnt) \
			VALUES ('%s', %d, '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s', %d, %d)",
			"alarm_record", "", alarm_msg->protocol_id, alarm_msg->protocol_name,
			alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
			alarm_msg->param_value, alarm_msg->param_unit, alarm_msg->enum_value,
			alarm_msg->enum_desc, alarm_msg->alarm_desc, alarm_msg->alarm_type, 1);
	} else {
        sprintf(sql, "INSERT INTO %s (sent_time, protocol_id, protocol_name, param_id, param_name, param_desc, \
			param_type, analog_value, unit, enum_value, enum_desc, alarm_desc, alarm_type, send_cnt) \
			VALUES ('%s', %d, '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s', %d, %d)",
			"alarm_record", "", alarm_msg->protocol_id, alarm_msg->protocol_name,
			alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
			alarm_msg->param_value, alarm_msg->param_unit, alarm_msg->enum_value,
			alarm_msg->enum_desc, alarm_msg->alarm_desc, alarm_msg->alarm_type, priv->send_times);
	}
	priv->email_alarm_db_handle->action(priv->email_alarm_db_handle, sql, error_msg);
}

static void update_email_contact(priv_info_t *priv)
{
	priv->email_contact_cnt = 0;

	char error_msg[512] = {0};
	char sql[512] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s order by id", "email_user");
	memset(&query_result, 0, sizeof(query_result_t));
	priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);

	if (query_result.row > 0) {
		int i = 0;
		for (i = 1; i < (query_result.row + 1); i++) {
			strcpy(priv->email_user_array[priv->email_contact_cnt].name,
				query_result.result[i * query_result.column + 1]);
			strcpy(priv->email_user_array[priv->email_contact_cnt].email_addr,
				query_result.result[i * query_result.column + 2]);
			priv->email_contact_cnt++;
		}
	}
	priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);
}

static void send_to_contact(priv_info_t *priv, alarm_msg_t *alarm_msg)
{
	int ret = -1;
	int i = 0;
	msg_t *msg = NULL;

	email_param_t email_param;
	memset(&email_param, 0, sizeof(email_param_t));

	char email_content[128] = {0};
	sprintf(email_content, "%s, %s", alarm_msg->protocol_name, alarm_msg->alarm_desc);

	email_server_t email_server_param;
	email_server_param = priv->pref_handle->get_email_server_param(priv->pref_handle);

	strcpy(email_param.smtp_server, email_server_param.smtp_server);
	strcpy(email_param.user, email_server_param.email_addr);
	strcpy(email_param.password, email_server_param.password);

	strcpy(email_param.title, "Alarm warning");
	strcpy(email_param.content, email_content);
	for (i = 0; i < priv->email_contact_cnt; i++) {
		msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
		if (msg == NULL) {
			printf("memory pool is empty\n");
			continue;
		}

		strcpy(email_param.to_addr, priv->email_user_array[i].email_addr);
		if (ret == 0) {
			if (priv->email_handle->send_email(priv->email_handle, &email_param) == 0) {
		        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, param_id, \
					param_name, param_desc, name, email, send_status, email_content) \
					VALUES (%d, '%s', %d, '%s', '%s', '%s', '%s', %d, '%s')", "eamil_record",
		                alarm_msg->protocol_id, alarm_msg->protocol_name,
		                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc,
						priv->email_user_array[i].name, priv->email_user_array[i].email_addr,
						0, email_content);
			} else {
		        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, param_id, \
					param_name, param_desc, name, email, send_status, email_content) \
					VALUES (%d, '%s', %d, '%s', '%s', '%s', '%s', %d, '%s')", "eamil_record",
		                alarm_msg->protocol_id, alarm_msg->protocol_name,
		                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc,
						priv->email_user_array[i].name, priv->email_user_array[i].email_addr,
						1, email_content);
			}
		} else {
	        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, param_id, \
				param_name, param_desc, name, email, send_status, email_content) \
				VALUES (%d, '%s', %d, '%s', '%s', '%s', '%s', %d, '%s')", "eamil_record",
	                alarm_msg->protocol_id, alarm_msg->protocol_name,
	                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc,
					priv->email_user_array[i].name, priv->email_user_array[i].email_addr,
					1, email_content);
		}

		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;
	}
}

static void send_alarm_email(priv_info_t *priv)
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	char time_in_sec[16] = {0};
	struct tm *tm = localtime(&(current_time.tv_sec));
	sprintf(time_in_sec, "%04d-%02d-%02d %02d:%02d:%02d",
                        tm->tm_year + 1900, tm->tm_mon + 1,
                        tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	current_time.tv_sec -= priv->send_interval * 300;
	tm = localtime(&(current_time.tv_sec));

	char dead_line[16] = {0};
	sprintf(dead_line, "%04d-%02d-%02d %02d:%02d:%02d",
                        tm->tm_year + 1900, tm->tm_mon + 1,
                        tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	char sql[512] = {0};
	char error_msg[512] = {0};
	sprintf(sql, "SELECT * FROM %s WHERE sent_time='' or sent_time < '%s' order by id",
		"alarm_record", dead_line);

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	priv->email_alarm_db_handle->query(priv->email_alarm_db_handle, sql, &query_result);

	if (query_result.row > 0) {
		alarm_msg_t alarm_msg;
		memset(&alarm_msg, 0, sizeof(alarm_msg_t));
		int i = 0;
		for (i = 1; i < (query_result.row + 1); i++) {
			alarm_msg.protocol_id = atoi(query_result.result[i * query_result.column + 2]);
			strcpy(alarm_msg.protocol_name, query_result.result[i * query_result.column + 3]);
			alarm_msg.param_id = atoi(query_result.result[i * query_result.column + 4]);
			strcpy(alarm_msg.param_name, query_result.result[i * query_result.column + 5]);
			strcpy(alarm_msg.param_desc, query_result.result[i * query_result.column + 6]);
			strcpy(alarm_msg.param_unit, query_result.result[i * query_result.column + 7]);
			alarm_msg.param_type = atoi(query_result.result[i * query_result.column + 8]);
			alarm_msg.param_value = atof(query_result.result[i * query_result.column + 9]);
			alarm_msg.enum_value = atoi(query_result.result[i * query_result.column + 10]);
			strcpy(alarm_msg.enum_desc, query_result.result[i * query_result.column + 11]);
			strcpy(alarm_msg.alarm_desc, query_result.result[i * query_result.column + 12]);
			alarm_msg.alarm_type = atoi(query_result.result[i * query_result.column + 13]);
			alarm_msg.send_times = atoi(query_result.result[i * query_result.column + 14]);

			send_to_contact(priv, &alarm_msg);
			alarm_msg.send_times--;

			memset(sql, 0, sizeof(sql));
			if (alarm_msg.send_times == 0) {
		        sprintf(sql, "DELETE FROM %s WHERE id=%d", "alarm_record",
					atoi(query_result.result[i * query_result.column]));
			} else {
		        sprintf(sql, "UPDATE %s SET sent_time='%s', send_cnt=%d WHERE id=%d",
					"alarm_record", time_in_sec, alarm_msg.send_times,
					atoi(query_result.result[i * query_result.column]));
			}
			priv->email_alarm_db_handle->action(priv->email_alarm_db_handle, sql, error_msg);
		}
	}
	priv->email_alarm_db_handle->free_table(priv->email_alarm_db_handle, query_result.result);
}

static void *email_alarm_process(void *arg)
{
	email_alarm_thread_param_t *thread_param = (email_alarm_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->email_alarm_db_handle = (db_access_t *)thread_param->email_alarm_db_handle;
	priv->sys_db_handle	= (db_access_t *)thread_param->sys_db_handle;
	priv->pref_handle = (preference_t *)thread_param->pref_handle;

	priv->rb_handle = (ring_buffer_t *)thread_param->rb_handle;
	priv->mpool_handle = (mem_pool_t *)thread_param->mpool_handle;

	priv->email_rb_handle = (ring_buffer_t *)thread_param->email_rb_handle;
	priv->alarm_pool_handle = (mem_pool_t *)thread_param->alarm_pool_handle;

	update_email_contact(priv);
	priv->email_handle = email_create();

	alarm_msg_t *alarm_msg = NULL;
	priv->send_times = 1;
	priv->send_interval = 0;
	while (thiz->thread_status) {
		if (priv->email_rb_handle->pop(priv->email_rb_handle, (void **)&alarm_msg) == 0) {
			update_alarm_table(priv, alarm_msg);
			priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
			alarm_msg = NULL;
		}

		/*if (priv->pref_handle->get_sms_contact_flag(priv->pref_handle)) {
			priv->pref_handle->set_sms_contact_flag(priv->pref_handle, 0);
			update_email_contact(priv);
		}*/

		send_alarm_email(priv);
		alarm_msg = NULL;
	}

	clear_ring_buffer(priv->email_alarm_db_handle, priv->rb_handle, priv->mpool_handle);
	priv->email_handle->destroy(priv->email_handle);
	priv->email_handle = NULL;

	priv = NULL;
	thiz = NULL;
	thread_param = NULL;

	return (void *)0;
}

thread_t *email_alarm_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t) + sizeof(priv_info_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= email_alarm_process;

		strcpy(thiz->thread_name, "email_alarm");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= thread_destroy;
	}

	return thiz;
}
