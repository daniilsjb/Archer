#ifndef GC_H
#define GC_H

#include "common.h"

struct VM;
struct Obj;

typedef struct {
    struct Obj* allocatedObjects;
    
    size_t bytesAllocated;
    size_t threshold;

    size_t grayCount;
    size_t grayCapacity;
    struct Obj** grayStack;
} GC;

void gc_init(GC* gc);
void gc_free(GC* gc, struct VM* vm);

void gc_allocate_bytes(GC* gc, size_t size);
void gc_deallocate_bytes(GC* gc, size_t size);
void gc_attempt_collection(GC* gc, struct VM* vm);

void gc_mark_object(struct VM* vm, struct Obj* object);
void gc_append_object(GC* gc, struct Obj* object);

#endif
