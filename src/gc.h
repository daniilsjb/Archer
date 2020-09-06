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

void GC_Init(GC* gc);
void GC_Free(GC* gc);

void GC_AllocateBytes(GC* gc, size_t size);
void GC_DeallocateBytes(GC* gc, size_t size);
void GC_AttemptCollection(GC* gc);

void GC_MarkObject(GC* gc, Object* object);
void GC_MarkValue(GC* gc, Value value);
void GC_MarkArray(GC* gc, ValueArray* array);
void GC_MarkTable(GC* gc, Table* table);

void GC_AppendObject(GC* gc, Object* object);

#endif
