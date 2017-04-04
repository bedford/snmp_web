#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol_types.h"
#include "protocol_interfaces.h"

#include "ups.h"

enum
{
    READ_UPS_STATUS_CMD = 0x01,
};

static cmd_t ups_status_cmd(void)
{
    cmd_t tmp_cmd;
    memset(&tmp_cmd, 0, sizeof(cmd_t));

    tmp_cmd.cmd_id  = READ_UPS_STATUS_CMD;
    sprintf(tmp_cmd.cmd_name, "%s", "读UPS状态");

    tmp_cmd.cmd_code[0]  = 0x51;
    tmp_cmd.cmd_code[1]  = 0x31;
    tmp_cmd.cmd_code[2]  = 0x0d;
    tmp_cmd.cmd_len = 3;
    tmp_cmd.end_len = 0;

    tmp_cmd.cmd_format  = CMD_FORMAT_HEX;
    tmp_cmd.check_len   = 47;
    tmp_cmd.read_timeout= 2;     /* 等待时间，单位:秒 */
    tmp_cmd.verify_type = VERIFY_TYPE_NONE;

    return tmp_cmd;
}

static list_t *ups_status_param_desc(void)
{
    list_t *desc_list = list_create(sizeof(param_desc_t));
    param_desc_t param;
    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "当前市电电压");
    sprintf(param.param_unit, "%s", "V");
    param.param_id  = 1;
    param.up_limit  = 250.0;
    param.up_free   = 235.0;
    param.low_limit = 170.0;
    param.low_free  = 190.0;
    //param.update_interval   = 30;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "市电电压最低值");
    sprintf(param.param_unit, "%s", "V");
    param.param_id  = 2;
    param.up_limit  = 240.0;
    param.up_free   = 235.0;
    param.low_limit = 170.0;
    param.low_free  = 190.0;
    //param.update_interval   = 30;
    param.update_threshold  = 230.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "输出电压值");
    sprintf(param.param_unit, "%s", "V");
    param.param_id  = 3;
    param.up_limit  = 240.0;
    param.up_free   = 235.0;
    param.low_limit = 180.0;
    param.low_free  = 190.0;
    //param.update_interval   = 30;
    param.update_threshold  = 230.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "当前负载百分比");
    sprintf(param.param_unit, "%s", "\%");
    param.param_id  = 4;
    param.up_limit  = 95.0;
    param.up_free   = 90.0;
    param.low_limit = 20.0;
    param.low_free  = 50.0;
    //param.update_interval   = 30;
    param.update_threshold  = 80.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "市电频率");
    sprintf(param.param_unit, "%s", "Hz");
    param.param_id  = 5;
    param.up_limit  = 60.0;
    param.up_free   = 53.0;
    param.low_limit = 40.0;
    param.low_free  = 47.0;
    //param.update_interval   = 30;
    param.update_threshold  = 55.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "电池电量");
    sprintf(param.param_unit, "%s", "\%");
    param.param_id  = 6;
    param.up_limit  = 100.0;
    param.up_free   = 90.0;
    param.low_limit = 10.0;
    param.low_free  = 20.0;
    //param.update_interval   = 30;
    param.update_threshold  = 100.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "温度");
    sprintf(param.param_unit, "%s", "℃");
    param.param_id  = 7;
    param.up_limit  = 70.0;
    param.up_free   = 65.0;
    param.low_limit = 15.0;
    param.low_free  = 18.0;
    //param.update_interval   = 30;
    param.update_threshold  = 50.0;
    param.param_type = PARAM_TYPE_ANALOG;
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "市电状态");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 8;
    param.up_limit  = 0;
    param.up_free   = 0;
    param.low_limit = 0;
    param.low_free  = 0;
    //param.update_interval   = 30;
    param.update_threshold  = 50.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
    sprintf(param.param_enum[0].desc, "%s", "市电正常");
    param.param_enum[1].value = 1;
    sprintf(param.param_enum[1].desc, "%s", "市电异常");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "电池状态");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 9;
    param.up_limit  = 0;
    param.up_free   = 0;
    param.low_limit = 0;
    param.low_free  = 0;
    //param.update_interval   = 30;
    param.update_threshold  = 50.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
    sprintf(param.param_enum[0].desc, "%s", "电池正常");
    param.param_enum[0].value = 1;
    sprintf(param.param_enum[1].desc, "%s", "电池低");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "输出方式");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 10;
    param.up_limit  = 0;
    param.up_free   = 0;
    param.low_limit = 0;
    param.low_free  = 0;
    //param.update_interval   = 30;
    param.update_threshold  = 50.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
    sprintf(param.param_enum[0].desc, "%s", "逆变");
    param.param_enum[0].value = 1;
    sprintf(param.param_enum[1].desc, "%s", "旁路");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "UPS状态");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 11;
    param.up_limit  = 0;
    param.up_free   = 0;
    param.low_limit = 0;
    param.low_free  = 0;
    //param.update_interval   = 30;
    param.update_threshold  = 50.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
    sprintf(param.param_enum[0].desc, "%s", "正常");
    param.param_enum[0].value = 1;
    sprintf(param.param_enum[1].desc, "%s", "异常");
    desc_list->push_back(desc_list, &param);

    return desc_list;
}

static int calculate_device_status(cmd_t    *cmd,
                                   list_t   *param_desc_list,
                                   char     *data,
                                   int      data_len,
                                   list_t   *valid_value)
{
    int ret = -1;

    if (data_len != cmd->check_len) { //包长度校验
        return ret;
    }

    if ((data[0] != '(') && (data[data_len - 1] != 0x0d)) { //包头,包尾校验
        return ret;
    }

    param_desc_t *desc = NULL;
    param_value_t tmp_value;

#if 0
    int index = 0;
    char *tmp = strtok(data + 1, " ");
    while (tmp != NULL) {
        printf("%s\n", tmp);
        desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, index);
        index++;
        memset(&tmp_value, 0, sizeof(param_value_t));
        tmp_value.param_id = desc->param_id;
        strcpy(tmp_value.param_name, desc->param_name);
        tmp_value.param_type = PARAM_TYPE_ANALOG;
        tmp_value.param_value = atof(tmp);
        valid_value->push_back(valid_value, &tmp_value);
        tmp = strtok(NULL, " ");
    }

    printf("file %s, func %s, line %d\n", __FILE__, __func__, __LINE__);
    int i = 0;
    index -= 1;
    unsigned char status[4] = {0};
    memcpy(status, data + 38, 4);
    printf("%x, %x, %x, %x\n", status[0], status[1], status[2], status[3]);
    for (i = 0; i < 4; i++) {
        printf("index %d, %d\n", index, param_desc_list->get_list_size(param_desc_list));
        desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, index);
        index++;
        memset(&tmp_value, 0, sizeof(param_value_t));
        tmp_value.param_id = desc->param_id;
        strcpy(tmp_value.param_name, desc->param_name);
        tmp_value.param_type = PARAM_TYPE_ENUM;
        tmp_value.param_enum_value = status[i] - 0x30;
        valid_value->push_back(valid_value, &tmp_value);
    }
#else
    char tmp[8] = {0};
    int i = 0;
    int j = 0;
    int index = 0;
    unsigned char status[4] = {0};
    for (i = 1; i < data_len; i++) {
        if (data[i] == 0x20) {
            desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, index);
            index++;
            memset(&tmp_value, 0, sizeof(param_value_t));
            tmp_value.param_id = desc->param_id;
            //strcpy(tmp_value.param_name, desc->param_name);
            //tmp_value.param_type = PARAM_TYPE_ANALOG;
            tmp_value.param_value = atof(tmp);
            valid_value->push_back(valid_value, &tmp_value);
            j = 0;
            memset(tmp, 0, sizeof(tmp));
        } else if (data[i] == 0x0D) {
            memcpy(status, tmp, sizeof(status));
        } else {
            tmp[j] = data[i];
            j++;
        }
    }

    for (i = 0; i < 4; i++) {
        desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, index);
        index++;
        memset(&tmp_value, 0, sizeof(param_value_t));
        tmp_value.param_id = desc->param_id;
        //strcpy(tmp_value.param_name, desc->param_name);
        //tmp_value.param_type = PARAM_TYPE_ENUM;
        tmp_value.enum_value = status[i] - 0x30;
        valid_value->push_back(valid_value, &tmp_value);
    }
#endif
    ret = 0;

    return ret;
}

static int calculate_device_data(property_t *property, char *data, int len, list_t *valid_value)
{
    cmd_t *cmd = &(property->cmd);
    list_t *param_desc_list = property->param_desc;

    int ret = -1;
    switch (cmd->cmd_id) {
    case READ_UPS_STATUS_CMD:
        calculate_device_status(cmd, param_desc_list, data, len, valid_value);
        ret = 0;
        break;
    default:
        break;
    }

    return ret;
}

static int get_ups_property(list_t *property_list)
{
    property_t property;
    property.cmd = ups_status_cmd();
    property.param_desc = ups_status_param_desc();

    property_list->push_back(property_list, &property);

    return 0;
}

static void release_ups_property(list_t *property_list)
{
    property_t *property    = NULL;
    list_t *param_desc_list = NULL;
    int list_len = property_list->get_list_size(property_list);
    int i = 0;
    for (i = 0; i < list_len; i++) {
        property = property_list->get_index_value(property_list, i);
        param_desc_list = property->param_desc;
        param_desc_list->destroy_list(param_desc_list);
        param_desc_list = NULL;
    }
    property_list->destroy_list(property_list);
    property_list = NULL;
}

int ups_register(list_t *protocol_list)
{
    protocol_t protocol;

    protocol.protocol_id = UPS | C_KS;
    strcpy(protocol.protocol_name, "UPS-C1KS");
    strcpy(protocol.protocol_desc, "C1KS型号UPS协议,波特率2400");
    strcpy(protocol.device_brand, "C1KS");

    protocol.get_property       = get_ups_property;
    protocol.calculate_data     = calculate_device_data;
    protocol.release_property   = release_ups_property;

    protocol_list->push_back(protocol_list, &protocol);

    return 0;
}
