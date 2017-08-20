#ifndef _LOGGER_H_
#define _LOGGER_H_

#define LOG_FILE	"/opt/data/log.txt"

#define LOG_FILE_SIZE   (1024 * 1024)

#define DEBUG   (0)
#define INFO    (1)
#define WARN    (2)
#define ERR     (4)

/**
 * @brief   snmp_log_init   初始化日志记录
 * @param   base_leve       基准记录级别
 * @return
 */
int snmp_log_init(int base_leve);

/**
 * @brief   snmp_log        写日志
 * @param   level           日志级别
 * @param   fmt
 * @param   ...
 */
void snmp_log(int level, char *fmt, ...);

/**
 * @brief   clear_snmp_log 手动清除日志文件
 */
void clear_snmp_log(void);

#endif
