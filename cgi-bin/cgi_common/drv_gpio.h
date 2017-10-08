#ifndef _DRV_GPIO_H_
#define _DRV_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   GPIO序号枚举定义
 */
enum GPIO_NAME {
    DIGITAL_IN_0 = 0,   /* DI1 */
    DIGITAL_IN_1,       /* DI2 */
    DIGITAL_IN_2,       /* DI3 */
    DIGITAL_IN_3,       /* DI4 */

    DIGITAL_OUT_0,      /* DO1 */
    DIGITAL_OUT_1,      /* DO2 */
    DIGITAL_OUT_2,      /* DO3 */
    DIGITAL_OUT_3,      /* DO4 */

    POFF_PIN,           /* 关闭电池电源引脚 */
    PD_INT_PIN,         /* 电池启动引脚 */

    WATCHDOG_PIN,       /* 外加的硬件看门狗喂狗引脚(已去除) */

    RS485_ENABLE,       /* RS485读\写切换使能引脚 */

    COM2_SELECTOR,      /* 串口2 RS485或RS232选通状态引脚 0：RS485；1：RS232*/
    COM2_RS485_ENABLE,  /* 串口2 RS485 读写切换使能引脚 */

    MAX_GPIO_NAME,
};

#define RET_OK              (0)
#define RET_ERR             (-1)
#define RET_ERR_PARAM       (-2)
#define RET_ERR_NOT_EXIST   (-3)

#define RET_ERR_OPEN        (-0x100)
#define RET_ERR_WRITE       (-0x101)
#define RET_ERR_READ        (-0x102)

/**
 * @brief   drv_gpio_open   打开指定序号GPIO
 * @param   gpio_name       GPIO序号(见GPIO_NAME枚举)
 * @return
 */
int drv_gpio_open(unsigned int gpio_name);

/**
 * @brief   drv_gpio_write  写GPIO指定引脚状态
 * @param   gpio_name       GPIO序号
 * @param   val             电平值(0, 1)
 */
int drv_gpio_write(unsigned int gpio_name, unsigned char val);

/**
 * @brief   drv_gpio_read   读GPIO指定引脚状态
 * @param   gpio_name       GPIO序号
 * @param   pval            电平值(0, 1)
 */
int drv_gpio_read(unsigned int gpio_name, unsigned char *pval);

/**
 * @brief   drv_gpio_close  关闭指定GPIO
 * @param   gpio_name       GPIO序号
 * @return
 */
int drv_gpio_close(unsigned int gpio_name);

#ifdef __cplusplus
extern "C" {
#endif

#endif
