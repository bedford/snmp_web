#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "iniparser.h"

#define INI_FILE_NAME	"param.ini"

typedef struct {
    int     req_len;
    int     max_len;
    char    *buf;

    char    *fb_buf;    /* 返回值内存指针 */
    int     fb_len;     /* 返回值长度 */
} req_buf_t;

typedef struct {
    int     value;
    char    *text;
} map_t;

map_t uart_param[] = {
    {0,     "2400"},
    {1,     "4800"},
    {2,     "9600"},
    {3,     "19200"},
    {4,     "38400"},
    {5,     "57600"},
    {6,     "115200"}
};

map_t protocol_param[] = {
    {100,   "UPS100"},
    {101,   "UPS101"},
    {200,   "UPS200"}
};

static int get_network_param(req_buf_t *req_buf, dictionary *dic)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mac_addr", "F0:FF:04:00:5D:F4");
    cJSON_AddStringToObject(root, "ip_addr", iniparser_getstring(dic, "NETWORK:ip_addr", "192.168.0.100"));
    cJSON_AddStringToObject(root, "gateway", iniparser_getstring(dic, "NETWORK:gateway", "192.168.0.1"));
    cJSON_AddStringToObject(root, "netmask", iniparser_getstring(dic, "NETWORK:netmask", "255.255.255.0"));
    cJSON_AddStringToObject(root, "master_dns", iniparser_getstring(dic, "NETWORK:master_dns", "192.168.8.8"));
    cJSON_AddStringToObject(root, "slave_dns", iniparser_getstring(dic, "NETWORK:slave_dns", "8.8.8.8"));
    req_buf->fb_buf = cJSON_Print(root);
    cJSON_Delete(root);

    return 0;
}

static int get_snmp_param(req_buf_t *req_buf, dictionary *dic)
{
    cJSON *sub_dir;
    cJSON *child;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "snmp_union",
            iniparser_getstring(dic, "SNMP:snmp_union", "public"));
    cJSON_AddStringToObject(root, "trap_server_ip",
            iniparser_getstring(dic, "SNMP:trap_server_ip", "192.168.0.100"));
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "authority_ip", sub_dir);

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

    req_buf->fb_buf = cJSON_Print(root);
    cJSON_Delete(root);

    return 0;
}

static int get_io_param(req_buf_t *req_buf, dictionary *dic)
{
    cJSON *sub_dir;
    cJSON *child;

    cJSON *root = cJSON_CreateObject();
	int i = 0;
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "uart_param", sub_dir);
	for (i = 0; i < 7; i++) {
    	child = cJSON_CreateObject();
    	cJSON_AddNumberToObject(child, "value", uart_param[i].value);
    	cJSON_AddStringToObject(child, "text", uart_param[i].text);
    	cJSON_AddItemToArray(sub_dir, child);
	}

    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "protocol_param", sub_dir);
    for (i = 0; i < 3; i++) {
        child = cJSON_CreateObject();
    	cJSON_AddNumberToObject(child, "value", protocol_param[i].value);
    	cJSON_AddStringToObject(child, "text", protocol_param[i].text);
    	cJSON_AddItemToArray(sub_dir, child);
    }

    int io_status[4] = {1, 0, 0, 1};
    sub_dir = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "io_status", sub_dir);
    for (i = 0; i < 4; i++) {
        child = cJSON_CreateObject();
    	cJSON_AddNumberToObject(child, "value", io_status[i]);
    	cJSON_AddItemToArray(sub_dir, child);
    }

    cJSON_AddNumberToObject(root, "rs232_protocol",
            iniparser_getint(dic, "PROTOCOL:rs232_protocol", 101));
    cJSON_AddNumberToObject(root, "rs232_baudrate",
            iniparser_getint(dic, "PROTOCOL:rs232_baudrate", 1));
    cJSON_AddNumberToObject(root, "rs232_flag",
            iniparser_getint(dic, "PROTOCOL:rs232_flag", 1));

    cJSON_AddNumberToObject(root, "rs485_protocol",
            iniparser_getint(dic, "PROTOCOL:rs485_protocol", 101));
    cJSON_AddNumberToObject(root, "rs485_baudrate",
            iniparser_getint(dic, "PROTOCOL:rs485_baudrate", 1));
    cJSON_AddNumberToObject(root, "rs485_flag",
            iniparser_getint(dic, "PROTOCOL:rs485_flag", 0));

    req_buf->fb_buf = cJSON_Print(root);
    cJSON_Delete(root);

    return 0;
}

static int get_ntp_param(req_buf_t *req_buf, dictionary *dic)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ntp_server_ip",
            iniparser_getstring(dic, "NTP:ntp_server_ip", "192.168.0.201"));
    cJSON_AddNumberToObject(root, "ntp_interval",
            iniparser_getint(dic, "NTP:ntp_interval", 60));

    req_buf->fb_buf = cJSON_Print(root);
    cJSON_Delete(root);

    return 0;
}

static int parse_get_param(cJSON *root, req_buf_t *req_buf, dictionary *dic)
{
    int ret = -1;
    int cmd_type = cJSON_GetObjectItem(root, "cmd_type")->valueint;
    switch (cmd_type) {
    case 0: /* 网络参数 */
        get_network_param(req_buf, dic);
        ret = 0;
        break;
    case 1:
        get_snmp_param(req_buf, dic);
        ret = 0;
        break;
    case 2:
        get_io_param(req_buf, dic);
        ret = 0;
        break;
    case 3:
        get_ntp_param(req_buf, dic);
        ret = 0;
    default:
        break;
    }

    return ret;
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

static int set_network_param(cJSON *root, req_buf_t *req_buf, dictionary *dic)
{
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    write_profile(dic, "NETWORK", "ip_addr", cJSON_GetObjectItem(cfg, "ip_addr")->valuestring);
    write_profile(dic, "NETWORK", "gateway", cJSON_GetObjectItem(cfg, "gateway")->valuestring);
    write_profile(dic, "NETWORK", "netmask", cJSON_GetObjectItem(cfg, "netmask")->valuestring);
    write_profile(dic, "NETWORK", "master_dns", cJSON_GetObjectItem(cfg, "master_dns")->valuestring);
    write_profile(dic, "NETWORK", "slave_dns", cJSON_GetObjectItem(cfg, "slave_dns")->valuestring);
    dump_profile(dic, INI_FILE_NAME);

    cJSON *response;
    response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);
    req_buf->fb_buf = cJSON_Print(response);
    cJSON_Delete(response);

    return 0;
}

static int set_snmp_param(cJSON *root, req_buf_t *req_buf, dictionary *dic)
{
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

static int set_ntp_param(cJSON *root, req_buf_t *req_buf, dictionary *dic)
{
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

static int set_device_time(cJSON *root, req_buf_t *req_buf)
{
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

static int parse_set_param(cJSON *root, req_buf_t *req_buf, dictionary *dic)
{
    int ret = -1;
    int cmd_type = cJSON_GetObjectItem(root, "cmd_type")->valueint;
    switch (cmd_type) {
    case 0: /* 网络参数 */
        ret = set_network_param(root, req_buf, dic);
        break;
    case 1:
        ret = set_snmp_param(root, req_buf, dic);
        break;
    case 2:
        //set_io_param(root, req_buf, dic);
        ret = 0;
        break;
    case 3:
        ret = set_ntp_param(root, req_buf, dic);
        break;
    case 4:
        ret = set_device_time(root, req_buf);
        break;
    default:
        break;
    }

    return ret;
}

static int parse_query_data(cJSON *root, req_buf_t *req_buf)
{
    return 0;
}

static int system_setting(cJSON *root, req_buf_t *req_buf, dictionary *dic)
{
    int ret = -1;
    int cmd_type = cJSON_GetObjectItem(root, "cmd_type")->valueint;
    cJSON *cfg = cJSON_GetObjectItem(root, "cfg");
    cJSON *response = cJSON_CreateObject();
    switch (cmd_type) {
    case 0: /* 获取系统参数 */
        cJSON_AddStringToObject(response, "site",
                iniparser_getstring(dic, "SYSTEM:site", ""));
        cJSON_AddStringToObject(response, "device_number",
                iniparser_getstring(dic, "SYSTEM:device_number", "20170201007"));
        req_buf->fb_buf = cJSON_Print(response);

        ret = 0;
        break;
    case 1: /* 设置系统参数 */
        write_profile(dic, "SYSTEM", "site",
                cJSON_GetObjectItem(cfg, "site")->valuestring);
        write_profile(dic, "SYSTEM", "device_number",
                cJSON_GetObjectItem(cfg, "device_number")->valuestring);
        dump_profile(dic, INI_FILE_NAME);

        cJSON_AddNumberToObject(response, "status", 1);
        req_buf->fb_buf = cJSON_Print(response);

        ret = 0;
        break;
    case 2: /* 重启设备 */
        ret = 0;
        break;
    case 3: /* 恢复出厂设置 */
        ret = 0;
        break;
    case 4: /* 获取系统信息 */
        ret = 0;
        break;
    case 5: /* 用户信息获取 */
        ret = 0;
        break;
    case 6: /* 新增用户信息 */
        ret = 0;
        break;
    case 7: /* 删除用户信息 */
        ret = 0;
        break;
    case 8: /* 修改用户信息 */
        ret = 0;
        break;
    default:
        break;
    }
    cJSON_Delete(response);

    return ret;
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

#define RET_BUF_MAX (512 * 1024)

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
	dictionary *ini = iniparser_load(INI_FILE_NAME);

    req_buf_t request;
    memset(&request, 0, sizeof(req_buf_t));
    request.max_len = 512 * 1024;
    request.buf     = (char *)calloc(1, request.max_len);
    ret = parse_request(&request);
    if (ret == 0) {
        cJSON *root = cJSON_Parse(request.buf);
        request.fb_buf = NULL;
        int msg_type = cJSON_GetObjectItem(root, "msg_type")->valueint;
        switch (msg_type) {
        case 0:
            ret = parse_get_param(root, &request, ini);
            break;
        case 1:
            ret = parse_set_param(root, &request, ini);
            break;
        case 2:
            ret = parse_query_data(root, &request);
            break;
        case 3: //重启及恢复出厂设置、固件更新等
            ret = system_setting(root, &request, ini);
            break;
        case 4: //报警设置相关操作
            break;
        default:
            break;
        }

        if (request.fb_buf) {
            fprintf(stdout, "Content-type: text/html\n\n");
            fprintf(stdout, "%s", request.fb_buf);
            free(request.fb_buf);
            request.fb_buf = NULL;
        }
        cJSON_Delete(root);
    } else if (ret == 1) {
        //下载MIB
        if (strstr(request.buf, "mib") != NULL) {
            ret = mib_download(&request, "param.ini");
        }
    }
    free(request.buf);
    request.buf = NULL;

	iniparser_freedict(ini);
	ini = NULL;

    return ret;
}
