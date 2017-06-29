#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "cJSON.h"
#include "iniparser.h"
#include "db_access.h"
#include "mib_create.h"

#include "common_type.h"
#include "shm_object.h"
#include "semaphore.h"

#define INI_FILE_NAME	"/opt/app/param.ini"
#define MIB_FILE_NAME	"/tmp/JT_Gaurd.mib"

typedef struct {
    int     req_len;
    int     max_len;
    char    *buf;

    char    *fb_buf;    /* 返回值内存指针 */
    int     fb_len;     /* 返回值长度 */
} req_buf_t;

typedef struct {
	req_buf_t	request;
	db_access_t *sys_db_handle;
	db_access_t *data_db_handle;
	dictionary	*dic;
} priv_info_t;

static int get_network_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "mac_addr", "F0:FF:04:00:5D:F4");
    cJSON_AddStringToObject(response, "ip_addr",
            iniparser_getstring(dic, "NETWORK:ip_addr", "192.168.0.100"));
    cJSON_AddStringToObject(response, "gateway",
            iniparser_getstring(dic, "NETWORK:gateway", "192.168.0.1"));
    cJSON_AddStringToObject(response, "netmask",
            iniparser_getstring(dic, "NETWORK:netmask", "255.255.255.0"));
    cJSON_AddStringToObject(response, "master_dns",
            iniparser_getstring(dic, "NETWORK:master_dns", "192.168.8.8"));
    cJSON_AddStringToObject(response, "slave_dns",
            iniparser_getstring(dic, "NETWORK:slave_dns", "8.8.8.8"));
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_snmp_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *sub_dir = NULL;
    cJSON *child = NULL;

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "snmp_union",
            iniparser_getstring(dic, "SNMP:snmp_union", "public"));
    cJSON_AddStringToObject(response, "trap_server_ip",
            iniparser_getstring(dic, "SNMP:trap_server_ip", "192.168.0.100"));
    cJSON_AddNumberToObject(response, "enterprise_code",
            iniparser_getint(dic, "SNMP:enterprise_code", 999));
    cJSON_AddStringToObject(response, "enterprise_name",
            iniparser_getstring(dic, "SNMP:enterprise_name", "Jitong"));
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "authority_ip", sub_dir);

	int i = 0;
	char item_name[32] = {0};
	for (i = 0; i < 4; i++) {
    	child = cJSON_CreateObject();
		sprintf(item_name, "SNMP:valid_flag_%d", i);
    	cJSON_AddNumberToObject(child, "valid_flag",
                iniparser_getint(dic, item_name, 0));
		sprintf(item_name, "SNMP:authority_ip_%d", i);
    	cJSON_AddStringToObject(child, "ip",
                iniparser_getstring(dic, item_name, "192.168.0.100"));
    	cJSON_AddItemToArray(sub_dir, child);
	}

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_uart_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

    cJSON *sub_dir = NULL;
    cJSON *child = NULL;
    cJSON *response = cJSON_CreateObject();

	int i = 0;
	query_result_t query_result;
	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY list_index", "support_list");
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);
	cJSON_AddNumberToObject(response, "support_list_count", query_result.row);
	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "support_list", sub_dir);
		for (i = 1; i < (query_result.row + 1); i++) {
        	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "list_index", query_result.result[i * query_result.column]);
			cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 1]);
			cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[i * query_result.column + 3]);
    		cJSON_AddItemToArray(sub_dir, child);
    	}
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s WHERE port=2", "uart_cfg");
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);
	if (query_result.row > 0) {
		sub_dir = cJSON_CreateObject();
    	cJSON_AddItemToObject(response, "rs232_cfg", sub_dir);
    	cJSON_AddNumberToObject(sub_dir, "port", atoi(query_result.result[query_result.column]));
		cJSON_AddNumberToObject(sub_dir, "protocol_id", atoi(query_result.result[query_result.column + 1]));
		cJSON_AddNumberToObject(sub_dir, "baud", atoi(query_result.result[query_result.column + 2]));
		cJSON_AddNumberToObject(sub_dir, "data_bits", atoi(query_result.result[query_result.column + 3]));
		cJSON_AddNumberToObject(sub_dir, "stops_bits", atoi(query_result.result[query_result.column + 4]));
		cJSON_AddNumberToObject(sub_dir, "parity", atoi(query_result.result[query_result.column + 5]));
		cJSON_AddNumberToObject(sub_dir, "enable", atoi(query_result.result[query_result.column + 6]));
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s WHERE port=3", "uart_cfg");
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);
	if (query_result.row > 0) {
		sub_dir = cJSON_CreateObject();
    	cJSON_AddItemToObject(response, "rs485_cfg", sub_dir);
    	cJSON_AddNumberToObject(sub_dir, "port", atoi(query_result.result[query_result.column]));
		cJSON_AddNumberToObject(sub_dir, "protocol_id", atoi(query_result.result[query_result.column + 1]));
		cJSON_AddNumberToObject(sub_dir, "baud", atoi(query_result.result[query_result.column + 2]));
		cJSON_AddNumberToObject(sub_dir, "data_bits", atoi(query_result.result[query_result.column + 3]));
		cJSON_AddNumberToObject(sub_dir, "stops_bits", atoi(query_result.result[query_result.column + 4]));
		cJSON_AddNumberToObject(sub_dir, "parity", atoi(query_result.result[query_result.column + 5]));
		cJSON_AddNumberToObject(sub_dir, "enable", atoi(query_result.result[query_result.column + 6]));
	}

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_ntp_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "ntp_server_ip",
            iniparser_getstring(dic, "NTP:ntp_server_ip", "192.168.0.201"));
    cJSON_AddNumberToObject(response, "ntp_interval",
            iniparser_getint(dic, "NTP:ntp_interval", 60));

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_do_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "beep_alarm_enable",
            iniparser_getint(dic, "DO:beep_alarm_enable", 0));

    cJSON *sub_dir = cJSON_CreateArray();
	cJSON *child = NULL;
    cJSON_AddItemToObject(response, "do_setting", sub_dir);
	char value_item_name[16] = {0};
	char item_name[16] = {0};
	int i = 0;
    for (i = 0; i < 3; i++) {
		sprintf(value_item_name, "DO:do%d_value", i + 2);
		sprintf(item_name, "DO:do%d_name", i + 2);

        child = cJSON_CreateObject();
		cJSON_AddNumberToObject(child, "setting",
			iniparser_getint(dic, value_item_name, 0));
		cJSON_AddStringToObject(child, "name",
			iniparser_getstring(dic, item_name, "干接点输出"));
    	cJSON_AddItemToArray(sub_dir, child);
    }

    sub_dir = cJSON_CreateArray();
	cJSON_AddItemToObject(response, "do_status", sub_dir);
	unsigned char value = 0;
    for (i = 0; i < 4; i++) {
		drv_gpio_open(i + 4);
		drv_gpio_read(i + 4, &value);
        child = cJSON_CreateObject();
		cJSON_AddNumberToObject(child, "value", value);
    	cJSON_AddItemToArray(sub_dir, child);
    }

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_di_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

    cJSON *sub_dir = NULL;
    cJSON *child = NULL;
    cJSON *response = cJSON_CreateObject();

	int i = 0;
	query_result_t query_result;
	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER by id", "di_cfg");
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);
	cJSON_AddNumberToObject(response, "count", query_result.row);

	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "di_param", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddNumberToObject(child, "id", atoi(query_result.result[i * query_result.column]));
			cJSON_AddStringToObject(child, "di_name", query_result.result[i * query_result.column + 1]);
			cJSON_AddStringToObject(child, "di_desc", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "device_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "low_desc", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "high_desc", query_result.result[i * query_result.column + 5]);
			cJSON_AddNumberToObject(child, "alarm_level", atoi(query_result.result[i * query_result.column + 6]));
			cJSON_AddNumberToObject(child, "enable", atoi(query_result.result[i * query_result.column + 7]));
			cJSON_AddNumberToObject(child, "alarm_method", atoi(query_result.result[i * query_result.column + 8]));
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static void write_profile(dictionary    *dic,
                          const char    *section,
                          const char    *key,
                          const char    *value)
{
    if (!iniparser_find_entry(dic, section)) {
            printf("dump section %s\n", section);
            iniparser_set(dic, section, NULL);
    }

    char tmp[256] = {0};
    sprintf(tmp, "%s:%s", section, key);
    iniparser_set(dic, tmp, value);
}

static void dump_profile(dictionary *dic, char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {
        iniparser_dump_ini(dic, fp);
        fclose(fp);
        fp = NULL;
    }
}

static int set_network_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "NETWORK", "ip_addr",
            cJSON_GetObjectItem(cfg, "ip_addr")->valuestring);
    write_profile(dic, "NETWORK", "gateway",
            cJSON_GetObjectItem(cfg, "gateway")->valuestring);
    write_profile(dic, "NETWORK", "netmask",
            cJSON_GetObjectItem(cfg, "netmask")->valuestring);
    write_profile(dic, "NETWORK", "master_dns",
            cJSON_GetObjectItem(cfg, "master_dns")->valuestring);
    write_profile(dic, "NETWORK", "slave_dns",
            cJSON_GetObjectItem(cfg, "slave_dns")->valuestring);
    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();

	char buf[64] = {0};
	FILE *fp = fopen("/etc/resolv.conf", "wb+");
	sprintf(buf, "nameserver %s\n", cJSON_GetObjectItem(cfg, "master_dns")->valuestring);
	cJSON_AddStringToObject(response, "master_dns", buf);
	int ret = fwrite(buf, 1, strlen(buf), fp);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "nameserver %s\n", cJSON_GetObjectItem(cfg, "slave_dns")->valuestring);
	cJSON_AddStringToObject(response, "slave_dns", buf);
	ret = fwrite(buf, 1, strlen(buf), fp);
	fclose(fp);
	fp = NULL;

    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_snmp_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "SNMP", "snmp_union",
            cJSON_GetObjectItem(cfg, "snmp_union")->valuestring);
    write_profile(dic, "SNMP", "trap_server_ip",
            cJSON_GetObjectItem(cfg, "trap_server_ip")->valuestring);
	write_profile(dic, "SNMP", "enterprise_code",
			cJSON_GetObjectItem(cfg, "enterprise_code")->valuestring);
	write_profile(dic, "SNMP", "enterprise_name",
			cJSON_GetObjectItem(cfg, "enterprise_name")->valuestring);

    cJSON *array_item = cJSON_GetObjectItem(cfg, "authority_ip");
    if (array_item != NULL) {
        int size = cJSON_GetArraySize(array_item);
        int i = 0;
        char item_name[32] = {0};
        char item_value[32] = {0};
        cJSON *object = NULL;
        for (i = 0; i < size; i++) {
            object = cJSON_GetArrayItem(array_item, i);
            sprintf(item_name, "valid_flag_%d", i);
            sprintf(item_value, "%d",
                    cJSON_GetObjectItem(object, "valid_flag")->valueint);
            write_profile(dic, "SNMP", item_name, item_value);

            memset(item_name, 0, sizeof(item_name));
            sprintf(item_name, "authority_ip_%d", i);
            write_profile(dic, "SNMP", item_name,
                    cJSON_GetObjectItem(object, "ip")->valuestring);
        }
        object = NULL;
    }
    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_uart_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	cJSON *rs232_cfg = cJSON_GetObjectItem(cfg, "rs232_cfg");

	int protocol_id = atoi(cJSON_GetObjectItem(rs232_cfg, "protocol_id")->valuestring);
	int baud = atoi(cJSON_GetObjectItem(rs232_cfg, "baud")->valuestring);
	int data_bits = atoi(cJSON_GetObjectItem(rs232_cfg, "data_bits")->valuestring);
	int stops_bits = atoi(cJSON_GetObjectItem(rs232_cfg, "stops_bits")->valuestring);
	int parity = atoi(cJSON_GetObjectItem(rs232_cfg, "parity")->valuestring);
	int enable = atoi(cJSON_GetObjectItem(rs232_cfg, "enable")->valuestring);

	char sql[512] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "UPDATE %s SET protocol_id=%d, baud=%d, data_bits=%d, \
		stops_bits=%d, parity=%d, enable=%d WHERE port=2",
		"uart_cfg", protocol_id, baud, data_bits, stops_bits, parity, enable);
	db_handle->action(db_handle, sql, error_msg);

	cJSON *rs485_cfg = cJSON_GetObjectItem(cfg, "rs485_cfg");
	protocol_id = atoi(cJSON_GetObjectItem(rs485_cfg, "protocol_id")->valuestring);
	baud = atoi(cJSON_GetObjectItem(rs485_cfg, "baud")->valuestring);
	data_bits = atoi(cJSON_GetObjectItem(rs485_cfg, "data_bits")->valuestring);
	stops_bits = atoi(cJSON_GetObjectItem(rs485_cfg, "stops_bits")->valuestring);
	parity = atoi(cJSON_GetObjectItem(rs485_cfg, "parity")->valuestring);
	enable = atoi(cJSON_GetObjectItem(rs485_cfg, "enable")->valuestring);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "UPDATE %s SET protocol_id=%d, baud=%d, data_bits=%d, \
		stops_bits=%d, parity=%d, enable=%d WHERE port=3",
		"uart_cfg", protocol_id, baud, data_bits, stops_bits, parity, enable);
	db_handle->action(db_handle, sql, error_msg);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_di_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	dictionary *dic		= priv->dic;
	db_access_t *db_handle = priv->sys_db_handle;

    cJSON *response;
    response = cJSON_CreateObject();

	char sql[512] = {0};
	char error_msg[256] = {0};
    cJSON *array_item = cJSON_GetObjectItem(root, "cfg");
    if (array_item != NULL) {
        int size = cJSON_GetArraySize(array_item);
        int i = 0;
        cJSON *object = NULL;
		int enable = 0;
		int alarm_level = 0;
		int alarm_method = 0;
		int id = 0;
		char *device_name = NULL;
		char *low_desc = NULL;
		char *high_desc = NULL;
		int ret = 0;
        for (i = 0; i < size; i++) {
            object = cJSON_GetArrayItem(array_item, i);
			id = atoi(cJSON_GetObjectItem(object, "id")->valuestring);
			enable = atoi(cJSON_GetObjectItem(object, "enable")->valuestring);
			alarm_level = atoi(cJSON_GetObjectItem(object, "alarm_level")->valuestring);
			alarm_method = atoi(cJSON_GetObjectItem(object, "alarm_method")->valuestring);
			device_name = cJSON_GetObjectItem(object, "device_name")->valuestring;
			low_desc = cJSON_GetObjectItem(object, "low_desc")->valuestring;
			high_desc = cJSON_GetObjectItem(object, "high_desc")->valuestring;
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "UPDATE %s SET device_name='%s', low_desc='%s', high_desc='%s', alarm_level=%d, enable=%d, alarm_method=%d WHERE id=%d",
					"di_cfg", device_name, low_desc, high_desc,
					alarm_level, enable, alarm_method, id);
			ret = db_handle->action(db_handle, sql, error_msg);
			if (ret != 0) {
				cJSON_AddStringToObject(response, "sql", sql);
				cJSON_AddStringToObject(response, "error_msg", error_msg);
			}
        }
		device_name = NULL;
		low_desc = NULL;
		high_desc = NULL;
        object = NULL;
    }

    write_profile(dic, "ALARM", "di_alarm_flag", "1");
    dump_profile(dic, INI_FILE_NAME);

    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_ntp_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "NTP", "ntp_server_ip",
            cJSON_GetObjectItem(cfg, "ntp_server_ip")->valuestring);
    write_profile(dic, "NTP", "ntp_interval",
            cJSON_GetObjectItem(cfg, "ntp_interval")->valuestring);
    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_do_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

	unsigned char status[3] = {0};

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "DO", "beep_alarm_enable",
            cJSON_GetObjectItem(cfg, "beep_alarm_enable")->valuestring);

    cJSON *array_item = cJSON_GetObjectItem(cfg, "io_status");
    if (array_item != NULL) {
        int size = cJSON_GetArraySize(array_item);
        int i = 0;
        cJSON *object = NULL;
		char item_name[16] = {0};
		char item_value[16] = {0};
        for (i = 0; i < size; i++) {
            object = cJSON_GetArrayItem(array_item, i);

			memset(item_name, 0, sizeof(item_name));
			sprintf(item_name, "do%d_name", i + 2);
		    write_profile(dic, "DO", item_name,
		            cJSON_GetObjectItem(object, "name")->valuestring);

			memset(item_value, 0, sizeof(item_value));
			status[i] = cJSON_GetObjectItem(object, "value")->valueint;
			sprintf(item_value, "%d", status[i]);

			memset(item_name, 0, sizeof(item_name));
			sprintf(item_name, "do%d_value", i + 2);
			write_profile(dic, "DO", item_name, item_value);
        }
        object = NULL;
    }
    dump_profile(dic, INI_FILE_NAME);

	int i = 0;
    for (i = 0; i < 3; i++) {
		drv_gpio_open(i + 5);
		drv_gpio_write(i + 5, status[i]);
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_device_time(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    char cmd[128] = {0};
    sprintf(cmd, "date -s \"%s\"; hwclock -w",
            cJSON_GetObjectItem(cfg, "calibration_pc_time")->valuestring);
    int ret = system(cmd);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int system_reboot(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

	FILE *fp = fopen("/tmp/reboot", "wb");
	fclose(fp);
	fp = NULL;

    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int upgrade_apply(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");

    cJSON *response = cJSON_CreateObject();
	int ret = system(cJSON_GetObjectItem(cfg, "cmd1")->valuestring);
    cJSON_AddNumberToObject(response, "status1", ret);

	ret = system(cJSON_GetObjectItem(cfg, "cmd2")->valuestring);
    cJSON_AddNumberToObject(response, "status", ret);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int system_recovery(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    write_profile(dic, "SYSTEM", "init_flag", "1");

    write_profile(dic, "NETWORK", "ip_addr", "192.168.0.233");
    write_profile(dic, "NETWORK", "gateway", "192.168.0.1");
    write_profile(dic, "NETWORK", "netmask", "255.255.255.0");
    write_profile(dic, "NETWORK", "master_dns", "114.114.114.114");
    write_profile(dic, "NETWORK", "slave_dns", "8.8.8.8");

    write_profile(dic, "SNMP", "trap_server_ip", "192.168.0.200");
    write_profile(dic, "SNMP", "authority_ip_0", "192.168.0.200");
    write_profile(dic, "SNMP", "authority_ip_1", "192.168.0.201");
    write_profile(dic, "SNMP", "authority_ip_2", "192.168.0.202");
    write_profile(dic, "SNMP", "authority_ip_3", "192.168.0.203");
    write_profile(dic, "SNMP", "valid_flag_0", "0");
    write_profile(dic, "SNMP", "valid_flag_1", "0");
    write_profile(dic, "SNMP", "valid_flag_2", "0");
    write_profile(dic, "SNMP", "valid_flag_3", "0");
    write_profile(dic, "SNMP", "enterprise_code", "999");
    write_profile(dic, "SNMP", "enterprise_name", "Jitong");

    write_profile(dic, "NTP", "ntp_interval", "10");
    write_profile(dic, "NTP", "ntp_server_ip", "202.120.2.101");

    write_profile(dic, "SMS", "send_times", "3");
    write_profile(dic, "SMS", "send_interval", "1");

    write_profile(dic, "EMAIL", "send_times", "3");
    write_profile(dic, "EMAIL", "send_interval", "1");
    write_profile(dic, "EMAIL", "port", "25");
    write_profile(dic, "EMAIL", "smtp_server", "");
    write_profile(dic, "EMAIL", "email_addr", "");
    write_profile(dic, "EMAIL", "password", "");

    write_profile(dic, "DO", "beep_alarm_enable", "0");
    write_profile(dic, "DO", "do2_name", "");
    write_profile(dic, "DO", "do2_value", "1");
    write_profile(dic, "DO", "do3_name", "");
    write_profile(dic, "DO", "do3_value", "1");
    write_profile(dic, "DO", "do4_name", "");
    write_profile(dic, "DO", "do4_value", "1");

    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int system_runtime_info(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *response = cJSON_CreateObject();

	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	char time_in_sec[32] = {0};
	struct tm *tm = localtime(&(current_time.tv_sec));
	sprintf(time_in_sec, "%04d-%02d-%02d %02d:%02d:%02d",
                        tm->tm_year + 1900, tm->tm_mon + 1,
                        tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    cJSON_AddStringToObject(response, "device_time", time_in_sec);

	FILE *fp = fopen("/tmp/version", "rb");
	if (fp != NULL) {
		char tmp[256] = {0};
		if (fgets(tmp, 256, fp) != NULL) {
			cJSON_AddStringToObject(response, "app_version", tmp);
		}

		if (fgets(tmp, 256, fp) != NULL) {
			cJSON_AddStringToObject(response, "protocol_version", tmp);
		}

		if (fgets(tmp, 256, fp) != NULL) {
			cJSON_AddStringToObject(response, "run_time", tmp);
		}
	} else {
		cJSON_AddStringToObject(response, "app_version", "未知");
		cJSON_AddStringToObject(response, "protocol_version", "未知");
		cJSON_AddStringToObject(response, "run_time", "未知");
	}
	fclose(fp);

	fp = fopen("/bsp_version", "rb");
	if (fp != NULL) {
		char tmp[128] = {0};
		if (fgets(tmp, 128, fp) != NULL) {
			cJSON_AddStringToObject(response, "bsp_version", tmp);
		}
	} else {
		cJSON_AddStringToObject(response, "bsp_version", "未知");
	}

	cJSON_AddStringToObject(response, "hardware_version", "V1.2");
	cJSON_AddStringToObject(response, "serial_number", "20160820010001");

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_system_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    cJSON *response = cJSON_CreateObject();

    cJSON_AddStringToObject(response, "site",
            iniparser_getstring(dic, "SYSTEM:site", ""));
    cJSON_AddStringToObject(response, "device_number",
            iniparser_getstring(dic, "SYSTEM:device_number", "20170201007"));
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_system_param(cJSON *root, priv_info_t *priv)
{
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    cJSON *response = cJSON_CreateObject();
    write_profile(dic, "SYSTEM", "site",
            cJSON_GetObjectItem(cfg, "site")->valuestring);
    write_profile(dic, "SYSTEM", "device_number",
            cJSON_GetObjectItem(cfg, "device_number")->valuestring);
    dump_profile(dic, INI_FILE_NAME);

    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);

    cJSON_Delete(response);

    return 0;
}

static int get_phone_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY id", "phone_user");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddNumberToObject(response, "count", query_result.row);

	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "phone_user", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
    		cJSON_AddStringToObject(child, "name", query_result.result[i * query_result.column + 1]);
    		cJSON_AddStringToObject(child, "phone", query_result.result[i * query_result.column + 2]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int add_phone_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "INSERT INTO %s (name, phone) VALUES ('%s', '%s')",
		"phone_user", cJSON_GetObjectItem(root, "name")->valuestring,
		cJSON_GetObjectItem(root, "phone")->valuestring);
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s ORDER BY id", "phone_user");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	cJSON_AddNumberToObject(response, "count", query_result.row);

	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "phone_user", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
    		cJSON_AddStringToObject(child, "name", query_result.result[i * query_result.column + 1]);
    		cJSON_AddStringToObject(child, "phone", query_result.result[i * query_result.column + 2]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int modify_phone_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "UPDATE %s SET name='%s', phone='%s' WHERE id='%d'",
		"phone_user", cJSON_GetObjectItem(root, "name")->valuestring,
		cJSON_GetObjectItem(root, "phone")->valuestring,
		atoi(cJSON_GetObjectItem(root, "id")->valuestring));
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int delete_phone_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "DELETE FROM %s WHERE id='%d'",
		"phone_user", atoi(cJSON_GetObjectItem(root, "id")->valuestring));
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_email_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY id", "email_user");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddNumberToObject(response, "count", query_result.row);

	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "email_user", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
    		cJSON_AddStringToObject(child, "name", query_result.result[i * query_result.column + 1]);
    		cJSON_AddStringToObject(child, "email", query_result.result[i * query_result.column + 2]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int add_email_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "INSERT INTO %s (name, email) VALUES ('%s', '%s')",
		"email_user", cJSON_GetObjectItem(root, "name")->valuestring,
		cJSON_GetObjectItem(root, "email")->valuestring);
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s ORDER BY id", "email_user");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	cJSON_AddNumberToObject(response, "count", query_result.row);

	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "email_user", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
    		cJSON_AddStringToObject(child, "name", query_result.result[i * query_result.column + 1]);
    		cJSON_AddStringToObject(child, "email", query_result.result[i * query_result.column + 2]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int modify_email_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "UPDATE %s SET name='%s', email='%s' WHERE id='%d'",
		"email_user", cJSON_GetObjectItem(root, "name")->valuestring,
		cJSON_GetObjectItem(root, "email")->valuestring,
		atoi(cJSON_GetObjectItem(root, "id")->valuestring));
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int delete_email_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "DELETE FROM %s WHERE id='%d'",
		"email_user", atoi(cJSON_GetObjectItem(root, "id")->valuestring));
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_sms_rule(cJSON *root, priv_info_t *priv)
{
    dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "send_times",
            iniparser_getint(dic, "SMS:send_times", 3));
    cJSON_AddNumberToObject(response, "send_interval",
            iniparser_getint(dic, "SMS:send_interval", 1));
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_sms_rule(cJSON *root, priv_info_t *priv)
{
    dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "SMS", "send_times",
            cJSON_GetObjectItem(cfg, "send_times")->valuestring);
	write_profile(dic, "SMS", "send_interval",
	        cJSON_GetObjectItem(cfg, "send_interval")->valuestring);

    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_email_rule(cJSON *root, priv_info_t *priv)
{
    dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "send_times",
            iniparser_getint(dic, "EMAIL:send_times", 3));
    cJSON_AddNumberToObject(response, "send_interval",
            iniparser_getint(dic, "EMAIL:send_interval", 1));
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_email_rule(cJSON *root, priv_info_t *priv)
{
    dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "EMAIL", "send_times",
            cJSON_GetObjectItem(cfg, "send_times")->valuestring);
	write_profile(dic, "EMAIL", "send_interval",
	        cJSON_GetObjectItem(cfg, "send_interval")->valuestring);

    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_email_server(cJSON *root, priv_info_t *priv)
{
    dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);
    cJSON *response = cJSON_CreateObject();

    cJSON_AddStringToObject(response, "smtp_server",
            iniparser_getstring(dic, "EMAIL:smtp_server", "smtp.163.com"));
    cJSON_AddStringToObject(response, "email_addr",
            iniparser_getstring(dic, "EMAIL:email_addr", ""));
    cJSON_AddStringToObject(response, "password",
            iniparser_getstring(dic, "EMAIL:password", ""));
    cJSON_AddNumberToObject(response, "port",
            iniparser_getint(dic, "EMAIL:port", 25));

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_email_server(cJSON *root, priv_info_t *priv)
{
    dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "EMAIL", "smtp_server",
            cJSON_GetObjectItem(cfg, "smtp_server")->valuestring);
	write_profile(dic, "EMAIL", "email_addr",
	        cJSON_GetObjectItem(cfg, "email_addr")->valuestring);
	write_profile(dic, "EMAIL", "password",
	        cJSON_GetObjectItem(cfg, "password")->valuestring);
	write_profile(dic, "EMAIL", "port",
	        cJSON_GetObjectItem(cfg, "port")->valuestring);

    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_user_list(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY id", "user_manager");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddNumberToObject(response, "count", query_result.row);
	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "user_list", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
    		cJSON_AddStringToObject(child, "user", query_result.result[i * query_result.column + 1]);
    		cJSON_AddStringToObject(child, "password", query_result.result[i * query_result.column + 2]);
    		cJSON_AddStringToObject(child, "permit", query_result.result[i * query_result.column + 3]);
    		cJSON_AddStringToObject(child, "description", query_result.result[i * query_result.column + 4]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int update_password(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "UPDATE %s SET password='%s', permit=%d WHERE id='%d'",
		"user_manager", cJSON_GetObjectItem(root, "password")->valuestring,
		atoi(cJSON_GetObjectItem(root, "permit")->valuestring),
		atoi(cJSON_GetObjectItem(root, "id")->valuestring));

	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddStringToObject(response, "sql", sql);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int delete_system_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "DELETE FROM %s WHERE id='%d'",
		"user_manager", atoi(cJSON_GetObjectItem(root, "id")->valuestring));
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int add_system_user(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;
    cJSON *response = cJSON_CreateObject();

	char description[32] = {0};
	int permit = atoi(cJSON_GetObjectItem(root, "permit")->valuestring);
	if (permit == 1) {
		sprintf(description, "%s", "管理员");
	} else if (permit == 2) {
		sprintf(description, "%s", "控制权");
	} else if (permit == 4) {
		sprintf(description, "%s", "监查权");
	} else if (permit == 8) {
		sprintf(description, "%s", "查看权");
	}

	char sql[256] = {0};
	char error_msg[256] = {0};
	sprintf(sql, "INSERT INTO %s (user, password, permit, description) VALUES ('%s', '%s', %d, '%s')",
		"user_manager", cJSON_GetObjectItem(root, "name")->valuestring,
		cJSON_GetObjectItem(root, "password")->valuestring, permit, description);
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s ORDER BY id", "user_manager");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	cJSON_AddNumberToObject(response, "count", query_result.row);

	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "user_list", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
    		cJSON_AddStringToObject(child, "user", query_result.result[i * query_result.column + 1]);
    		cJSON_AddStringToObject(child, "password", query_result.result[i * query_result.column + 2]);
    		cJSON_AddStringToObject(child, "permit", query_result.result[i * query_result.column + 3]);
    		cJSON_AddStringToObject(child, "description", query_result.result[i * query_result.column + 4]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int login(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	char sql[512] = {0};
	sprintf(sql, "SELECT * FROM %s WHERE user='%s'",
		"user_manager", cJSON_GetObjectItem(cfg, "user")->valuestring);

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	int ret = 0;
    cJSON *response = cJSON_CreateObject();
	char user[32] = {0};
	char password[32] = {0};
	if (query_result.row > 0) {
    	memcpy(user, query_result.result[query_result.column + 1], sizeof(user));
    	memcpy(password, query_result.result[query_result.column + 2], sizeof(password));
		if ((strcmp(user, cJSON_GetObjectItem(cfg, "user")->valuestring) == 0)
			&& (strcmp(password, cJSON_GetObjectItem(cfg, "password")->valuestring) == 0)) {
			cJSON_AddStringToObject(response, "permit", query_result.result[query_result.column + 3]);
			ret = 1;
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    cJSON_AddNumberToObject(response, "status", ret);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_data_record(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->data_db_handle;

	char sql[256] = {0};
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	char *start_time = cJSON_GetObjectItem(cfg, "start_time")->valuestring;
	char *end_time = cJSON_GetObjectItem(cfg, "end_time")->valuestring;
	char *device_id_string = cJSON_GetObjectItem(cfg, "device_id")->valuestring;
	char *param_id_string = cJSON_GetObjectItem(cfg, "param_id")->valuestring;
	if (strcmp(device_id_string, "all") == 0) {
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' ORDER BY id DESC",
			"data_record", start_time, end_time);
	} else if (strcmp(param_id_string, "all") == 0) {
		int device_id = atoi(device_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' AND protocol_id=%d ORDER BY id DESC",
			"data_record", start_time, end_time, device_id);
	} else {
		int device_id = atoi(device_id_string);
		int param_id = atoi(param_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' AND protocol_id=%d AND param_id=%d ORDER BY id DESC",
			"data_record", start_time, end_time, device_id, param_id);
	}
	cfg = NULL;
	start_time = NULL;
	end_time = NULL;
	device_id_string = NULL;

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "sql", sql);
	cJSON_AddNumberToObject(response, "count", query_result.row);
	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "data_record", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "created_time", query_result.result[i * query_result.column + 1]);
    		//cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 2]);
			//cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[i * query_result.column + 4]);
			//cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 5]);
			//cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 7]);
			if (strcmp(query_result.result[i * query_result.column + 8], "1") == 0) {
				char tmp[64] = {0};
				sprintf(tmp, "%s%s", query_result.result[i * query_result.column + 9],
					query_result.result[i * query_result.column + 10]);
				cJSON_AddStringToObject(child, "analog_value", tmp);
				//cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 9]);
				//cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 10]);
				cJSON_AddStringToObject(child, "enum_value", "-");
				//cJSON_AddStringToObject(child, "enum_en_desc", "-");
				cJSON_AddStringToObject(child, "enum_cn_desc", "-");
			} else {
				cJSON_AddStringToObject(child, "analog_value", "-");
				//cJSON_AddStringToObject(child, "unit", "");
				cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 11]);
				//cJSON_AddStringToObject(child, "enum_en_desc", query_result.result[i * query_result.column + 12]);
				cJSON_AddStringToObject(child, "enum_cn_desc", query_result.result[i * query_result.column + 13]);
			}
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_alarm_record(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->data_db_handle;

	char sql[256] = {0};
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	char *start_time = cJSON_GetObjectItem(cfg, "start_time")->valuestring;
	char *end_time = cJSON_GetObjectItem(cfg, "end_time")->valuestring;
	char *device_id_string = cJSON_GetObjectItem(cfg, "device_id")->valuestring;
	char *param_id_string = cJSON_GetObjectItem(cfg, "param_id")->valuestring;
	if (strcmp(device_id_string, "all") == 0) {
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' ORDER BY id DESC",
			"alarm_record", start_time, end_time);
	} else if (strcmp(param_id_string, "all") == 0) {
		int device_id = atoi(device_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' AND protocol_id=%d ORDER BY id DESC",
			"alarm_record", start_time, end_time, device_id);
	} else {
		int device_id = atoi(device_id_string);
		int param_id = atoi(param_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' AND protocol_id=%d AND param_id=%d ORDER BY id DESC",
			"alarm_record", start_time, end_time, device_id, param_id);
	}

	cfg = NULL;
	start_time = NULL;
	end_time = NULL;
	device_id_string = NULL;

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "sql", sql);
	cJSON_AddNumberToObject(response, "count", query_result.row);
	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "alarm_record", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "created_time", query_result.result[i * query_result.column + 1]);
    		//cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 2]);
			//cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[i * query_result.column + 4]);
			//cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 5]);
			//cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 7]);
			if (strcmp(query_result.result[i * query_result.column + 8], "1") == 0) {
				char tmp[64] = {0};
				sprintf(tmp, "%s%s", query_result.result[i * query_result.column + 9],
					query_result.result[i * query_result.column + 10]);
				cJSON_AddStringToObject(child, "analog_value", tmp);
				//cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 9]);
				//cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 10]);
				cJSON_AddStringToObject(child, "enum_value", "-");
				//cJSON_AddStringToObject(child, "enum_en_desc", "-");
				//cJSON_AddStringToObject(child, "enum_cn_desc", "-");
			} else {
				cJSON_AddStringToObject(child, "analog_value", "-");
				//cJSON_AddStringToObject(child, "unit", "");
				cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 11]);
				//cJSON_AddStringToObject(child, "enum_en_desc", query_result.result[i * query_result.column + 12]);
				//cJSON_AddStringToObject(child, "enum_cn_desc", query_result.result[i * query_result.column + 13]);
			}
			cJSON_AddStringToObject(child, "alarm_desc", query_result.result[i * query_result.column + 14]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_sms_record(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->data_db_handle;

	char sql[512] = {0};
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	char *start_time = cJSON_GetObjectItem(cfg, "start_time")->valuestring;
	char *end_time = cJSON_GetObjectItem(cfg, "end_time")->valuestring;
	char *device_id_string = cJSON_GetObjectItem(cfg, "device_id")->valuestring;
	unsigned int send_status = atoi(cJSON_GetObjectItem(cfg, "sent_status")->valuestring);
	switch (send_status) {
	case 0:
		if (strcmp(device_id_string, "all") == 0) {
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND send_status=%d ORDER BY id DESC",
				"sms_record", start_time, end_time, 0);
		} else {
			int device_id = atoi(device_id_string);
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND protocol_id=%d AND send_status=%d ORDER BY id DESC",
				"sms_record", start_time, end_time, device_id, 0);
		}
		break;
	case 1:
		if (strcmp(device_id_string, "all") == 0) {
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND send_status=%d ORDER BY id DESC",
				"sms_record", start_time, end_time, 1);
		} else {
			int device_id = atoi(device_id_string);
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND protocol_id=%d AND send_status=%d ORDER BY id DESC",
				"sms_record", start_time, end_time, device_id, 1);
		}
		break;
	default:
		if (strcmp(device_id_string, "all") == 0) {
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' ORDER BY id DESC",
				"sms_record", start_time, end_time);
		} else {
			int device_id = atoi(device_id_string);
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND protocol_id=%d ORDER BY id DESC",
				"sms_record", start_time, end_time, device_id);
		}
		break;
	}

	cfg = NULL;
	start_time = NULL;
	end_time = NULL;
	device_id_string = NULL;

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddNumberToObject(response, "count", query_result.row);
	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "sms_record", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "send_time", query_result.result[i * query_result.column + 1]);
			//cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 2]);
			//cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[i * query_result.column + 4]);
			//cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 5]);
			//cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 7]);
			//cJSON_AddStringToObject(child, "param_type", query_result.result[i * query_result.column + 8]);
			//cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 9]);
			//cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 10]);
			//cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 11]);
			cJSON_AddStringToObject(child, "user", query_result.result[i * query_result.column + 12]);
			//cJSON_AddStringToObject(child, "phone", query_result.result[i * query_result.column + 13]);
			if (atoi(query_result.result[i * query_result.column + 14]) == 1) {
				cJSON_AddStringToObject(child, "send_status", "失败");
			} else {
				cJSON_AddStringToObject(child, "send_status", "成功");
			}
			cJSON_AddStringToObject(child, "sms_content", query_result.result[i * query_result.column + 15]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_email_record(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->data_db_handle;

	char sql[512] = {0};
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	char *start_time = cJSON_GetObjectItem(cfg, "start_time")->valuestring;
	char *end_time = cJSON_GetObjectItem(cfg, "end_time")->valuestring;
	char *device_id_string = cJSON_GetObjectItem(cfg, "device_id")->valuestring;
	unsigned int send_status = atoi(cJSON_GetObjectItem(cfg, "sent_status")->valuestring);
	switch (send_status) {
	case 0:
		if (strcmp(device_id_string, "all") == 0) {
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND send_status=%d ORDER BY id DESC",
				"email_record", start_time, end_time, 0);
		} else {
			int device_id = atoi(device_id_string);
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND protocol_id=%d AND send_status=%d ORDER BY id DESC",
				"email_record", start_time, end_time, device_id, 0);
		}
		break;
	case 1:
		if (strcmp(device_id_string, "all") == 0) {
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND send_status=%d ORDER BY id DESC",
				"email_record", start_time, end_time, 1);
		} else {
			int device_id = atoi(device_id_string);
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND protocol_id=%d AND send_status=%d ORDER BY id DESC",
				"email_record", start_time, end_time, device_id, 1);
		}
		break;
	default:
		if (strcmp(device_id_string, "all") == 0) {
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' ORDER BY id DESC",
				"email_record", start_time, end_time);
		} else {
			int device_id = atoi(device_id_string);
			sprintf(sql, "SELECT * FROM %s WHERE send_time BETWEEN '%s' AND '%s' AND protocol_id=%d ORDER BY id DESC",
				"email_record", start_time, end_time, device_id);
		}
		break;
	}

	cfg = NULL;
	start_time = NULL;
	end_time = NULL;
	device_id_string = NULL;

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON_AddNumberToObject(response, "count", query_result.row);
	if (query_result.row > 0) {
    	cJSON *sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "email_record", sub_dir);

    	cJSON *child = NULL;
		int i = 1;
		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "send_time", query_result.result[i * query_result.column + 1]);
			//cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 2]);
			//cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[i * query_result.column + 4]);
			//cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 5]);
			//cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 7]);
			//cJSON_AddStringToObject(child, "param_type", query_result.result[i * query_result.column + 8]);
			//cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 9]);
			//cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 10]);
			//cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 11]);
			cJSON_AddStringToObject(child, "user", query_result.result[i * query_result.column + 12]);
			//cJSON_AddStringToObject(child, "email", query_result.result[i * query_result.column + 13]);
			if (atoi(query_result.result[i * query_result.column + 14]) == 1) {
				cJSON_AddStringToObject(child, "send_status", "失败");
			} else {
				cJSON_AddStringToObject(child, "send_status", "成功");
			}
			cJSON_AddStringToObject(child, "email_content", query_result.result[i * query_result.column + 15]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_real_uart_data(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	int ret = 0;

	shm_object_t *rs232_shm_handle;
	shm_object_t *rs485_shm_handle;
	int rs232_sem_id = semaphore_create(RS232_KEY);
	int rs485_sem_id = semaphore_create(RS485_KEY);

	uart_realdata_t *uart_realdata = NULL;
	do {
		rs232_shm_handle = shm_object_create(RS232_SHM_KEY, sizeof(uart_realdata_t));
		if (rs232_shm_handle == NULL) {
			printf("create RS232 share memory queue failed\n");
			ret = -1;
			break;
		}

		rs485_shm_handle = shm_object_create(RS485_SHM_KEY, sizeof(uart_realdata_t));
		if (rs485_shm_handle == NULL) {
			printf("create RS485 share memory queue failed\n");
			ret = -1;
			break;
		}

		uart_realdata = calloc(1, sizeof(uart_realdata_t));
		if (uart_realdata == NULL) {
			printf("create uart realdata memory failed\n");
			ret = -1;
			break;
		}
	} while(0);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;
	int i = 0;
	if (ret == -1) {
		cJSON_AddNumberToObject(response, "count", 1000);
	} else {
		semaphore_p(rs232_sem_id);
		ret = rs232_shm_handle->shm_get(rs232_shm_handle, (void *)uart_realdata);
		semaphore_v(rs232_sem_id);
		if (ret == 0) {
			cJSON_AddNumberToObject(response, "rs232_count", uart_realdata->cnt);
			sub_dir = cJSON_CreateArray();
			cJSON_AddItemToObject(response, "real_data_rs232", sub_dir);
			for (i = 0; i < uart_realdata->cnt; i++) {
				child = cJSON_CreateObject();
				cJSON_AddNumberToObject(child, "protocol_id", uart_realdata->data[i].protocol_id);
				cJSON_AddStringToObject(child, "protocol_desc", uart_realdata->data[i].protocol_desc);
				cJSON_AddStringToObject(child, "param_desc", uart_realdata->data[i].param_desc);
				cJSON_AddNumberToObject(child, "param_type", uart_realdata->data[i].param_type);
				cJSON_AddNumberToObject(child, "alarm_type", uart_realdata->data[i].alarm_type);
				if (uart_realdata->data[i].param_type == 1) {
					char tmp[64] = {0};
					sprintf(tmp, "%.1f%s", uart_realdata->data[i].analog_value, uart_realdata->data[i].param_unit);
					cJSON_AddStringToObject(child, "analog_value", tmp);
					cJSON_AddStringToObject(child, "enum_value", "-");
					cJSON_AddStringToObject(child, "enum_cn_desc", "-");
				} else {
					cJSON_AddStringToObject(child, "analog_value", "-");
					cJSON_AddNumberToObject(child, "enum_value", uart_realdata->data[i].enum_value);
					cJSON_AddStringToObject(child, "enum_cn_desc", uart_realdata->data[i].enum_cn_desc);
				}
				cJSON_AddItemToArray(sub_dir, child);
			}
		} else {
			cJSON_AddNumberToObject(response, "rs232_count", 0);
		}

		semaphore_p(rs485_sem_id);
		ret = rs485_shm_handle->shm_get(rs485_shm_handle, (void *)uart_realdata);
		semaphore_v(rs485_sem_id);
		if (ret == 0) {
			cJSON_AddNumberToObject(response, "rs485_count", uart_realdata->cnt);
			sub_dir = cJSON_CreateArray();
			cJSON_AddItemToObject(response, "real_data_rs485", sub_dir);
			for (i = 0; i < uart_realdata->cnt; i++) {
				child = cJSON_CreateObject();
				cJSON_AddNumberToObject(child, "protocol_id", uart_realdata->data[i].protocol_id);
				cJSON_AddStringToObject(child, "protocol_desc", uart_realdata->data[i].protocol_desc);
				cJSON_AddStringToObject(child, "param_desc", uart_realdata->data[i].param_desc);
				cJSON_AddNumberToObject(child, "param_type", uart_realdata->data[i].param_type);
				cJSON_AddNumberToObject(child, "alarm_type", uart_realdata->data[i].alarm_type);
				if (uart_realdata->data[i].param_type == 1) {
					char tmp[64] = {0};
					sprintf(tmp, "%.1f%s", uart_realdata->data[i].analog_value, uart_realdata->data[i].param_unit);
					cJSON_AddStringToObject(child, "analog_value", tmp);
					cJSON_AddStringToObject(child, "enum_value", "-");
					cJSON_AddStringToObject(child, "enum_cn_desc", "-");
				} else {
					cJSON_AddStringToObject(child, "analog_value", "-");
					cJSON_AddNumberToObject(child, "enum_value", uart_realdata->data[i].enum_value);
					cJSON_AddStringToObject(child, "enum_cn_desc", uart_realdata->data[i].enum_cn_desc);
				}
				cJSON_AddItemToArray(sub_dir, child);
			}
		} else {
			cJSON_AddNumberToObject(response, "rs485_count", 0);
		}
	}

	if (uart_realdata) {
		free(uart_realdata);
		uart_realdata = NULL;
	}

	rs232_shm_handle->shm_destroy(rs232_shm_handle);
	rs485_shm_handle->shm_destroy(rs485_shm_handle);
	rs232_shm_handle = NULL;
	rs485_shm_handle = NULL;

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_real_di_data(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	int ret = 0;

	shm_object_t *di_shm_handle;
	int di_sem_id = semaphore_create(DI_KEY);

	di_realdata_t *di_realdata = NULL;
	do {
		di_shm_handle = shm_object_create(DI_SHM_KEY, sizeof(di_realdata_t));
		if (di_shm_handle == NULL) {
			printf("create DI share memory queue failed\n");
			ret = -1;
			break;
		}

		di_realdata = calloc(1, sizeof(di_realdata_t));
		if (di_realdata == NULL) {
			ret = -1;
			break;
		}
	} while(0);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;
	int i = 0;
	if (ret == -1) {
		cJSON_AddNumberToObject(response, "count", 1000);
	} else {
		semaphore_p(di_sem_id);
		ret = di_shm_handle->shm_get(di_shm_handle, (void *)di_realdata);
		semaphore_v(di_sem_id);
		if (ret == 0) {
			cJSON_AddNumberToObject(response, "di_count", di_realdata->cnt);
			sub_dir = cJSON_CreateArray();
			cJSON_AddItemToObject(response, "real_data_di", sub_dir);
			for (i = 0; i < di_realdata->cnt; i++) {
				child = cJSON_CreateObject();
				char tmp[32] = {0};
				sprintf(tmp, "%.1f", di_realdata->data[i].analog_value);
				cJSON_AddNumberToObject(child, "protocol_id", di_realdata->data[i].protocol_id);
				cJSON_AddStringToObject(child, "protocol_name", di_realdata->data[i].protocol_name);
				cJSON_AddStringToObject(child, "protocol_desc", di_realdata->data[i].protocol_desc);
				cJSON_AddNumberToObject(child, "param_id", di_realdata->data[i].param_id);
				cJSON_AddStringToObject(child, "param_name", di_realdata->data[i].param_name);
				cJSON_AddStringToObject(child, "param_desc", di_realdata->data[i].param_desc);
				cJSON_AddNumberToObject(child, "param_type", di_realdata->data[i].param_type);
				cJSON_AddStringToObject(child, "analog_value", tmp);
				cJSON_AddStringToObject(child, "param_unit", di_realdata->data[i].param_unit);
				cJSON_AddNumberToObject(child, "enum_value", di_realdata->data[i].enum_value);
				cJSON_AddStringToObject(child, "enum_en_desc", di_realdata->data[i].enum_en_desc);
				cJSON_AddStringToObject(child, "enum_cn_desc", di_realdata->data[i].enum_cn_desc);
				cJSON_AddNumberToObject(child, "alarm_type", di_realdata->data[i].alarm_type);
				cJSON_AddItemToArray(sub_dir, child);
			}
		} else {
			cJSON_AddNumberToObject(response, "di_count", 0);
		}
	}

	if (di_realdata) {
		free(di_realdata);
		di_realdata = NULL;
	}

	di_shm_handle->shm_destroy(di_shm_handle);
	di_shm_handle = NULL;

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_di_support(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s WHERE enable=1 ORDER by id", "di_cfg");

    cJSON *response = cJSON_CreateObject();

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);
	if (query_result.row > 0) {
		cJSON_AddNumberToObject(response, "support_list_count", 1);
	} else {
		cJSON_AddNumberToObject(response, "support_list_count", 0);
	}
	db_handle->free_table(db_handle, query_result.result);

	cJSON_AddStringToObject(response, "list_index", "2");
	cJSON_AddStringToObject(response, "protocol_id", "1");
	cJSON_AddStringToObject(response, "protocol_name", "di");
	cJSON_AddStringToObject(response, "protocol_desc", "干接点输入");

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_support_device(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s WHERE enable=1 ORDER BY protocol_id", "uart_cfg");

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	unsigned int protocol_id_array[2] = {0};
	unsigned int protocol_id_cnt = 0;
	int i = 0;
	if (query_result.row > 0) {
		for (i = 0; i < query_result.row; i++) {
			protocol_id_array[i] = atoi(query_result.result[(i + 1 ) * query_result.column + 1]);
			protocol_id_cnt++;
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;

	sub_dir = cJSON_CreateArray();
	cJSON_AddItemToObject(response, "support_list", sub_dir);

	int index = 0;
	for (i = 0; i < protocol_id_cnt; i++) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d ORDER BY list_index",
			"support_list", protocol_id_array[i]);

		memset(&query_result, 0, sizeof(query_result_t));
		db_handle->query(db_handle, sql, &query_result);
		if (query_result.row > 0) {
        	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "list_index", query_result.result[query_result.column]);
			cJSON_AddStringToObject(child, "protocol_id", query_result.result[query_result.column + 1]);
			cJSON_AddStringToObject(child, "protocol_name", query_result.result[query_result.column + 2]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[query_result.column + 3]);
    		cJSON_AddItemToArray(sub_dir, child);
			index++;
		}
		db_handle->free_table(db_handle, query_result.result);
	}
	cJSON_AddNumberToObject(response, "support_list_count", index);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_support_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *sys_db_handle = priv->sys_db_handle;

	char sql[256] = {0};
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	int device_id = atoi(cJSON_GetObjectItem(cfg, "device_id")->valuestring);
	sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d ORDER BY id", "parameter", device_id);
	cfg = NULL;

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	sys_db_handle->query(sys_db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;
	int i = 0;
	cJSON_AddNumberToObject(response, "param_list_count", query_result.row);
	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "param_list", sub_dir);
		for (i = 1; i < (query_result.row + 1); i++) {
        	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 7]);
    		cJSON_AddItemToArray(sub_dir, child);
    	}
	}
	sys_db_handle->free_table(sys_db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int get_protocol_alarm_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *sys_db_handle = priv->sys_db_handle;

	char sql[256] = {0};
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
	char *device_id_string = cJSON_GetObjectItem(cfg, "device_id")->valuestring;
	if (strcmp(device_id_string, "all") == 0) {
		sprintf(sql, "SELECT * FROM %s ORDER BY id", "parameter");
	} else {
		int device_id = atoi(device_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d ORDER BY id", "parameter", device_id);
	}

	device_id_string = NULL;
	cfg = NULL;

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	sys_db_handle->query(sys_db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;
	int i = 0;
	cJSON_AddNumberToObject(response, "param_list_count", query_result.row);
	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "param_list", sub_dir);
		for (i = 1; i < (query_result.row + 1); i++) {
        	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
			cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 1]);
			cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "protocol_desc", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "param_unit", query_result.result[i * query_result.column + 8]);
			cJSON_AddStringToObject(child, "up_limit", query_result.result[i * query_result.column + 9]);
			cJSON_AddStringToObject(child, "up_free", query_result.result[i * query_result.column + 10]);
			cJSON_AddStringToObject(child, "low_limit", query_result.result[i * query_result.column + 11]);
			cJSON_AddStringToObject(child, "low_free", query_result.result[i * query_result.column + 12]);
			cJSON_AddStringToObject(child, "param_type", query_result.result[i * query_result.column + 13]);
			cJSON_AddStringToObject(child, "update_threshold", query_result.result[i * query_result.column + 14]);
			cJSON_AddStringToObject(child, "low_en_desc", query_result.result[i * query_result.column + 15]);
			cJSON_AddStringToObject(child, "low_cn_desc", query_result.result[i * query_result.column + 16]);
			cJSON_AddStringToObject(child, "high_en_desc", query_result.result[i * query_result.column + 17]);
			cJSON_AddStringToObject(child, "high_cn_desc", query_result.result[i * query_result.column + 18]);
			cJSON_AddStringToObject(child, "alarm_enable", query_result.result[i * query_result.column + 19]);
    		cJSON_AddItemToArray(sub_dir, child);
    	}
	}
	sys_db_handle->free_table(sys_db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_protocol_alarm_param(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	dictionary *dic		= priv->dic;
	db_access_t *db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	char error_msg[256] = {0};
    cJSON *array_item = cJSON_GetObjectItem(root, "cfg");
    if (array_item != NULL) {
        int size = cJSON_GetArraySize(array_item);
        int i = 0;
        cJSON *object = NULL;
		double up_limit = 0.0;
		double up_free = 0.0;
		double low_free = 0.0;
		double low_limit = 0.0;
		double update_threshold = 0.0;
		int alarm_enable = 0;
		int id = 0;
        for (i = 0; i < size; i++) {
            object = cJSON_GetArrayItem(array_item, i);
			id = atoi(cJSON_GetObjectItem(object, "id")->valuestring);
			up_limit = atof(cJSON_GetObjectItem(object, "up_limit")->valuestring);
			up_free = atof(cJSON_GetObjectItem(object, "up_free")->valuestring);
			low_limit = atof(cJSON_GetObjectItem(object, "low_limit")->valuestring);
			low_free = atof(cJSON_GetObjectItem(object, "low_free")->valuestring);
			update_threshold = atof(cJSON_GetObjectItem(object, "update_threshold")->valuestring);
			alarm_enable = atoi(cJSON_GetObjectItem(object, "alarm_enable")->valuestring);
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "UPDATE %s SET up_limit=%.1f, up_free=%.1f, low_limit=%.1f, low_free=%.1f, update_threshold=%.1f, alarm_enable=%d WHERE id=%d",
					"parameter", up_limit, up_free, low_limit, low_free, update_threshold, alarm_enable, id);
			db_handle->action(db_handle, sql, error_msg);
        }
        object = NULL;
    }

    write_profile(dic, "ALARM", "rs232_alarm_flag", "1");
    write_profile(dic, "ALARM", "rs485_alarm_flag", "1");
    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

typedef int (*msg_fun)(cJSON *root, priv_info_t *priv);

typedef struct {
    const char  *cmd_type;
    msg_fun     action;
} cmd_fun_t;

typedef struct {
    const char  *msg_type;
    cmd_fun_t   *cmd_fun_array;
    int         cmd_num;
} msg_fun_t;

/* 获取和设置参数相关命令及操作 */
cmd_fun_t cmd_param_setting[] = {
	{
		"get_network_param",
		get_network_param
	},
	{
		"set_network_param",
		set_network_param
	},
    {
        "get_snmp_param",
        get_snmp_param
    },
    {
        "set_snmp_param",
        set_snmp_param
    },
    {
        "get_uart_param",
        get_uart_param
	},
    {
        "set_uart_param",
        set_uart_param
	},
    {
        "get_do_param",
        get_do_param
	},
    {
        "set_do_param",
        set_do_param
	}
};

/* 系统参数控制 */
cmd_fun_t cmd_system_setting[] = {
    {
        "get_system_param",
        get_system_param
    },
    {
        "set_system_param",
        set_system_param
    },
	{
		"get_user_list",
		get_user_list
	},
	{
		"update_password",
		update_password
	},
	{
		"delete_system_user",
		delete_system_user
	},
	{
		"add_system_user",
		add_system_user
	},
    {
        "get_ntp_param",
        get_ntp_param
    },
    {
        "set_ntp_param",
        set_ntp_param
    },
    {
        "calibration",
        set_device_time
    },
	{
		"login",
		login
	},
	{
		"system_runtime_info",
		system_runtime_info
	},
	{
		"system_reboot",
		system_reboot
	},
	{
		"upgrade_apply",
		upgrade_apply
	},
	{
		"system_recovery",
		system_recovery
	}
};

cmd_fun_t cmd_alarm_setting[] = {
	{
		"get_phone_user",
		get_phone_user
	},
	{
		"add_phone_user",
		add_phone_user
	},
	{
		"modify_phone_user",
		modify_phone_user
	},
	{
		"delete_phone_user",
		delete_phone_user
	},
	{
		"get_email_user",
		get_email_user
	},
	{
		"add_email_user",
		add_email_user
	},
	{
		"modify_email_user",
		modify_email_user
	},
	{
		"delete_email_user",
		delete_email_user
	},
    {
        "get_sms_rule",
        get_sms_rule
    },
    {
        "set_sms_rule",
        set_sms_rule
    },
	{
		"get_protocol_alarm_param",
		get_protocol_alarm_param
	},
	{
		"set_protocol_alarm_param",
		set_protocol_alarm_param
	},
	{
		"get_di_param",
		get_di_param
	},
	{
		"set_di_param",
		set_di_param
	},
	{
		"get_email_rule",
		get_email_rule
	},
	{
		"set_email_rule",
		set_email_rule
	},
	{
		"get_email_server",
		get_email_server
	},
	{
		"set_email_server",
		set_email_server
	}
};

cmd_fun_t cmd_query_data[] = {
	{
		"query_data_record",
		query_data_record
	},
	{
		"query_alarm_record",
		query_alarm_record
	},
	{
		"query_sms_record",
		query_sms_record
	},
	{
		"query_email_record",
		query_email_record
	},
	{
		"query_real_uart_data",
		query_real_uart_data
	},
	{
		"query_real_di_data",
		query_real_di_data
	},
	{
		"query_support_device",
		query_support_device
	},
	{
		"query_support_param",
		query_support_param
	},
	{
		"query_di_support",
		query_di_support
	}
};

/* 消息类型及其对应的 命令:操作函数 数组 */
msg_fun_t msg_flow[] = {
    {
        "param_setting",
        cmd_param_setting,
        sizeof(cmd_param_setting) / sizeof(cmd_fun_t)
    },
    {
        "system_setting",
        cmd_system_setting,
        sizeof(cmd_system_setting) / sizeof(cmd_fun_t)
    },
	{
		"alarm_setting",
		cmd_alarm_setting,
		sizeof(cmd_alarm_setting) / sizeof(cmd_fun_t)
	},
	{
		"query_data",
		cmd_query_data,
		sizeof(cmd_query_data) / sizeof(cmd_fun_t)
	}
};

#define MAX_MIB_SIZE (1024 * 1024)
static int mib_download(req_buf_t *req_buf, priv_info_t *priv, const char *filename)
{
	db_access_t *db_handle = priv->sys_db_handle;
	unsigned int offset = 0;
	unsigned int i = 0;

    int enterprise_code = iniparser_getint(priv->dic, "SNMP:enterprise_code", 999);
	char enterprise_name[32] = {0};
    strcpy(enterprise_name, iniparser_getstring(priv->dic, "SNMP:enterprise_name", "Jitong"));

	char *buf = calloc(1, MAX_MIB_SIZE);
	offset = fill_mib_header(buf, offset, enterprise_name, enterprise_code);
	offset = fill_do_mib(buf, offset, enterprise_name);

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER by id", "di_cfg");

	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	if (query_result.row > 0) {
		di_param_t di_param;
		for (i = 1; i < (query_result.row + 1); i++) {
			memset(&di_param, 0, sizeof(di_param_t));
			di_param.id = atoi(query_result.result[i * query_result.column]);
			strcpy(di_param.di_name, query_result.result[i * query_result.column + 1]);
			strcpy(di_param.di_desc, query_result.result[i * query_result.column + 2]);
			strcpy(di_param.device_name, query_result.result[i * query_result.column + 3]);
			strcpy(di_param.low_desc, query_result.result[i * query_result.column + 4]);
			strcpy(di_param.high_desc, query_result.result[i * query_result.column + 5]);
			di_param.enable = atoi(query_result.result[i * query_result.column + 7]);

			if (di_param.enable) {
				offset = fill_di_mib(buf, offset, &di_param);
			}
		}
	}
	db_handle->free_table(db_handle, query_result.result);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s WHERE enable=1 ORDER BY protocol_id", "uart_cfg");

	memset(&query_result, 0, sizeof(query_result_t));
	db_handle->query(db_handle, sql, &query_result);

	unsigned int protocol_id_array[2] = {0};
	unsigned int protocol_id_cnt = 0;
	if (query_result.row > 0) {
		for (i = 0; i < query_result.row; i++) {
			protocol_id_array[i] = atoi(query_result.result[(i + 1 ) * query_result.column + 1]);
			protocol_id_cnt++;
		}
	}
	db_handle->free_table(db_handle, query_result.result);

	for (i = 0; i < protocol_id_cnt; i++) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "SELECT * FROM %s WHERE protocol_id=%d ORDER BY id",
				"parameter", protocol_id_array[i]);
		memset(&query_result, 0, sizeof(query_result_t));
		db_handle->query(db_handle, sql, &query_result);

		if (query_result.row > 0) {
			offset = fill_protocol_mib(buf, offset, protocol_id_array[i],
				query_result.result[query_result.column + 2], enterprise_name);

			unsigned int j = 0;
			param_desc_t param_desc;
			for (j = 1; j < query_result.row + 1; j++) {
				memset(&param_desc, 0, sizeof(param_desc_t));
				param_desc.param_id = atoi(query_result.result[j * query_result.column + 5]);
				strcpy(param_desc.param_name, query_result.result[j * query_result.column + 6]);
				strcpy(param_desc.param_desc, query_result.result[j * query_result.column + 7]);
				strcpy(param_desc.param_unit, query_result.result[j * query_result.column + 8]);
				param_desc.param_type = atoi(query_result.result[j * query_result.column + 13]);
				strcpy(param_desc.param_enum[0].desc,
					query_result.result[j * query_result.column + 15]);
				strcpy(param_desc.param_enum[1].desc,
					query_result.result[j * query_result.column + 16]);
				offset = fill_param_mib(buf, offset,
					query_result.result[query_result.column + 2], &param_desc);
			}
		}
		db_handle->free_table(db_handle, query_result.result);
	}
	offset = fill_mib_tail(buf, offset);

    printf("Content-Type: application/octet-stream\r\n");
    printf("Content-Length: %ld\r\n", offset);
    printf("Content-Disposition: attachment; filename=%s\r\n\r\n", filename);

#if 0
    FILE *fp = fopen(MIB_FILE_NAME, "wb");
	fwrite(buf, 1, offset, fp);
    fclose(fp);
    fp = NULL;
#endif
	int ret = fwrite(buf, 1, offset, stdout);

    return 0;
}

static int parse_request(req_buf_t *req_buf)
{
    int len = 0;
    int ret = -1;

    char *req_method = getenv("REQUEST_METHOD");
    if (strcmp(req_method, "GET") == 0) {
        char *query_string = getenv("QUERY_STRING");
        if (query_string != NULL) {
            strcpy(req_buf->buf, query_string);
            ret = 1;
        }
    } else if (strcmp(req_method, "POST") == 0) {
        char *env_string = getenv("CONTENT_LENGTH");
        if (env_string != NULL) {
            len = atoi(env_string);
            if (len >= req_buf->max_len) {
                char *buf = (char *)calloc(1, len + 1);
                free(req_buf->buf);
                req_buf->buf = buf;
                buf = NULL;
                req_buf->max_len = len + 1;
                req_buf->req_len = len;
            }
            ret = fread(req_buf->buf, 1, len, stdin);
            req_buf->buf[len] = '\0';

            ret = 0;
        }
        env_string = NULL;
    }

    return ret;
}

int main(void)
{
    int ret = -1;
	priv_info_t *priv = (priv_info_t *)calloc(1, sizeof(priv_info_t));
	priv->dic = iniparser_load(INI_FILE_NAME);

	priv->sys_db_handle = db_access_create("/opt/app/sys.db");
    priv->data_db_handle = db_access_create("/opt/data/data.db");

	priv->request.max_len = 512 * 1024;
    priv->request.buf     = (char *)calloc(1, priv->request.max_len);
    ret = parse_request(&(priv->request));
    if (ret == 0) {
        cJSON *root = cJSON_Parse(priv->request.buf);
        priv->request.fb_buf = NULL;
        int i = 0;
        int j = 0;
        char *msg_type = cJSON_GetObjectItem(root, "msg_type")->valuestring;
        char *cmd_type = cJSON_GetObjectItem(root, "cmd_type")->valuestring;
        int type_num = sizeof(msg_flow) / sizeof(msg_fun_t);
        for (i = 0; i < type_num; i++) {
            if (0 == strcmp(msg_flow[i].msg_type, msg_type)) {
                cmd_fun_t *cmd_fun = msg_flow[i].cmd_fun_array;
                for (j = 0; j < msg_flow[i].cmd_num; j++) {
                    if (0 == strcmp(cmd_fun[j].cmd_type, cmd_type)) {
                            cmd_fun[j].action(root, priv);
                    }
                }
            }
        }

        if (priv->request.fb_buf) {
            fprintf(stdout, "Content-type: text/html\n\n");
            fprintf(stdout, "%s", priv->request.fb_buf);
            free(priv->request.fb_buf);
            priv->request.fb_buf = NULL;
        }
        cJSON_Delete(root);
    } else if (ret == 1) {
        //下载MIB
        if (strstr(priv->request.buf, "mib") != NULL) {
            ret = mib_download(&(priv->request), priv, "JT_Guard.mib");
        }
    }
    free(priv->request.buf);
    priv->request.buf = NULL;

    priv->sys_db_handle->destroy(priv->sys_db_handle);
	priv->sys_db_handle = NULL;

    priv->data_db_handle->destroy(priv->data_db_handle);
	priv->data_db_handle = NULL;

	iniparser_freedict(priv->dic);
	priv->dic = NULL;

	free(priv);
	priv = NULL;

    return ret;
}
