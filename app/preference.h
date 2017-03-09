#ifndef _PREFERENCE_H_
#define _PREFERENCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INI_FILE_NAME	"param.ini"

typedef struct _preference preference_t;

typedef int (*_get_init_flag)(preference_t *thiz);
typedef void (*_set_init_flag)(preference_t *thiz, int flag);

typedef void (*_preference_destroy)(preference_t *thiz);
typedef int (*_preference_reload)(preference_t *thiz);

struct _preference {
	_get_init_flag		get_init_flag;
    _set_init_flag		set_init_flag;

    _preference_destroy	destroy;
    _preference_reload	reload;

    char priv[1];
};

preference_t *preference_create(void);

#ifdef __cplusplus
}
#endif

#endif
