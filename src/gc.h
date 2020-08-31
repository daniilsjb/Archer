#ifndef GC_H
#define GC_H

#include <stdint.h>

typedef struct VM VM;
typedef struct Obj Obj;

typedef struct GC {
    VM* vm;

    Obj* allocatedObjects;
    
    size_t bytesAllocated;
    size_t threshold;

    size_t grayCount;
    size_t grayCapacity;
    Obj** grayStack;
} GC;

void gc_init(GC* gc);
void gc_free(GC* gc);

void gc_allocate_bytes(GC* gc, size_t size);
void gc_deallocate_bytes(GC* gc, size_t size);
void gc_attempt_collection(GC* gc);

void gc_mark_object(GC* gc, Obj* object);
void gc_append_object(GC* gc, Obj* object);

#endif
