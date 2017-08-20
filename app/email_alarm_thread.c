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
#include "lf_queue.h"
#include "common_type.h"

#include "smtp.h"

#define MAX_QUEUE_NUMBER	(16)
#define MAX_EMAIL_USER_NUM	(10)

/**
 * @brief   邮件联系人
 */
typedef struct {
	char	name[32];
	char	email_addr[32];
} email_contact_t;

/**
 * @brief   私有成员变量
 */
typedef struct {
	int				email_contact_cnt;
	email_contact_t	email_user_array[MAX_EMAIL_USER_NUM];

	int				send_times;
	int				send_interval;

	preference_t	*pref_handle;
	db_access_t 	*sys_db_handle;

	alarm_msg_t		*alarm_queue[MAX_QUEUE_NUMBER];
	struct timeval	last_send_timing[MAX_QUEUE_NUMBER];
	mem_pool_t		*local_mpool_handle;

	ring_buffer_t 	*rb_handle;
	mem_pool_t		*mpool_handle;

	ring_buffer_t 	*email_rb_handle;
	mem_pool_t 		*alarm_pool_handle;

	smtp_t			*smtp_handle;
} priv_info_t;

/**
 * @brief   clear_ring_buffer   清除邮件报警环形缓存
 * @param   rb_handle           环形缓存句柄
 * @param   mpool_handle        报警信息内存池句柄 
 */
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

/**
 * @brief   insert_queue    插入一个新的邮件报警信息到队列中
 * @param   priv            私有成员指针
 * @param   msg             邮件报警信息
 */
static void insert_queue(priv_info_t *priv, alarm_msg_t *msg)
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

/**
 * @brief   update_alarm_queue  更新邮件报警队列
 * @param   priv                私有变量指针
 * @param   alarm_msg           报警信息
 */
static void update_alarm_queue(priv_info_t *priv, alarm_msg_t *alarm_msg)
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

			insert_queue(priv, msg);
			msg = NULL;
		}
	}
}

/**
 * @brief   update_email_contact 更新邮件报警联系人信息
 * @param   priv
 */
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

/**
 * @brief   send_to_contact 发送邮件
 *
 * @param   priv
 * @param   alarm_msg
 */
static void send_to_contact(priv_info_t *priv, alarm_msg_t *alarm_msg)
{
	priv->smtp_handle = smtp_create();
	char email_content[256] = {0};
	sprintf(email_content, "%s", alarm_msg->alarm_desc);

	email_server_t email_server_param;
	email_server_param = priv->pref_handle->get_email_server_param(priv->pref_handle);

	email_param_t email_param;
	memset(&email_param, 0, sizeof(email_param_t));

	int ret = -1;
	int i = 0;
	msg_t *msg = NULL;

	strcpy(email_param.server_addr, email_server_param.smtp_server);
	strcpy(email_param.sender_name, email_server_param.email_addr);
	strcpy(email_param.sender_email, email_server_param.email_addr);
	strcpy(email_param.password, email_server_param.password);

	strcpy(email_param.title, "Alarm warning");
	strcpy(email_param.content, email_content);

	switch (email_server_param.port) {
	case 25:
		email_param.port = 25;
		email_param.security_type = NO_SECURITY;
	break;
	case 587:
		email_param.port = 587;
		email_param.security_type = USE_TLS;
	break;
	case 465:
	default:
		email_param.port = 465;
		email_param.security_type = USE_SSL;
	break;
	}

	email_param.email_receiver_cnt = priv->email_contact_cnt;
	for (i = 0; i < priv->email_contact_cnt; i++) {
		strcpy(email_param.email_receiver[i].receiver_mail, priv->email_user_array[i].email_addr);
		strcpy(email_param.email_receiver[i].receiver_name, priv->email_user_array[i].name);
	}

	ret = priv->smtp_handle->send_email(priv->smtp_handle, &email_param);

	for (i = 0; i < priv->email_contact_cnt; i++) {
		msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
		if (msg == NULL) {
			printf("memory pool is empty\n");
			continue;
		}

		if (ret == 0) {
	        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
				param_name, param_desc, param_type, analog_value, enum_value, enum_desc, name, email, send_status, email_content) \
				VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, %d, '%s', '%s', '%s', %d, '%s')", "email_record",
	                alarm_msg->protocol_id, alarm_msg->protocol_name, alarm_msg->protocol_desc,
	                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
					alarm_msg->param_value, alarm_msg->enum_value, alarm_msg->enum_desc,
					priv->email_user_array[i].name, priv->email_user_array[i].email_addr,
					0, email_content);
		} else {
	        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
				param_name, param_desc, param_type, analog_value, enum_value, enum_desc, name, email, send_status, email_content) \
				VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, %d, '%s', '%s', '%s', %d, '%s')", "email_record",
	                alarm_msg->protocol_id, alarm_msg->protocol_name, alarm_msg->protocol_desc,
	                alarm_msg->param_id, alarm_msg->param_name, alarm_msg->param_desc, alarm_msg->param_type,
					alarm_msg->param_value, alarm_msg->enum_value, alarm_msg->enum_desc,
					priv->email_user_array[i].name, priv->email_user_array[i].email_addr,
					1, email_content);
		}

		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;
	}
	priv->smtp_handle->destroy(priv->smtp_handle);
	priv->smtp_handle = NULL;
}

/**
 * @brief   check_alarm_email_queue 检查邮件报警队列状态，确定是否需要发送邮件
 * @param   priv
 */
static void check_alarm_email_queue(priv_info_t *priv)
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
			msg->send_times--;
			priv->last_send_timing[i] = current_time;
		} else if ((current_time.tv_sec - priv->last_send_timing[i].tv_sec) >= (priv->send_interval * 5 * 60)) {
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

/**
 * @brief   email_alarm_process 邮件发送线程入口
 * @param   arg
 * @return
 */
static void *email_alarm_process(void *arg)
{
	email_alarm_thread_param_t *thread_param = (email_alarm_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->sys_db_handle	= (db_access_t *)thread_param->sys_db_handle;
	priv->pref_handle = (preference_t *)thread_param->pref_handle;

	priv->rb_handle = (ring_buffer_t *)thread_param->rb_handle;
	priv->mpool_handle = (mem_pool_t *)thread_param->mpool_handle;

	priv->email_rb_handle = (ring_buffer_t *)thread_param->email_rb_handle;
	priv->alarm_pool_handle = (mem_pool_t *)thread_param->alarm_pool_handle;

	update_email_contact(priv);

	lf_queue_t	queue;
	if (lf_queue_init(&queue, SHM_KEY_TRAP_ALARM, sizeof(alarm_msg_t), 3) < 0) {
		printf("######################### create SHM_KEY_TRAP_ALARM failed ####################\n");
		return (void *)-1;
	}

	alarm_msg_t *alarm_msg = NULL;
	priv->send_times = 1;
	priv->send_interval = 0;
	while (thiz->thread_status) {
        priv->send_times = priv->pref_handle->get_send_email_times(priv->pref_handle);
		priv->send_interval = priv->pref_handle->get_send_email_interval(priv->pref_handle);

		if (priv->email_rb_handle->pop(priv->email_rb_handle, (void **)&alarm_msg) == 0) {
			update_alarm_queue(priv, alarm_msg);

			if (lf_queue_push(queue, alarm_msg) < 0) {
				printf("##############  push to lock-free queue failed ##############\n");
			} else {
				printf("##############  push to lock-free queue done ##############\n");	
			}

			priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
			alarm_msg = NULL;
		} else {
			sleep(1);
		}

		if (priv->pref_handle->get_email_contact_flag(priv->pref_handle)) {
			priv->pref_handle->set_email_contact_flag(priv->pref_handle, 0);
			update_email_contact(priv);
		}

		check_alarm_email_queue(priv);
		alarm_msg = NULL;
	}

	clear_ring_buffer(priv->rb_handle, priv->mpool_handle);
    lf_queue_fini(&queue);

	priv = NULL;
	thiz = NULL;
	thread_param = NULL;

	return (void *)0;
}

/**
 * @brief   email_alarm_thread_create 创建邮件发送线程对象
 * @return
 */
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

		priv_info_t *priv = (priv_info_t *)thiz->priv;
		priv->local_mpool_handle = mem_pool_create(sizeof(alarm_msg_t), MAX_QUEUE_NUMBER);
		if (priv->local_mpool_handle == NULL) {
			printf("create local mpool hanlde faile\n");
		}
	}

	return thiz;
}
