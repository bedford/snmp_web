/*
 * 应用层通过sys文件接口操作IO端口
 *
 * 参阅以下的文档:
 * [1]https://www.kernel.org/doc/Documentation/gpio/sysfs.txt
 * [2]http://blog.csdn.net/mirkerson/article/details/8464231
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "drv_gpio.h"

#define DRV_GPIO_EXPORT_NAME    "/sys/class/gpio/export"
#define DRV_GPIO_DIR_NAME       "/sys/class/gpio/gpio%d/direction"
#define DRV_GPIO_EDGE_NAME      "/sys/class/gpio/gpio%d/edge"
#define DRV_GPIO_VAL_NAME       "/sys/class/gpio/gpio%d/value"
#define DRV_GPIO_UNEXPORT_NAME  "/sys/class/gpio/unexport"

typedef struct
{
    int   fd;         /* IO文件描述符 */
    unsigned int  port;       /* 端口,如GPA,GPB,GPC, 则GPA为0 */
    unsigned int  pin;        /* 对应端口的顺序号,从0~31 */
    unsigned char   direction;  /* 0:out, 1:in */
    unsigned char   edge;       /* 0:无,电平触平; 1:上升沿; 2:下降沿; 3:兼用上升沿和下降沿两种 */
    unsigned char   cur_val;    /* 当前值 */
    char    *name;      /* GPIO的名称 */
} gpio_info_t;

static gpio_info_t gpio_info[] = {
    /* 干接点输入 */
    {   -1,     6,      5,      1,      0,      0,      "GPG5"  },  /* GPG5 */
    {   -1,     6,      4,      1,      0,      0,      "GPG4"  },  /* GPG4 */
    {   -1,     5,      4,      1,      0,      0,      "GPF4"  },  /* GPF4 */
    {   -1,     5,      3,      1,      0,      0,      "GPF3"  },  /* GPF3 */
    /* 干接点输出 */
    {   -1,     5,      2,      0,      0,      0,      "GPF2"  },  /* GPF2 */
    {   -1,     5,      1,      0,      0,      0,      "GPF1"  },  /* GPF1 */
    {   -1,     5,      0,      0,      0,      0,      "GPF0"  },  /* GPF0 */
    {   -1,     1,      2,      0,      0,      0,      "GPF6"  },  /* GPF6 */
    /* 电源管理 */
    {   -1,     6,      3,      0,      0,      0,      "GPG3"  },  /* GPG3 */
    {   -1,     5,      5,      1,      0,      0,      "GPF5"  },  /* GPF5 */
    /* 看门狗喂狗引脚 */
    {   -1,     2,      14,     0,      0,      0,      "GPC14" },  /* GPC14 */
    /* RS485使能端口 */
    {   -1,     3,      11,     0,      0,      0,      "GPD11" },  /* GPD11 */
};

int drv_gpio_open(enum GPIO_NAME gpio_name)
{
    int   ret = RET_OK;
    int   tmp_fd  = -1;
    int   pin_num = 0;
    char    fname[64] = {0};

    if (gpio_name >= MAX_GPIO_NAME) {
        return RET_ERR_PARAM;
    }

    if (gpio_info[gpio_name].fd >= 0) {
        return ret; 
    }

    printf("open export\n");
    tmp_fd = open(DRV_GPIO_EXPORT_NAME, O_WRONLY);
    if (tmp_fd < 0) {
        printf("export file open %s err:%s\n",
                gpio_info[gpio_name].name, strerror(errno));
        return RET_ERR_OPEN;
    }

    printf("write export\n");
    pin_num = gpio_info[gpio_name].port * 32 + gpio_info[gpio_name].pin;
    sprintf(fname, "%d", pin_num);
    ret = write(tmp_fd, fname, strlen(fname));
    fsync(tmp_fd);
    close(tmp_fd);

    memset(fname, 0, sizeof(fname));
    sprintf(fname, DRV_GPIO_DIR_NAME, pin_num);
    tmp_fd = open(fname, O_RDWR);
    if (tmp_fd < 0) {
        printf("export file open %s err:%s\n",
                gpio_info[gpio_name].name, strerror(errno));
        return RET_ERR_OPEN;
    }

    /* 设置GPIO为输入或输出 */
    if (gpio_info[gpio_name].direction == 1) {
        strcpy(fname, "in");
    } else {
        strcpy(fname, "out");
    }

    ret = write(tmp_fd, fname, strlen(fname));
    if (ret != strlen(fname)) {
        printf("gpio %s gpio_dir file:%s, write err:%s\n",
                gpio_info[gpio_name].name, fname, strerror(errno));
        return RET_ERR_WRITE;
    }
    fsync(tmp_fd);
    close(tmp_fd);

    if (gpio_info[gpio_name].edge != 0) {
        memset(fname, 0, sizeof(fname));
        sprintf(fname, DRV_GPIO_EDGE_NAME, gpio_name);
        tmp_fd = open(fname, O_RDWR );
        if (tmp_fd < 0) {
            return RET_ERR_OPEN;
        }

        switch (gpio_info[gpio_name].edge) {
            case 0x00:
                strcpy(fname, "none");
                break;
            case 0x01:
                strcpy(fname, "rising");
                break;
            case 0x02:
                strcpy(fname, "falling");
                break;
            case 0x03:
                strcpy(fname, "both");
                break;
            default:
                close(tmp_fd);
                return RET_ERR_PARAM;
                break;
        }
        ret = write(tmp_fd, fname, strlen(fname));
        if (ret != strlen(fname)) {
            printf("gpio %s, gpio_edge file:%s, write err:%s\n",
                    gpio_info[gpio_name].name, fname, strerror(errno));
            return RET_ERR_WRITE;
        }
        fsync(tmp_fd);
        close(tmp_fd);
    }

    memset(fname, 0, sizeof(fname));
    sprintf(fname, DRV_GPIO_VAL_NAME, pin_num);
    tmp_fd = open(fname, O_RDWR);
    if (tmp_fd < 0) {
        printf("gpio %s, gpio_value file:%s, open err:%s\n",
                gpio_info[gpio_name].name, fname, strerror(errno));
        return RET_ERR_OPEN;
    }

    gpio_info[gpio_name].fd = tmp_fd;
    return ret;
}

int drv_gpio_write(unsigned int gpio_name, unsigned char val)
{
    int ret = RET_OK;
    char  asc_val = val + 0x30;

    if (gpio_name >= MAX_GPIO_NAME) {
        return RET_ERR_PARAM;
    }

    if (gpio_info[gpio_name].fd < 0) {
        return RET_ERR_NOT_EXIST;
    }

    if (gpio_info[gpio_name].direction == 1) { /* 如果配置成输入IO,则不能写入 */
        return RET_ERR;
    }

    if (gpio_info[gpio_name].cur_val == val) {
        return RET_OK;
    }

    ret = write(gpio_info[gpio_name].fd, &asc_val, 1);
    if (ret < 0) {
        printf("gpio_write %s err:%s\n", gpio_info[gpio_name].name, strerror(errno));
        ret = errno;
        return ret;
    }
    fsync(gpio_info[gpio_name].fd);
    gpio_info[gpio_name].cur_val = val;

    return RET_OK;
}

int drv_gpio_read(unsigned int gpio_name, unsigned char *pval)
{
    int   ret = RET_OK;

    if (gpio_name >= MAX_GPIO_NAME) {
        return RET_ERR_PARAM;
    }

    if (gpio_info[gpio_name].fd < 0) {
        return RET_ERR_NOT_EXIST;
    }

    if (gpio_info[gpio_name].direction == 0) {
        return RET_ERR;
    }

    ret = lseek(gpio_info[gpio_name].fd, 0, SEEK_SET);
    if (ret < 0) {
        ret = errno;
        printf("gpio_read %s, lseek err[%s]...\n",
                gpio_info[gpio_name].name, strerror(errno));
    }

    ret = read(gpio_info[gpio_name].fd, pval, 1);
    if (ret < 0) {
        ret = errno;
        printf("gpio_read %s, err:%s\n", gpio_info[gpio_name].name, strerror(errno));
        return ret;
    }

    *pval = *pval - 0x30;

    return RET_OK;
}

int drv_gpio_close(unsigned int gpio_name)
{
    int   ret = RET_OK;
    int   tmp_fd = -1;
    int   pin_num = 0;
    char    fname[32] = {0};

    if (gpio_name > MAX_GPIO_NAME) {
        return RET_ERR_PARAM;
    }

    if (gpio_info[gpio_name].fd < 0) {
        return ret;
    }

    close(gpio_info[gpio_name].fd);
    gpio_info[gpio_name].fd = -1;

    tmp_fd = open(DRV_GPIO_UNEXPORT_NAME, O_WRONLY);
    if (tmp_fd < 0) {
        printf("unexport file open err:%s", strerror(errno));
        return RET_ERR_OPEN;
    }
    pin_num = gpio_info[gpio_name].port * 32 + gpio_info[gpio_name].pin;
    sprintf(fname, "%d", pin_num);
    ret = write(tmp_fd, fname, strlen(fname));
    fsync(tmp_fd);
    close(tmp_fd);

    return ret;
}

