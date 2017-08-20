#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "protocol_interfaces.h"

#include "thread_process.h"
#include "rs485_thread.h"
#include "debug.h"
#include "db_access.h"
#include "uart.h"
#include "preference.h"

#include "drv_gpio.h"
#include "types.h"
#include "ring_buffer.h"
#include "mem_pool.h"

#include "common_type.h"
#include "shm_object.h"
#include "semaphore.h"

typedef struct {
	db_access_t		*sys_db_handle;
	ring_buffer_t	*rb_handle;
	mem_pool_t		*mpool_handle;
	preference_t	*pref_handle;
	protocol_t 		*protocol;

	ring_buffer_t	*sms_rb_handle;
	ring_buffer_t	*email_rb_handle;
	mem_pool_t		*alarm_pool_handle;

	uart_realdata_t *rs485_realdata;

	int				sem_id;
	shm_object_t	*shm_handle;

	uart_param_t	uart_param;
	int 			protocol_id;
	int 			rs485_enable;

	struct timeval	last_record_time;
} priv_info_t;

static void print_snmp_protocol(protocol_t *snmp_protocol)
{
        printf("#############################################\n");
        printf("protocol_id:%d\n", snmp_protocol->protocol_id);
        printf("protocol_description:%s\n", snmp_protocol->protocol_name);
        printf("get_cmd_info func %p\n", snmp_protocol->get_property);
        printf("cal_dev_data func %p\n", snmp_protocol->calculate_data);
        printf("#############################################\n");
}

static void print_param_value(list_t *param_value_list)
{
    int len = param_value_list->get_list_size(param_value_list);
    param_value_t *param_value = NULL;
    int i = 0;
    for (i = 0; i < len; i++) {
        //printf("len %d\n", len);
        param_value = param_value_list->get_index_value(param_value_list, i);
        printf("param_id %d\n", param_value->param_id);
        printf("param_value %.1f\n", param_value->param_value);
    }
}

static void create_last_param_value_list(priv_info_t *priv, property_t *property, list_t *valid_value, db_access_t *data_db_handle)
{
	list_t *last_value_list = list_create(sizeof(param_value_t));
	list_t *desc_list = property->param_desc;
	param_value_t last_value;
	param_desc_t *param_desc = NULL;
	param_value_t *current_value = NULL;
	int list_size = valid_value->get_list_size(valid_value);

	int i = 0;
	msg_t *msg = NULL;
	for (i = 0; i < list_size; i++) {
		memset(&last_value, 0, sizeof(param_value_t));
		current_value = valid_value->get_index_value(valid_value, i);
		param_desc = desc_list->get_index_value(desc_list, i);
		last_value.param_id = current_value->param_id;
		last_value.param_value = current_value->param_value;
		last_value.enum_value = current_value->enum_value;
		last_value_list->push_back(last_value_list, &last_value);

		msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
		if (msg == NULL) {
			printf("memory pool is empty\n");
			continue;
		}
        sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
			param_name, param_desc, param_type, analog_value, unit, enum_value, enum_en_desc, enum_cn_desc) \
			VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s')", "data_record",
                priv->protocol->protocol_id, priv->protocol->protocol_name, priv->protocol->protocol_desc,
                current_value->param_id, param_desc->param_name, param_desc->param_desc, param_desc->param_type,
				current_value->param_value, param_desc->param_unit, current_value->enum_value,
                param_desc->param_enum[current_value->enum_value].en_desc, param_desc->param_enum[current_value->enum_value].cn_desc);
		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;
	}

	property->last_param_value = last_value_list;
	gettimeofday(&(priv->last_record_time), NULL);
}

static int compare_values(priv_info_t *priv, property_t *property, list_t *valid_value, int offset)
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	list_t *last_value_list = property->last_param_value;
	list_t *desc_list = property->param_desc;

	param_desc_t *param_detail = NULL;
	param_value_t *last_value = NULL;
	param_value_t *current_value = NULL;
	int list_size = valid_value->get_list_size(valid_value);
	int i = 0;
	int timeout_record_flag = 0;
	if ((current_time.tv_sec - priv->last_record_time.tv_sec) > (60 * 30)) {
		priv->last_record_time = current_time;
		timeout_record_flag = 1;
	}

	msg_t *msg = NULL;
	alarm_msg_t *alarm_msg = NULL;
	int data_record_flag = 0;
	int alarm_record_flag = 0;
	unsigned int alarm_status = NORMAL;
	char alarm_desc[256] = {0};
	for (i = 0; i < list_size; i++) {
		data_record_flag = 0;
		alarm_record_flag = 0;
		current_value = valid_value->get_index_value(valid_value, i);
		last_value = last_value_list->get_index_value(last_value_list, i);
		param_detail = desc_list->get_index_value(desc_list, i);

		alarm_status = last_value->status;
		if (param_detail->param_type == PARAM_TYPE_ANALOG) {
			if (abs(current_value->param_value - last_value->param_value)
						> param_detail->update_threshold) {
				data_record_flag = 1;
			}
		} else {
		    if (current_value->enum_value != last_value->enum_value) {
				data_record_flag = 1;
			}
		}

		if (timeout_record_flag) {
			data_record_flag = 1;
		}

		/* 上一次读取状态时，为上限报警状态 */
        if (last_value->status == UP_ALARM_ON) {
			if (param_detail->alarm_enable & 0x01) { /* 报警上限值存在 */
				if (param_detail->alarm_enable & 0x02) { /* 报警上限解除值存在 */
					if (param_detail->param_type == PARAM_TYPE_ANALOG) {
						if (current_value->param_value < param_detail->up_free) { /* 当前值小于报警上限解除值时报警解除 */
							alarm_status = UP_ALARM_OFF;
							alarm_record_flag = 1;
						}
					} else {
						if (current_value->enum_value < param_detail->up_free) { /* 当前值小于报警上限解除值时报警解除 */
							alarm_status = UP_ALARM_OFF;
							alarm_record_flag = 1;
						}
					}
				} else { /* 报警上限解除值不存在 */
					if (param_detail->param_type == PARAM_TYPE_ANALOG) {
						if (current_value->param_value < param_detail->up_limit) { /* 当前值小于报警上限值时报警解除 */
							alarm_status = UP_ALARM_OFF;
							alarm_record_flag = 1;
						}
					} else {
						if (current_value->enum_value < param_detail->up_limit) { /* 当前值小于报警上限值时报警解除 */
							alarm_status = UP_ALARM_OFF;
							alarm_record_flag = 1;
						}
					}
				}
			} else { /* 上限值不存在，则解除报警 */
				alarm_status = UP_ALARM_OFF;
				alarm_record_flag = 1;
			}
        } else if (last_value->status == LOW_ALARM_ON) { /* 上一次读取状态时，为下限报警状态 */
			if (param_detail->alarm_enable & 0x04) { /* 报警下限值存在 */
				if (param_detail->alarm_enable & 0x08) { /* 报警下限解除值存在 */
					if (param_detail->param_type == PARAM_TYPE_ANALOG) {
						if (current_value->param_value > param_detail->low_free) { /* 当前值大于报警下限解除值时报警解除 */
							alarm_status = LOW_ALARM_OFF;
							alarm_record_flag = 1;
						}
					} else {
						if (current_value->enum_value > param_detail->low_free) { /* 当前值大于报警下限解除值时报警解除 */
							alarm_status = LOW_ALARM_OFF;
							alarm_record_flag = 1;
						}
					}
				} else { /* 报警下限解除值不存在 */
					if (param_detail->param_type == PARAM_TYPE_ANALOG) {
						if (current_value->param_value > param_detail->low_limit) { /* 当前值大于报警下限值时报警解除 */
							alarm_status = LOW_ALARM_OFF;
							alarm_record_flag = 1;
						}
					} else {
						if (current_value->enum_value > param_detail->low_limit) { /* 当前值大于报警下限值时报警解除 */
							alarm_status = LOW_ALARM_OFF;
							alarm_record_flag = 1;
						}
					}
				}
			} else { /* 下限值不存在，则解除报警 */
				alarm_status = LOW_ALARM_OFF;
				alarm_record_flag = 1;
			}
        } else { /* 上一次读取状态时，为正常状态 */
			if (param_detail->alarm_enable & 0x01) {
				if (param_detail->param_type == PARAM_TYPE_ANALOG) {
					if (current_value->param_value > param_detail->up_limit) {
						alarm_status = UP_ALARM_ON;
						alarm_record_flag = 1;
					}
				} else {
					if (current_value->enum_value >= param_detail->up_limit) {
						alarm_status = UP_ALARM_ON;
						alarm_record_flag = 1;
					}
				}
			}

			if (param_detail->alarm_enable & 0x04) {
				if (param_detail->param_type == PARAM_TYPE_ANALOG) {
					if (current_value->param_value < param_detail->low_limit) {
						alarm_status = LOW_ALARM_ON;
						alarm_record_flag = 1;
					}
				} else {
					if (current_value->enum_value <= param_detail->low_limit) {
						alarm_status = LOW_ALARM_ON;
						alarm_record_flag = 1;
					}
				}
			}
		}

		last_value->param_value = current_value->param_value;
		last_value->enum_value = current_value->enum_value;

		if (alarm_record_flag) {
			memset(alarm_desc, 0, sizeof(alarm_desc));

			struct timeval now_time;
			struct tm *tm = NULL;
			gettimeofday(&now_time, NULL);
			tm = localtime(&(now_time.tv_sec));
			if (alarm_status & 0x10) {	//解除报警
				switch (alarm_status & 0x0F) {
				case 0x1:
					sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s,%s:%s",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						priv->protocol->protocol_name,
						param_detail->param_desc, "上限报警解除");
					break;
				case 0x2:
					sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s,%s:%s",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						priv->protocol->protocol_name,
						param_detail->param_desc, "下限报警解除");
					break;
				case 0x4:
					sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s,%s:%s",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						priv->protocol->protocol_name,
						param_detail->param_desc,
						(current_value->enum_value == 1) ? "低电平报警解除" : "高电平报警解除");
					break;
				default:
					break;
				}
				alarm_status = NORMAL;
			} else {
				switch (alarm_status & 0x0F) {
				case 0x1:
					sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s,%s:%.1f%s%s",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						priv->protocol->protocol_name,
						param_detail->param_desc, current_value->param_value,
						param_detail->param_unit, ",超过阈值");
					break;
				case 0x2:
					sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s,%s:%.1f%s%s",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						priv->protocol->protocol_name,
						param_detail->param_desc, current_value->param_value,
						param_detail->param_unit, ",低于阈值");
					break;
				case 0x4:
					sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s,%s:%s",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						priv->protocol->protocol_name,
						param_detail->param_desc,
						(current_value->enum_value == 1) ? "高电平报警" : "低电平报警");
					break;
				default:
					break;
				}
			}

			msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
			if (msg == NULL) {
				printf("memory pool is empty\n");
				continue;
			}
            sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
				param_name, param_desc, param_type, analog_value, unit, enum_value, enum_en_desc, enum_cn_desc, alarm_desc) \
				VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s', '%s')", "alarm_record",
	                priv->protocol->protocol_id, priv->protocol->protocol_name, priv->protocol->protocol_desc,
	                current_value->param_id, param_detail->param_name, param_detail->param_desc, param_detail->param_type,
	                current_value->param_value, param_detail->param_unit, current_value->enum_value,
	                param_detail->param_enum[current_value->enum_value].en_desc,
					param_detail->param_enum[current_value->enum_value].cn_desc, alarm_desc);
			if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
				printf("ring buffer is full\n");
				priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
			}
			msg = NULL;

			alarm_msg = (alarm_msg_t *)priv->alarm_pool_handle->mpool_alloc(priv->alarm_pool_handle);
			if (alarm_msg == NULL) {
				printf("memory pool is empty\n");
				continue;
			}

			alarm_msg_t *tmp_alarm_msg = (alarm_msg_t *)priv->alarm_pool_handle->mpool_alloc(priv->alarm_pool_handle);
			if (tmp_alarm_msg == NULL) {
				printf("memory pool is empty\n");
				priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
				continue;
			}

            if (alarm_status == NORMAL) {
				alarm_msg->alarm_type = ALARM_DISCARD;
			} else {
				alarm_msg->alarm_type = ALARM_RAISE;
			}
			alarm_msg->protocol_id = priv->protocol->protocol_id;
			strcpy(alarm_msg->protocol_name, priv->protocol->protocol_name);
			strcpy(alarm_msg->protocol_desc, priv->protocol->protocol_desc);
			alarm_msg->param_id = current_value->param_id;
			strcpy(alarm_msg->param_name, param_detail->param_name);
			strcpy(alarm_msg->param_desc, param_detail->param_desc);
			strcpy(alarm_msg->param_unit, param_detail->param_unit);
			alarm_msg->param_type = param_detail->param_type;
			alarm_msg->param_value = current_value->param_value;
			alarm_msg->enum_value = current_value->enum_value;
			strcpy(alarm_msg->enum_desc, param_detail->param_enum[current_value->enum_value].cn_desc);
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
		last_value->status = alarm_status;

		if (data_record_flag) {	//记录一次历史数据
			msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
			if (msg == NULL) {
				printf("memory pool is empty\n");
				continue;
			}
			sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
				param_name, param_desc, param_type, analog_value, unit, enum_value, enum_en_desc, enum_cn_desc) \
				VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s')", "data_record",
					priv->protocol->protocol_id, priv->protocol->protocol_name, priv->protocol->protocol_desc,
					current_value->param_id, param_detail->param_name, param_detail->param_desc, param_detail->param_type,
					current_value->param_value, param_detail->param_unit, current_value->enum_value,
					param_detail->param_enum[current_value->enum_value].en_desc, param_detail->param_enum[current_value->enum_value].cn_desc);
			if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
				printf("ring buffer is full\n");
				priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
			}
			msg = NULL;
		}

        priv->rs485_realdata->data[i + offset].protocol_id = priv->protocol->protocol_id;
		strcpy(priv->rs485_realdata->data[i + offset].protocol_name, priv->protocol->protocol_name);
		strcpy(priv->rs485_realdata->data[i + offset].protocol_desc, priv->protocol->protocol_desc);
		priv->rs485_realdata->data[i + offset].param_id = current_value->param_id;
		strcpy(priv->rs485_realdata->data[i + offset].param_name, param_detail->param_name);
		strcpy(priv->rs485_realdata->data[i + offset].param_desc, param_detail->param_desc);
		priv->rs485_realdata->data[i + offset].param_type = param_detail->param_type;
		priv->rs485_realdata->data[i + offset].analog_value = current_value->param_value;
		strcpy(priv->rs485_realdata->data[i + offset].param_unit, param_detail->param_unit);
		priv->rs485_realdata->data[i + offset].enum_value = current_value->enum_value;
		strcpy(priv->rs485_realdata->data[i + offset].enum_en_desc, param_detail->param_enum[current_value->enum_value].en_desc);
		strcpy(priv->rs485_realdata->data[i + offset].enum_cn_desc, param_detail->param_enum[current_value->enum_value].cn_desc);
		priv->rs485_realdata->data[i + offset].alarm_type = alarm_status;
	}
	priv->rs485_realdata->cnt = list_size + offset;

	return list_size + offset;
}

static int record_com_fail(priv_info_t *priv, int alarm_status)
{
	msg_t *msg = NULL;
	alarm_msg_t *alarm_msg = NULL;

	char alarm_desc[256] = {0};

	struct timeval now_time;
	struct tm *tm = NULL;
	gettimeofday(&now_time, NULL);
	tm = localtime(&(now_time.tv_sec));
	if (alarm_status == 0) {	//解除报警
		sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s:%s",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec,
			priv->protocol->protocol_name, "设备通信失败解除");
	} else {
		sprintf(alarm_desc, "[%04d-%02d-%02d %02d:%02d:%02d]%s:%s",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec,
			priv->protocol->protocol_name, "设备通信失败");
	}

	msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
	if (msg == NULL) {
		printf("memory pool is empty\n");
		return -1;
	}
	sprintf(msg->buf, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, param_id, \
		param_name, param_desc, param_type, analog_value, unit, enum_value, enum_en_desc, enum_cn_desc, alarm_desc) \
		VALUES (%d, '%s', '%s', %d, '%s', '%s', %d, %.1f, '%s', %d, '%s', '%s', '%s')", "alarm_record",
			priv->protocol->protocol_id, priv->protocol->protocol_name, priv->protocol->protocol_desc,
			0, "", "", PARAM_TYPE_ENUM, 0.0, "", alarm_status, (alarm_status == 0) ? "com_normal" : "com_fail",
			(alarm_status == 0) ? "通信恢复" : "通信失败", alarm_desc);

	if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
		printf("ring buffer is full\n");
		priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
	}
	msg = NULL;

	alarm_msg = (alarm_msg_t *)priv->alarm_pool_handle->mpool_alloc(priv->alarm_pool_handle);
	if (alarm_msg == NULL) {
		printf("memory pool is empty\n");
		return -1;
	}

	alarm_msg_t *tmp_alarm_msg = (alarm_msg_t *)priv->alarm_pool_handle->mpool_alloc(priv->alarm_pool_handle);
	if (tmp_alarm_msg == NULL) {
		printf("memory pool is empty\n");
		priv->alarm_pool_handle->mpool_free(priv->alarm_pool_handle, (void *)alarm_msg);
		return -1;
	}

    if (alarm_status == 0) {
		alarm_msg->alarm_type = ALARM_DISCARD;
	} else {
		alarm_msg->alarm_type = ALARM_RAISE;
	}
	alarm_msg->protocol_id = priv->protocol->protocol_id;
	strcpy(alarm_msg->protocol_name, priv->protocol->protocol_name);
	strcpy(alarm_msg->protocol_desc, priv->protocol->protocol_desc);
	alarm_msg->param_id = 0;
	strcpy(alarm_msg->param_name, "");
	strcpy(alarm_msg->param_desc, "");
	strcpy(alarm_msg->param_unit, "");
	alarm_msg->param_type = PARAM_TYPE_ENUM;
	alarm_msg->param_value = 0;
	alarm_msg->enum_value = alarm_status;
	if (alarm_status) {
		strcpy(alarm_msg->enum_desc, "通信失败");
	} else {
		strcpy(alarm_msg->enum_desc, "通信恢复");
	}
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

	return 0;
}

static void update_alarm_param(priv_info_t *priv, property_t *property)
{
	char sql[256] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d AND cmd_id=%d order by id",
			"parameter", priv->protocol->protocol_id, property->cmd.cmd_id);

	memset(&query_result, 0, sizeof(query_result_t));
	priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);

	if (query_result.row > 0) {
		list_t *desc_list = property->param_desc;
		param_desc_t *param_desc = NULL;
		int list_size = desc_list->get_list_size(desc_list);
		int i = 0;
		for (i = 0; i < list_size; i++) {
			param_desc = desc_list->get_index_value(desc_list, i);
			param_desc->alarm_enable = 0;
			if (strlen(query_result.result[(i + 1) * query_result.column + 9]) != 0) {
				param_desc->up_limit = atof(query_result.result[(i + 1) * query_result.column + 9]);
				param_desc->alarm_enable |= 0x01;
			}

			if (strlen(query_result.result[(i + 1) * query_result.column + 10]) != 0) {
				param_desc->up_free = atof(query_result.result[(i + 1) * query_result.column + 10]);
				param_desc->alarm_enable |= 0x02;
			}

			if (strlen(query_result.result[(i + 1) * query_result.column + 11]) != 0) {
				param_desc->low_limit = atof(query_result.result[(i + 1) * query_result.column + 11]);
				param_desc->alarm_enable |= 0x04;
			}

			if (strlen(query_result.result[(i + 1) * query_result.column + 12]) != 0) {
				param_desc->low_free = atof(query_result.result[(i + 1) * query_result.column + 12]);
				param_desc->alarm_enable |= 0x08;
			}
			param_desc->update_threshold = atof(query_result.result[(i + 1) * query_result.column + 14]);
		}
		desc_list = NULL;
		param_desc = NULL;
	}

	priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);
}

static void *rs485_process(void *arg)
{
	rs485_thread_param_t *thread_param = (rs485_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->sys_db_handle = (db_access_t *)thread_param->sys_db_handle;
	priv->rb_handle = (ring_buffer_t *)thread_param->rb_handle;
	priv->mpool_handle = (mem_pool_t *)thread_param->mpool_handle;
	priv->pref_handle = (preference_t *)thread_param->pref_handle;
    priv->uart_param.device_index = 3;

	priv->sms_rb_handle = (ring_buffer_t *)thread_param->sms_rb_handle;
	priv->email_rb_handle = (ring_buffer_t *)thread_param->email_rb_handle;
	priv->alarm_pool_handle = (mem_pool_t *)thread_param->alarm_pool_handle;

    char sql[256] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s WHERE port=3", "uart_cfg");
    memset(&query_result, 0, sizeof(query_result_t));
    priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);
    if (query_result.row <= 0) {
		priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);
		return (void *)0;
    }

    priv->protocol_id = atoi(query_result.result[query_result.column + 1]);
    priv->uart_param.baud = atoi(query_result.result[query_result.column + 2]);
    priv->uart_param.bits = atoi(query_result.result[query_result.column + 3]);
    priv->uart_param.stops = atoi(query_result.result[query_result.column + 4]);
    priv->uart_param.parity = atoi(query_result.result[query_result.column + 5]);
    priv->rs485_enable = atoi(query_result.result[query_result.column + 6]);
    printf("protocol_id %d, baud %d, bits %d, stops %d, parity %d, enable %d\n",
            priv->protocol_id, priv->uart_param.baud, priv->uart_param.bits,
            priv->uart_param.stops, priv->uart_param.parity, priv->rs485_enable);
    priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);

    list_t *protocol_list = list_create(sizeof(protocol_t));
    init_protocol_lib(protocol_list);
    protocol_t *protocol = NULL;
    protocol = get_protocol_handle(protocol_list, priv->protocol_id);

	priv->shm_handle = shm_object_create(RS485_SHM_KEY, sizeof(uart_realdata_t));
	int ret = 0;
	if (priv->shm_handle == NULL) {
		printf("create shm object failed\n");
		ret = -1;
	}

	while ((protocol == NULL) || (priv->rs485_enable == 0)
			|| (ret < 0)) {
		sleep(3);

		if (thiz->thread_status == 0) {
			deinit_protocol_lib(protocol_list);
			protocol_list = NULL;

			priv->shm_handle->shm_destroy(priv->shm_handle);
			return (void *)0;
		}

		priv->rs485_realdata->enable = 0;
		priv->rs485_realdata->cnt = 0;
		semaphore_p(priv->sem_id);
		priv->shm_handle->shm_put(priv->shm_handle, (void *)(priv->rs485_realdata));
		semaphore_v(priv->sem_id);
	}

	priv->protocol = protocol;
    print_snmp_protocol(protocol);

    uart_t *uart = uart_create(&(priv->uart_param));
    uart->open(uart);
	drv_gpio_open(RS485_ENABLE);

    list_t *property_list = list_create(sizeof(property_t));
    protocol->get_property(property_list);
	int property_list_size = property_list->get_list_size(property_list);

	int index = 0;
	char buf[256] = {0};
    property_t *property = NULL;
	int update_alarm_param_flag = 1;
	int param_cnt = 0;
	int com_fail_count = 0;
	int com_fail_flag = 0;
	while (thiz->thread_status) {
		param_cnt = 0;
		for (index = 0; index < property_list_size; index++) {
			property = property_list->get_index_value(property_list, index);
			if (update_alarm_param_flag) {
				update_alarm_param(priv, property);
			}

			memset(buf, 0, sizeof(buf));
			print_com_info(3, protocol->protocol_name, 0, property->cmd.cmd_code, property->cmd.cmd_len, 0);
			drv_gpio_write(RS485_ENABLE, 1);
			ret = uart->write(uart, property->cmd.cmd_code, property->cmd.cmd_len, 2);
			usleep(8000);
			drv_gpio_write(RS485_ENABLE, 0);
			if (ret == property->cmd.cmd_len) {
				int len = uart->read(uart, buf, property->cmd.check_len, 2);
				list_t *value_list = list_create(sizeof(param_value_t));
				ret = protocol->calculate_data(property, buf, len, value_list);
				if (ret == 0) {
					if (property->last_param_value == NULL) {
						create_last_param_value_list(priv, property, value_list,
							(db_access_t *)thread_param->data_db_handle);
					} else {
						param_cnt = compare_values(priv, property, value_list, param_cnt);
					}

					com_fail_count = 0;
					if (com_fail_flag) {
						com_fail_flag = 0;
						record_com_fail(priv, 0);
					}
				} else if (ret == ERR_RETURN_LEN_ZERO) {
					priv->rs485_realdata->cnt = 0;
					if (com_fail_flag == 0) {
						com_fail_count++;
						if (com_fail_count > 1) {
							com_fail_flag = 1;
							record_com_fail(priv, 1);
						}
					}
				}
				print_com_info(3, protocol->protocol_name, 1, buf, len, ret);
				value_list->destroy_list(value_list);
				value_list = NULL;
	        } else {
	            printf("write cmd failed------------\n");
	        }
			sleep(1);
		}
		priv->rs485_realdata->enable = 1;
		semaphore_p(priv->sem_id);
		priv->shm_handle->shm_put(priv->shm_handle, (void *)(priv->rs485_realdata));
		semaphore_v(priv->sem_id);

        if (update_alarm_param_flag) {
            update_alarm_param_flag = 0;
            priv->pref_handle->set_rs485_alarm_flag(priv->pref_handle, 0);
        } else {
            update_alarm_param_flag = priv->pref_handle->get_rs485_alarm_flag(priv->pref_handle);
        }
	}
	drv_gpio_close(RS485_ENABLE);

    property = NULL;
    protocol->release_property(property_list);
    property_list = NULL;

    deinit_protocol_lib(protocol_list);
    protocol_list = NULL;

    uart->destroy(uart);
    uart = NULL;

	priv->shm_handle->shm_destroy(priv->shm_handle);

	priv = NULL;
	thiz = NULL;
	thread_param = NULL;

	return (void *)0;
}

static void rs485_thread_destroy(thread_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
		if (priv->rs485_realdata) {
			memset(priv->rs485_realdata, 0, sizeof(uart_realdata_t));
			priv->rs485_realdata = NULL;
		}

        memset(thiz, 0, sizeof(thread_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

thread_t *rs485_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t) + sizeof(priv_info_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= rs485_process;

		strcpy(thiz->thread_name, "rs485_process");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= rs485_thread_destroy;

        priv_info_t *priv = (priv_info_t *)thiz->priv;
		priv->rs485_realdata = calloc(1, sizeof(uart_realdata_t));
		if (priv->rs485_realdata == NULL) {
			rs485_thread_destroy(thiz);
			thiz = NULL;
		}
		priv->sem_id = semaphore_create(RS485_KEY);
	}

	return thiz;
}
