#include "alloc.h"
#include <stdio.h>

#define SIZE_PAGE sysconf(_SC_PAGE_SIZE) /* get memory one page size */

struct meta_data 
{
    size_t size;
    meta_data* next;
    void* data;
    int free;
};

struct control_s
{
    meta_data* head;
    meta_data* tmp;
};

control_s *init_control(void)
{
    static control_s *ctrl_t = NULL;

    if (ctrl_t == NULL)
    {
        ctrl_t = sbrk(SIZE_PAGE * 2);
        ctrl_t->head = NULL;
        ctrl_t->tmp = NULL;
    }
    return (ctrl_t);
}

meta_data* get_last_node(control_s* ctrl_t)
{
    ctrl_t->tmp = ctrl_t->head;
    while(ctrl_t->tmp->next)
    {
        ctrl_t->tmp = ctrl_t->tmp->next;
    }
    return ctrl_t->tmp;
}

meta_data* search_node_by_data(control_s* ctrl_t, void* data)
{
    meta_data* to_return = NULL;

    ctrl_t->tmp = ctrl_t->head;
    while(ctrl_t->tmp && !to_return)
    {
        if(ctrl_t->tmp->data == data)
            to_return = ctrl_t->tmp;
        else
            ctrl_t->tmp = ctrl_t->tmp->next;
    }

    return to_return;
}

void make_some_space(control_s* ctrl_t, size_t size)
{
    void* brk_pos = sbrk(0);
    void* last_pos = NULL;

    size_t needed_space = 0;

    if(!ctrl_t->head)
        last_pos = (void *)ctrl_t + sizeof(control_s);
    else
    {
        ctrl_t->tmp = get_last_node(ctrl_t);
        last_pos = (void *)ctrl_t->tmp + sizeof(meta_data) + ctrl_t->tmp->size;
    }

    /* check the size left between brk and the last allocated */
    while(needed_space + (int)(brk_pos - last_pos) < sizeof(meta_data) + size)
    {
        needed_space += SIZE_PAGE * 2;
    }

    sbrk(needed_space);
}

/* check to use a freed space */
meta_data* try_freed_space(control_s* ctrl_t, size_t size)
{
    meta_data* to_return = NULL;

    ctrl_t->tmp = ctrl_t->head;
    while(ctrl_t->tmp)
    {
        if(ctrl_t->tmp->free == 1 && ctrl_t->tmp->size > size)
        {
            if(to_return)
            {
                if(ctrl_t->tmp->size < to_return->size)
                {
                    to_return = ctrl_t->tmp;
                    to_return->free = 0;
                }
            }
            else
            {
                to_return = ctrl_t->tmp;
                to_return->free = 0;
            }
        }
        
        ctrl_t->tmp = ctrl_t->tmp->next;
    }

    return to_return;
}

void* malloc(size_t alloc_size)
{
    if(alloc_size == 0)
        return NULL;

    control_s* ctrl_t = init_control(); /* get the control struct */
    meta_data* md_t = NULL; /* the new meta data */
    
    ctrl_t->tmp = try_freed_space(ctrl_t, alloc_size);

    if(!ctrl_t->tmp)
    {
        make_some_space(ctrl_t, alloc_size);

        if(!ctrl_t->head)
        {
            md_t = (void *)ctrl_t + sizeof(control_s);
            ctrl_t->head = md_t;
        }
        else
        {
            ctrl_t->tmp = get_last_node(ctrl_t);
            md_t = (void *)ctrl_t->tmp + sizeof(meta_data) + ctrl_t->tmp->size;
            ctrl_t->tmp->next = md_t;
        }

        md_t->size = alloc_size;
        md_t->next = NULL;
        md_t->data = (void *)md_t + sizeof(meta_data);
        md_t->free = 0;

        return md_t->data;
    }
    else
    {
        ctrl_t->tmp->data = (void *)md_t + sizeof(meta_data);
        ctrl_t->tmp->free = 0;

        return ctrl_t->tmp->data;
    }
}

void free(void* data)
{
    control_s* ctrl_t = init_control();
    meta_data* target = search_node_by_data(ctrl_t, data);

    if(target)
        target->free = 1;
}