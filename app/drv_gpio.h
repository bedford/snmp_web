#ifndef _DRV_GPIO_H_
#define _DRV_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

enum GPIO_NAME {
    DIGITAL_IN_0 = 0,
    DIGITAL_IN_1,
    DIGITAL_IN_2,
    DIGITAL_IN_3,

    DIGITAL_OUT_0,
    DIGITAL_OUT_1,
    DIGITAL_OUT_2,
    DIGITAL_OUT_3,

    POFF_PIN,
    PD_INT_PIN,

    WATCHDOG_PIN,

    RS485_ENABLE,

    MAX_GPIO_NAME,
};

#define RET_OK              (0)
#define RET_ERR             (-1)
#define RET_ERR_PARAM       (-2)
#define RET_ERR_NOT_EXIST   (-3)

#define RET_ERR_OPEN        (-0x100)
#define RET_ERR_WRITE       (-0x101)
#define RET_ERR_READ        (-0x102)

int drv_gpio_open(unsigned int gpio_name);
int drv_gpio_write(unsigned int gpio_name, unsigned char val);
int drv_gpio_read(unsigned int gpio_name, unsigned char *pval);
int drv_gpio_close(unsigned int gpio_name);

#ifdef __cplusplus
extern "C" {
#endif

#endif
