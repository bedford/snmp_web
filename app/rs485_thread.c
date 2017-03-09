#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "protocol_interfaces.h"
#include "thread_process.h"
#include "rs485_thread.h"
#include "debug.h"
#include "db_access.h"
#include "uart.h"
#include "drv_gpio.h"

typedef struct {
	db_access_t *sys_db_handle;
	uart_param_t uart_param;
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
static void *rs485_process(void *arg)
{
	rs485_thread_param_t *thread_param = (rs485_thread_param_t *)arg;
	thread_t *thiz = thread_param->self;

	priv_info_t *priv = (priv_info_t *)thiz->priv;
	priv->sys_db_handle = (db_access_t *)thread_param->sys_db_handle;
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
	                //record_data(priv->data_db_handle, value_list, protocol);
	                value_list->destroy_list(value_list);
	                value_list = NULL;
	            }
	        } else {
	            printf("write cmd failed------------\n");
	        }
			sleep(1);
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
