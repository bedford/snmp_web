#include "dll.h"

// DLL stands for Double-Linked List

void dll_init(dll_t *p_dll)
{
        p_dll->head = p_dll->tail = 0;
        p_dll->count = 0;
}

void dll_insert_before(dll_t *p_dll, dll_node_t *p_ref, dll_node_t *p_inserted)
{
        if(p_ref->prev == 0) {
                p_dll->head = p_inserted;
                p_ref->prev = p_inserted;
                p_inserted->next = p_ref;
                p_inserted->prev = 0;
        } else {
                p_ref->prev->next = p_inserted;
                p_inserted->prev = p_ref->prev;
                p_inserted->next = p_ref;
                p_ref->prev = p_inserted;

        }
        p_dll->count++;
}

void dll_insert_after(dll_t *p_dll, dll_node_t *p_ref, dll_node_t *p_inserted)
{
        if(p_ref->next == 0) {
                p_dll->tail = p_inserted;
                p_ref->next = p_inserted;
                p_inserted->next = 0;
                p_inserted->prev = p_ref;
        } else{
                p_ref->next->prev = p_inserted;
                p_inserted->prev = p_ref;
                p_inserted->next = p_ref->next;
                p_ref->next = p_inserted;
        }
        p_dll->count++;
}

void dll_push_head(dll_t *p_dll, dll_node_t *p_node)
{
        if (p_dll->head == 0) {
                p_dll->head = p_dll->tail = p_node;
                p_node->next = p_node->prev = 0;
        } else {
                p_node->next = p_dll->head;
                p_node->prev = 0;
                p_dll->head->prev = p_node;
                p_dll->head = p_node;
        }

        p_dll->count++;
}

void dll_push_tail(dll_t *p_dll, dll_node_t *p_node)
{
        if (p_dll->tail == 0) {
                p_dll->head = p_dll->tail = p_node;
                p_node->next = p_node->prev = 0;
        } else {
                dll_node_t *p_tail = p_dll->tail;

                p_tail->next = p_node;
                p_node->prev = p_tail;
                p_node->next = 0;
                p_dll->tail = p_node;
        }

        p_dll->count++;
}

dll_node_t *dll_pop_head(dll_t *p_dll)
{
        dll_node_t *p_node = p_dll->head;

        if (p_node != 0) {
                p_dll->count--;
                p_dll->head = p_node->next;
                if (p_dll->head == 0) {
                        p_dll->tail = 0;
                } else {
                        p_node->next->prev = 0;
                }
        }

        return p_node;
}

dll_node_t *dll_pop_tail(dll_t *p_dll)
{
        dll_node_t *p_node = p_dll->tail;

        if (p_node != 0) {
                p_dll->count --;
                p_dll->tail = p_node->prev;
                if (p_dll->tail == 0) {
                        p_dll->head = 0;
                } else {
                        p_node->prev->next = 0;
                }
        }

        return p_node;
}

void dll_remove(dll_t *p_dll, dll_node_t *p_node)
{
        if (p_node->prev == 0) {
                p_dll->head = p_node->next;
        } else {
                p_node->prev->next = p_node->next;
        }

        if (p_node->next == 0) {
                p_dll->tail = p_node->prev;
        } else {
                p_node->next->prev = p_node->prev;
        }

        p_dll->count--;
}

dll_node_t *dll_traverse(dll_t *p_dll, dll_cb_t cb, void *p_arg)
{
        if (cb == 0) {
                return 0;
        }

        dll_node_t *p_node = p_dll->head;
        while ((p_node != 0) && ((*cb) (p_node, p_arg))) {
                p_node = p_node->next;
        }

        return p_node;
}

unsigned int dll_size(dll_t *p_dll)
{
        return p_dll->count;
}

dll_node_t *dll_head(dll_t *p_dll)
{
        return p_dll->head;
}

dll_node_t *dll_tail(dll_t *p_dll)
{
        return p_dll->tail;
}
