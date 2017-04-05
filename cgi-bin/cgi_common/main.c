#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "iniparser.h"
#include "db_access.h"
#include "mib_create.h"

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
			cJSON_AddStringToObject(child, "device_brand", query_result.result[i * query_result.column + 4]);
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
    cJSON_AddItemToObject(response, "io_status", sub_dir);
	char value_item_name[16] = {0};
	char item_name[16] = {0};
	int i = 0;
    for (i = 0; i < 3; i++) {
		sprintf(value_item_name, "DO:do%d_value", i + 2);
		sprintf(item_name, "DO:do%d_name", i + 2);

        child = cJSON_CreateObject();
		cJSON_AddNumberToObject(child, "value",
			iniparser_getint(dic, value_item_name, 0));
		cJSON_AddStringToObject(child, "name",
			iniparser_getstring(dic, item_name, "干接点输出"));
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

    write_profile(dic, "ALARM", "di_cfg_flag", "1");
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
		drv_gpio_open(i + 4);
		drv_gpio_write(i + 4, status[i]);
		drv_gpio_close(i + 4);
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
    sprintf(cmd, "date -s \"%s\"",
            cJSON_GetObjectItem(cfg, "calibration_pc_time")->valuestring);
    system(cmd);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
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
	sprintf(sql, "UPDATE %s SET phone='%s' WHERE id='%d'",
		"phone_user", cJSON_GetObjectItem(root, "phone")->valuestring,
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
	sprintf(sql, "UPDATE %s SET email='%s' WHERE id='%d'",
		"email_user", cJSON_GetObjectItem(root, "email")->valuestring,
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

    cJSON_AddStringToObject(response, "smtp_server",
            iniparser_getstring(dic, "EMAIL:smtp_server", "smtp.163.com"));
    cJSON_AddStringToObject(response, "email_addr",
            iniparser_getstring(dic, "EMAIL:email_addr", ""));
    cJSON_AddStringToObject(response, "password",
            iniparser_getstring(dic, "EMAIL:password", ""));

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_email_rule(cJSON *root, priv_info_t *priv)
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
	sprintf(sql, "UPDATE %s SET password='%s' WHERE id='%d'",
		"user_manager", cJSON_GetObjectItem(root, "password")->valuestring,
		atoi(cJSON_GetObjectItem(root, "id")->valuestring));
	int ret = db_handle->action(db_handle, sql, error_msg);
    cJSON_AddNumberToObject(response, "status", ret);
	cJSON_AddStringToObject(response, "err_msg", error_msg);

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
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' ORDER BY id",
			"data_record", start_time, end_time);
	} else if (strcmp(param_id_string, "all") == 0) {
		int device_id = atoi(device_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' \
				AND device_id=%d ORDER BY id",
			"data_record", start_time, end_time, device_id);
	} else {
		int device_id = atoi(device_id_string);
		int param_id = atoi(param_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' \
				AND device_id=%d AND param_id=%d ORDER BY id",
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
    		cJSON_AddStringToObject(child, "device_id", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "device_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 6]);
			if (strcmp(query_result.result[i * query_result.column + 7], "1") == 0) {
				cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 8]);
				cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 9]);
				cJSON_AddStringToObject(child, "enum_value", "-");
				cJSON_AddStringToObject(child, "enum_desc", "-");
			} else {
				cJSON_AddStringToObject(child, "analog_value", "-");
				cJSON_AddStringToObject(child, "unit", "");
				cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 10]);
				cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 11]);
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
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' ORDER BY id",
			"alarm_record", start_time, end_time);
	} else if (strcmp(param_id_string, "all") == 0) {
		int device_id = atoi(device_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' \
				AND device_id=%d ORDER BY id",
			"alarm_record", start_time, end_time, device_id);
	} else {
		int device_id = atoi(device_id_string);
		int param_id = atoi(param_id_string);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' \
				AND device_id=%d AND param_id=%d ORDER BY id",
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
    		cJSON_AddStringToObject(child, "device_id", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "device_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 6]);
			if (strcmp(query_result.result[i * query_result.column + 7], "1") == 0) {
				cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 8]);
				cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 9]);
				cJSON_AddStringToObject(child, "enum_value", "-");
				cJSON_AddStringToObject(child, "enum_desc", "-");
			} else {
				cJSON_AddStringToObject(child, "analog_value", "-");
				cJSON_AddStringToObject(child, "unit", "");
				cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 10]);
				cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 11]);
			}
			cJSON_AddStringToObject(child, "alarm_desc", query_result.result[i * query_result.column + 12]);
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

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY id DESC limit 10", "sms_record");
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
			cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "user", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "phone", query_result.result[i * query_result.column + 8]);
			cJSON_AddStringToObject(child, "send_status", query_result.result[i * query_result.column + 9]);
			cJSON_AddStringToObject(child, "sms_content", query_result.result[i * query_result.column + 10]);
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

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY id DESC limit 10", "email_record");
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
			cJSON_AddStringToObject(child, "device_name", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "alarm_desc", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "email", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "send_status", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "email_content", query_result.result[i * query_result.column + 8]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	db_handle->free_table(db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_real_data(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *data_db_handle = priv->data_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY id DESC", "real_data");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	data_db_handle->query(data_db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;
	int i = 0;
	cJSON_AddNumberToObject(response, "count", query_result.row);
	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "real_data", sub_dir);

		for (i = 1; i < (query_result.row + 1); i++) {
	    	child = cJSON_CreateObject();
    		cJSON_AddStringToObject(child, "created_time", query_result.result[i * query_result.column + 1]);
			cJSON_AddStringToObject(child, "device_id", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "device_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_type", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 8]);
			cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 9]);
			cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 10]);
			cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 11]);
			cJSON_AddStringToObject(child, "alarm_type", query_result.result[i * query_result.column + 12]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	data_db_handle->free_table(data_db_handle, query_result.result);

    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "io_status", sub_dir);
    for (i = 0; i < 8; i++) {
		drv_gpio_open(i);
        child = cJSON_CreateObject();
		unsigned char io_value = 0;
		drv_gpio_read(i, &io_value);
    	cJSON_AddNumberToObject(child, "value", io_value);
		//drv_gpio_close(i);  /* 因为di_thread已经打开，如果关闭会导致di_thread异常 */
    	cJSON_AddItemToArray(sub_dir, child);
    }

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_support_device(cJSON *root, priv_info_t *priv)
{
	req_buf_t *req_buf	= &(priv->request);
	db_access_t *sys_db_handle = priv->sys_db_handle;

	char sql[256] = {0};
	sprintf(sql, "SELECT * FROM %s ORDER BY list_index", "support_list");
	query_result_t query_result;
	memset(&query_result, 0, sizeof(query_result_t));
	sys_db_handle->query(sys_db_handle, sql, &query_result);

    cJSON *response = cJSON_CreateObject();
	cJSON *sub_dir = NULL;
	cJSON *child = NULL;
	int i = 0;
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
			cJSON_AddStringToObject(child, "device_brand", query_result.result[i * query_result.column + 4]);
    		cJSON_AddItemToArray(sub_dir, child);
    	}
	}
	sys_db_handle->free_table(sys_db_handle, query_result.result);

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
	cJSON_AddNumberToObject(response, "support_list_count", query_result.row);
	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "support_list", sub_dir);
		for (i = 1; i < (query_result.row + 1); i++) {
        	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 6]);
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
	cJSON_AddNumberToObject(response, "support_list_count", query_result.row);
	if (query_result.row > 0) {
    	sub_dir = cJSON_CreateArray();
    	cJSON_AddItemToObject(response, "support_list", sub_dir);
		for (i = 1; i < (query_result.row + 1); i++) {
        	child = cJSON_CreateObject();
			cJSON_AddStringToObject(child, "id", query_result.result[i * query_result.column]);
			cJSON_AddStringToObject(child, "protocol_id", query_result.result[i * query_result.column + 1]);
			cJSON_AddStringToObject(child, "protocol_name", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "param_id", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "param_desc", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "param_unit", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "up_limit", query_result.result[i * query_result.column + 8]);
			cJSON_AddStringToObject(child, "up_free", query_result.result[i * query_result.column + 9]);
			cJSON_AddStringToObject(child, "low_limit", query_result.result[i * query_result.column + 10]);
			cJSON_AddStringToObject(child, "low_free", query_result.result[i * query_result.column + 11]);
			cJSON_AddStringToObject(child, "update_threshold", query_result.result[i * query_result.column + 13]);
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
		int id = 0;
        for (i = 0; i < size; i++) {
            object = cJSON_GetArrayItem(array_item, i);
			id = atoi(cJSON_GetObjectItem(object, "id")->valuestring);
			up_limit = atof(cJSON_GetObjectItem(object, "up_limit")->valuestring);
			up_free = atof(cJSON_GetObjectItem(object, "up_free")->valuestring);
			low_limit = atof(cJSON_GetObjectItem(object, "low_limit")->valuestring);
			low_free = atof(cJSON_GetObjectItem(object, "low_free")->valuestring);
			update_threshold = atof(cJSON_GetObjectItem(object, "update_threshold")->valuestring);
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "UPDATE %s SET up_limit=%.1f, up_free=%.1f, low_limit=%.1f, \
					low_free=%.1f, update_threshold=%.1f WHERE id=%d",
					"parameter", up_limit, up_free, low_limit, low_free, update_threshold, id);
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
    /*},
	{
		"login"
		login
	*/}
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
		"query_real_data",
		query_real_data
	},
	{
		"query_support_device",
		query_support_device
	},
	{
		"query_support_param",
		query_support_param
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

	char *buf = calloc(1, MAX_MIB_SIZE);
	offset = fill_mib_header(buf, offset);
	offset = fill_do_mib(buf, offset);

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
				query_result.result[query_result.column + 2]);

			unsigned int j = 0;
			param_desc_t param_desc;
			for (j = 1; j < query_result.row + 1; j++) {
				memset(&param_desc, 0, sizeof(param_desc_t));
				param_desc.param_id = atoi(query_result.result[j * query_result.column + 4]);
				strcpy(param_desc.param_name, query_result.result[j * query_result.column + 5]);
				strcpy(param_desc.param_desc, query_result.result[j * query_result.column + 6]);
				strcpy(param_desc.param_unit, query_result.result[j * query_result.column + 7]);
				param_desc.param_type = atoi(query_result.result[j * query_result.column + 12]);
				strcpy(param_desc.param_enum[0].desc,
					query_result.result[j * query_result.column + 14]);
				strcpy(param_desc.param_enum[0].desc,
					query_result.result[j * query_result.column + 15]);
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
	fwrite(buf, 1, offset, stdout);

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
            fread(req_buf->buf, 1, len, stdin);
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
