#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    cJSON *root;
    root = cJSON_CreateObject();
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
    cJSON *root;
    cJSON *sub_dir;
    cJSON *child;
    root = cJSON_CreateObject();

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
    cJSON *root;
    cJSON *sub_dir;
    cJSON *child;
    root = cJSON_CreateObject();

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
    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ntp_server_ip", "192.168.0.201");
    cJSON_AddStringToObject(root, "ntp_interval", "60");

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
    printf("ip %s\n", cJSON_GetObjectItem(cfg, "ip_addr")->valuestring);
    dump_profile(dic, INI_FILE_NAME);

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
        set_network_param(root, req_buf, dic);
        ret = 0;
        break;
    case 1:
       // set_snmp_param(req_buf, dic);
        ret = 0;
        break;
    case 2:
       // set_io_param(req_buf, dic);
        ret = 0;
        break;
    case 3:
        //set_ntp_param(req_buf, dic);
        ret = 0;
    default:
        break;
    }

    return ret;
}

static int parse_query_data(cJSON *root, req_buf_t *req_buf)
{
    return 0;
}

static int parse_system_ctl(cJSON *root, req_buf_t *req_buf)
{
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
        printf("request method: GET, query_string %s\n", query_string);
        query_string = NULL;
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
	iniparser_dump(ini, stdout);

    req_buf_t request;
    memset(&request, 0, sizeof(req_buf_t));
    request.max_len = 512 * 1024;
    request.buf     = (char *)calloc(1, request.max_len);
    if (parse_request(&request) == 0) {
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
        case 3:
            ret = parse_system_ctl(root, &request);
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
    }
    free(request.buf);
    request.buf = NULL;

	iniparser_freedict(ini);
	ini = NULL;

    return ret;
}
