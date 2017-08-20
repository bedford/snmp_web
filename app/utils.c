#include <unistd.h>
#include <stdlib.h>

/**
 * @brief   file_exist 检查文件是否存在
 * @param   file_name
 * @return
 */
int file_exist(const char *file_name)
{
    int ret = -1;
    if (file_name != NULL) {
        ret = access(file_name, F_OK);
    }

    return ret;
}

/**
 * @brief   file_remove 删除文件 
 * @param   file_name
 * @return
 */
int file_remove(const char *file_name)
{
    int ret = -1;
    if (file_name != NULL) {
        ret = remove(file_name);
    }

    return ret;
}
