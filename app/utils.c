#include <unistd.h>
#include <stdlib.h>

int file_exist(const char *file_name)
{
    int ret = -1;
    if (file_name != NULL) {
        ret = access(file_name, F_OK);
    }

    return ret;
}

int file_remove(const char *file_name)
{
    int ret = -1;
    if (file_name != NULL) {
        ret = remove(file_name);
    }

    return ret;
}
