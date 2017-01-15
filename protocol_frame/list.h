#ifndef _LIST_H_
#define _LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _list list_t;

typedef void (*_push_back)(list_t *thiz, void *data);
typedef void *(*_get_index_value)(list_t *thiz, int index);
typedef int  (*_get_list_size)(list_t *thiz);
typedef void (*_empty_list)(list_t *thiz);
typedef void (*_destroy_list)(list_t *thiz);

struct _list
{
    _push_back              push_back;
    _get_index_value        get_index_value;
    _get_list_size          get_list_size;
    _empty_list             empty_list;
    _destroy_list           destroy_list;

    char                    priv[1];
};

list_t *list_create(unsigned int buffer_size);

#ifdef __cplusplus
}
#endif
#endif
