#ifndef _COMMON_TYPE_H_
#define _COMMON_TYPE_H_

#define RS232_SHM_KEY       (6780)
#define RS485_SHM_KEY       (6781)
#define DI_SHM_KEY          (6782)

#define SHM_KEY_TRAP_ALARM  (6789)

#define MIN_BUF_SIZE        (32)
#define UART_DATA_MAX_NUM   (1000)

typedef struct {
	unsigned int	protocol_id;
	char			protocol_name[MIN_BUF_SIZE];
	char			protocol_desc[MIN_BUF_SIZE];
	unsigned int	param_id;
	char			param_name[MIN_BUF_SIZE];
	char			param_desc[MIN_BUF_SIZE];
	unsigned int	param_type;
	float           analog_value;
	char			param_unit[MIN_BUF_SIZE];
	unsigned int    enum_value;
	char			enum_en_desc[MIN_BUF_SIZE];
	char			enum_cn_desc[MIN_BUF_SIZE];
	unsigned int	alarm_type;
} element_data_t;

typedef struct {
    unsigned int    cnt;
    element_data_t  data[UART_DATA_MAX_NUM];
} uart_realdata_t;

typedef struct {
    unsigned int    cnt;
    element_data_t  data[4];
} di_realdata_t;

#endif
