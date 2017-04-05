#ifndef _MIB_CREATE_H_
#define _MIB_CREATE_H_

#define MAX_PARAM_LEN	(128)
#define MIN_PARAM_LEN	(32)
#define PARAM_ENUM_NUM	(2)

typedef struct
{
    unsigned char   desc[MIN_PARAM_LEN];	/* 枚举量描述 */
    unsigned int    value;					/* 枚举量值 */
} param_enum_t;

typedef struct
{
    unsigned int    param_id;                   /* 参数编号 */
    unsigned char   param_name[MIN_PARAM_LEN];	/* 参数名称 */
    unsigned char   param_unit[MIN_PARAM_LEN];	/* 参数单位 */
    unsigned char   param_desc[MAX_PARAM_LEN];	/* 参数描述 */

    float           up_limit;					/* 上限 */
    float           up_free;					/* 上限解除 */
    float           low_limit;					/* 下限 */
    float           low_free;					/* 下限解除 */
    float           update_threshold;           /* 模拟量入库阈值(数字枚举量状态切换时自动入库一次) */
    unsigned int	param_type;					/* 参数类型(模拟量还是数字枚举量?) */

	unsigned int	enum_alarm_value;			/* 枚举量报警值 */
    param_enum_t    param_enum[PARAM_ENUM_NUM]; /* 数字枚举量 */
} param_desc_t;

typedef struct {
	char			di_name[MIN_PARAM_LEN];		/* DI名称 */
	char			di_desc[MIN_PARAM_LEN];		/* DI描述 */
	char			device_name[MIN_PARAM_LEN];
	char			low_desc[MIN_PARAM_LEN]; 	/* 低电平描述 */
	char			high_desc[MIN_PARAM_LEN]; 	/* 高电平描述 */
	unsigned int	id;							/* DI编号 0, 1, 2, 3*/
	unsigned int	enable;						/* 是否使能 */
	unsigned int	alarm_level;				/* 报警电平 */
	unsigned int	alarm_method;				/* 报警方式 */
} di_param_t;

unsigned int fill_mib_header(unsigned char *buffer, unsigned int offset);

unsigned int fill_di_mib(unsigned char *buffer, unsigned int offset, di_param_t *di_param);

unsigned int fill_do_mib(unsigned char *buffer, unsigned int offset);

unsigned int fill_protocol_mib(unsigned char *buffer, unsigned int offset,
		unsigned int protocol_id, char *protocol_name);

unsigned int fill_param_mib(unsigned char *buffer, unsigned int offset,
				char *protocol_name, param_desc_t *param_desc);

unsigned int fill_mib_tail(unsigned char *buffer, unsigned int offset);

#endif
