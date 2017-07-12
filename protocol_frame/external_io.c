#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol_types.h"
#include "protocol_interfaces.h"
#include "device_id_define.h"

#include "external_io.h"
#include "crc16.h"

enum
{
    READ_DI_CMD = 0x01,
	READ_DO_CMD,
};

/*
* @brief    get_status_cmd  获取查询温湿度传感器状态的命令参数
*
* @return
*/
static cmd_t get_di_status(void)
{
    cmd_t tmp_cmd;
    memset(&tmp_cmd, 0, sizeof(cmd_t));

    tmp_cmd.cmd_id  = READ_DI_CMD;
    sprintf(tmp_cmd.cmd_name, "%s", "读di实时状态");

    tmp_cmd.cmd_code[0]  = 0x01;
    tmp_cmd.cmd_code[1]  = 0x02;
    tmp_cmd.cmd_code[2]  = 0x00;
    tmp_cmd.cmd_code[3]  = 0x00;
    tmp_cmd.cmd_code[4]  = 0x00;
    tmp_cmd.cmd_code[5]  = 0x08;
    unsigned short crc16 = create_crc16_code(tmp_cmd.cmd_code, 6);
    tmp_cmd.cmd_code[6] = crc16 % 256;
    tmp_cmd.cmd_code[7] = crc16 / 256;
    tmp_cmd.cmd_len = 8;    //含CRC
    tmp_cmd.end_len = 0;

    tmp_cmd.cmd_format  = CMD_FORMAT_HEX;
    tmp_cmd.check_len   = 6;
    tmp_cmd.read_timeout= 2;     /* 等待时间，单位:秒 */
    tmp_cmd.verify_type = VERIFY_TYPE_CRC;

    return tmp_cmd;
}

/*
* @brief    get_status_cmd  获取查询温湿度传感器状态的命令参数
*
* @return
*/
static cmd_t get_do_status(void)
{
    cmd_t tmp_cmd;
    memset(&tmp_cmd, 0, sizeof(cmd_t));

    tmp_cmd.cmd_id  = READ_DO_CMD;
    sprintf(tmp_cmd.cmd_name, "%s", "读do实时状态");

    tmp_cmd.cmd_code[0]  = 0x01;
    tmp_cmd.cmd_code[1]  = 0x01;
    tmp_cmd.cmd_code[2]  = 0x00;
    tmp_cmd.cmd_code[3]  = 0x00;
    tmp_cmd.cmd_code[4]  = 0x00;
    tmp_cmd.cmd_code[5]  = 0x04;
    unsigned short crc16 = create_crc16_code(tmp_cmd.cmd_code, 6);
    tmp_cmd.cmd_code[6] = crc16 % 256;
    tmp_cmd.cmd_code[7] = crc16 / 256;
    tmp_cmd.cmd_len = 8;    //含CRC
    tmp_cmd.end_len = 0;

    tmp_cmd.cmd_format  = CMD_FORMAT_HEX;
    tmp_cmd.check_len   = 6;
    tmp_cmd.read_timeout= 2;     /* 等待时间，单位:秒 */
    tmp_cmd.verify_type = VERIFY_TYPE_CRC;

    return tmp_cmd;
}


/**
 * @brief   environment_status_param_desc 温湿度传感器获取命令返回参数的初始化
 *
 * @return
 */
static list_t *di_param_desc(void)
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

    return desc_list;
}

/**
 * @brief   environment_status_param_desc 温湿度传感器获取命令返回参数的初始化
 *
 * @return
 */
static list_t *do_param_desc(void)
{
    list_t *desc_list = list_create(sizeof(param_desc_t));
    param_desc_t param;
	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "do1");
    sprintf(param.param_desc, "%s", "DO通道1");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 7;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "do1_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DO1低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "do1_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DO1高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "do2");
    sprintf(param.param_desc, "%s", "DO通道2");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 8;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "do2_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DO2低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "do2_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DO2高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "do3");
    sprintf(param.param_desc, "%s", "DO通道3");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 9;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "do3_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DO3低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "do3_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DO3高电平");
    desc_list->push_back(desc_list, &param);

	memset(&param, 0, sizeof(param_desc_t));
    sprintf(param.param_name, "%s", "do4");
    sprintf(param.param_desc, "%s", "DO通道4");
    sprintf(param.param_unit, "%s", "");
    param.param_id  = 10;
    param.update_threshold  = 1.0;
    param.param_type = PARAM_TYPE_ENUM;
    param.param_enum[0].value = 0;
	sprintf(param.param_enum[0].en_desc, "%s", "do4_low_voltage");
    sprintf(param.param_enum[0].cn_desc, "%s", "DO4低电平");
    param.param_enum[1].value = 1;
	sprintf(param.param_enum[1].en_desc, "%s", "do4_high_voltage");
    sprintf(param.param_enum[1].cn_desc, "%s", "DO4高电平");
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
static int calculate_do_status(cmd_t    *cmd,
                                   list_t   *param_desc_list,
                                   char     *data,
                                   int      data_len,
                                   list_t   *valid_value)
{
    int ret = -1;
    if (data_len != cmd->check_len) { //包长度校验
        printf("file %s, func %s, line %d, data len %d, check %d\n",
                __FILE__, __func__, __LINE__, data_len, cmd->check_len);
        return ret;
    }

    if ((data[0] != cmd->cmd_code[0]) && (data[1] != cmd->cmd_code[1])) {   //包头地址和命令校验
        printf("file %s, func %s, line %d\n", __FILE__, __func__, __LINE__);
        return ret;
    }

    unsigned short crc = create_crc16_code(data, cmd->check_len - 2);
    unsigned char crc_low = crc % 256;
    unsigned char crc_high = crc / 256;
    if (((unsigned char)data[data_len - 1] != crc_high)
            && ((unsigned char)data[data_len - 2] != crc_low)) { //CRC校验异常
        printf("file %s, func %s, line %d, crc low %x, crc high %x, %x, %x\n",
                __FILE__, __func__, __LINE__, crc_low, crc_high, data[data_len - 1], data[data_len - 2]);
        return ret;
    }

    param_desc_t *desc = NULL;
    param_value_t tmp_value;

	int i = 0;
	for (i = 0; i < 4; i++) {
		memset(&tmp_value, 0, sizeof(param_value_t));
        desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, i);
		tmp_value.param_id = desc->param_id;
		tmp_value.param_value = 0;
		tmp_value.enum_value = ((((unsigned char)data[3]&(0x01<<i)) > 0) ? 1 : 0);
		valid_value->push_back(valid_value, &tmp_value);
	}

    ret = 0;

    return ret;
}

static int calculate_di_status(cmd_t    *cmd,
                                   list_t   *param_desc_list,
                                   char     *data,
                                   int      data_len,
                                   list_t   *valid_value)
{
    int ret = -1;
    if (data_len != cmd->check_len) { //包长度校验
        printf("file %s, func %s, line %d, data len %d, check %d\n",
                __FILE__, __func__, __LINE__, data_len, cmd->check_len);
        return ret;
    }

    if ((data[0] != cmd->cmd_code[0]) && (data[1] != cmd->cmd_code[1])) {   //包头地址和命令校验
        printf("file %s, func %s, line %d\n", __FILE__, __func__, __LINE__);
        return ret;
    }

    unsigned short crc = create_crc16_code(data, cmd->check_len - 2);
    unsigned char crc_low = crc % 256;
    unsigned char crc_high = crc / 256;
    if (((unsigned char)data[data_len - 1] != crc_high)
            && ((unsigned char)data[data_len - 2] != crc_low)) { //CRC校验异常
        printf("file %s, func %s, line %d, crc low %x, crc high %x, %x, %x\n",
                __FILE__, __func__, __LINE__, crc_low, crc_high, data[data_len - 1], data[data_len - 2]);
        return ret;
    }

    param_desc_t *desc = NULL;
    param_value_t tmp_value;

	int i = 0;
	for (i = 0; i < 6; i++) {
        desc = (param_desc_t *)param_desc_list->get_index_value(param_desc_list, i);
		memset(&tmp_value, 0, sizeof(param_value_t));
		tmp_value.param_id = desc->param_id;
		tmp_value.param_value = 0;
		tmp_value.enum_value = ((((unsigned char)data[3]&(0x01<<i)) > 0) ? 1 : 0);
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
    case READ_DI_CMD:
        calculate_di_status(cmd, param_desc_list, data, len, valid_value);
        ret = 0;
        break;
	case READ_DO_CMD:
        calculate_do_status(cmd, param_desc_list, data, len, valid_value);
        ret = 0;
        break;
    default:
        break;
    }

    return ret;
}

static int get_environment_property(list_t *property_list)
{
    property_t property;
	memset(&property, 0, sizeof(property_t));
    property.cmd = get_di_status();
    property.param_desc = di_param_desc();
	property.last_param_value = NULL;
    property_list->push_back(property_list, &property);

	memset(&property, 0, sizeof(property_t));
    property.cmd = get_do_status();
    property.param_desc = do_param_desc();
	property.last_param_value = NULL;

    property_list->push_back(property_list, &property);

    return 0;
}

static void release_environment_property(list_t *property_list)
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

int external_io_register(list_t *protocol_list)
{
    protocol_t protocol;

    protocol.protocol_id = EXTERNAL_IO | OAO_860;
    strcpy(protocol.protocol_name, "oao-860k");
    strcpy(protocol.protocol_desc, "OAO-860IO传感器");

    protocol.get_property       = get_environment_property;
    protocol.calculate_data     = calculate_device_data;
    protocol.release_property   = release_environment_property;

    protocol_list->push_back(protocol_list, &protocol);

    return 0;
}
