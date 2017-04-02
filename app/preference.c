#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>

#include "preference.h"
#include "iniparser.h"

typedef struct {
	dictionary		*dic;
	pthread_mutex_t	lock_mutex;
	time_t			modify_time;

	int				init_flag;
	int 			rs232_alarm_flag;
	int				rs485_alarm_flag;
	int				di_alarm_flag;

	int				sms_contact_flag;
	int				email_contact_flag;

	int				send_times;
	int				send_interval;

	email_server_t	email_server_param;
} priv_info_t;

static void write_profile(dictionary    *dic,
                          const char    *section,
                          const char    *key,
                          const char    *value)
{
    if (!iniparser_find_entry(dic, section)) {
            printf("dump section %s\n", section);
            iniparser_set(dic, section, NULL);
    }

    char tmp[256] = {0};
    sprintf(tmp, "%s:%s", section, key);
    iniparser_set(dic, tmp, value);
}

static void dump_profile(dictionary *dic, char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {
        iniparser_dump_ini(dic, fp);
        fclose(fp);
        fp = NULL;
    }
}

static int load_system_param(priv_info_t *priv)
{
	priv->init_flag = iniparser_getint(priv->dic, "SYSTEM:init_flag", 1);

	priv->rs232_alarm_flag = iniparser_getint(priv->dic, "ALARM:rs232_alarm_flag", 1);
	priv->rs485_alarm_flag = iniparser_getint(priv->dic, "ALARM:rs485_alarm_flag", 1);
	priv->di_alarm_flag = iniparser_getint(priv->dic, "ALARM:di_alarm_flag", 1);

	priv->sms_contact_flag = iniparser_getint(priv->dic, "ALARM:sms_contact_flag", 1);
	priv->email_contact_flag = iniparser_getint(priv->dic, "ALARM:email_contact_flag", 1);

	return 0;
}

static void load_sms_send_param(priv_info_t *priv)
{
	priv->send_times = iniparser_getint(priv->dic, "SMS:send_times", 3);
	priv->send_interval = iniparser_getint(priv->dic, "SMS:send_interval", 1);
}

static void load_email_server_param(priv_info_t *priv)
{
	strncpy(priv->email_server_param.smtp_server,
		iniparser_getstring(priv->dic, "EMAIL:smtp_server", "smtp.163.com"),
		sizeof(priv->email_server_param.smtp_server));

	strncpy(priv->email_server_param.email_addr,
		iniparser_getstring(priv->dic, "EMAIL:email_addr", ""),
		sizeof(priv->email_server_param.email_addr));

	strncpy(priv->email_server_param.password,
		iniparser_getstring(priv->dic, "EMAIL:password", ""),
		sizeof(priv->email_server_param.password));
}

static email_server_t get_email_server_preference(preference_t *thiz)
{
    email_server_t email_server_param;
    memset(&email_server_param, 0, sizeof(email_server_t));
    if (thiz != NULL) {
		priv_info_t *priv = (priv_info_t *)thiz->priv;
		email_server_param = priv->email_server_param;
    }

    return email_server_param;
}

static int get_send_times(preference_t *thiz)
{
	int send_times = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		send_times = priv->send_times;
	}

	return send_times;
}

static int get_send_interval(preference_t *thiz)
{
	int send_interval = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		send_interval = priv->send_interval;
	}

	return send_interval;
}

static void set_init_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info_t *priv =  (priv_info_t *)thiz->priv;
	priv->init_flag = flag;
    sprintf(tmp, "%d", priv->init_flag);
    write_profile(priv->dic, "SYSTEM", "init_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);

    printf("setting init_flag %d\n", priv->init_flag);
}

static int get_init_flag_preference(preference_t *thiz)
{
	int init_flag = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		init_flag = priv->init_flag;
	}

	return init_flag;
}

static void set_rs232_alarm_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info_t *priv =  (priv_info_t *)thiz->priv;
	priv->rs232_alarm_flag = flag;
    sprintf(tmp, "%d", priv->rs232_alarm_flag);
    write_profile(priv->dic, "ALARM", "rs232_alarm_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);
}

static int get_rs232_alarm_flag_preference(preference_t *thiz)
{
	int rs232_alarm_flag = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		rs232_alarm_flag = priv->rs232_alarm_flag;
	}

	return rs232_alarm_flag;
}

static void set_rs485_alarm_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info_t *priv =  (priv_info_t *)thiz->priv;
	priv->rs485_alarm_flag = flag;
    sprintf(tmp, "%d", priv->rs485_alarm_flag);
    write_profile(priv->dic, "ALARM", "rs485_alarm_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);

    printf("setting init_flag %d\n", priv->rs485_alarm_flag);
}

static int get_rs485_alarm_flag_preference(preference_t *thiz)
{
	int rs485_alarm_flag = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		rs485_alarm_flag = priv->rs485_alarm_flag;
	}

	return rs485_alarm_flag;
}

static void set_di_alarm_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info_t *priv =  (priv_info_t *)thiz->priv;
	priv->di_alarm_flag = flag;
    sprintf(tmp, "%d", priv->di_alarm_flag);
    write_profile(priv->dic, "ALARM", "di_alarm_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);

    printf("setting init_flag %d\n", priv->di_alarm_flag);
}

static int get_di_alarm_flag_preference(preference_t *thiz)
{
	int di_alarm_flag = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		di_alarm_flag = priv->di_alarm_flag;
	}

	return di_alarm_flag;
}

static void set_sms_contact_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info_t *priv =  (priv_info_t *)thiz->priv;
	priv->sms_contact_flag = flag;
    sprintf(tmp, "%d", priv->sms_contact_flag);
    write_profile(priv->dic, "ALARM", "sms_contact_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);

    printf("setting sms_contact_flag %d\n", priv->sms_contact_flag);
}

static int get_sms_contact_flag_preference(preference_t *thiz)
{
	int sms_contact_flag = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		sms_contact_flag = priv->sms_contact_flag;
	}

	return sms_contact_flag;
}

static void set_email_contact_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info_t *priv =  (priv_info_t *)thiz->priv;
	priv->email_contact_flag = flag;
    sprintf(tmp, "%d", priv->email_contact_flag);
    write_profile(priv->dic, "ALARM", "email_contact_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);

    printf("setting email_contact_flag %d\n", priv->email_contact_flag);
}

static int get_email_contact_flag_preference(preference_t *thiz)
{
	int email_contact_flag = 0;

	if (thiz != 0) {
		priv_info_t *priv =  (priv_info_t *)thiz->priv;
		email_contact_flag = priv->email_contact_flag;
	}

	return email_contact_flag;
}

static void load_preference(priv_info_t *priv)
{
	struct stat st;
	lstat(INI_FILE_NAME, &st);
	priv->modify_time = st.st_mtime;

	load_system_param(priv);
	load_sms_send_param(priv);
}

static int preference_reload(preference_t *thiz)
{
    int ret = -1;
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
        struct stat ini_st;
        lstat(INI_FILE_NAME, &ini_st);
        if (ini_st.st_mtime == priv->modify_time) {
                return 0;
        }

		printf("############# reload preference ##########\n");
        if (priv->dic) {
                iniparser_freedict(priv->dic);
                priv->dic = NULL;
        }

        do {
                if (pthread_mutex_trylock(&priv->lock_mutex) != 0) {
                        break;
                }

                priv->dic = iniparser_load(INI_FILE_NAME);
                load_preference(priv);
        } while(0);

        pthread_mutex_unlock(&priv->lock_mutex);
    }

    return ret;
}

static void preference_destroy(preference_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv =  (priv_info_t *)thiz->priv;
        if (priv->dic) {
			iniparser_freedict(priv->dic);
			priv->dic = NULL;
        }

        pthread_mutex_destroy(&priv->lock_mutex);

        memset(thiz, 0, sizeof(preference_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

preference_t *preference_create(void)
{
    preference_t *thiz = NULL;
    thiz = (preference_t *)calloc(1, sizeof(preference_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
        thiz->destroy		= preference_destroy;
        thiz->reload		= preference_reload;

        thiz->get_init_flag	= get_init_flag_preference;
        thiz->set_init_flag	= set_init_flag_preference;

		thiz->get_rs232_alarm_flag	= get_rs232_alarm_flag_preference;
		thiz->get_rs485_alarm_flag	= get_rs485_alarm_flag_preference;
		thiz->get_di_alarm_flag 	= get_di_alarm_flag_preference;

		thiz->get_sms_contact_flag		= get_sms_contact_flag_preference;
		thiz->get_email_contact_flag 	= get_email_contact_flag_preference;

		thiz->set_rs232_alarm_flag	= set_rs232_alarm_flag_preference;
		thiz->set_rs485_alarm_flag	= set_rs485_alarm_flag_preference;
		thiz->set_di_alarm_flag		= set_di_alarm_flag_preference;

		thiz->set_sms_contact_flag		= set_sms_contact_flag_preference;
		thiz->set_email_contact_flag 	= set_email_contact_flag_preference;

		thiz->get_send_times	= get_send_times;
		thiz->get_send_interval	= get_send_interval;

		thiz->get_email_server_param = get_email_server_preference;

        priv_info_t *priv = (priv_info_t *)thiz->priv;
        priv->dic = iniparser_load(INI_FILE_NAME);

        load_preference(priv);
        pthread_mutex_init(&priv->lock_mutex, NULL);
    }

    return thiz;
}
