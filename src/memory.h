#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include "memlib.h"

typedef struct GC GC;

#define ALLOCATE(gc, type, length)                                                                  \
    (type*)allocate(gc, sizeof(type) * (length))                                                    \

#define FREE(gc, type, pointer)                                                                     \
    deallocate(gc, pointer, sizeof(type))                                                           \

#define GROW_CAPACITY(capacity)                                                                     \
    ((capacity) < 8 ? 8 : (capacity) * 2)                                                           \

#define GROW_ARRAY(gc, type, pointer, oldCapacity, newCapacity)                                     \
    (type*)reallocate(gc, pointer, sizeof(type) * (oldCapacity), sizeof(type) * (newCapacity))      \

#define FREE_ARRAY(gc, type, pointer, capacity)                                                     \
    deallocate(gc, pointer, sizeof(type) * capacity)                                                \

void* allocate(GC* gc, size_t size);
void deallocate(GC* gc, void* pointer, size_t size);
void* reallocate(GC* gc, void* pointer, size_t oldSize, size_t newSize);

#endif
