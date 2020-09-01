#include "gc.h"
#include "object.h"
#include "objstring.h"
#include "objfunction.h"
#include "vm.h"
#include "common.h"
#include "memory.h"
#include "memlib.h"
#include "value.h"
#include "table.h"
#include "compiler.h"

#if DEBUG_LOG_GC
#include <stdio.h>
#endif

#define GC_THRESHOLD_GROW_FACTOR 2

void gc_init(GC* gc)
{
    gc->vm = NULL;
    gc->allocatedObjects = NULL;

    gc->bytesAllocated = 0;
    gc->threshold = 1024 * 1024;

    gc->grayCount = 0;
    gc->grayCapacity = 0;
    gc->grayStack = NULL;
}

void gc_free(GC* gc)
{
    Object* current = gc->allocatedObjects;

    while (current) {
        Object* next = current->next;
        free_object(current, gc);
        current = next;
    }

    raw_deallocate(gc->grayStack);
}

void gc_allocate_bytes(GC* gc, size_t size)
{
    gc->bytesAllocated += size;
}

void gc_deallocate_bytes(GC* gc, size_t size)
{
    gc->bytesAllocated -= size;
}

void gc_mark_object(GC* gc, Object* object)
{
    if (!object || object->marked) {
        return;
    }

#if DEBUG_LOG_GC
    printf("%p mark", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    object->marked = true;

    if (gc->grayCapacity < gc->grayCount + 1) {
        gc->grayCapacity = GROW_CAPACITY(gc->grayCapacity);

        void* reallocated = raw_reallocate(gc->grayStack, sizeof(Object*) * gc->grayCapacity);
        if (reallocated) {
            gc->grayStack = reallocated;
        }
    }

    gc->grayStack[gc->grayCount++] = object;
}

void gc_mark_value(GC* gc, Value value)
{
    if (IS_OBJ(value)) {
        gc_mark_object(gc, AS_OBJ(value));
    }
}

void gc_mark_array(GC* gc, ValueArray* array)
{
    for (size_t i = 0; i < array->count; i++) {
        gc_mark_value(gc, array->data[i]);
    }
}

void gc_mark_table(GC* gc, Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        gc_mark_object(gc, (Object*)entry->key);
        gc_mark_value(gc, entry->value);
    }
}

static void mark_roots(GC* gc)
{
    VM* vm = gc->vm;
    for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
        gc_mark_value(gc, *slot);
    }

    for (size_t i = 0; i < vm->frameCount; i++) {
        gc_mark_object(gc, (Object*)vm->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        gc_mark_object(gc, (Object*)upvalue);
    }

    gc_mark_table(gc, &vm->globals);
    mark_compiler_roots(gc->vm);
    gc_mark_object(gc, (Object*)vm->initString);
}

static void trace_references(GC* gc)
{
    VM* vm = gc->vm;
    while (gc->grayCount > 0) {
        Object* object = gc->grayStack[--gc->grayCount];
        traverse_object(object, gc);
    }
}

void table_remove_white(Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->base.marked) {
            table_remove(table, entry->key);
}
    }
}

static void sweep(GC* gc)
{
    Object* previous = NULL;
    Object* object = gc->allocatedObjects;

    while (object != NULL) {
        if (object->marked) {
            object->marked = false;
            previous = object;
            object = object->next;
        } else {
            Object* unreached = object;

            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                gc->allocatedObjects = object;
            }

            free_object(unreached, gc);
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
    table_remove_white(&gc->vm->strings);
    sweep(gc);

    gc->threshold = gc->bytesAllocated * GC_THRESHOLD_GROW_FACTOR;

#if DEBUG_LOG_GC
    printf("-- GC End\n");
    printf("-- Collected %zu bytes (from %zu to %zu), next at %zu\n", before - gc->bytesAllocated, before, gc->bytesAllocated, gc->threshold);
#endif
}

void gc_attempt_collection(GC* gc)
{
    if (gc->bytesAllocated > gc->threshold) {
        perform_collection(gc);
    }
}

void gc_append_object(GC* gc, Object* object)
{
    object->next = gc->allocatedObjects;
    gc->allocatedObjects = object;
}
