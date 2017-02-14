#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "iniparser.h"
#include "db_access.h"

#define INI_FILE_NAME	"param.ini"

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
	dictionary	*dic;
} priv_info_t;

typedef struct {
    int     value;
    char    *text;
} map_t;

map_t protocol_param[] = {
    {100,   "UPS100"},
    {101,   "UPS101"},
    {200,   "UPS200"}
};

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
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *sub_dir = NULL;
    cJSON *child = NULL;

    cJSON *response = cJSON_CreateObject();
	int i = 0;
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "protocol_param", sub_dir);
    for (i = 0; i < 3; i++) {
        child = cJSON_CreateObject();
    	cJSON_AddNumberToObject(child, "value", protocol_param[i].value);
    	cJSON_AddStringToObject(child, "text", protocol_param[i].text);
    	cJSON_AddItemToArray(sub_dir, child);
    }

    int io_status[4] = {1, 0, 0, 1};
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "io_status", sub_dir);
    for (i = 0; i < 4; i++) {
        child = cJSON_CreateObject();
    	cJSON_AddNumberToObject(child, "value", io_status[i]);
    	cJSON_AddItemToArray(sub_dir, child);
    }

    cJSON_AddNumberToObject(response, "rs232_protocol",
            iniparser_getint(dic, "PROTOCOL:rs232_protocol", 101));
    cJSON_AddNumberToObject(response, "rs232_baudrate",
            iniparser_getint(dic, "PROTOCOL:rs232_baudrate", 1));
    cJSON_AddNumberToObject(response, "rs232_databits",
            iniparser_getint(dic, "PROTOCOL:rs232_databits", 3));
    cJSON_AddNumberToObject(response, "rs232_stopbits",
            iniparser_getint(dic, "PROTOCOL:rs232_stopbits", 0));
    cJSON_AddNumberToObject(response, "rs232_parity",
            iniparser_getint(dic, "PROTOCOL:rs232_parity", 0));
    cJSON_AddNumberToObject(response, "rs232_flag",
            iniparser_getint(dic, "PROTOCOL:rs232_flag", 1));

    cJSON_AddNumberToObject(response, "rs485_protocol",
            iniparser_getint(dic, "PROTOCOL:rs485_protocol", 101));
    cJSON_AddNumberToObject(response, "rs485_baudrate",
            iniparser_getint(dic, "PROTOCOL:rs485_baudrate", 1));
    cJSON_AddNumberToObject(response, "rs485_databits",
            iniparser_getint(dic, "PROTOCOL:rs485_databits", 3));
    cJSON_AddNumberToObject(response, "rs485_stopbits",
            iniparser_getint(dic, "PROTOCOL:rs485_stopbits", 0));
    cJSON_AddNumberToObject(response, "rs485_parity",
            iniparser_getint(dic, "PROTOCOL:rs485_parity", 0));
    cJSON_AddNumberToObject(response, "rs485_flag",
            iniparser_getint(dic, "PROTOCOL:rs485_flag", 0));

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
	dictionary *dic		= priv->dic;
	req_buf_t *req_buf	= &(priv->request);

    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "PROTOCOL", "rs232_protocol",
            cJSON_GetObjectItem(cfg, "rs232_protocol")->valuestring);
	write_profile(dic, "PROTOCOL", "rs232_baudrate",
	        cJSON_GetObjectItem(cfg, "rs232_baudrate")->valuestring);
	write_profile(dic, "PROTOCOL", "rs232_databits",
	        cJSON_GetObjectItem(cfg, "rs232_databits")->valuestring);
	write_profile(dic, "PROTOCOL", "rs232_stopbits",
	        cJSON_GetObjectItem(cfg, "rs232_stopbits")->valuestring);
	write_profile(dic, "PROTOCOL", "rs232_parity",
	        cJSON_GetObjectItem(cfg, "rs232_parity")->valuestring);
	write_profile(dic, "PROTOCOL", "rs232_flag",
	        cJSON_GetObjectItem(cfg, "rs232_flag")->valuestring);

    write_profile(dic, "PROTOCOL", "rs485_protocol",
            cJSON_GetObjectItem(cfg, "rs485_protocol")->valuestring);
	write_profile(dic, "PROTOCOL", "rs485_baudrate",
	        cJSON_GetObjectItem(cfg, "rs485_baudrate")->valuestring);
	write_profile(dic, "PROTOCOL", "rs485_databits",
	        cJSON_GetObjectItem(cfg, "rs485_databits")->valuestring);
	write_profile(dic, "PROTOCOL", "rs485_stopbits",
	        cJSON_GetObjectItem(cfg, "rs485_stopbits")->valuestring);
	write_profile(dic, "PROTOCOL", "rs485_parity",
	        cJSON_GetObjectItem(cfg, "rs485_parity")->valuestring);
	write_profile(dic, "PROTOCOL", "rs485_flag",
	        cJSON_GetObjectItem(cfg, "rs485_flag")->valuestring);

    dump_profile(dic, INI_FILE_NAME);

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
	}
};

static int parse_query_data(cJSON *root, req_buf_t *req_buf)
{
    return 0;
}

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
	priv->sys_db_handle = db_access_create("sys.db");

	char error_msg[256] = {0};
	char sql[256] = {0};
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

	iniparser_freedict(priv->dic);
	priv->dic = NULL;

	priv->sys_db_handle->destroy(priv->sys_db_handle);
	priv->sys_db_handle = NULL;

	free(priv);
	priv = NULL;

    return ret;
}
