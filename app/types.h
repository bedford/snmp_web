#ifndef _TYPES_H_
#define _TYPES_H_

#define MIN_BUF_SIZE	(32)
#define MED_BUF_SIZE	(64)
#define MAX_BUF_SIZE	(512)

enum
{
	NORMAL			= 0x0,	/* 正常 */
	UP_ALARM_ON		= 0x1,	/* 上限报警 */
	LOW_ALARM_ON	= 0x2,	/* 下限报警 */
	LEVEL_ALARM_ON	= 0x4,	/* 枚举量状态异常报警 */
	UP_ALARM_OFF	= 0x11,
	LOW_ALARM_OFF	= 0x12,
	LEVEL_ALARM_OFF	= 0x14
};

/* 入库消息结构 */
typedef struct
{
	char buf[MAX_BUF_SIZE];
} msg_t;

enum {
	ALARM_RAISE = 1,
	ALARM_DISCARD
};

/* 报警消息结构 */
typedef struct
{
	unsigned int	protocol_id;
	char			protocol_name[MIN_BUF_SIZE];
	unsigned int	param_id;
	char			param_name[MIN_BUF_SIZE];
	char			param_unit[MIN_BUF_SIZE];
	unsigned int	param_type;
	float           param_value;
	unsigned int    enum_value;
	char			enum_desc[MIN_BUF_SIZE];
	char			alarm_desc[MED_BUF_SIZE];
	unsigned int	alarm_type;
	unsigned int	send_times;
	char			reserved[4];
} alarm_msg_t;

typedef struct {
	char	smtp_server[32];
	char	email_addr[32];
	char	password[32];
} email_server_t;

typedef struct {
	unsigned char status[3];
	unsigned char beep_enable;
} do_param_t;

#endif
