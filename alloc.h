#ifndef _ALLOC_LIB_
#define _ALLOC_LIB_

#include <unistd.h>

typedef struct meta_data meta_data;
typedef struct control_s control_s;

control_s *init_control(void);
void* malloc(size_t alloc_size);
void free(void* target);

#endif