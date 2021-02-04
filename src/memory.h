#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>

typedef struct GC GC;

#define ALLOCATE(gc, type, length)                                                                  \
    (type*)Mem_Allocate(gc, sizeof(type) * (length))                                                \

#define FREE(gc, type, pointer)                                                                     \
    Mem_Deallocate(gc, pointer, sizeof(type))                                                       \

#define GROW_CAPACITY(capacity)                                                                     \
    ((capacity) < 8 ? 8 : (capacity) * 2)                                                           \

#define GROW_ARRAY(gc, type, pointer, oldCapacity, newCapacity)                                     \
    (type*)Mem_Reallocate(gc, pointer, sizeof(type) * (oldCapacity), sizeof(type) * (newCapacity))  \

#define FREE_ARRAY(gc, type, pointer, capacity)                                                     \
    Mem_Deallocate(gc, pointer, sizeof(type) * capacity)                                            \

void* Mem_Allocate(GC* gc, size_t size);
void Mem_Deallocate(GC* gc, void* pointer, size_t size);
void* Mem_Reallocate(GC* gc, void* pointer, size_t oldSize, size_t newSize);

#endif
