#include <stdio.h>
#include <string.h>

#include "protocol_interfaces.h"

static void print_snmp_protocol(protocol_t *snmp_protocol)
{
        printf("#############################################\n");
        printf("protocol_id:%d\n", snmp_protocol->protocol_id);
        printf("protocol_description:%s\n", snmp_protocol->protocol_name);
        printf("get_cmd_info func %p\n", snmp_protocol->get_property);
        printf("cal_dev_data func %p\n", snmp_protocol->calculate_data);
        printf("#############################################\n");
}

static void print_param_value(list_t *param_value_list)
{
    int len = param_value_list->get_list_size(param_value_list);
    param_value_t *param_value = NULL;
    int i = 0;
    for (i = 0; i < len; i++) {
        param_value = param_value_list->get_index_value(param_value_list, i);
        printf("param_id %d\n", param_value->param_id);
        printf("param_value %.1f\n", param_value->param_value);
        printf("param_enum %d\n", param_value->enum_value);
    }
}

int main(void)
{
    list_t *protocol_list = list_create(sizeof(protocol_t));
    init_protocol_lib(protocol_list);

#if 0
    protocol_t *protocol = get_protocol_handle(protocol_list, UPS | C_KS);
    print_snmp_protocol(protocol);

    list_t *property_list = list_create(sizeof(property_t));
    protocol->get_property(property_list);

    property_t *property = property_list->get_index_value(property_list, 0);

    char data[48] = {0};
    strcpy(data, "(222.4 222.4 221.0 019 50.0 2.27 54.0 00000000");
    data[46] = 0x0d;
    list_t *value_list = list_create(sizeof(param_value_t));
    protocol->calculate_data(property, data, sizeof(data), value_list);
#else

#if 0
    protocol_t *protocol = get_protocol_handle(protocol_list, TEMP_HUM_DEVICE | 0x01);
    print_snmp_protocol(protocol);

    list_t *property_list = list_create(sizeof(property_t));
    protocol->get_property(property_list);

    property_t *property = property_list->get_index_value(property_list, 0);

    char data[] = {0x01, 0x04, 0x04, 0x00, 0x9C, 0x02, 0x72, 0xBA, 0xEF};
    list_t *value_list = list_create(sizeof(param_value_t));
    protocol->calculate_data(property, data, sizeof(data), value_list);
#else
    protocol_t *protocol = get_protocol_handle(protocol_list, EXTERNAL_IO | 0x01);
    print_snmp_protocol(protocol);

    list_t *property_list = list_create(sizeof(property_t));
    protocol->get_property(property_list);

    property_t *property = property_list->get_index_value(property_list, 1);

    char data[] = {0x01, 0x01, 0x01, 0x01, 0x90, 0x48};
    list_t *value_list = list_create(sizeof(param_value_t));
    protocol->calculate_data(property, data, sizeof(data), value_list);
#endif
#endif

    print_param_value(value_list);
    property = NULL;

    protocol->release_property(property_list);
    property_list = NULL;

    value_list->destroy_list(value_list);
    value_list = NULL;

    deinit_protocol_lib(protocol_list);
    protocol_list = NULL;

    return 0;
}
