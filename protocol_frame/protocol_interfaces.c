#include <stdlib.h>

#include "protocol_interfaces.h"
#include "ups.h"
#include "environment_k25.h"
#include "external_io_860.h"
#include "external_io_816d.h"

static char *version_string = "multi_ups_20170920";

int init_protocol_lib(list_t *protocol_list)
{
    /* 设备注册函数调用 */
    ups_register(protocol_list);

    environment_k25_register(protocol_list);

    //environment_k20_register(protocol_list);

    external_io_860_register(protocol_list);

    external_io_816d_register(protocol_list);
    /* 后面添加类型设备协议注册函数即可 */

    return 0;
}

protocol_t *get_protocol_handle(list_t *protocol_list, unsigned int protocol_id)
{
    if (protocol_list == NULL) {
        return NULL;
    }

    protocol_t *p   = NULL;
    protocol_t *tmp = NULL;
    int list_size = protocol_list->get_list_size(protocol_list);
    int i = 0;
    for (i = 0; i < list_size; i++) {
        tmp = protocol_list->get_index_value(protocol_list, i);
        if (tmp->protocol_id == protocol_id) {
            p = tmp;
            break;
        }
    }

    return p;
}

void deinit_protocol_lib(list_t *protocol_list)
{
    protocol_list->destroy_list(protocol_list);
}

char *get_protocol_version(void)
{
    return version_string;
}
