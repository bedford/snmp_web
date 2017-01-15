#include <string.h>
#include <stdlib.h>
#include "list.h"

typedef struct node_s
{
    struct node_s   *prev;
    struct node_s   *next;
    void            *data;
} node_t;

typedef struct
{
    node_t          *head;
    unsigned int    count;
    unsigned int    buf_size;
} priv_info_t;

static node_t *node_create(void *data, unsigned int buf_size)
{
    node_t *n = NULL;
    n = (node_t *)calloc(1, sizeof(node_t) + buf_size);
    if (n != NULL) {
        n->data = (void *)((unsigned char *)n + sizeof(node_t));
        n->prev = NULL;
        n->next = NULL;
        memcpy(n->data, data, buf_size);
    }

    return n;
}

static void push_back(list_t *thiz, void *data)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv; 
    node_t *n = node_create(data, priv->buf_size);

    if (!priv->count) {
        n->next = n;
        n->prev = n;

        priv->head = n;
    } else {
        node_t *head = priv->head;
        node_t *prev = head->prev;

        n->next = head;
        n->prev = head->prev;

        head->prev = n;
        prev->next = n;
    }

    priv->count++;
}

static void *get_index_value(list_t *thiz, int index)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv; 
    if ((priv->count == 0) || (index >= priv->count)) {
        return NULL;
    }

    node_t *current = priv->head;
    int i = 0;
    for (i = 0; i < index; i++) {
        current = current->next;
    }

    return current->data;
}

static int get_list_size(list_t *thiz)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv; 
    return priv->count;
}

static void empty_list(list_t *thiz)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv; 
    if (priv->count == 0) {
        return;
    }

    node_t *current = priv->head;
    node_t *next    = NULL;

    int i = 0;
    for (i = 0; i < priv->count; i++) {
        next = current->next;
        memset(current, 0, sizeof(node_t) + priv->buf_size);
        free(current);
        current = NULL;

        if (i < (priv->count - 1)) {
            current = next;
        }
    }

    priv->head      = NULL;
    priv->count     = 0;
}

static void destroy_list(list_t *thiz)
{
    if (thiz != NULL) {
        empty_list(thiz);

        memset(thiz, 0, sizeof(list_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

list_t *list_create(unsigned int buf_size)
{
    list_t *thiz = NULL;
    thiz = (list_t *)calloc(1, sizeof(list_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
        thiz->push_back         = push_back;
        thiz->get_index_value   = get_index_value;
        thiz->get_list_size     = get_list_size;
        thiz->empty_list        = empty_list;
        thiz->destroy_list      = destroy_list;

        priv_info_t *priv       = (priv_info_t *)thiz->priv; 
        priv->head              = NULL;
        priv->count             = 0; 
        priv->buf_size          = buf_size;
    }

    return thiz;
}
