#ifndef _DEVICE_ID_DEFINE_H_
#define _DEVICE_ID_DEFINE_H_

#include "list.h"

/* 设备类型掩码，主设备类型取第8到第16位 */
enum
{
    PROTOCOL_DEVICE_TYPE_MASK   = 0x00FF0000,   /* 设备主类型掩码 */
    PROTOCOL_SUB_TYPE_MASK      = 0x0000FF00,   /* 设备子类型掩码 */
    PROTOCOL_ADDR_MASK          = 0x0000000F,   /* 设备地址掩码   */
};

/* 主设备类型枚举 */
enum _DEVICE_MAIN_TYPE
{
    LOCAL_DI            = 0x00000001,   /* 设备本身的DI */
    TEMP_HUM_DEVICE     = 0x00010000,   /* 温湿度检测类设备 */
    WATER_LEAK_DETECT   = 0x00020000,   /* 漏水检测类 */
    AIR_CONDITION       = 0x00030000,   /* 空调检测类 */
    UPS                 = 0x00040000,   /* UPS */
    EXTERNAL_IO         = 0x00050000,   /* 外接IO传感器 */
};

/* UPS设备次类型枚举 */
enum _UPS_SUB_TYPE
{
    C_KS = 0x0100,
};

/* 温湿度检测设备次类型枚举 */
enum _TEMP_HUM_SUB_TYPE
{
    OAO_210_K20 = 0x0100,
    OAO_210_K25 = 0x0200,
};

/* 外接IO传感器次类型枚举 */
enum _EXTERNAL_IO_SUB_TYPE
{
    OAO_860     = 0x0100,
    OAO_816D    = 0x0200,
};

#endif
