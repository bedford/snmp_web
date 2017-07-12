#ifndef _DEVICE_ID_DEFINE_H_
#define _DEVICE_ID_DEFINE_H_

#include "list.h"

/* 设备类型掩码，主设备类型取第8到第16位 */
enum
{
    PROTOCOL_DEVICE_TYPE_MASK   = 0x0000FF00,   /* 设备主类型掩码 */
    PROTOCOL_SUB_TYPE_MASK      = 0x000000FF,   /* 设备子类型掩码 */
};

/* 主设备类型枚举 */
enum _DEVICE_MAIN_TYPE
{
    LOCAL_DI            = 0x00000001,   /* 设备本身的DI */
    TEMP_HUM_DEVICE     = 0x00000100,   /* 温湿度检测类设备 */
    WATER_LEAK_DETECT   = 0x00000200,   /* 漏水检测类 */
    AIR_CONDITION       = 0x00000300,   /* 空调检测类 */
    UPS                 = 0x00000400,   /* UPS */
    EXTERNAL_IO         = 0x00000500,   /* 外接IO传感器 */
};

/* UPS设备次类型枚举 */
enum _UPS_SUB_TYPE
{
    C_KS = 0x01,
};

/* 温湿度检测设备次类型枚举 */
enum _TEMP_HUM_SUB_TYPE
{
    OAO_210 = 0x01,
};

/* 外接IO传感器次类型枚举 */
enum _EXTERNAL_IO_SUB_TYPE
{
    OAO_860 = 0x01,
};

#endif
