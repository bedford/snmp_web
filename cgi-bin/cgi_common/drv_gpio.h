#ifndef _DRV_GPIO_H_
#define _DRV_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

enum GPIO_NAME {
    DIGITAL_IN_0 = 0,   /* DI_1 */
    DIGITAL_IN_1,       /* DI_2 */
    DIGITAL_IN_2,       /* DI_3 */
    DIGITAL_IN_3,       /* DI_4 */

    DIGITAL_OUT_0,      /* DO_1 */
    DIGITAL_OUT_1,      /* DO_2 */
    DIGITAL_OUT_2,      /* DO_3 */
    DIGITAL_OUT_3,      /* DO_4 */

    POFF_PIN,           /* 关闭电池供电引脚 */
    PD_INT_PIN,         /* 启用电源检测引脚 */

    WATCHDOG_PIN,       /* 外部看门狗控制引脚 */

    RS485_ENABLE,       /* RS485写使能引脚 */

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
 * @brief   drv_gpio_open 打开指定的IO口
 *
 * @param   gpio_name
 *
 * @return
 */
int drv_gpio_open(unsigned int gpio_name);


/**
 * @brief   drv_gpio_write 往指定的IO口写入值
 *
 * @param   gpio_name
 * @param   val
 *
 * @return
 */
int drv_gpio_write(unsigned int gpio_name, unsigned char val);


/**
 * @brief   drv_gpio_read 读取指定IO口的当前值
 *
 * @param   gpio_name
 * @param   pval
 *
 * @return
 */
int drv_gpio_read(unsigned int gpio_name, unsigned char *pval);


/**
 * @brief   drv_gpio_close 关闭指定的IO口
 *
 * @param   gpio_name
 *
 * @return
 */
int drv_gpio_close(unsigned int gpio_name);

#ifdef __cplusplus
extern "C" {
#endif

#endif
