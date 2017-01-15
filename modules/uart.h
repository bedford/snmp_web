#ifndef _UART_H_
#define _UART_H_

typedef enum
{
    UART_BAUD_300   = 0,
    UART_BAUD_2400,
    UART_BAUD_4800,
    UART_BAUD_9600,
    UART_BAUD_19200,
    UART_BAUD_38400,
    UART_BAUD_57600,
    UART_BAUD_115200,
    UART_BAUD_MAX 
} uart_baud_t;

typedef enum
{
    UART_BITS_5 = 0,
    UART_BITS_6,
    UART_BITS_7,
    UART_BITS_8,
    UART_BITS_MAX    
} uart_data_bits_t;

typedef enum
{
    UART_STOP_1 = 0,
    UART_STOP_2,
    UART_STOP_MAX
} uart_stop_bits_t;

typedef enum
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
    UART_PARITY_MAX
} uart_parity_t;

typedef struct
{
    unsigned int        device_index;
    uart_baud_t         baud;
    uart_data_bits_t    bits;
    uart_parity_t       parity;
    uart_stop_bits_t    stops;
} uart_param_t;

typedef struct _uart uart_t;

typedef int (*_uart_open)(uart_t *thiz);
//typedef int (*_uart_read)(uart_t *thiz, char *buf, int timeout);
typedef int (*_uart_read)(uart_t *thiz, char *buf, int len);
typedef int (*_uart_write)(uart_t *thiz, const char *buf, int len);
typedef void (*_uart_destroy)(uart_t *thiz);

struct _uart
{
    _uart_open      open;
    _uart_read      read;
    _uart_write     write;
    _uart_destroy   destroy;

    char            priv[1];
};

uart_t *uart_create(uart_param_t *param);

#endif
