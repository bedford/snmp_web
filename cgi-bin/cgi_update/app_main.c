#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cJSON.h"

static int get_filename(const char *src, char *filename)
{
    int ret = -1;

    char *start = strstr(src, "filename=\"");
    if (start != NULL) {
        start += strlen("filename=\"");
        char *end = strchr(start, '\"');
        if (end != NULL) {
            int len = end - start;
            strncpy(filename, start, len);
            filename[len] = '\0';
            ret = 0;
        }
    }

    return ret;
}

int main(void)
{
    int ret = -1;

    int len = 0;
    char *env_string = getenv("CONTENT_LENGTH");
    if (env_string != NULL) {
        len = atoi(env_string);
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "status", 1);

    char boundary[256] = {0};
    boundary[0] = '\r';
    boundary[1] = '\n';
    boundary[2] = '\0';

    /* 提取boundary */
    if (fgets(&boundary[2], sizeof(boundary) - 2, stdin)) {
        char *ps = NULL;
        if ((ps = strchr(&boundary[2], '\r')) != NULL) {
            *ps = '\0';
        }

        if ((ps = strchr(&boundary[2], '\n')) != NULL) {
            *ps = '\0';
        }

        /* 提取文件名 */
        char tmp_buf[512] = {0};
        char filename[64] = {0};
        char file_path[256] = "/tmp/";
        fgets(tmp_buf, sizeof(tmp_buf), stdin);
        if (get_filename(tmp_buf, filename) == 0) {
            strcat(file_path, filename);

            /* 读取Content-Type */
            memset(tmp_buf, 0, sizeof(tmp_buf));
            fgets(tmp_buf, sizeof(tmp_buf), stdin);

            /* 读取/r/n */
            memset(tmp_buf, 0, sizeof(tmp_buf));
            fgets(tmp_buf, sizeof(tmp_buf), stdin);

            FILE *fp = fopen(file_path, "wb+");
            if (fp != NULL) {
                int count = fread(tmp_buf, 1, sizeof(tmp_buf), stdin);
                while (count > 0) {
                    fwrite(tmp_buf, 1, count, fp);
                    count = fread(tmp_buf, 1, sizeof(tmp_buf), stdin);
                }

                count = ftell(fp);
                if (count > 128) {
                    count = 128;
                }

                fseek(fp, -count, SEEK_END);
                long total = ftell(fp);
                memset(tmp_buf, 0, sizeof(tmp_buf));
                fread(tmp_buf, 1, count, fp);
                int i = 0;
                for (i = 0; i < count; i++) {
                    if (tmp_buf[i] == boundary[0]) {
                        if (strncmp(boundary, &tmp_buf[i], strlen(boundary)) == 0) {
                            total += i;
                            break;
                        }
                    }
                }

                if (i < count) {
                    cJSON_AddNumberToObject(response, "i", i);
                    cJSON_AddNumberToObject(response, "count", count);
                    cJSON_AddNumberToObject(response, "totol_1", total);
                    fseek(fp, total, SEEK_SET);
                    total = ftell(fp);
                    cJSON_AddNumberToObject(response, "total", total);

                    int fd = fileno(fp);
                    ftruncate(fd, total);
                    fflush(fp);
                    fclose(fp);
                }
            }
        }
    }

    char *fb_string = cJSON_Print(response);
    cJSON_Delete(response);

    fprintf(stdout, "Content-type: text/html\n\n");
    fprintf(stdout, "%s", fb_string);
    free(fb_string);
    fb_string = NULL;

    return ret;
}
