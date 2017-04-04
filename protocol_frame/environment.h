#ifndef _ENVIROMENT_H_
#define _ENVIROMENT_H_

#include "list.h"

enum
{
    OAO_210 = 0x01,
};

#ifdef __cplusplus
extern "C" {
#endif

int environment_register(list_t *protocol_list);

#ifdef __cplusplus
}
#endif

#endif

