#ifndef _PROTOCOL_TYPES_H_
#define _PROTOCOL_TYPES_H_

#include <sys/time.h>

/* 命令字符长度 */
#define MAX_CMD_LEN (128)
#define MIN_CMD_LEN (8)

/**
 * @brief 数据格式类型
 */
enum
{
    CMD_FORMAT_ASCII    = 1,    /* ASCII格式 */
    CMD_FORMAT_HEX      = 2,    /* HEX格式   */
};

/**
 * @brief 数据校验方式
 */
enum
{
    VERIFY_TYPE_NONE    = 1,	/* 无数据校验 */
    VERIFY_TYPE_CRC,			/* CRC校验 */
};

/**
 * @brief 参数类型
 */
enum
{
    PARAM_TYPE_ANALOG   = 1,	/* 模拟量 */
    PARAM_TYPE_ENUM,			/* 枚举量 */
};

/**
 * @brief 指令类型声明
 */
typedef struct
{
    unsigned int    cmd_id;					/* 指令编号 */
    unsigned char   cmd_name[MAX_CMD_LEN];	/* 指令名称及说明 */
    unsigned char   cmd_code[MAX_CMD_LEN];	/* 指令对应的数据 */
    unsigned char   end_code[MIN_CMD_LEN];	/* 指令结束符 */
    unsigned int    cmd_len;                /* 指令数据长度 */
    unsigned int    end_len;                /* 结束符长度 */
    unsigned int    cmd_format;				/* 指令数据格式类型 */
    unsigned int    read_timeout;			/* 读取返回数据超时时间 */
    unsigned int    check_len;				/* 指令返回数据长度 */
    unsigned int    verify_type;			/* 指令返回数据校验方式 */
    unsigned int    record_interval;		/* 入库最大时间间隔 */
} cmd_t;

enum
{
	PROTOCOL_DEVICE_TYPE_MASK	= 0x0000FF00,
	PROTOCOL_SUB_TYPE_MASK		= 0x000000FF,
};

enum
{
	LOCAL_DI			= 0x00000001,	/* 设备本身的DI */
    TEMP_HUM_DEVICE     = 0x00000100,   /* 温湿度检测类设备 */
    WATER_LEAK_DETECT   = 0x00000200,   /* 漏水检测类 */
    AIR_CONDITION       = 0x00000300,   /* 空调检测类 */
    UPS                 = 0x00000400,   /* UPS */
};


#define MAX_PARAM_LEN	(128)
#define MIN_PARAM_LEN	(32)
#define PARAM_ENUM_NUM	(2)

typedef struct
{
	unsigned char	en_desc[MIN_PARAM_LEN];	/* 枚举量英文描述 */
    unsigned char   cn_desc[MIN_PARAM_LEN];	/* 枚举量中文描述 */
    unsigned int    value;					/* 枚举量值 */
} param_enum_t;

typedef struct
{
    unsigned int    param_id;                   /* 参数编号 */
    unsigned char   param_name[MIN_PARAM_LEN];	/* 参数名称 */
    unsigned char   param_unit[MIN_PARAM_LEN];	/* 参数单位 */
    unsigned char   param_desc[MIN_PARAM_LEN];	/* 参数描述 */

    float           up_limit;					/* 上限 */
    float           up_free;					/* 上限解除 */
    float           low_limit;					/* 下限 */
    float           low_free;					/* 下限解除 */

    unsigned int    param_type;					/* 参数类型(模拟量还是数字枚举量?) */

    unsigned int    alarm_enable;				/* 是否启用报警 */
    float           update_threshold;           /* 模拟量入库阈值(数字枚举量状态切换时自动入库一次) */

	unsigned int	enum_alarm_value;			/* 枚举量报警值 */
    param_enum_t    param_enum[PARAM_ENUM_NUM]; /* 数字枚举量 */
} param_desc_t;

typedef struct
{
    unsigned int    param_id;
    float           param_value;
    unsigned int    enum_value;
	unsigned int	status;		//0, 正常; 1,上限报警; 2,下限报警; 4,阈值报警
} param_value_t;


#endif
