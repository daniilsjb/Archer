#include "gc.h"
#include "object.h"
#include "objstring.h"
#include "objfunction.h"
#include "vm.h"
#include "common.h"
#include "memory.h"
#include "value.h"
#include "table.h"
#include "compiler.h"

#if DEBUG_LOG_GC
#include <stdio.h>
#endif

#define GC_THRESHOLD_GROW_FACTOR 2

void GC_Init(GC* gc)
{
    gc->vm = NULL;
    gc->allocatedObjects = NULL;

    gc->bytesAllocated = 0;
    gc->threshold = 1024 * 1024;

    gc->grayCount = 0;
    gc->grayCapacity = 0;
    gc->grayStack = NULL;
}

static void free_objects(GC* gc)
{
    Object* current = gc->allocatedObjects;

    while (current) {
        Object* next = current->next;
        Object_Free(current, gc);
        current = next;
    }
}

void GC_Free(GC* gc)
{
    free_objects(gc);
    raw_deallocate(gc->grayStack);
}

void GC_AllocateBytes(GC* gc, size_t size)
{
    gc->bytesAllocated += size;
}

void GC_DeallocateBytes(GC* gc, size_t size)
{
    gc->bytesAllocated -= size;
}

static void ensure_graylist_capacity(GC* gc)
{
    if (gc->grayCapacity < gc->grayCount + 1) {
        gc->grayCapacity = GROW_CAPACITY(gc->grayCapacity);

        void* reallocated = raw_reallocate(gc->grayStack, sizeof(Object*) * gc->grayCapacity);
        if (reallocated) {
            gc->grayStack = reallocated;
        }
    }
}

void GC_MarkObject(GC* gc, Object* object)
{
    if (!object || object->marked) {
        return;
    }

    object->marked = true;

#if DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    ensure_graylist_capacity(gc);
    gc->grayStack[gc->grayCount++] = object;
}

void GC_MarkValue(GC* gc, Value value)
{
    if (IS_OBJ(value)) {
        GC_MarkObject(gc, AS_OBJ(value));
    }
}

void GC_MarkArray(GC* gc, ValueArray* array)
{
    for (size_t i = 0; i < array->count; i++) {
        GC_MarkValue(gc, array->data[i]);
    }
}

void GC_MarkTable(GC* gc, Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        GC_MarkValue(gc, entry->key);
        GC_MarkValue(gc, entry->value);
    }
}

static void mark_roots(GC* gc)
{
    VM* vm = gc->vm;
    
    for (size_t i = 0; i < vm->temporaryCount; i++) {
        GC_MarkValue(gc, vm->temporaries[i]);
    }

    GC_MarkTable(gc, &vm->modules);
    GC_MarkTable(gc, &vm->builtins);
    GC_MarkObject(gc, (Object*)vm->coroutine);

    GC_MarkObject(gc, (Object*)vm->stringType);
    GC_MarkObject(gc, (Object*)vm->nativeType);
    GC_MarkObject(gc, (Object*)vm->functionType);
    GC_MarkObject(gc, (Object*)vm->upvalueType);
    GC_MarkObject(gc, (Object*)vm->closureType);
    GC_MarkObject(gc, (Object*)vm->boundMethodType);
    GC_MarkObject(gc, (Object*)vm->coroutineFunctionType);
    GC_MarkObject(gc, (Object*)vm->coroutineType);
    GC_MarkObject(gc, (Object*)vm->listType);
    GC_MarkObject(gc, (Object*)vm->mapType);
    GC_MarkObject(gc, (Object*)vm->moduleType);
    GC_MarkObject(gc, (Object*)vm->iteratorType);
    GC_MarkObject(gc, (Object*)vm->rangeType);
    GC_MarkObject(gc, (Object*)vm->tupleType);

    GC_MarkObject(gc, (Object*)vm->initString);
    mark_compiler_roots(gc->vm);
}

static void trace_references(GC* gc)
{
    VM* vm = gc->vm;
    while (gc->grayCount > 0) {
        Object* object = gc->grayStack[--gc->grayCount];
        Object_Traverse(object, gc);
    }
}

void table_remove_white_strings(Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        if (!IS_UNDEFINED(entry->key) && !VAL_AS_STRING(entry->key)->base.marked) {
            table_remove(table, entry->key);
        }
    }
}

static void sweep(GC* gc)
{
    Object* previous = NULL;
    Object* current = gc->allocatedObjects;

    while (current) {
        if (current->marked) {
            current->marked = false;
            previous = current;
            current = current->next;
        } else {
            Object* unreached = current;

            current = current->next;
            if (previous) {
                previous->next = current;
            } else {
                gc->allocatedObjects = current;
            }

            Object_Free(unreached, gc);
        }
    }
}

static void perform_collection(GC* gc)
{
#if DEBUG_LOG_GC
    printf("-- GC Begin\n");
    size_t before = gc->bytesAllocated;
#endif

    mark_roots(gc);
    trace_references(gc);
    table_remove_white_strings(&gc->vm->strings);
    sweep(gc);

    gc->threshold = gc->bytesAllocated * GC_THRESHOLD_GROW_FACTOR;

#if DEBUG_LOG_GC
    printf("-- GC End\n");
    printf("-- Collected %zu bytes (from %zu to %zu), next at %zu\n", before - gc->bytesAllocated, before, gc->bytesAllocated, gc->threshold);
#endif
}

void GC_AttemptCollection(GC* gc)
{
#if DEBUG_STRESS_GC
    perform_collection(gc);
#else
    if (gc->bytesAllocated > gc->threshold) {
        perform_collection(gc);
    }
#endif
}

void GC_AppendObject(GC* gc, Object* object)
{
    object->next = gc->allocatedObjects;
    gc->allocatedObjects = object;
}
