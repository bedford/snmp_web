#ifndef _TYPES_H_
#define _TYPES_H_

enum
{
	REAL_DATA = 1,		/* 实时数据 */
	RECORD_DATA,		/* 历史数据 */
};

typedef struct
{
	unsigned int	msg_type;
	char			buf[512];
	char			reserved[4];
} msg_t;

#endif
