#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mib_create.h"

#define IO_DEVICE_NAME	"device_io"
#define MIB_NODE_NAME	"JT_Guard"

/**
 * @brief   fill_mib_header 生成MIB文件头
 *
 * @param   buffer
 * @param   offset
 * @param   spec_name
 * @param   spec_code
 *
 * @return
 */
unsigned int fill_mib_header(unsigned char *buffer, unsigned int offset, char *spec_name, int spec_code)
{
	unsigned char *tmp_buf = buffer + offset;
	sprintf(tmp_buf, "%s DEFINITIONS ::= BEGIN \n\n"
					 "IMPORTS\n"
					 "\tenterprises, IpAddress\n"
					 "\t\tFROM RFC1155-SMI\n"
					 "\tDisplayString\n"
					 "\t\tFROM RFC1213-MIB\n"
					 "\tOBJECT-TYPE\n"
					 "\t\tFROM RFC-1212\n"
					 "\tTRAP-TYPE\n"
					 "\t\tFROM RFC-1215;\n\n"
					 "%s OBJECT IDENTIFIER ::= { enterprises %d }\n\n",
					 MIB_NODE_NAME, spec_name, spec_code);

	offset += strlen(tmp_buf);

	return offset;
}

/**
 * @brief   fill_di_mib 生成DI输入的MIB信息
 *
 * @param   buffer
 * @param   offset
 * @param   di_param
 *
 * @return
 */
unsigned int fill_di_mib(unsigned char *buffer, unsigned int offset, di_param_t *di_param)
{
	unsigned char *tmp_buf = buffer + offset;
	sprintf(tmp_buf, "%s OBJECT-TYPE\n"
			"\tSYNTAX INTEGER {\n"
			"\t\t%s(0),\n"
			"\t\t%s(1)\n"
			"\t}\n"
			"\tACCESS read-only\n"
			"\tSTATUS mandatory\n"
			"\tDESCRIPTION\n"
			"\t\t\"%s\"\n"
			"\t::= { %s %d }\n\n",
			di_param->di_name,
			//di_param->low_desc, di_param->high_desc,
			"open", "close",
			di_param->di_name,
			//di_param->di_desc, di_param->device_name,
			IO_DEVICE_NAME, di_param->id);
	offset += strlen(tmp_buf);

	return offset;
}

/**
 * @brief   fill_do_mib 生成DO的MIB信息
 *
 * @param   buffer
 * @param   offset
 * @param   spec_name
 *
 * @return
 */
unsigned int fill_do_mib(unsigned char *buffer, unsigned int offset, char *spec_name)
{
	unsigned char *tmp_buf = buffer + offset;
    sprintf(tmp_buf, "%s\tOBJECT IDENTIFIER ::= { %s %d }\n\n", IO_DEVICE_NAME, spec_name, 0x01);
	offset += strlen(tmp_buf);

	int i = 0;
	for (i = 0; i < 4; i++) {
		tmp_buf = buffer + offset;
		sprintf(tmp_buf, "do%d OBJECT-TYPE\n"
				"\tSYNTAX INTEGER {\n"
				"\t\t%s(0),\n"
				"\t\t%s(1)\n"
				"\t}\n"
				"\tACCESS read-write\n"
				"\tSTATUS mandatory\n"
				"\tDESCRIPTION\n"
				"\t\t\"%s\"\n"
				"\t::= { %s %d }\n\n",
				i + 1, "low_level", "high_level",
				"DO status", IO_DEVICE_NAME, i + 5);
		offset += strlen(tmp_buf);
	}

	return offset;
}

/**
 * @brief   fill_protocol_mib 生成协议库的MIB信息
 *
 * @param   buffer
 * @param   offset
 * @param   protocol_id
 * @param   protocol_name
 * @param   spec_name
 *
 * @return
 */
unsigned int fill_protocol_mib(unsigned char *buffer, unsigned int offset,
		unsigned int protocol_id, char *protocol_name, char *spec_name)
{
    unsigned char *tmp_buf = buffer + offset;
    sprintf(tmp_buf, "%s\tOBJECT IDENTIFIER ::= { %s %d }\n\n",
			protocol_name, spec_name, protocol_id);
	offset += strlen(tmp_buf);

	return offset;
}

/**
 * @brief   fill_param_mib 根据协议库参数生成MIB文件信息
 *
 * @param   buffer
 * @param   offset
 * @param   protocol_name
 * @param   param_desc
 *
 * @return
 */
unsigned int fill_param_mib(unsigned char *buffer, unsigned int offset,
				char *protocol_name, param_desc_t *param_desc)
{
    unsigned char *tmp_buf = buffer + offset;

	if (param_desc->param_type == 1) {	//模拟量
		sprintf(tmp_buf, "%s OBJECT-TYPE\n"
				"\tSYNTAX DisplayString\n"
				"\tACCESS read-only\n"
				"\tSTATUS mandatory\n"
				"\tDESCRIPTION\n"
				"\t\t\"%s\"\n"
				"\t::= { %s %d }\n\n", param_desc->param_name,
				param_desc->param_name,
				protocol_name, param_desc->param_id);
	} else { //枚举量
		sprintf(tmp_buf, "%s OBJECT-TYPE\n"
				"\tSYNTAX INTEGER {\n"
				"\t\tclose(0),\n"
				"\t\topen(1)\n"
				"\t}\n"
				"\tACCESS read-only\n"
				"\tSTATUS mandatory\n"
				"\tDESCRIPTION\n"
				"\t\t\"%s\"\n"
				"\t::= { %s %d }\n\n", param_desc->param_name,
				//param_desc->param_enum[0].desc, param_desc->param_enum[1].desc,
				param_desc->param_name, protocol_name, param_desc->param_id);
	}

	offset += strlen(tmp_buf);

	return offset;
}

/**
 * @brief   fill_mib_tail 生成MIB文件尾
 *
 * @param   buffer
 * @param   offset
 *
 * @return
 */
unsigned int fill_mib_tail(unsigned char *buffer, unsigned int offset)
{
	unsigned char *tmp_buf = buffer + offset;
	sprintf(tmp_buf, "%s", "\nEND\n");

	return strlen(tmp_buf) + offset;
}
