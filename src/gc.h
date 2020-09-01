#ifndef GC_H
#define GC_H

#include <stdint.h>

#include "value.h"

typedef struct VM VM;
typedef struct Object Object;
typedef struct Table Table;

typedef struct GC {
    VM* vm;

    Object* allocatedObjects;
    
    size_t bytesAllocated;
    size_t threshold;

    size_t grayCount;
    size_t grayCapacity;
    Object** grayStack;
} GC;

void gc_init(GC* gc);
void gc_free(GC* gc);

void gc_allocate_bytes(GC* gc, size_t size);
void gc_deallocate_bytes(GC* gc, size_t size);
void gc_attempt_collection(GC* gc);

void gc_mark_object(GC* gc, Object* object);
void gc_mark_value(GC* gc, Value value);
void gc_mark_array(GC* gc, ValueArray* array);
void gc_mark_table(GC* gc, Table* table);

void gc_append_object(GC* gc, Object* object);

#endif
