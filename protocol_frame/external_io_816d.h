#ifndef _EXTERNAL_IO_816d_H_
#define _EXTERNAL_IO_816d_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   external_io_816d_register IO传感器注册接口
 *
 * @param   protocol_list
 *
 * @return
 */
int external_io_816d_register(list_t *protocol_list);

#ifdef __cplusplus
}
#endif

#endif
