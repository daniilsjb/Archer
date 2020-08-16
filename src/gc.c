#include <stdlib.h>

#include "gc.h"
#include "memory.h"
#include "memlib.h"
#include "object.h"
#include "value.h"
#include "table.h"
#include "vm.h"

#if DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_THRESHOLD_GROW_FACTOR 2

void gc_init(GC* gc)
{
    gc->allocatedObjects = NULL;

    gc->bytesAllocated = 0;
    gc->threshold = 1024 * 1024;

    gc->grayCount = 0;
    gc->grayCapacity = 0;
    gc->grayStack = NULL;
}

static void free_object(VM* vm, Obj* object)
{
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            deallocate(vm, string, string->length + 1);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            chunk_free(vm, &function->chunk);
            FREE(vm, ObjFunction, function);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(vm, ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(vm, ObjClosure, closure);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(vm, ObjUpvalue, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(vm, ObjNative, object);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* loxClass = (ObjClass*)object;
            table_free(vm, &loxClass->methods);
            FREE(vm, ObjClass, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            table_free(vm, &instance->fields);
            FREE(vm, ObjInstance, object);
            break;
        }
        case OBJ_BOUND_METHOD: {
            FREE(vm, ObjBoundMethod, object);
            break;
        }
    }
}

void gc_free(GC* gc, VM* vm)
{
    Obj* object = gc->allocatedObjects;
    while (object != NULL) {
        Obj* next = object->next;
        free_object(vm, object);
        object = next;
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

void gc_mark_object(VM* vm, Obj* object)
{
    if (object == NULL || object->marked) {
        return;
    }

#if DEBUG_LOG_GC
    printf("%p mark", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    object->marked = true;

    GC* gc = &vm->gc;
    if (gc->grayCapacity < gc->grayCount + 1) {
        gc->grayCapacity = GROW_CAPACITY(gc->grayCapacity);

        void* reallocated = raw_reallocate(gc->grayStack, sizeof(Obj*) * gc->grayCapacity);
        if (!reallocated) {
            //TODO: Throw runtime error in the VM
            exit(1);
        }

        gc->grayStack = reallocated;
    }

    gc->grayStack[gc->grayCount++] = object;
}

static void mark_value(VM* vm, Value value)
{
    if (!IS_OBJ(value)) {
        return;
    }

    gc_mark_object(vm, AS_OBJ(value));
}

static void mark_array(VM* vm, ValueArray* array) {
    for (size_t i = 0; i < array->count; i++) {
        mark_value(vm, array->values[i]);
    }
}

void mark_table(VM* vm, Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        gc_mark_object(vm, (Obj*)entry->key);
        mark_value(vm, entry->value);
    }
}

static void mark_roots(GC* gc, VM* vm)
{
    for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
        mark_value(vm, *slot);
    }

    for (size_t i = 0; i < vm->frameCount; i++) {
        gc_mark_object(vm, (Obj*)vm->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        gc_mark_object(vm, (Obj*)upvalue);
    }

    mark_table(vm, &vm->globals);
    mark_compiler_roots(vm);
    gc_mark_object(vm, (Obj*)vm->initString);
}

static void traverse_object(VM* vm, Obj* object)
{
#if DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_UPVALUE: {
            mark_value(vm, ((ObjUpvalue*)object)->closed);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            gc_mark_object(vm, (Obj*)function->name);
            mark_array(vm, &function->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            gc_mark_object(vm, (Obj*)closure->function);
            for (size_t i = 0; i < closure->upvalueCount; i++) {
                gc_mark_object(vm, (Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_CLASS: {
            ObjClass* loxClass = (ObjClass*)object;
            gc_mark_object(vm, (Obj*)loxClass->name);
            mark_table(vm, &loxClass->methods);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            gc_mark_object(vm, (Obj*)instance->loxClass);
            mark_table(vm, &instance->fields);
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* boundMethod = (ObjBoundMethod*)object;
            mark_value(vm, boundMethod->receiver);
            gc_mark_object(vm, (Obj*)boundMethod->method);
            break;
        }
        case OBJ_NATIVE: {
            break;
        }
        case OBJ_STRING: {
            break;
        }
    }
}

static void trace_references(GC* gc, VM* vm)
{
    while (gc->grayCount > 0) {
        Obj* object = gc->grayStack[--gc->grayCount];
        traverse_object(vm, object);
    }
}

void table_remove_white(Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.marked) {
            table_remove(table, entry->key);
}
    }
}

static void sweep(GC* gc, VM* vm)
{
    Obj* previous = NULL;
    Obj* object = gc->allocatedObjects;

    while (object != NULL) {
        if (object->marked) {
            object->marked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;

            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                gc->allocatedObjects = object;
            }

            free_object(vm, unreached);
        }
    }
}

static void perform_collection(GC* gc, VM* vm)
{
#if DEBUG_LOG_GC
    printf("-- GC Begin\n");
    size_t before = gc->bytesAllocated;
#endif

    mark_roots(gc, vm);
    trace_references(gc, vm);
    table_remove_white(&vm->strings);
    sweep(gc, vm);

    gc->threshold = gc->bytesAllocated * GC_THRESHOLD_GROW_FACTOR;

#if DEBUG_LOG_GC
    printf("-- GC End\n");
    printf("-- Collected %zu bytes (from %zu to %zu), next at %zu\n", before - gc->bytesAllocated, before, gc->bytesAllocated, gc->threshold);
#endif
}

void gc_attempt_collection(GC* gc, VM* vm)
{
    if (gc->bytesAllocated > gc->threshold) {
        perform_collection(gc, vm);
    }
}

void gc_append_object(GC* gc, Obj* object)
{
    object->next = gc->allocatedObjects;
    gc->allocatedObjects = object;
}
