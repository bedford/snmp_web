#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "protocol_interfaces.h"

#include "thread_process.h"
#include "rs232_thread.h"
#include "debug.h"
#include "db_access.h"
#include "uart.h"

typedef struct {
	db_access_t		*sys_db_handle;
	uart_param_t	uart_param;
	int 			protocol_id;
	int 			rs232_enable;
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

static void create_last_param_value_list(property_t *property, list_t *valid_value)
{
	list_t *last_value_list = list_create(sizeof(param_value_t));
	param_value_t value;
	param_value_t *current_value = NULL;
	int list_size = valid_value->get_list_size(valid_value);
	int i = 0;
	for (i = 0; i < list_size; i++) {
		memset(&value, 0, sizeof(param_value_t));
		current_value = valid_value->get_index_value(valid_value, i);
		value.param_id = current_value->param_id;
		value.param_value = current_value->param_value;
		value.enum_value = current_value->enum_value;
		last_value_list->push_back(last_value_list, &value);
	}

	property->last_param_value = last_value_list;
	gettimeofday(&(property->last_record_time), NULL);
}

static void compare_values(property_t *property, list_t *valid_value)
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
	int record_flag = 0;
	if ((current_time.tv_sec - property->last_record_time.tv_sec) > (60 *30)) {
		property->last_record_time = current_time;
		record_flag = 1;
	}

	for (i = 0; i < list_size; i++) {
		current_value = valid_value->get_index_value(valid_value, i);
		last_value = last_value_list->get_index_value(last_value_list, i);
		param_desc = desc_list->get_index_value(desc_list, i);
		unsigned int status = 0;
		if (param_desc->param_type == PARAM_TYPE_ANALOG) {
			if (abs(current_value->param_value - last_value->param_value)
						> param_desc->update_threshold) {
				status = THRESHOLD_ALARM;
			}

			if (current_value->param_value > param_desc->up_limit) {
				status = UP_ALARM;
			} else if (current_value->param_value > param_desc->low_limit) {
				status = LOW_ALARM;
			}
		} else {
			if (current_value->enum_value == param_desc->enum_alarm_value) {
				status = ABNORMAL_ALARM;
			}
		}
		last_value->param_value = current_value->param_value;
		last_value->enum_value = current_value->enum_value;

		if ((record_flag)
			|| (status != NORMAL)
			|| (status != last_value->status)) { //报警状态或报警解除状态下
			printf("record data or alarm data\n");
		}
		last_value->status = status;
	}
}

static void *rs232_process(void *arg)
{
	rs232_thread_param_t *thread_param = (rs232_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->sys_db_handle = (db_access_t *)thread_param->sys_db_handle;
    priv->uart_param.device_index = 2;

    list_t *protocol_list = list_create(sizeof(protocol_t));
    init_protocol_lib(protocol_list);
    protocol_t *protocol = NULL;

	char sql[256] = {0};
	query_result_t query_result;
	sprintf(sql, "SELECT * FROM %s WHERE port=2", "uart_cfg");

	while (1) {
		memset(&query_result, 0, sizeof(query_result_t));
		priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);
		if (query_result.row > 0) {
			priv->protocol_id = atoi(query_result.result[query_result.column + 1]);
		    priv->uart_param.baud = atoi(query_result.result[query_result.column + 2]);
		    priv->uart_param.bits = atoi(query_result.result[query_result.column + 3]);
		    priv->uart_param.stops = atoi(query_result.result[query_result.column + 4]);
		    priv->uart_param.parity = atoi(query_result.result[query_result.column + 5]);
			priv->rs232_enable = atoi(query_result.result[query_result.column + 6]);
			printf("protocol_id %d, baud %d, bits %d, stops %d, parity %d, enable %d\n",
				priv->protocol_id, priv->uart_param.baud, priv->uart_param.bits,
				priv->uart_param.stops, priv->uart_param.parity, priv->rs232_enable);
		}
		priv->sys_db_handle->free_table(priv->sys_db_handle, query_result.result);

		protocol = get_protocol_handle(protocol_list, priv->protocol_id);
		if ((protocol != NULL) && (priv->rs232_enable)) {
			break;
		}
		sleep(5);
	}

    print_snmp_protocol(protocol);

    uart_t *uart = uart_create(&(priv->uart_param));
    uart->open(uart);

    list_t *property_list = list_create(sizeof(property_t));
    protocol->get_property(property_list);
	int property_list_size = property_list->get_list_size(property_list);

	int index = 0;
	char buf[256] = {0};
    property_t *property = NULL;
	while (thiz->thread_status) {
		for (index = 0; index < property_list_size; index++) {
			property = property_list->get_index_value(property_list, index);
	        memset(buf, 0, sizeof(buf));
    		print_buf(property->cmd.cmd_code, property->cmd.cmd_len);

	        if (uart->write(uart, property->cmd.cmd_code, property->cmd.cmd_len, 2)
							== property->cmd.cmd_len) {
	            int len = uart->read(uart, buf, property->cmd.check_len, 2);
	            printf("read len %d\n", len);
	            print_buf(buf, len);
	            if (len == property->cmd.check_len) {
	                list_t *value_list = list_create(sizeof(param_value_t));
	                protocol->calculate_data(property, buf, len, value_list);
	                print_param_value(value_list);
					if (property->last_param_value == NULL) {
						create_last_param_value_list(property, value_list);
					} else {
						compare_values(property, value_list);
					}
	                value_list->destroy_list(value_list);
	                value_list = NULL;
	            }
	        } else {
	            printf("write cmd failed------------\n");
	        }
			sleep(1);
		}
	}

    property = NULL;
    protocol->release_property(property_list);
    property_list = NULL;

    deinit_protocol_lib(protocol_list);
    protocol_list = NULL;

    uart->destroy(uart);
    uart = NULL;

	return (void *)0;
}

static void rs232_thread_destroy(thread_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;

        memset(thiz, 0, sizeof(thread_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

thread_t *rs232_thread_create(void)
{
	thread_t *thiz = (thread_t *)calloc(1, sizeof(thread_t) + sizeof(priv_info_t));
	if (thiz != NULL) {
		thiz->thread_ID			= 0;
		thiz->thread_status		= 0;
		thiz->thread_routine	= rs232_process;

		strcpy(thiz->thread_name, "rs232_process");

		thiz->terminate	= thread_terminate;
        thiz->start		= thread_start;
        thiz->join		= thread_join;
        thiz->destroy	= rs232_thread_destroy;
	}

	return thiz;
}
