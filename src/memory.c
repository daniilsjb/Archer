#include <stdlib.h>

#include "memory.h"
#include "compiler.h"
#include "vm.h"

#if DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(VM* vm, void* pointer, size_t oldSize, size_t newSize)
{
    vm->bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
#if DEBUG_STRESS_GC
        collect_garbage(vm);
#endif

        if (vm->bytesAllocated > vm->nextGC) {
            collect_garbage(vm);
        }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) {
        exit(1);
    }

    return result;
}

static void free_object(VM* vm, Obj* object)
{
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            reallocate(vm, string, sizeof(ObjString) + string->length + 1, 0);
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

void mark_object(VM* vm, Obj* object)
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

    if (vm->grayCapacity < vm->grayCount + 1) {
        vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
        void* reallocated = realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);
        if (!reallocated) {
            exit(1);
        }

        vm->grayStack = reallocated;
    }

    vm->grayStack[vm->grayCount++] = object;
}

void mark_array(VM* vm, ValueArray* array) {
    for (size_t i = 0; i < array->count; i++) {
        mark_value(vm, array->values[i]);
    }
}

void mark_value(VM* vm, Value value)
{
    if (!IS_OBJ(value)) {
        return;
    }

    mark_object(vm, AS_OBJ(value));
}

static void blacken_object(VM* vm, Obj* object)
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
            mark_object(vm, (Obj*)function->name);
            mark_array(vm, &function->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            mark_object(vm, (Obj*)closure->function);
            for (size_t i = 0; i < closure->upvalueCount; i++) {
                mark_object(vm, (Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_CLASS: {
            ObjClass* loxClass = (ObjClass*)object;
            mark_object(vm, (Obj*)loxClass->name);
            mark_table(vm, &loxClass->methods);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            mark_object(vm, (Obj*)instance->loxClass);
            mark_table(vm, &instance->fields);
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* boundMethod = (ObjBoundMethod*)object;
            mark_value(vm, boundMethod->receiver);
            mark_object(vm, (Obj*)boundMethod->method);
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

static void mark_roots(VM* vm)
{
    for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
        mark_value(vm, *slot);
    }

    for (size_t i = 0; i < vm->frameCount; i++) {
        mark_object(vm, (Obj*)vm->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        mark_object(vm, (Obj*)upvalue);
    }

    mark_table(vm, &vm->globals);
    mark_compiler_roots(vm);
    mark_object(vm, (Obj*)vm->initString);
}

static void trace_references(VM* vm)
{
    while (vm->grayCount > 0) {
        Obj* object = vm->grayStack[--vm->grayCount];
        blacken_object(vm, object);
    }
}

static void sweep(VM* vm)
{
    Obj* previous = NULL;
    Obj* object = vm->objects;

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
                vm->objects = object;
            }

            free_object(vm, unreached);
        }
    }
}

void collect_garbage(VM* vm)
{
#if DEBUG_LOG_GC
    printf("-- GC Begin\n");
    size_t before = vm->bytesAllocated;
#endif

    mark_roots(vm);
    trace_references(vm);
    table_remove_white(&vm->strings);
    sweep(vm);

    vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#if DEBUG_LOG_GC
    printf("-- GC End\n");
    printf("-- Collected %ld bytes (from %ld to %ld), next at %ld\n", before - vm->bytesAllocated, before, vm->bytesAllocated, vm->nextGC);
#endif
}

void free_objects(VM* vm)
{
    Obj* object = vm->objects;
    while (object != NULL) {
        Obj* next = object->next;
        free_object(vm, object);
        object = next;
    }

    free(vm->grayStack);
}
