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

static void load_preference(priv_info *priv)
{
	load_system_param(priv);
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
        //thiz->reload		= preference_reload;

        thiz->get_init_flag	= get_init_flag_preference;
        thiz->set_init_flag	= set_init_flag_preference;

        priv_info *priv = (priv_info *)thiz->priv;
        priv->dic = iniparser_load(INI_FILE_NAME);

        load_preference(priv);
        pthread_mutex_init(&priv->lock_mutex, NULL);
    }

    return thiz;
}
