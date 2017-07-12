#ifndef _ENVIROMENT_H_
#define _ENVIROMENT_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   environment_register 温湿度传感器注册接口
 *
 * @param   protocol_list
 *
 * @return
 */
int environment_register(list_t *protocol_list);

#ifdef __cplusplus
}
#endif

#endif

