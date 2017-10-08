#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <linux/watchdog.h>
#include <fcntl.h>

#include "db_access.h"
#include "debug.h"
#include "preference.h"

#include "protocol_interfaces.h"
#include "drv_gpio.h"
#include "utils.h"

#include "types.h"
#include "mem_pool.h"
#include "ring_buffer.h"

#include "rs232_thread.h"
#include "rs485_thread.h"
#include "di_thread.h"

#include "sms_alarm_thread.h"
#include "email_alarm_thread.h"
#include "data_write_thread.h"
#include "beep_thread.h"
#include "version.h"
#include "logger.h"

/**
  @brief    主函数私有成员结构
 */
typedef struct {
	db_access_t		*sys_db_handle;
	db_access_t		*data_db_handle;

	preference_t	*pref_handle;
} priv_info_t;

/**
  @brief    主线程是否正常运行状态标记
 */
static int runnable = 1;

/**
  @brief    信号捕获函数
  @param    signum 信号编号
  @return

  根据信号值判断是否要退出程序
 */
static void signal_processing(int signum)
{
        switch (signum) {
        case SIGKILL:
                runnable = 0;
                break;
        case SIGTERM:
        case SIGINT:
        case SIGSEGV:
                runnable = 0;
                break;
        default:
                break;
        }
}

/**
  @brief    设置信号处理函数
  @param    signum 信号编号
  @param	func(int) 信号处理函数指针
  @return
 */
static void set_signal_handler(int signum, void func(int))
{
        struct sigaction sigAction;
        sigAction.sa_flags = 0;
        sigemptyset(&sigAction.sa_mask);
        sigaddset(&sigAction.sa_mask, SIGINT);
        sigAction.sa_handler = func;
        sigaction(signum, &sigAction, NULL);
}

/**
  @brief    启用硬件看门狗
  @return	0	启用硬件看门狗成功
			-1	启用硬件看门狗失败
 */
static int watchdog_open(void)
{
    int fd = open("/dev/watchdog",O_RDWR);
    if(fd < 0){
        perror("/dev/watchdog");
        return -1;
    }

    return fd;
}

/**
  @brief    喂硬件看门狗
  @param	fd 硬件看门狗设备文件句柄
  @return
 */
static void watchdog_feed(int fd)
{
    ioctl(fd, WDIOC_KEEPALIVE);
}

/**
  @brief    设置硬件看门狗超时时长
  @param	fd 硬件看门狗设备文件句柄
  @param	timeout 超时时间，因驱动中限制，不得超过30秒
  @return	-1	设置失败
			0	设置成功
 */
static int watchdog_set_timeout(int fd, unsigned long timeout)
{
    if (ioctl(fd, WDIOC_SETTIMEOUT, &timeout) < 0) {
        perror("set watch dog timeout time failed");
        return -1;
    }

    return 0;
}

/**
  @brief    创建数据表
  @param	priv 私有成员指针
  @return
 */
void create_data_table(priv_info_t *priv)
{
	char error_msg[512] = {0};
	char sql[1024] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "data_record");
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
			 protocol_desc VARCHAR(128), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
			 param_desc	VARCHAR(128), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_en_desc VARCHAR(32), \
             enum_cn_desc VARCHAR(32))", "data_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 protocol_desc VARCHAR(128), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
			 param_desc	VARCHAR(128), \
             param_type INTEGER, \
             analog_value DOUBLE, \
			 unit VARCHAR(32), \
             enum_value INTEGER, \
             enum_en_desc VARCHAR(32), \
             enum_cn_desc VARCHAR(32), \
             alarm_desc VARCHAR(256))", "alarm_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             send_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 protocol_desc VARCHAR(128), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
			 param_desc	VARCHAR(128), \
			 param_type INTEGER, \
             analog_value DOUBLE, \
			 enum_value INTEGER, \
			 enum_desc VARCHAR(32), \
			 name VARCHAR(32), \
		 	 phone VARCHAR(32), \
		 	 send_status INTEGER, \
             sms_content VARCHAR(256))", "sms_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             send_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             protocol_id INTEGER, \
             protocol_name VARCHAR(32), \
			 protocol_desc VARCHAR(128), \
			 param_id INTEGER, \
             param_name VARCHAR(32), \
			 param_desc	VARCHAR(128), \
			 param_type INTEGER, \
             analog_value DOUBLE, \
			 enum_value INTEGER, \
			 enum_desc VARCHAR(32), \
			 name VARCHAR(32), \
		 	 email VARCHAR(64), \
		 	 send_status INTEGER, \
             email_content VARCHAR(256))", "email_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);
}

/**
  @brief    创建或重建串口配置表、协议库支持设备表、协议库设备参数信息表
  @param	priv 私有成员指针
  @return
 */
void update_uart_cfg(priv_info_t *priv)
{
	//初始化
	char error_msg[512] = {0};
	char sql[1024] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "uart_cfg");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "protocol_cfg");
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
	     baud INTEGER, \
	     data_bits INTEGER, \
	     stops_bits INTEGER, \
	     parity INTEGER)", "uart_cfg");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
		(id INTEGER PRIMARY KEY AUTOINCREMENT, \
		com_index INTEGER, \
		seq_index INTEGER, \
		protocol_id INTEGER)", "protocol_cfg");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
		(list_index INTEGER PRIMARY KEY, \
		protocol_id INTEGER, \
		protocol_name VARCHAR(32), \
		protocol_desc VARCHAR(128))", "support_list");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
			(id INTEGER PRIMARY KEY AUTOINCREMENT, \
			protocol_id INTEGER, \
			protocol_name VARCHAR(32), \
			protocol_desc VARCHAR(128), \
			cmd_id INTEGER, \
			param_id INTEGER, \
			param_name VARCHAR(32), \
			param_desc VARCHAR(128), \
			param_unit VARCHAR(8), \
			up_limit DOUBLE, \
			up_free DOUBLE, \
			low_limit DOUBLE, \
			low_free DOUBLE, \
			param_type INTEGER, \
			update_threshold DOUBLE, \
			low_en_desc VARCHAR(32), \
			low_cn_desc VARCHAR(32), \
			high_en_desc VARCHAR(32), \
			high_cn_desc VARCHAR(32))", "parameter");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
    sprintf(sql, "INSERT INTO %s \
            (port, baud, data_bits, stops_bits, parity) \
			VALUES (%d, %d, %d, %d, %d)",
			"uart_cfg", 1, 3, 8, 1, 0);
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "INSERT INTO %s \
            (port, baud, data_bits, stops_bits, parity) \
			VALUES (%d, %d, %d, %d, %d)",
			"uart_cfg", 2, 3, 8, 1, 0);
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	int i = 0;
	for (i = 0; i < 4; i++) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "INSERT INTO %s \
				(com_index, seq_index, protocol_id) VALUES (%d, %d, %d)",
				"protocol_cfg", 1, i + 1, 0);
		priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
	}

	for (i = 0; i < 4; i++) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "INSERT INTO %s \
				(com_index, seq_index, protocol_id) VALUES (%d, %d, %d)",
				"protocol_cfg", 2, i + 1, 0);
		priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
	}

	list_t *protocol_list = list_create(sizeof(protocol_t));
	init_protocol_lib(protocol_list);
	int list_size = protocol_list->get_list_size(protocol_list);
	protocol_t *tmp = NULL;
	for (i = 0; i < list_size; i++) {
		tmp = protocol_list->get_index_value(protocol_list, i);
		memset(sql, 0, sizeof(sql));
	    sprintf(sql, "INSERT INTO %s (list_index, protocol_id, protocol_name, protocol_desc) \
				VALUES (%d, %d, '%s', '%s')",
				"support_list", i, tmp->protocol_id, tmp->protocol_name, tmp->protocol_desc);
				priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
	}

	for (i = 0; i < list_size; i++) {
		tmp = protocol_list->get_index_value(protocol_list, i);
		list_t *property_list = list_create(sizeof(property_t));
		tmp->get_property(property_list, tmp->rs485_addr);

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
				sprintf(sql, "INSERT INTO %s (protocol_id, protocol_name, protocol_desc, \
						cmd_id, param_id, param_name, param_desc, param_unit, \
						up_limit, up_free, low_limit, low_free, param_type, update_threshold, \
						low_en_desc, low_cn_desc, high_en_desc, high_cn_desc) \
						VALUES (%d, '%s', '%s', %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s',\
							%d, '%.1f', '%s', '%s', '%s', '%s')",
						"parameter", tmp->protocol_id, tmp->protocol_name, tmp->protocol_desc,
						cmd->cmd_id, param->param_id, param->param_name, param->param_desc,
						param->param_unit, "", "", "", "", param->param_type,
						param->update_threshold, param->param_enum[0].en_desc, param->param_enum[0].cn_desc,
						param->param_enum[1].en_desc, param->param_enum[1].cn_desc);
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

/**
  @brief    创建或重建DI信息表
  @param	priv 私有成员指针
  @return
 */
void create_di_cfg(priv_info_t *priv)
{
	//初始化 di 配置表
	char error_msg[512] = {0};
	char sql[512] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "di_cfg");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

    sprintf(sql, "create table if not exists %s \
	    (id INTEGER PRIMARY KEY, \
	     di_name VARCHAR(32), \
		 di_desc VARCHAR(32), \
		 device_name VARCHAR(32), \
	     low_desc VARCHAR(32), \
	     high_desc VARCHAR(32), \
	     alarm_level INTEGER, \
	     enable INTEGER, \
	     alarm_method INTEGER)", "di_cfg");
    priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	int i = 0;
	char di_name[32] = {0};
	char di_desc[32] = {0};
	for (i = 0; i < 4; i++) {
		memset(sql, 0, sizeof(sql));
		memset(di_desc, 0, sizeof(di_desc));
		memset(di_name, 0, sizeof(di_name));
		sprintf(di_desc, "干接点输入%d", i + 1);
		sprintf(di_name, "di%d", i + 1);
		sprintf(sql, "INSERT INTO %s \
				(id, di_name, di_desc, device_name, low_desc, high_desc, alarm_level,\
				enable, alarm_method) VALUES (%d, '%s', '%s', '%s', '%s', '%s', %d, %d, %d)",
				"di_cfg", i + 1, di_name, di_desc, "", "", "", 0, 0, 0);
		priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
	}
}

/**
  @brief    创建或重建用户信息表、短信报警和邮件报警联系人表
  @param	priv 私有成员指针
  @return
 */
static void create_user(priv_info_t *priv)
{
	//初始化 用户表
	char error_msg[512] = {0};
	char sql[256] = {0};
	sprintf(sql, "DROP TABLE IF EXISTS %s", "user_manager");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
		(id INTEGER PRIMARY KEY, \
			user VARCHAR(32), \
			password VARCHAR(32), \
			permit VARCHAR(32), \
			description VARCHAR(32))", "user_manager");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "INSERT INTO %s \
			(user, password, permit, description) \
			VALUES ('%s', '%s', %d, '%s')", "user_manager",
			"admin", "admin", 1, "管理员");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "INSERT INTO %s \
			(user, password, permit, description) \
			VALUES ('%s', '%s', %d, '%s')", "user_manager",
			"ctrl", "ctrl", 2, "控制操作员");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "INSERT INTO %s \
			(user, password, permit, description) \
			VALUES ('%s', '%s', %d, '%s')", "user_manager",
			"monitor", "monitor", 4, "监查人员");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "INSERT INTO %s \
			(user, password, permit, description) \
			VALUES ('%s', '%s', %d, '%s')", "user_manager",
			"guest", "guest", 8, "访客");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "phone_user");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
					(id INTEGER PRIMARY KEY AUTOINCREMENT, \
					name VARCHAR(32), \
					phone VARCHAR(32))", "phone_user");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "DROP TABLE IF EXISTS %s", "email_user");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
					(id INTEGER PRIMARY KEY AUTOINCREMENT, \
					name VARCHAR(32), \
					email VARCHAR(32))", "email_user");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);
}

/**
  @brief    初始化DO输出口
  @param	priv 私有成员指针
  @return
 */
static void init_do_output(priv_info_t *priv)
{
	do_param_t param = priv->pref_handle->get_do_param(priv->pref_handle);

	int i = 0;
    for (i = 0; i < 3; i++) {
		drv_gpio_open(i + 5);
		drv_gpio_write(i + 5, param.status[i]);
		//drv_gpio_close(i + 5);
    }
}

/**
  @brief    生成设备断电报警信息并推送到邮件和短信报警信息队列
  @param	sms_rb_handle 短信报警队列
  @param	email_rb_handle 邮件报警队列
  @param	alarm_pool_handle 报警信息内存池
  @return
 */
static void send_alarm_msg(ring_buffer_t	*sms_rb_handle,
						   ring_buffer_t	*email_rb_handle,
						   mem_pool_t		*alarm_pool_handle)
{
	alarm_msg_t *alarm_msg = (alarm_msg_t *)alarm_pool_handle->mpool_alloc(alarm_pool_handle);
	if (alarm_msg == NULL) {
		printf("memory pool is empty\n");
		return;
	}

	alarm_msg_t *tmp_alarm_msg = (alarm_msg_t *)alarm_pool_handle->mpool_alloc(alarm_pool_handle);
	if (tmp_alarm_msg == NULL) {
		printf("memory pool is empty\n");
		alarm_pool_handle->mpool_free(alarm_pool_handle, (void *)alarm_msg);
		return;
	}

	alarm_msg->alarm_type = ALARM_DISCARD;
	alarm_msg->protocol_id = 0x00000001;
	strcpy(alarm_msg->protocol_name, "DI");
	alarm_msg->param_id = 0;
	strcpy(alarm_msg->param_name, "input_power");
	strcpy(alarm_msg->param_desc, "设备供电");
	strcpy(alarm_msg->param_unit, "");
	alarm_msg->param_type = PARAM_TYPE_ENUM;
	alarm_msg->param_value = 0.0;
	alarm_msg->enum_value = 1;
	strcpy(alarm_msg->enum_desc, "lost_power");
	strcpy(alarm_msg->alarm_desc, "设备停电");
	memcpy(tmp_alarm_msg, alarm_msg, sizeof(alarm_msg_t));

	if (sms_rb_handle->push(sms_rb_handle, (void *)alarm_msg)) {
		printf("sms ring buffer is full\n");
		alarm_pool_handle->mpool_free(alarm_pool_handle, (void *)alarm_msg);
	}
	alarm_msg = NULL;

	if (email_rb_handle->push(email_rb_handle, (void *)tmp_alarm_msg)) {
		printf("email ring buffer is full\n");
		alarm_pool_handle->mpool_free(alarm_pool_handle, (void *)tmp_alarm_msg);
	}
	tmp_alarm_msg = NULL;
}

/**
  @brief    输出程序版本信息到/tmp/version文件中
  @return
 */
static void create_version_info(void)
{
	char current_timing[32] = {0};
	struct timeval now_time;
	struct tm *tm = NULL;
	gettimeofday(&now_time, NULL);
    tm = localtime(&(now_time.tv_sec));

	sprintf(current_timing, "%04d-%02d-%02d %02d:%02d:%02d",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	char tmp[1024] = {0};
	sprintf(tmp, "%s\n%s\n%s\n", APP_VERSION, get_protocol_version(), current_timing);

	FILE *fp = fopen("/tmp/version", "wb");
	int ret = fwrite(tmp, 1, strlen(tmp), fp);
	fclose(fp);
	fp = NULL;
}

/**
  @brief    主程序入口
  @return
 */
int main(void)
{
	create_version_info();
	snmp_log_init(INFO);

	priv_info_t *priv = (priv_info_t *)calloc(1, sizeof(priv_info_t));
	priv->pref_handle = preference_create();
	int init_flag = file_exist("/opt/app/recovery_default");

	priv->sys_db_handle = db_access_create("/opt/app/sys.db");
	priv->data_db_handle = db_access_create("/opt/data/data.db");

	init_do_output(priv);

	if (init_flag == 0) {
		create_user(priv);
		create_di_cfg(priv);
		create_data_table(priv);
		update_uart_cfg(priv);
		file_remove("/opt/app/recovery_default");
		snmp_log(WARN, "recovery to default setting\n");
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
	sms_alarm_param.sys_db_handle		= priv->sys_db_handle;
	sms_alarm_param.pref_handle 		= priv->pref_handle;
	sms_alarm_param.rb_handle			= rb_handle;
	sms_alarm_param.mpool_handle		= mpool_handle;
	sms_alarm_param.sms_rb_handle		= sms_rb_handle;
	sms_alarm_param.alarm_pool_handle	= alarm_pool_handle;
	sms_alarm_thread->start(sms_alarm_thread, (void *)&sms_alarm_param);

	thread_t *email_alarm_thread = email_alarm_thread_create();
	if (!email_alarm_thread) {
		printf("create email alarm thread failed\n");
		return -1;
	}
	email_alarm_thread_param_t email_alarm_param;
	email_alarm_param.self					= email_alarm_thread;
	email_alarm_param.sys_db_handle			= priv->sys_db_handle;
	email_alarm_param.pref_handle 			= priv->pref_handle;
	email_alarm_param.rb_handle				= rb_handle;
	email_alarm_param.mpool_handle			= mpool_handle;
	email_alarm_param.email_rb_handle		= email_rb_handle;
	email_alarm_param.alarm_pool_handle		= alarm_pool_handle;
	email_alarm_thread->start(email_alarm_thread, (void *)&email_alarm_param);

	unsigned char com2_status = 0;
	drv_gpio_open(COM2_SELECTOR);
	drv_gpio_read(COM2_SELECTOR, &com2_status);
	drv_gpio_close(COM2_SELECTOR);

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
	rs232_thread_param.data_db_handle	= priv->data_db_handle;
	rs232_thread_param.com_selector		= com2_status;
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
	rs485_thread_param.data_db_handle	= priv->data_db_handle;
	rs485_thread->start(rs485_thread, (void *)&rs485_thread_param);

	thread_t *di_thread = di_thread_create();
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
	di_thread->start(di_thread, (void *)&di_thread_param);

	thread_t *beep_thread = beep_thread_create();
	if (!beep_thread) {
		printf("create beep thread failed\n");
		return -1;
	}
	beep_thread_param_t beep_thread_param;
	beep_thread_param.self			= beep_thread;
	beep_thread_param.pref_handle	= priv->pref_handle;
	beep_thread->start(beep_thread, (void *)&beep_thread_param);

	snmp_log(WARN, "All threads running, application start\n");

	/* 使能电源管理模块 */
	unsigned char power_status = 0;
	drv_gpio_open(POFF_PIN);
	drv_gpio_write(POFF_PIN, 1);
	drv_gpio_open(PD_INT_PIN);

	int cnt = 0;
	int power_off_cnt = 0;
	int alarm_flag = 0;

	/* 打开看门狗 */
    int fd = watchdog_open();
    if (fd < 0) {
        exit(1);
    }
    unsigned long timeout = 30;
    watchdog_set_timeout(fd, timeout);	/* 设置看门狗超时时间，不得超过30秒 */

	int reboot_flag = 0;
    while (runnable) {
		/* 喂狗 */
		watchdog_feed(fd);

		sleep(1);
		drv_gpio_read(PD_INT_PIN, &power_status);
		if (power_status) {
			if (alarm_flag == 0) {
				alarm_flag = 1;
				send_alarm_msg(sms_rb_handle, email_rb_handle, alarm_pool_handle);
				snmp_log(WARN, "Power supply off\n");
			}
			power_off_cnt++;
			if (power_off_cnt > 20) {
				drv_gpio_write(POFF_PIN, 0);
			}
		} else {
			power_off_cnt = 0;
			alarm_flag = 0;
		}

		cnt++;
		if (cnt > 5) {
			priv->pref_handle->reload(priv->pref_handle);
			cnt = 0;

			FILE *fp = fopen("/tmp/reboot", "rb");
			if (fp != NULL) {
				fclose(fp);
				reboot_flag = 1;
				snmp_log(WARN, "Mannual reboot the device\n");
				break;
			}
		}
    }

	rs232_thread->terminate(rs232_thread);
	rs485_thread->terminate(rs485_thread);
	di_thread->terminate(di_thread);

	rs232_thread->join(rs232_thread);
	rs485_thread->join(rs485_thread);
	di_thread->join(di_thread);

	rs232_thread->destroy(rs232_thread);
	rs485_thread->destroy(rs485_thread);
	di_thread->destroy(di_thread);

	rs232_thread = NULL;
	rs485_thread = NULL;
	di_thread = NULL;

	sms_alarm_thread->terminate(sms_alarm_thread);
	sms_alarm_thread->join(sms_alarm_thread);
	sms_alarm_thread->destroy(sms_alarm_thread);
	sms_alarm_thread = NULL;

	email_alarm_thread->terminate(email_alarm_thread);
	email_alarm_thread->join(email_alarm_thread);
	email_alarm_thread->destroy(email_alarm_thread);
	email_alarm_thread = NULL;

	data_write_thread->terminate(data_write_thread);
	data_write_thread->join(data_write_thread);
	data_write_thread->destroy(data_write_thread);
	data_write_thread = NULL;

	beep_thread->terminate(beep_thread);
	beep_thread->join(beep_thread);
	beep_thread->destroy(beep_thread);
	beep_thread = NULL;

	if (priv->sys_db_handle) {
    	priv->sys_db_handle->destroy(priv->sys_db_handle);
		priv->sys_db_handle = NULL;
	}

	if (priv->data_db_handle) {
    	priv->data_db_handle->destroy(priv->data_db_handle);
		priv->data_db_handle = NULL;
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

	int i = 0;
    for (i = 0; i < 8; i++) {
		drv_gpio_close(i);
    }

	if (reboot_flag == 1) {
		reboot(RB_AUTOBOOT);
	}

    return 0;
}
