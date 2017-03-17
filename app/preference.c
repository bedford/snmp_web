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
} priv_info;

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

static int load_system_param(priv_info *priv)
{
	priv->init_flag = iniparser_getint(priv->dic, "SYSTEM:init_flag", 1);

	priv->rs232_alarm_flag = iniparser_getint(priv->dic, "ALARM:rs232_alarm_flag", 1);
	priv->rs485_alarm_flag = iniparser_getint(priv->dic, "ALARM:rs485_alarm_flag", 1);
	priv->di_alarm_flag = iniparser_getint(priv->dic, "ALARM:di_alarm_flag", 1);

	return 0;
}

static void set_init_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info *priv =  (priv_info *)thiz->priv;
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
		priv_info *priv =  (priv_info *)thiz->priv;
		init_flag = priv->init_flag;
	}

	return init_flag;
}

static void set_rs232_alarm_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info *priv =  (priv_info *)thiz->priv;
	priv->rs232_alarm_flag = flag;
    sprintf(tmp, "%d", priv->rs232_alarm_flag);
    write_profile(priv->dic, "ALARM", "rs232_alarm_flag", tmp);
    dump_profile(priv->dic, INI_FILE_NAME);
}

static int get_rs232_alarm_flag_preference(preference_t *thiz)
{
	int rs232_alarm_flag = 0;

	if (thiz != 0) {
		priv_info *priv =  (priv_info *)thiz->priv;
		rs232_alarm_flag = priv->rs232_alarm_flag;
	}

	return rs232_alarm_flag;
}

static void set_rs485_alarm_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info *priv =  (priv_info *)thiz->priv;
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
		priv_info *priv =  (priv_info *)thiz->priv;
		rs485_alarm_flag = priv->rs485_alarm_flag;
	}

	return rs485_alarm_flag;
}

static void set_di_alarm_flag_preference(preference_t *thiz, int flag)
{
    char tmp[32] = {0};
    priv_info *priv =  (priv_info *)thiz->priv;
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
		priv_info *priv =  (priv_info *)thiz->priv;
		di_alarm_flag = priv->di_alarm_flag;
	}

	return di_alarm_flag;
}

static void load_preference(priv_info *priv)
{
	struct stat st;
	lstat(INI_FILE_NAME, &st);
	priv->modify_time = st.st_mtime;

	load_system_param(priv);
}

static int preference_reload(preference_t *thiz)
{
    int ret = -1;
    if (thiz != NULL) {
        priv_info *priv = (priv_info *)thiz->priv;
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
        priv_info *priv =  (priv_info *)thiz->priv;
        if (priv->dic) {
			iniparser_freedict(priv->dic);
			priv->dic = NULL;
        }

        pthread_mutex_destroy(&priv->lock_mutex);

        memset(thiz, 0, sizeof(preference_t) + sizeof(priv_info));
        free(thiz);
        thiz = NULL;
    }
}

preference_t *preference_create(void)
{
    preference_t *thiz = NULL;
    thiz = (preference_t *)calloc(1, sizeof(preference_t) + sizeof(priv_info));
    if (thiz != NULL) {
        thiz->destroy		= preference_destroy;
        thiz->reload		= preference_reload;

        thiz->get_init_flag	= get_init_flag_preference;
        thiz->set_init_flag	= set_init_flag_preference;

		thiz->get_rs232_alarm_flag	= get_rs232_alarm_flag_preference;
		thiz->get_rs485_alarm_flag	= get_rs485_alarm_flag_preference;
		thiz->get_di_alarm_flag 	= get_di_alarm_flag_preference;

		thiz->set_rs232_alarm_flag	= set_rs232_alarm_flag_preference;
		thiz->set_rs485_alarm_flag	= set_rs485_alarm_flag_preference;
		thiz->set_di_alarm_flag		= set_di_alarm_flag_preference;

        priv_info *priv = (priv_info *)thiz->priv;
        priv->dic = iniparser_load(INI_FILE_NAME);

        load_preference(priv);
        pthread_mutex_init(&priv->lock_mutex, NULL);
    }

    return thiz;
}
