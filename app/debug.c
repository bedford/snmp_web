#include <stdio.h>

#include "debug.h"

#define ERR_RETURN_LEN_ZERO         (100)   /* 读取到的数据长度为0 */
#define ERR_RETURN_LEN_UNMATCH      (101)   /* 读取到的数据长度不匹配 */
#define ERR_RETURN_CRC_UNMATCH      (102)   /* 读取到的数据CRC校验错误 */
#define ERR_RETURN_ADDRESS_UNMATCH  (103)   /* MODBUS地址和读指令不匹配 */

void print_buf(unsigned char *buf, int len)
{
    int i = 0;
    for (i = 0; i < len; i++) {
        printf("%x  ", buf[i]);
    }
    printf("\n");
}

void print_com_info(int com_index, char *device_name, int dir, char *buf, int len, int err_code)
{
    printf("COM%d, 设备名称：%s,", com_index, device_name);

    int i = 0;
    switch (err_code) {
    case ERR_RETURN_LEN_ZERO:
        printf("提示信息: 接收数据长度为零; ");
        break;
    case ERR_RETURN_LEN_UNMATCH:
        printf("提示信息: 接收数据长度不匹配; ");
        for (i = 0; i < len; i++) {
            printf("%02X", buf[i]);
        }
        break;
    case ERR_RETURN_CRC_UNMATCH:
        printf("提示信息: 接收数据CRC校验不匹配; ");
        for (i = 0; i < len; i++) {
            printf("%02X", buf[i]);
        }
        break;
    case ERR_RETURN_ADDRESS_UNMATCH:
        printf("提示信息: 接收数据设备地址不匹配; ");
        for (i = 0; i < len; i++) {
            printf("%02X", buf[i]);
        }
        break;
    default:
        if (dir == 0) {
            printf("发送串:");
        } else {
            printf("接收串:");
        }
        for (i = 0; i < len; i++) {
            printf("%02X", buf[i]);
        }
        break;
    }

    printf("\n");
}

#if 0
void print_snmp_protocol(protocol_t *snmp_protocol)
{
        printf("#############################################\n");
        printf("protocol_id:%d\n", snmp_protocol->protocol_id);
        printf("protocol_description:%s\n", snmp_protocol->protocol_name);
        printf("brand_name:%s\n", snmp_protocol->device_brand);
        printf("get_cmd_info func %p\n", snmp_protocol->get_property);
        printf("cal_dev_data func %p\n", snmp_protocol->calculate_data);
        printf("#############################################\n");
}

void print_param_value(list_t *param_value_list)
{
    int len = param_value_list->get_list_size(param_value_list);
    param_value_t *param_value = NULL;
    int i = 0;
    for (i = 0; i < len; i++) {
        printf("len %d\n", len);
        param_value = param_value_list->get_index_value(param_value_list, i);
        printf("param_id %d\n", param_value->param_id);
        printf("param_value %.1f\n", param_value->param_value);
    }
}
#endif

