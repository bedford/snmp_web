#ifndef _UPS_H_
#define _UPS_H_

#include "list.h"

enum
{
    C_KS = 0x01,
};

#ifdef __cplusplus
extern "C" {
#endif

int ups_register(list_t *protocol_list);

#ifdef __cplusplus
}
#endif

#endif

