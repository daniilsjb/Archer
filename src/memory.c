#include <stdlib.h>

#include "memory.h"
#include "compiler.h"
#include "vm.h"

#if DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
    vm.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
#if DEBUG_STRESS_GC
        collect_garbage();
#endif

        if (vm.bytesAllocated > vm.nextGC) {
            collect_garbage();
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

static void free_object(Obj* object)
{
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            reallocate(string, sizeof(ObjString) + string->length + 1, 0);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            chunk_free(&function->chunk);
            FREE(ObjFunction, function);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, closure);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(ObjUpvalue, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* loxClass = (ObjClass*)object;
            table_free(&loxClass->methods);
            FREE(ObjClass, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            table_free(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_BOUND_METHOD: {
            FREE(ObjBoundMethod, object);
            break;
        }
    }
}

void mark_object(Obj* object)
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

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        void* reallocated = realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        if (!reallocated) {
            exit(1);
        }

        vm.grayStack = reallocated;
    }

    vm.grayStack[vm.grayCount++] = object;
}

void mark_array(ValueArray* array) {
    for (size_t i = 0; i < array->count; i++) {
        mark_value(array->values[i]);
    }
}

void mark_value(Value value)
{
    if (!IS_OBJ(value)) {
        return;
    }

    mark_object(AS_OBJ(value));
}

static void blacken_object(Obj* object)
{
#if DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_UPVALUE: {
            mark_value(((ObjUpvalue*)object)->closed);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            mark_object((Obj*)function->name);
            mark_array(&function->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            mark_object((Obj*)closure->function);
            for (size_t i = 0; i < closure->upvalueCount; i++) {
                mark_object((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_CLASS: {
            ObjClass* loxClass = (ObjClass*)object;
            mark_object((Obj*)loxClass->name);
            mark_table(&loxClass->methods);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            mark_object((Obj*)instance->loxClass);
            mark_table(&instance->fields);
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* boundMethod = (ObjBoundMethod*)object;
            mark_value(boundMethod->receiver);
            mark_object((Obj*)boundMethod->method);
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

static void mark_roots()
{
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        mark_value(*slot);
    }

    for (size_t i = 0; i < vm.frameCount; i++) {
        mark_object((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        mark_object((Obj*)upvalue);
    }

    mark_table(&vm.globals);
    mark_compiler_roots();
    mark_object((Obj*)vm.initString);
}

static void trace_references()
{
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blacken_object(object);
    }
}

static void sweep()
{
    Obj* previous = NULL;
    Obj* object = vm.objects;

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
                vm.objects = object;
            }

            free_object(unreached);
        }
    }
}

void collect_garbage()
{
#if DEBUG_LOG_GC
    printf("-- GC Begin\n");
    size_t before = vm.bytesAllocated;
#endif

    mark_roots();
    trace_references();
    table_remove_white(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#if DEBUG_LOG_GC
    printf("-- GC End\n");
    printf("-- Collected %ld bytes (from %ld to %ld), next at %ld\n", before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
#endif
}

void free_objects()
{
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        free_object(object);
        object = next;
    }

    free(vm.grayStack);
}
