#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "logger.h"

#define LOG_BUFFER_SIZE		(2096)

static int base_level;
static const char *month[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static char *rb_data = NULL;
static pthread_mutex_t log_mutex;

static void put_to_log(char *buf, int size)
{
	int fd;
	struct stat ostat;

	pthread_mutex_lock( &log_mutex );
	fd = open(LOG_FILE, O_CREAT | O_APPEND | O_WRONLY, 0666);
	if (fd < 0)
		goto EXIT;
	if (fstat(fd, &ostat) < 0) {
		close(fd);
		goto EXIT;
	}

	if (ostat.st_size >= LOG_FILE_SIZE) {
		close(fd);
		sync();
		remove(LOG_FILE);
		sync();
        goto EXIT;
	}

	int ret = write(fd, buf, size);
	close(fd);
	sync();

EXIT:
	pthread_mutex_unlock(&log_mutex);
}

int snmp_log_init(int base_level)
{
	pthread_mutex_init(&log_mutex, NULL);
	base_level = base_level;

	rb_data = (char *)malloc(LOG_BUFFER_SIZE);

	return 0;
}

void snmp_log(int level, char *fmt, ...)
{
	va_list ap;
	char newfmt[512], line[1024];
	struct tm tm;
	time_t t;
    int len;

	va_start(ap, fmt);
	if (level < base_level) {
		va_end(ap);
		return;
	}
	time( &t );
	localtime_r( &t, &tm );
	sprintf(newfmt, "%s %d %02d:%02d:%02d %s",
			month[tm.tm_mon], tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			fmt);
	len = vsnprintf(line, sizeof(line), newfmt, ap);
	va_end(ap);

	put_to_log(line, len);

	return;
}

void clear_snmp_log(void)
{
	pthread_mutex_lock(&log_mutex);
	remove(LOG_FILE);
	pthread_mutex_unlock(&log_mutex);
}