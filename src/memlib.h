#ifndef MEMLIB_H
#define MEMLIB_H

#include <stdint.h>

void* raw_allocate(size_t size);
void raw_deallocate(void* pointer);
void* raw_reallocate(void* pointer, size_t size);

#endif
