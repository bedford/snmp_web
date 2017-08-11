#ifndef _LOGGER_H_
#define _LOGGER_H_

#define LOG_FILE	"/opt/data/log.txt"

#define LOG_FILE_SIZE   (1024 * 1024)

#define DEBUG   (0)
#define INFO    (1)
#define WARN    (2)
#define ERR     (4)

int snmp_log_init(int base_leve);

void snmp_log(int level, char *fmt, ...);

void clear_snmp_log(void);

#endif
