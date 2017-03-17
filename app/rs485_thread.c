#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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

typedef struct {
	db_access_t		*sys_db_handle;
	ring_buffer_t	*rb_handle;
	mem_pool_t		*mpool_handle;
	preference_t	*pref_handle;
	protocol_t 		*protocol;

	uart_param_t 	uart_param;
	int 			protocol_id;
	int 			rs485_enable;
} priv_info_t;

static void print_snmp_protocol(protocol_t *snmp_protocol)
{
        printf("#############################################\n");
        printf("protocol_id:%d\n", snmp_protocol->protocol_id);
        printf("protocol_description:%s\n", snmp_protocol->protocol_name);
        printf("brand_name:%s\n", snmp_protocol->device_brand);
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

static void create_last_param_value_list(priv_info_t *priv, property_t *property, list_t *valid_value, int flag)
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

		unsigned int status = 0;
		msg->msg_type = REAL_DATA;
		if (flag == 1) {
	        sprintf(msg->buf, "INSERT INTO %s (device_id, device_name, param_id, param_name, param_type, analog_value, \
	            unit, enum_value, enum_desc, alarm_type) VALUES (%d, '%s', %d, '%s', %d, %.1f, '%s', %d, '%s', %d)",
	                "real_data",
	                priv->protocol->protocol_id, priv->protocol->protocol_name,
	                current_value->param_id, param_desc->param_name, param_desc->param_type,
	                current_value->param_value, param_desc->param_unit, current_value->enum_value,
	                param_desc->param_enum[current_value->enum_value].desc, status);
		} else {
	        sprintf(msg->buf, "UPDATE %s SET analog_value=%.1f, enum_value=%d, \
						enum_desc='%s', alarm_type=%d WHERE device_id=%d and param_id=%d",
	            		"real_data", current_value->param_value, current_value->enum_value,
		                param_desc->param_enum[current_value->enum_value].desc, status,
						priv->protocol->protocol_id, current_value->param_id);
		}
		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;

		msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
		if (msg == NULL) {
			printf("memory pool is empty\n");
			continue;
		}
		msg->msg_type = RECORD_DATA;
        sprintf(msg->buf, "INSERT INTO %s (device_id, device_name, param_id, \
			param_name, param_type, analog_value, unit, enum_value, enum_desc) \
			VALUES (%d, '%s', '%d', '%s', %d, %.1f, '%s', %d, '%s')", "data_record",
                priv->protocol->protocol_id, priv->protocol->protocol_name,
                current_value->param_id, param_desc->param_name, param_desc->param_type,
				current_value->param_value, param_desc->param_unit, current_value->enum_value,
                param_desc->param_enum[current_value->enum_value].desc);
		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;
	}

	property->last_param_value = last_value_list;
	gettimeofday(&(property->last_record_time), NULL);
}

static void compare_values(priv_info_t *priv, property_t *property, list_t *valid_value)
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	list_t *last_value_list = property->last_param_value;
	list_t *desc_list = property->param_desc;

	param_desc_t *param_desc = NULL;
	param_value_t *last_value = NULL;
	param_value_t *current_value = NULL;
	int list_size = valid_value->get_list_size(valid_value);
	int i = 0;
	int timeout_record_flag = 0;
	if ((current_time.tv_sec - property->last_record_time.tv_sec) > (60 * 30)) {
		property->last_record_time = current_time;
		timeout_record_flag = 1;
	}

	msg_t *msg = NULL;
	int data_record_flag = 0;
	int alarm_record_flag = 0;
	unsigned int alarm_status = NORMAL;
	char alarm_desc[64] = {0};
	for (i = 0; i < list_size; i++) {
		data_record_flag = 0;
		alarm_record_flag = 0;
		current_value = valid_value->get_index_value(valid_value, i);
		last_value = last_value_list->get_index_value(last_value_list, i);
		param_desc = desc_list->get_index_value(desc_list, i);

		alarm_status = last_value->status;
		if (param_desc->param_type == PARAM_TYPE_ANALOG) {
			if (abs(current_value->param_value - last_value->param_value)
						> param_desc->update_threshold) {
				data_record_flag = 1;
			}

			if (last_value->status == UP_ALARM_ON) {
				if (current_value->param_value < param_desc->up_free) {
					alarm_status = UP_ALARM_OFF;
					alarm_record_flag = 1;
				}
			} else if (last_value->status == LOW_ALARM_ON) {
				if (current_value->param_value < param_desc->up_free) {
					alarm_status = LOW_ALARM_OFF;
					alarm_record_flag = 1;
				}
			} else {
				if (current_value->param_value > param_desc->up_limit) {
					alarm_status = UP_ALARM_ON;
					alarm_record_flag = 1;
				} else if (current_value->param_value < param_desc->low_limit) {
					alarm_status = LOW_ALARM_ON;
					alarm_record_flag = 1;
				}
			}
		} else {
			if (current_value->enum_value != last_value->enum_value) {
				if (current_value->enum_value == param_desc->enum_alarm_value) {
					alarm_status = LEVEL_ALARM_ON;
				} else {
					alarm_status = LEVEL_ALARM_OFF;
				}
				alarm_record_flag = 1;
			}
		}
		last_value->param_value = current_value->param_value;
		last_value->enum_value = current_value->enum_value;

		if (timeout_record_flag) {
			data_record_flag = 1;
		}

		if (alarm_record_flag) {
			memset(alarm_desc, 0, sizeof(alarm_desc));
			if (alarm_status & 0x10) {	//解除报警
				switch (alarm_status & 0x0F) {
				case 0x1:
					sprintf(alarm_desc, "%s%s", param_desc->param_name, "上限报警解除");
					break;
				case 0x2:
					sprintf(alarm_desc, "%s%s", param_desc->param_name, "下限报警解除");
					break;
				case 0x4:
					sprintf(alarm_desc, "%s%s", param_desc->param_name, "恢复");
					break;
				default:
					break;
				}
				alarm_status = NORMAL;
			} else {
				switch (alarm_status & 0x0F) {
				case 0x1:
					sprintf(alarm_desc, "%s%s", param_desc->param_name, "超过上限报警");
					break;
				case 0x2:
					sprintf(alarm_desc, "%s%s", param_desc->param_name, "低于下限报警");
					break;
				case 0x4:
					sprintf(alarm_desc, "%s%s", param_desc->param_name, "异常");
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
			msg->msg_type = RECORD_DATA;
	        sprintf(msg->buf, "INSERT INTO %s (device_id, device_name, param_id, \
				param_name, param_type, analog_value, unit, enum_value, enum_desc, alarm_desc) \
				VALUES (%d, '%s', %d, '%s', %d, %.1f, '%s', %d, '%s', '%s')", "alarm_record",
	                priv->protocol->protocol_id, priv->protocol->protocol_name,
	                current_value->param_id, param_desc->param_name, param_desc->param_type,
	                current_value->param_value, param_desc->param_unit, current_value->enum_value,
	                param_desc->param_enum[current_value->enum_value].desc, alarm_desc);
			if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
				printf("ring buffer is full\n");
				priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
			}
			msg = NULL;
		}
		last_value->status = alarm_status;

		if (data_record_flag) {	//记录一次历史数据
			msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
			if (msg == NULL) {
				printf("memory pool is empty\n");
				continue;
			}
			msg->msg_type = RECORD_DATA;
	        sprintf(msg->buf, "INSERT INTO %s (device_id, device_name, param_id, \
				param_name, param_type, analog_value, unit, enum_value, enum_desc) \
				VALUES (%d, '%s', %d, '%s', %d, %.1f, '%s', %d, '%s')", "data_record",
	                priv->protocol->protocol_id, priv->protocol->protocol_name,
	                current_value->param_id, param_desc->param_name, param_desc->param_type,
	                current_value->param_value, param_desc->param_unit, current_value->enum_value,
	                param_desc->param_enum[current_value->enum_value].desc);
			if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
				printf("ring buffer is full\n");
				priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
			}
			msg = NULL;
		}

		msg = (msg_t *)priv->mpool_handle->mpool_alloc(priv->mpool_handle);
		if (msg == NULL) {
			printf("memory pool is empty\n");
			continue;
		}
		msg->msg_type = RECORD_DATA;
        sprintf(msg->buf, "UPDATE %s SET analog_value=%.1f, enum_value=%d, \
					enum_desc='%s', alarm_type=%d WHERE device_id=%d and param_id=%d",
            		"real_data", current_value->param_value, current_value->enum_value,
	                param_desc->param_enum[current_value->enum_value].desc, alarm_status,
					priv->protocol->protocol_id, current_value->param_id);
		if (priv->rb_handle->push(priv->rb_handle, (void *)msg)) {
			printf("ring buffer is full\n");
			priv->mpool_handle->mpool_free(priv->mpool_handle, (void *)msg);
		}
		msg = NULL;
	}
}

static void update_alarm_param(priv_info_t *priv, property_t *property)
{
	char sql[256] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d AND cmd_id=%d order by id",
			"parameter", priv->protocol->protocol_id, property->cmd.cmd_id);
	memset(&query_result, 0, sizeof(query_result_t));
	priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);

	list_t *desc_list = property->param_desc;
	param_desc_t *param_desc = NULL;
	int list_size = desc_list->get_list_size(desc_list);
	int i = 0;
	if (query_result.row > 0) {
		for (i = 0; i < list_size; i++) {
			param_desc = desc_list->get_index_value(desc_list, i);
			param_desc->up_limit = atof(query_result.result[(i + 1) * query_result.column + 7]);
			param_desc->up_free = atof(query_result.result[(i + 1) * query_result.column + 8]);
			param_desc->low_limit = atof(query_result.result[(i + 1) * query_result.column + 9]);
			param_desc->low_free = atof(query_result.result[(i + 1) * query_result.column + 10]);
			param_desc->update_threshold = atof(query_result.result[(i + 1) * query_result.column + 12]);
		}
	}

	priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);

	desc_list = NULL;
	param_desc = NULL;
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

    list_t *protocol_list = list_create(sizeof(protocol_t));
    init_protocol_lib(protocol_list);
    protocol_t *protocol = NULL;

	char sql[256] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s WHERE port=3", "uart_cfg");

	while (1) {
		memset(&query_result, 0, sizeof(query_result_t));
		priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);
		if (query_result.row > 0) {
			priv->protocol_id = atoi(query_result.result[query_result.column + 1]);
		    priv->uart_param.baud = atoi(query_result.result[query_result.column + 2]);
		    priv->uart_param.bits = atoi(query_result.result[query_result.column + 3]);
		    priv->uart_param.stops = atoi(query_result.result[query_result.column + 4]);
		    priv->uart_param.parity = atoi(query_result.result[query_result.column + 5]);
			priv->rs485_enable = atoi(query_result.result[query_result.column + 6]);
			printf("protocol_id %d, baud %d, bits %d, stops %d, parity %d, enable %d\n",
				priv->protocol_id, priv->uart_param.baud, priv->uart_param.bits,
				priv->uart_param.stops, priv->uart_param.parity, priv->rs485_enable);
		}
		priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);

		protocol = get_protocol_handle(protocol_list, priv->protocol_id);
		if ((protocol != NULL) && (priv->rs485_enable)) {
			break;
		}
		sleep(5);
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
	while (thiz->thread_status) {
		for (index = 0; index < property_list_size; index++) {
			property = property_list->get_index_value(property_list, index);
	        memset(buf, 0, sizeof(buf));
    		print_buf(property->cmd.cmd_code, property->cmd.cmd_len);

			drv_gpio_write(RS485_ENABLE, 1);
	        if (uart->write(uart, property->cmd.cmd_code, property->cmd.cmd_len, 2)
						== property->cmd.cmd_len) {
            	usleep(20000);
            	drv_gpio_write(RS485_ENABLE, 0);

	            int len = uart->read(uart, buf, property->cmd.check_len, 2);
	            printf("read len %d\n", len);
	            print_buf(buf, len);
	            if (len == property->cmd.check_len) {
	                list_t *value_list = list_create(sizeof(param_value_t));
	                protocol->calculate_data(property, buf, len, value_list);
	                print_param_value(value_list);
					if (update_alarm_param_flag) {
						update_alarm_param(priv, property);
					}
					if (property->last_param_value == NULL) {
						create_last_param_value_list(priv, property, value_list,
							thread_param->init_flag);
					} else {
						compare_values(priv, property, value_list);
					}
	                value_list->destroy_list(value_list);
	                value_list = NULL;
	            }
	        } else {
	            printf("write cmd failed------------\n");
	        }
			sleep(5);
			if (update_alarm_param_flag) {
				update_alarm_param_flag = 0;
				priv->pref_handle->set_rs485_alarm_flag(priv->pref_handle, 0);
			} else {
				update_alarm_param_flag = priv->pref_handle->get_rs485_alarm_flag(priv->pref_handle);
			}
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

	return (void *)0;
}

static void rs485_thread_destroy(thread_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;

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
	}

	return thiz;
}
