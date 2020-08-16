#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

struct VM;

#define ALLOCATE(vm, type, length)                                                                  \
    (type*)allocate(vm, sizeof(type) * (length))                                                    \

#define FREE(vm, type, pointer)                                                                     \
    deallocate(vm, pointer, sizeof(type))                                                           \

#define GROW_CAPACITY(capacity)                                                                     \
    ((capacity) < 8 ? 8 : (capacity) * 2)                                                           \

#define GROW_ARRAY(vm, type, pointer, oldCapacity, newCapacity)                                     \
    (type*)reallocate(vm, pointer, sizeof(type) * (oldCapacity), sizeof(type) * (newCapacity))      \

#define FREE_ARRAY(vm, type, pointer, capacity)                                                     \
    deallocate(vm, pointer, sizeof(type) * capacity)                                                \

void* allocate(struct VM* vm, size_t size);
void deallocate(struct VM* vm, void* pointer, size_t size);
void* reallocate(struct VM* vm, void* pointer, size_t oldSize, size_t newSize);

#endif
