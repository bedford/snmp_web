#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>

#include "uart.h"

static char *device_name[] = {
    //"/dev/ttySAC0",
    "/dev/ttyUSB0",
    "/dev/ttySAC1",
    "/dev/ttySAC2",
    "/dev/ttySAC3",
};

typedef struct {
    int             fd;
    uart_param_t    param;
} priv_info_t;

static void set_uart_param(priv_info_t *priv)
{
    struct termios options;
    tcgetattr(priv->fd, &options);

    options.c_cflag     = (CLOCAL | CREAD);
    options.c_cflag     &= ~CSIZE;
    options.c_lflag     = 0;    /* 非标准模式 */
    options.c_oflag     = 0;
    options.c_iflag     = IGNPAR;
    options.c_cc[VMIN]  = 1;
    options.c_cc[VTIME] = 0;

    /* 设置波特率 */
    speed_t speed;
    switch (priv->param.baud) {
    case UART_BAUD_300:
        speed = B300;
        break;
    case UART_BAUD_2400:
        speed = B2400;
        break;
    case UART_BAUD_4800:
        speed = B4800;
        break;
    case UART_BAUD_9600:
        speed = B9600;
        break;
    case UART_BAUD_19200:
        speed = B19200;
        break;
    case UART_BAUD_38400:
        speed = B38400;
        break;
    case UART_BAUD_57600:
        speed = B57600;
        break;
    case UART_BAUD_115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        break;
    }
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    /* 设置数据位 */
    switch (priv->param.bits) {
    case UART_BITS_5:
        options.c_cflag |= CS5;
        break;
    case UART_BITS_6:
        options.c_cflag |= CS6;
        break;
    case UART_BITS_7:
        options.c_cflag |= CS7;
        break;
    case UART_BITS_8:
    default:
        options.c_cflag |= CS8;
        break;
    }

    /* 设置校验方式 */
    switch (priv->param.parity) {
    case UART_PARITY_ODD:   /* 奇校验 */
        options.c_cflag |= (PARENB | PARODD);
        options.c_iflag |= (INPCK | ISTRIP);
        break;
    case UART_PARITY_EVEN:  /* 偶校验 */
        options.c_cflag &= ~PARODD;
        options.c_cflag |= PARENB;
        options.c_iflag |= (INPCK | ISTRIP);
        break;
    case UART_PARITY_NONE:
    default:
        options.c_cflag &= ~PARENB;
        break;
    }

    /* 设置停止位 */
    switch (priv->param.stops) {
    case UART_STOP_2:
        options.c_cflag |= CSTOPB;
        break;
    case UART_STOP_1:
    default:
        options.c_cflag &=~CSTOPB;
        break;
    }

    tcflush(priv->fd, TCIFLUSH);
    tcsetattr(priv->fd, TCSANOW, &options);
}

static int uart_open(uart_t *thiz)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv;
    char *device = device_name[priv->param.device_index];
    //priv->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    priv->fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (priv->fd < 0) {
        return -1;
    }

    tcflush(priv->fd, TCIOFLUSH);

    set_uart_param(priv);

    return 0;
}

static int safe_read(int fd, char *buf, int len)
{
    int left = len;
    int read_num = 0;
    char *ptr = buf;

    while (left > 0) {
        read_num = read(fd, ptr, left);
        if (read_num < 0) {
            return -1;
        } else if (read_num == 0) {
            break;
        }

        left    -= read_num;
        ptr     += read_num;
    }

    return (len - left);
}

static int uart_read(uart_t *thiz, char *buf, int len)
{
    if (thiz == NULL) {
        return -1;
    }

    priv_info_t *priv = (priv_info_t *)thiz->priv;

    fd_set read_fds;
    struct timeval time;

    FD_ZERO(&read_fds);
    FD_SET(priv->fd, &read_fds);

    time.tv_sec     = 2;
    time.tv_usec    = 0;

    int ret = select(priv->fd + 1, &read_fds, NULL, NULL, &time);
    switch(ret) {
    case -1:
        printf("select error\n");
        break;
    case 0:
        printf("timeout\n");
        break;
    default:
        ret = safe_read(priv->fd, buf, len);
        break;
    }

    return ret;
}

static int uart_write(uart_t *thiz, const char *buf, int len)
{
    int bytes = 0;
    priv_info_t *priv = (priv_info_t *)thiz->priv;

    bytes = write(priv->fd, buf, len);

    return bytes;
}

static void uart_destroy(uart_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
        tcflush(priv->fd, TCIOFLUSH);
        usleep(100000);
        close(priv->fd);

        memset(thiz, 0, sizeof(uart_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

uart_t *uart_create(uart_param_t *param)
{
    uart_t *thiz = calloc(1, sizeof(uart_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
        thiz->open      = uart_open;
        thiz->read      = uart_read;
        thiz->write     = uart_write;
        thiz->destroy   = uart_destroy;

        priv_info_t *priv = (priv_info_t *)thiz->priv;
        priv->fd = -1;
        priv->param.device_index = param->device_index;
        priv->param.baud         = param->baud;
        priv->param.bits         = param->bits;
        priv->param.stops        = param->stops;
        priv->param.parity       = param->parity;
    }

    return thiz;
}
