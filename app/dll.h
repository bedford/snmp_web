#ifndef __DLL_H
#define __DLL_H

// DLL stands for Double-Linked List

typedef struct dll_node
{
        struct dll_node *prev;
        struct dll_node *next;
} dll_node_t;

typedef struct
{
        dll_node_t      *head;
        dll_node_t      *tail;
        unsigned int    count;
} dll_t;

typedef unsigned int (*dll_cb_t)(dll_node_t *, void *);

#ifdef  __cplusplus
extern "C"
{
#endif

void dll_init (dll_t *p_dll);
inline void dll_insert_before(dll_t *p_dll, dll_node_t *p_ref, dll_node_t *p_inserted);
inline void dll_insert_after(dll_t *p_dll, dll_node_t *p_ref, dll_node_t *p_inserted);
inline void dll_push_head(dll_t *p_dll, dll_node_t *p_node);
inline void dll_push_tail(dll_t *p_dll, dll_node_t *p_node);
inline dll_node_t *dll_pop_head(dll_t *p_dll);
inline dll_node_t *dll_pop_tail(dll_t *p_dll);
inline void dll_remove(dll_t *p_dll, dll_node_t *p_node);
inline dll_node_t *dll_traverse(dll_t *p_dll, dll_cb_t cb, void *p_arg);
inline unsigned int dll_size(dll_t *p_dll);
inline dll_node_t *dll_head(dll_t *p_dll);
inline dll_node_t *dll_tail(dll_t *p_dll);

#ifdef __cplusplus
}
#endif

#endif
