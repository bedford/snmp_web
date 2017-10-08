#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol_types.h"
#include "protocol_interfaces.h"
#include "device_id_define.h"

#include "external_io_816d.h"
#include "crc16.h"

enum
{
    READ_IO_CMD = 0x01,
};

/*
* @brief    get_io_status  获取io当前状态的命令参数
*
* @return
*/
static cmd_t get_io_status(unsigned char rs485_addr)
{
    cmd_t tmp_cmd;
    memset(&tmp_cmd, 0, sizeof(cmd_t));

    tmp_cmd.cmd_id  = READ_IO_CMD;
    sprintf(tmp_cmd.cmd_name, "%s", "获取io实时状态");

    tmp_cmd.cmd_code[0] = '$';
    tmp_cmd.cmd_code[1] = '0';
    tmp_cmd.cmd_code[2] = '1';
    tmp_cmd.cmd_code[3] = '6';
    tmp_cmd.cmd_code[4] = 0x0d;
    tmp_cmd.cmd_len = 5;
    tmp_cmd.end_len = 0;

    tmp_cmd.cmd_format  = CMD_FORMAT_HEX;
    tmp_cmd.check_len   = 8;
    tmp_cmd.read_timeout= 2;     /* 等待时间，单位:秒 */
    tmp_cmd.verify_type = VERIFY_TYPE_NONE;

    return tmp_cmd;
}

/**
 * @brief   io_param_desc IO传感器获取命令返回参数的初始化
 *
 * @return
 */
static list_t *io_param_desc(void)
{
    list_t *desc_list = list_create(sizeof(param_desc_t));
    param_desc_t param;
    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di1");
    sprintf(param.param_desc, "%s", "DI通道1");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 1;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di1_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI1低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di1_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI1高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di2");
    sprintf(param.param_desc, "%s", "DI通道2");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 2;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di2_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI2低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di2_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI2高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di3");
    sprintf(param.param_desc, "%s", "DI通道3");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 3;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di3_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI3低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di3_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI3高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di4");
    sprintf(param.param_desc, "%s", "DI通道4");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 4;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di4_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI4低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di4_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI4高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di5");
    sprintf(param.param_desc, "%s", "DI通道5");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 5;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di5_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI5低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di5_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI5高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di6");
    sprintf(param.param_desc, "%s", "DI通道6");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 6;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di6_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI6低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di6_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI6高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di7");
    sprintf(param.param_desc, "%s", "DI通道7");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 7;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di7_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI7低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di7_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI7高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di8");
    sprintf(param.param_desc, "%s", "DI通道8");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 8;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di8_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI8低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di8_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI8高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di9");
    sprintf(param.param_desc, "%s", "DI通道9");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 9;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di9_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI9低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di9_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI9高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di10");
    sprintf(param.param_desc, "%s", "DI通道10");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 10;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di10_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI10低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di10_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI10高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di11");
    sprintf(param.param_desc, "%s", "DI通道11");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 11;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di11_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI11低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di11_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI11高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di12");
    sprintf(param.param_desc, "%s", "DI通道12");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 12;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di12_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI12低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di12_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI12高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di13");
    sprintf(param.param_desc, "%s", "DI通道13");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 13;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di13_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI13低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di13_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI13高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di14");
    sprintf(param.param_desc, "%s", "DI通道14");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 14;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di14_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI14低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di14_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI14高电平");
    desc_list->push_back(desc_list, &param);

    memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di15");
    sprintf(param.param_desc, "%s", "DI通道15");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 15;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di15_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI15低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di15_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI15高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "di16");
    sprintf(param.param_desc, "%s", "DI通道16");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 16;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "di16_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DI16低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "di16_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DI16高电平");
    desc_list->push_back(desc_list, &param);

    return desc_list;
}

/**
 * @brief   calculate_device_status 查询温湿度传感器状态返回值处理
 *
 * @param   cmd
 * @param   param_desc_list
 * @param   data
 * @param   data_len
 * @param   valid_value
 *
 * @return
 */
static int calculate_io_status(cmd_t    *cmd,
                               list_t   *param_desc_list,
                               char     *data,
                               int      data_len,
                               list_t   *valid_value)
{
    int ret = -1;
    if (data_len == 0) {
        return ERR_RETURN_LEN_ZERO;
    }

    if (data_len != cmd->check_len) { //包长度校验
        printf("file %s, func %s, line %d, data len %d, check %d\n",
                __FILE__, __func__, __LINE__, data_len, cmd->check_len);
        return ERR_RETURN_LEN_UNMATCH;
    }

    param_desc_t *desc = NULL;
    param_value_t tmp_value;

    unsigned char tmp[8] = {0};
    tmp[0] = data[1];
    tmp[1] = data[2];
    tmp[2] = data[3];
    tmp[3] = data[4];

    unsigned int val = strtol(tmp, NULL, 16);
    
    int i = 0;
    for (i = 0; i < 16; i++) {
        memset(&tmp_value, 0, sizeof(param_value_t));
        desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, i);
        tmp_value.param_id = desc->param_id;
        tmp_value.param_value = 0;
        tmp_value.enum_value = ((val & (0x01 << i)) >> i);
        valid_value->push_back(valid_value, &tmp_value);
    }
    ret = 0;

    return ret;
}

/**
 * @brief   calculate_device_data 协议处理函数
 *          根据命令编号调用不同的处理函数,按协议标准进行解析
 *
 * @param   property
 * @param   data
 * @param   len
 * @param   valid_value
 *
 * @return
 */
static int calculate_device_data(property_t *property, char *data, int len, list_t *valid_value)
{
    cmd_t *cmd = &(property->cmd);
    list_t *param_desc_list = property->param_desc;

    int ret = -1;
    switch (cmd->cmd_id) {
    case READ_IO_CMD:
        ret = calculate_io_status(cmd, param_desc_list, data, len, valid_value);
        break;
    default:
        break;
    }

    return ret;
}

static int get_external_io_property(list_t *property_list, unsigned char rs485_addr)
{
    property_t property;
	memset(&property, 0, sizeof(property_t));
    property.cmd = get_io_status(rs485_addr);
    property.param_desc = io_param_desc();
	property.last_param_value = NULL;
    property_list->push_back(property_list, &property);

    return 0;
}

static void release_external_io_property(list_t *property_list)
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

int external_io_816d_register(list_t *protocol_list)
{
    protocol_t protocol;

    strcpy(protocol.protocol_name, "oao-816d");
    strcpy(protocol.protocol_desc, "OAO-816dIO传感器");
    protocol.rs485_addr = 0x00;
    protocol.protocol_id = EXTERNAL_IO | OAO_816D | protocol.rs485_addr;

    protocol.get_property       = get_external_io_property;
    protocol.calculate_data     = calculate_device_data;
    protocol.release_property   = release_external_io_property;

    protocol_list->push_back(protocol_list, &protocol);

    return 0;
}
