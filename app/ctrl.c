#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "db_access.h"
#include "debug.h"
#include "preference.h"

#include "protocol_interfaces.h"
#include "drv_gpio.h"

#include "types.h"
#include "mem_pool.h"
#include "ring_buffer.h"

#include "rs232_thread.h"
#include "rs485_thread.h"
#include "di_thread.h"

#include "sms_alarm_thread.h"
#include "data_write_thread.h"

typedef struct {
	db_access_t		*sys_db_handle;
	db_access_t		*data_db_handle;

	db_access_t		*sms_alarm_db_handle;
	db_access_t		*email_alarm_db_handle;

	preference_t	*pref_handle;
} priv_info_t;

static int runnable = 1;

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
        printf("len %d\n", len);
        param_value = param_value_list->get_index_value(param_value_list, i);
        printf("param_id %d\n", param_value->param_id);
        printf("param_value %.1f\n", param_value->param_value);
    }
}

static void signal_processing(int signum)
{
        switch (signum) {
        case SIGKILL:
                runnable = 0;
                //printf("signum[%d]\n", signum);
                break;
        case SIGTERM:
        case SIGINT:
        case SIGSEGV:
                runnable = 0;
                //printf("signum[%d]\n", signum);
                break;
        default:
                break;
        }
}

static void set_signal_handler(int signum, void func(int))
{
        struct sigaction sigAction;
        sigAction.sa_flags = 0;
        sigemptyset(&sigAction.sa_mask);
        sigaddset(&sigAction.sa_mask, SIGINT);
        sigAction.sa_handler = func;
        sigaction(signum, &sigAction, NULL);
}

void create_alarm_table(priv_info_t *priv)
{
	char error_msg[512] = {0};
	char sql[512] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "alarm_record");
	priv->sms_alarm_db_handle->action(priv->sms_alarm_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "alarm_record");
	priv->email_alarm_db_handle->action(priv->email_alarm_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             sent_time TIMESTAMP, \
             protocol_id INTEGER, \
			 protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_desc VARCHAR(32), \
			 alarm_desc VARCHAR(64), \
             alarm_type INTEGER, \
		 	 send_cnt INTEGER, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')))", "alarm_record");
    priv->sms_alarm_db_handle->action(priv->sms_alarm_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             sent_time TIMESTAMP, \
             protocol_id INTEGER, \
			 protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_desc VARCHAR(32), \
			 alarm_desc VARCHAR(64), \
             alarm_type INTEGER, \
		 	 send_cnt INTEGER, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')))", "alarm_record");
    priv->email_alarm_db_handle->action(priv->email_alarm_db_handle, sql, error_msg);
}

void create_data_table(priv_info_t *priv)
{
	char error_msg[512] = {0};
	char sql[512] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "data_record");
	priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "real_data");
	priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "alarm_record");
	priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "sms_record");
	priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "email_record");
	priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_desc VARCHAR(32), \
             alarm_type INTEGER)", "real_data");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_desc VARCHAR(32))", "data_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_desc VARCHAR(32), \
             alarm_desc VARCHAR(64))", "alarm_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             send_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
			 name VARCHAR(32), \
		 	 phone VARCHAR(32), \
		 	 send_status INTEGER, \
             sms_content VARCHAR(64))", "sms_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             send_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
			 name VARCHAR(32), \
		 	 email VARCHAR(64), \
		 	 send_status INTEGER, \
             email_content VARCHAR(64))", "email_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);
}

void update_uart_cfg(priv_info_t *priv)
{
	//初始化
	char error_msg[512] = {0};
	char sql[512] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "uart_cfg");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "support_list");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "parameter");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
	    (port INTEGER PRIMARY KEY, \
	     protocol_id INTEGER, \
	     baud INTEGER, \
	     data_bits INTEGER, \
	     stops_bits INTEGER, \
	     parity INTEGER, \
	     enable INTEGER)", "uart_cfg");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
		(list_index INTEGER PRIMARY KEY, \
		protocol_id INTEGER, \
		protocol_name VARCHAR(32), \
		protocol_desc VARCHAR(128), \
		device_brand VARCHAR(32))", "support_list");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
			(id INTEGER PRIMARY KEY AUTOINCREMENT, \
			protocol_id INTEGER, \
			protocol_name VARCHAR(32), \
			cmd_id INTEGER, \
			param_id INTEGER, \
			param_name VARCHAR(128), \
			param_unit VARCHAR(8), \
			up_limit DOUBLE, \
			up_free DOUBLE, \
			low_limit DOUBLE, \
			low_free DOUBLE, \
			param_type INTEGER, \
			update_threshold DOUBLE, \
			low_desc VARCHAR(8), \
			high_desc VARCHAR(8))", "parameter");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
    sprintf(sql, "INSERT INTO %s \
            (port, protocol_id, baud, data_bits, stops_bits, parity, enable) \
			VALUES (%d, %d, %d, %d, %d, %d, %d)",
			"uart_cfg", 2, 0, 3, 8, 1, 0, 0);
			//"uart_cfg", 2, 257, 3, 8, 1, 0, 1);
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "INSERT INTO %s \
            (port, protocol_id, baud, data_bits, stops_bits, parity, enable) \
			VALUES (%d, %d, %d, %d, %d, %d, %d)",
			"uart_cfg", 3, 0, 3, 8, 1, 0, 0);
			//"uart_cfg", 3, 257, 3, 8, 1, 0, 1);
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	list_t *protocol_list = list_create(sizeof(protocol_t));
	init_protocol_lib(protocol_list);
	int list_size = protocol_list->get_list_size(protocol_list);
	int i = 0;
	protocol_t *tmp = NULL;
	for (i = 0; i < list_size; i++) {
		tmp = protocol_list->get_index_value(protocol_list, i);
	    memset(sql, 0, sizeof(sql));
	    sprintf(sql, "INSERT INTO %s (list_index, protocol_id, protocol_name, \
			protocol_desc, device_brand) VALUES (%d, %d, '%s', '%s', '%s')",
		"support_list", i, tmp->protocol_id,
		tmp->protocol_name, tmp->protocol_desc, tmp->device_brand);
    	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
	}

	for (i = 0; i < list_size; i++) {
		tmp = protocol_list->get_index_value(protocol_list, i);
		list_t *property_list = list_create(sizeof(property_t));
		tmp->get_property(property_list);

		int property_list_size = property_list->get_list_size(property_list);
		property_t *property = NULL;
		list_t *param_desc_list = NULL;
		cmd_t *cmd = NULL;
		int j = 0;
		for (j = 0; j < property_list_size; j++) {
			property = property_list->get_index_value(property_list, j);
			param_desc_list = property->param_desc;
			cmd = &(property->cmd);
			int param_list_size = param_desc_list->get_list_size(param_desc_list);
			int index = 0;
			for (index = 0; index < param_list_size; index++) {
				param_desc_t *param = param_desc_list->get_index_value(param_desc_list, index);
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "INSERT INTO %s (protocol_id, protocol_name, cmd_id, param_id, param_name, \
						param_unit, up_limit, up_free, low_limit, low_free, \
						param_type, update_threshold, low_desc, high_desc) \
						VALUES (%d, '%s', %d, %d, '%s', '%s', '%.1f', '%.1f', '%.1f', '%.1f',\
							%d, '%.1f', '%s', '%s')",
						"parameter", tmp->protocol_id, tmp->protocol_name, cmd->cmd_id,
						param->param_id, param->param_name,
						param->param_unit, param->up_limit, param->up_free,
						param->low_limit, param->low_free, param->param_type,
						param->update_threshold, param->param_enum[0].desc,
						param->param_enum[1].desc);
    			priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
			}
		}

	    property = NULL;
	    tmp->release_property(property_list);
	    property_list = NULL;
	}

    deinit_protocol_lib(protocol_list);
    protocol_list = NULL;
}

void create_di_cfg(priv_info_t *priv)
{
	//初始化 di 配置表
	char error_msg[512] = {0};
	char sql[256] = {0};
    sprintf(sql, "create table if not exists %s \
	    (id INTEGER PRIMARY KEY, \
	     di_name VARCHAR(32), \
		 device_name VARCHAR(32), \
	     low_desc VARCHAR(32), \
	     high_desc VARCHAR(32), \
	     alarm_level INTEGER, \
	     enable INTEGER, \
	     alarm_method INTEGER)", "di_cfg");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s", "di_cfg");

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	priv->sys_db_handle->query(priv->sys_db_handle, sql, &query_result);

	if (query_result.row == 0) {
		int i = 0;
		char di_name[32] = {0};
		for (i = 0; i < 4; i++) {
			memset(sql, 0, sizeof(sql));
			memset(di_name, 0, sizeof(di_name));
			sprintf(di_name, "干接点输入%d", i+1);
		    sprintf(sql, "INSERT INTO %s \
		            (id, di_name, device_name, low_desc, high_desc, alarm_level,\
					enable, alarm_method) VALUES (%d, '%s', '%s', '%s', '%s', %d, %d, %d)",
					"di_cfg", i, di_name, "", "", "", 0, 0, 0);
			priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
		}
	}
}

int main(void)
{
	priv_info_t *priv = (priv_info_t *)calloc(1, sizeof(priv_info_t));
	priv->pref_handle = preference_create();
	int init_flag = priv->pref_handle->get_init_flag(priv->pref_handle);

	priv->sys_db_handle = db_access_create("/opt/app/sys.db");
	priv->data_db_handle = db_access_create("/opt/data/data.db");

	priv->sms_alarm_db_handle = db_access_create("/opt/app/sms_alarm.db");
	priv->email_alarm_db_handle = db_access_create("/opt/app/email_alarm.db");

	create_di_cfg(priv);

	if (init_flag == 1) {
		create_data_table(priv);
		update_uart_cfg(priv);
		create_alarm_table(priv);
		priv->pref_handle->set_init_flag(priv->pref_handle, 0);
	}

	ring_buffer_t *rb_handle = ring_buffer_create(32);
	mem_pool_t *mpool_handle = mem_pool_create(sizeof(msg_t), 32);
	if (mpool_handle == NULL) {
		printf("failed\n");
	}

	ring_buffer_t *sms_rb_handle = ring_buffer_create(5);
	ring_buffer_t *email_rb_handle = ring_buffer_create(5);
	mem_pool_t *alarm_pool_handle = mem_pool_create(sizeof(alarm_msg_t), 10);
	if (alarm_pool_handle == NULL) {
		printf("failed\n");
		return -1;
	}

	thread_t *data_write_thread = data_write_thread_create();
	if (!data_write_thread) {
		printf("create data write thread failed\n");
		return -1;
	}
	data_write_thread_param_t data_write_param;
	data_write_param.self			= data_write_thread;
	data_write_param.data_db_handle	= priv->data_db_handle;
	data_write_param.rb_handle		= rb_handle;
	data_write_param.mpool_handle	= mpool_handle;
	data_write_thread->start(data_write_thread, (void *)&data_write_param);

	thread_t *sms_alarm_thread = sms_alarm_thread_create();
	if (!sms_alarm_thread) {
		printf("create sms alarm thread failed\n");
		return -1;
	}
	sms_alarm_thread_param_t sms_alarm_param;
	sms_alarm_param.self				= sms_alarm_thread;
	sms_alarm_param.sms_alarm_db_handle	= priv->sms_alarm_db_handle;
	sms_alarm_param.sys_db_handle		= priv->sys_db_handle;
	sms_alarm_param.pref_handle 		= priv->pref_handle;
	sms_alarm_param.rb_handle			= rb_handle;
	sms_alarm_param.mpool_handle		= mpool_handle;
	sms_alarm_param.sms_rb_handle		= sms_rb_handle;
	sms_alarm_param.alarm_pool_handle	= alarm_pool_handle;
	sms_alarm_thread->start(sms_alarm_thread, (void *)&sms_alarm_param);

	thread_t *rs232_thread = rs232_thread_create();
	if (!rs232_thread) {
		printf("create rs232 thread failed\n");
		return -1;
	}
	rs232_thread_param_t rs232_thread_param;
	rs232_thread_param.self				= rs232_thread;
	rs232_thread_param.sys_db_handle	= priv->sys_db_handle;
	rs232_thread_param.rb_handle 		= rb_handle;
	rs232_thread_param.mpool_handle		= mpool_handle;
	rs232_thread_param.pref_handle		= priv->pref_handle;
	rs232_thread_param.sms_rb_handle	= sms_rb_handle;
	rs232_thread_param.email_rb_handle	= email_rb_handle;
	rs232_thread_param.alarm_pool_handle = alarm_pool_handle;
	rs232_thread_param.init_flag		= init_flag;
	rs232_thread->start(rs232_thread, (void *)&rs232_thread_param);

	thread_t *rs485_thread = rs485_thread_create();
	if (!rs485_thread) {
		printf("create rs485 thread failed\n");
		return -1;
	}
	rs485_thread_param_t rs485_thread_param;
	rs485_thread_param.self				= rs485_thread;
	rs485_thread_param.sys_db_handle	= priv->sys_db_handle;
	rs485_thread_param.rb_handle 		= rb_handle;
	rs485_thread_param.mpool_handle		= mpool_handle;
	rs485_thread_param.pref_handle		= priv->pref_handle;
	rs485_thread_param.sms_rb_handle	= sms_rb_handle;
	rs485_thread_param.email_rb_handle	= email_rb_handle;
	rs485_thread_param.alarm_pool_handle = alarm_pool_handle;
	rs485_thread_param.init_flag		= init_flag;
	rs485_thread->start(rs485_thread, (void *)&rs485_thread_param);

	/*thread_t *di_thread = di_thread_create();
	if (!di_thread) {
		printf("create di thread failed\n");
		return -1;
	}
	di_thread_param_t di_thread_param;
	di_thread_param.self			= di_thread;
	di_thread_param.sys_db_handle	= priv->sys_db_handle;
	di_thread_param.rb_handle 		= rb_handle;
	di_thread_param.mpool_handle	= mpool_handle;
	di_thread_param.pref_handle		= priv->pref_handle;
	di_thread_param.sms_rb_handle	= sms_rb_handle;
	di_thread_param.email_rb_handle	= email_rb_handle;
	di_thread_param.alarm_pool_handle = alarm_pool_handle;
	di_thread_param.init_flag		= init_flag;
	di_thread->start(di_thread, (void *)&di_thread_param);*/

    while (runnable) {
		sleep(3);
		priv->pref_handle->reload(priv->pref_handle);
    }

	rs232_thread->terminate(rs232_thread);
	rs485_thread->terminate(rs485_thread);
	//di_thread->terminate(di_thread);

	rs232_thread->join(rs232_thread);
	rs485_thread->join(rs485_thread);
	//di_thread->join(di_thread);

	rs232_thread->destroy(rs232_thread);
	rs485_thread->destroy(rs485_thread);
	//di_thread->destroy(di_thread);

	rs232_thread = NULL;
	rs485_thread = NULL;
	//di_thread = NULL;

	sms_alarm_thread->terminate(sms_alarm_thread);
	sms_alarm_thread->join(sms_alarm_thread);
	sms_alarm_thread->destroy(sms_alarm_thread);
	sms_alarm_thread = NULL;

	data_write_thread->terminate(data_write_thread);
	data_write_thread->join(data_write_thread);
	data_write_thread->destroy(data_write_thread);
	data_write_thread = NULL;

	if (priv->sys_db_handle) {
    	priv->sys_db_handle->destroy(priv->sys_db_handle);
		priv->sys_db_handle = NULL;
	}

	if (priv->data_db_handle) {
    	priv->data_db_handle->destroy(priv->data_db_handle);
		priv->data_db_handle = NULL;
	}

	if (priv->sms_alarm_db_handle) {
		priv->sms_alarm_db_handle->destroy(priv->sms_alarm_db_handle);
		priv->sms_alarm_db_handle = NULL;
	}

	if (priv->email_alarm_db_handle) {
		priv->email_alarm_db_handle->destroy(priv->email_alarm_db_handle);
		priv->email_alarm_db_handle = NULL;
	}

	if (priv->pref_handle) {
		priv->pref_handle->destroy(priv->pref_handle);
		priv->pref_handle = NULL;
	}

	rb_handle->destroy(rb_handle);
	rb_handle = NULL;

	sms_rb_handle->destroy(sms_rb_handle);
	sms_rb_handle = NULL;

	email_rb_handle->destroy(email_rb_handle);
	email_rb_handle = NULL;

	alarm_pool_handle->mpool_destroy(alarm_pool_handle);
	alarm_pool_handle = NULL;

	mpool_handle->mpool_destroy(mpool_handle);
	mpool_handle = NULL;

	free(priv);
	priv = NULL;

    return 0;
}
