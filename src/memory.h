#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
#include "object.h"

struct VM;

#define ALLOCATE(vm, type, length) (type*)reallocate(vm, NULL, 0, sizeof(type) * (length))

#define FREE(vm, type, pointer) reallocate(vm, pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(vm, type, pointer, oldCapacity, newCapacity) (type*)reallocate(vm, pointer, sizeof(type) * (oldCapacity), sizeof(type) * (newCapacity))

#define FREE_ARRAY(vm, type, pointer, capacity) (type*)reallocate(vm, pointer, sizeof(type) * capacity, 0)

void* reallocate(struct VM* vm, void* pointer, size_t oldSize, size_t newSize);

void mark_object(struct VM* vm, Obj* object);

void mark_value(struct VM* vm, Value value);

void collect_garbage(struct VM* vm);

void free_objects(struct VM* vm);

#endif
