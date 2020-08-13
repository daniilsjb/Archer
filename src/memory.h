#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(type, length) (type*)reallocate(NULL, 0, sizeof(type) * (length))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCapacity, newCapacity) (type*)reallocate(pointer, sizeof(type) * (oldCapacity), sizeof(type) * (newCapacity))

#define FREE_ARRAY(type, pointer, capacity) (type*)reallocate(pointer, sizeof(type) * capacity, 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

void mark_object(Obj* object);

void mark_value(Value value);

void collect_garbage();

void free_objects();

#endif
