#ifndef _PROTOCOL_INTERFACES_H_
#define _PROTOCOL_INTERFACES_H_

#include "protocol_types.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROTOCOL_LEN	(128)
#define MIN_PROTOCOL_LEN	(32)

typedef struct
{
    cmd_t   cmd;            	/* 读数据指令 */
    list_t  *param_desc;		/* 该指令对应的返回参数描述信息列表 */
	list_t  *last_param_value;	/* 上一次的参数值列表 */
	struct timeval last_record_time;
} property_t;

typedef int (*_get_property)(list_t *property_list);
typedef int (*_calculate_data)(property_t *property, char *data, int len, list_t *valid_value);
typedef void (*_release_property)(list_t *property_list);

/**
 * @brief 接口协议类声明
 */
typedef struct
{
	unsigned int 	    protocol_id;
	unsigned char	    protocol_name[MIN_PROTOCOL_LEN];
	unsigned char	    protocol_desc[MAX_PROTOCOL_LEN];
	unsigned char	    device_brand[MIN_PROTOCOL_LEN];

	_get_property       get_property;
    _release_property   release_property;
	_calculate_data     calculate_data;
} protocol_t;

/**
 * [init_protocol_lib 初始化协议库]
 * @param  list [协议库支持的协议列表]
 * @return      [是否初始化成功]
 * @retval 0: 成功
 *         1：失败
 */
int init_protocol_lib(list_t *list);

/**
 * [get_protocol_handle 获取协议库操作句柄]
 * @param  list        [协议库列表]
 * @param  protocol_id [指定协议库编号]
 * @return             [协议库操作句柄指针]
 */
protocol_t *get_protocol_handle(list_t *list, unsigned int protocol_id);

/**
 * [deinit_protocol_lib 释放协议库使用的资源]
 * @param list [协议库列表]
 */
void deinit_protocol_lib(list_t *list);

#ifdef __cplusplus
}
#endif

#endif
