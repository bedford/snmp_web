#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "iniparser.h"
#include "db_access.h"

#define INI_FILE_NAME	"/opt/app/param.ini"

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

static int get_io_param(cJSON *root, priv_info_t *priv)
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

    int io_status[4] = {1, 0, 0, 1};
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "io_status", sub_dir);
    for (i = 0; i < 4; i++) {
        child = cJSON_CreateObject();
    	cJSON_AddNumberToObject(child, "value", io_status[i]);
    	cJSON_AddItemToArray(sub_dir, child);
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

static int set_io_param(cJSON *root, priv_info_t *priv)
{
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
	if (strcmp(device_id_string, "all") == 0) {
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' ORDER BY id",
			"data_record", start_time, end_time);
	} else {
		int device_id = atoi(device_id_string);
		int param_id = atoi(cJSON_GetObjectItem(cfg, "param_id")->valuestring);
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
			if (strcmp(query_result.result[i * query_result.column + 6], "1") == 0) {
				cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 7]);
				cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 8]);
				cJSON_AddStringToObject(child, "enum_value", "-");
				cJSON_AddStringToObject(child, "enum_desc", "-");
			} else {
				cJSON_AddStringToObject(child, "analog_value", "-");
				cJSON_AddStringToObject(child, "unit", "");
				cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 9]);
				cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 10]);
			}
			cJSON_AddStringToObject(child, "alarm_type", query_result.result[i * query_result.column + 11]);
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
	if (strcmp(device_id_string, "all") == 0) {
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' AND alarm_type>0 ORDER BY id",
			"data_record", start_time, end_time);
	} else {
		int device_id = atoi(device_id_string);
		int param_id = atoi(cJSON_GetObjectItem(cfg, "param_id")->valuestring);
		sprintf(sql, "SELECT * FROM %s WHERE created_time BETWEEN '%s' AND '%s' \
				AND device_id=%d AND param_id=%d AND alarm_type>0 ORDER BY id",
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
			if (strcmp(query_result.result[i * query_result.column + 6], "1") == 0) {
				cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 7]);
				cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 8]);
				cJSON_AddStringToObject(child, "enum_value", "-");
				cJSON_AddStringToObject(child, "enum_desc", "-");
			} else {
				cJSON_AddStringToObject(child, "analog_value", "-");
				cJSON_AddStringToObject(child, "unit", "");
				cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 9]);
				cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 10]);
			}
			cJSON_AddStringToObject(child, "alarm_type", query_result.result[i * query_result.column + 11]);
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
			cJSON_AddStringToObject(child, "device_name", query_result.result[i * query_result.column + 2]);
			cJSON_AddStringToObject(child, "param_name", query_result.result[i * query_result.column + 3]);
			cJSON_AddStringToObject(child, "alarm_desc", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "phone", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "send_status", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "sms_content", query_result.result[i * query_result.column + 7]);
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
			cJSON_AddStringToObject(child, "alarm_desc", query_result.result[i * query_result.column + 4]);
			cJSON_AddStringToObject(child, "email", query_result.result[i * query_result.column + 5]);
			cJSON_AddStringToObject(child, "send_status", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "email_content", query_result.result[i * query_result.column + 7]);
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
	db_access_t *sys_db_handle = priv->sys_db_handle;

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
			cJSON_AddStringToObject(child, "param_type", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "analog_value", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "unit", query_result.result[i * query_result.column + 8]);
			cJSON_AddStringToObject(child, "enum_value", query_result.result[i * query_result.column + 9]);
			cJSON_AddStringToObject(child, "enum_desc", query_result.result[i * query_result.column + 10]);
			cJSON_AddStringToObject(child, "alarm_type", query_result.result[i * query_result.column + 11]);
    		cJSON_AddItemToArray(sub_dir, child);
		}
	}
	data_db_handle->free_table(data_db_handle, query_result.result);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM %s ORDER BY list_index", "support_list");
	memset(&query_result, 0, sizeof(query_result_t));
	sys_db_handle->query(sys_db_handle, sql, &query_result);
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

    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "io_status", sub_dir);
    for (i = 0; i < 8; i++) {
		drv_gpio_open(i);
        child = cJSON_CreateObject();
		unsigned char io_value = 0;
		drv_gpio_read(i, &io_value);
    	cJSON_AddNumberToObject(child, "value", io_value);
		drv_gpio_close(i);
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
    		cJSON_AddItemToArray(sub_dir, child);
    	}
	}
	sys_db_handle->free_table(sys_db_handle, query_result.result);

    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int query_alarm_param(cJSON *root, priv_info_t *priv)
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
			cJSON_AddStringToObject(child, "param_unit", query_result.result[i * query_result.column + 6]);
			cJSON_AddStringToObject(child, "up_limit", query_result.result[i * query_result.column + 7]);
			cJSON_AddStringToObject(child, "up_free", query_result.result[i * query_result.column + 8]);
			cJSON_AddStringToObject(child, "low_limit", query_result.result[i * query_result.column + 9]);
			cJSON_AddStringToObject(child, "low_free", query_result.result[i * query_result.column + 10]);
			cJSON_AddStringToObject(child, "update_threshold", query_result.result[i * query_result.column + 12]);
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
			sprintf(sql, "UPDATE %s SET up_limit=%.1f, up_free=%.1f, low_limit=%.1f, \
					low_free=%.1f, update_threshold=%.1f WHERE id=%d",
					"parameter", up_limit, up_free, low_limit, low_free, update_threshold, id);
			db_handle->action(db_handle, sql, error_msg);
        }
        object = NULL;
    }

    write_profile(dic, "ALARM", "rs232_alarm_flag", "0");
    write_profile(dic, "ALARM", "rs485_alarm_flag", "0");
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

/* 获取参数相关命令及操作 */
cmd_fun_t cmd_get_param[] = {
    {
        "network",
        get_network_param
    },
    {
        "snmp",
        get_snmp_param
    },
    {
        "io",
        get_io_param
    },
    {
        "ntp",
        get_ntp_param
    }
};

/* 设置参数相关命令及操作 */
cmd_fun_t cmd_set_param[] = {
    {
        "network",
        set_network_param
    },
    {
        "snmp",
        set_snmp_param
    },
    {
        "uart",
        set_uart_param
	},
	{
		"io",
		set_io_param
	},
    {
        "ntp",
        set_ntp_param
    },
    {
        "calibration",
        set_device_time
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
	}/*,
	{
		"login"
		login
	}*/
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
		"set_protocol_alarm_param",
		set_protocol_alarm_param
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
	},
	{
		"query_alarm_param",
		query_alarm_param
	}
};

/* 消息类型及其对应的 命令:操作函数 数组 */
msg_fun_t msg_flow[] = {
    {
        "get_param",
        cmd_get_param,
        sizeof(cmd_get_param) / sizeof(cmd_fun_t)
    },
    {
        "set_param",
        cmd_set_param,
        sizeof(cmd_set_param) / sizeof(cmd_fun_t)
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

static int mib_download(req_buf_t *req_buf, const char *filename)
{
    struct stat s;
    stat(filename, &s);
    printf("Content-Type: application/octet-stream\r\n");
    printf("Content-Length: %ld\r\n", s.st_size);
    printf("Content-Disposition: attachment; filename=%s\r\n\r\n", filename);

    FILE *fp = fopen(filename, "rb");
    char buf[1024] = {0};
    int n = 0;
    while ((n = fread(buf, 1, 1024, fp)) > 0) {
        fwrite(buf, 1, n, stdout);
    }
    fclose(fp);
    fp = NULL;

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

	char error_msg[512] = {0};
	char sql[512] = {0};
	sprintf(sql, "create table if not exists %s \
					(id INTEGER PRIMARY KEY AUTOINCREMENT, \
					name VARCHAR(32), \
					phone VARCHAR(32))", "phone_user");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table if not exists %s \
					(id INTEGER PRIMARY KEY AUTOINCREMENT, \
					name VARCHAR(32), \
					email VARCHAR(32))", "email_user");
	priv->sys_db_handle->action(priv->sys_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             device_id INTEGER, \
             device_name VARCHAR(32), \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
             enum_value INTEGER, \
             enum_desc VARCHAR(32))", "data_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             created_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             device_id INTEGER, \
             device_name VARCHAR(32), \
             param_name VARCHAR(32), \
             param_type INTEGER, \
             analog_value DOUBLE, \
             enum_value INTEGER, \
             enum_desc VARCHAR(32), \
             alarm_desc VARCHAR(32))", "alarm_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             send_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             device_name VARCHAR(32), \
             param_name VARCHAR(32), \
             alarm_desc VARCHAR(32), \
             phone VARCHAR(32), \
             send_status INTEGER, \
             sms_content VARCHAR(128))", "sms_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "create table if not exists %s \
            (id INTEGER PRIMARY KEY AUTOINCREMENT, \
             send_time TIMESTAMP NOT NULL DEFAULT (datetime('now', 'localtime')), \
             device_name VARCHAR(32), \
             param_name VARCHAR(32), \
             alarm_desc VARCHAR(32), \
             email VARCHAR(32), \
             send_status INTEGER, \
             email_content VARCHAR(128))", "email_record");
    priv->data_db_handle->action(priv->data_db_handle, sql, error_msg);

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
            ret = mib_download(&(priv->request), "param.ini");
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
