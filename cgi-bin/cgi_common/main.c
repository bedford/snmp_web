#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

typedef struct {
    int     req_len;
    int     max_len;
    char    *buf;

    char    *fb_buf;    /* 返回值内存指针 */
    int     fb_len;     /* 返回值长度 */
} req_buf_t;

static int get_network_param(req_buf_t *req_buf)
{
    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mac_addr", "F0:FF:04:00:5D:F4");
    cJSON_AddStringToObject(root, "ip_addr", "192.168.0.100");
    cJSON_AddStringToObject(root, "gateway", "192.168.0.1");
    cJSON_AddStringToObject(root, "netmask", "255.255.255.0");
    cJSON_AddStringToObject(root, "master_dns", "8.8.8.8");
    cJSON_AddStringToObject(root, "slave_dns", "");
    req_buf->fb_buf = cJSON_Print(root);
    cJSON_Delete(root);

    return 0;
}

static int parse_get_param(cJSON *root, req_buf_t *req_buf)
{
    int ret = -1;
    int cmd_type = cJSON_GetObjectItem(root, "cmd_type")->valueint;
    switch (cmd_type) {
    case 0: /* 网络参数 */
        get_network_param(req_buf);
        ret = 0;
        break;
    case 1:
        ret = 0;
        break;
    case 2:
        ret = 0;
        break;
    default:
        break;
    }

    return ret;
}

static int parse_set_param(cJSON *root, req_buf_t *req_buf)
{
    return 0;
}

static int parse_query_data(cJSON *root, req_buf_t *req_buf)
{
    return 0;
}

staitc int parse_system_ctl(cJSON *root, req_buf_t *req_buf)
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
        char env_string = getenv("CONTENT_LENGTH");
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
    }

    return ret;
}

int main(void)
{
    int ret = -1;
    req_buf_t request;
    memset(&request, 0, sizeof(req_buf_t));
    request.max_len = 512 * 1024;
    request.buf     = (char *)calloc(1, request.max_len);
    if (parse_request(&req_buf) == 0) {
        cJSON *root = cJSON_Parse(req_buf.buf);
        request.fb_buf = NULL;
        //request.fb_buf = (char *)calloc(1, RET_BUF_MAX);
        int msg_type = cJSON_GetObjectItem(root, "msg_type")->valueint;
        switch (msg_type) {
        case 0:
            ret = parse_get_param(root, &request);
            break;
        case 1:
            ret = parse_set_param(root, &request);
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
            fprintf(stdout, "%s", request.fb_buf);
            free(request.fb_buf);
            request.fb_buf = NULL;
        }
        cJSON_Delete(root);
    }
    free(request.buf);
    request.buf = NULL;

    return ret;
}
