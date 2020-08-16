#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "vm.h"

#define ALLOCATE_OBJ(vm, type, objectType) (type*)allocate_object(vm, sizeof(type), objectType)

Obj* allocate_object(VM* vm, size_t size, ObjType type) 
{
    Obj* object = (Obj*)reallocate(vm, NULL, 0, size);
    object->type = type;
    object->marked = false;

    gc_append_object(&vm->gc, object);

#if DEBUG_LOG_GC
    printf("%p allocated object of type %d\n", (void*)object, object->type);
#endif

    return object;
}

ObjFunction* new_function(VM* vm)
{
    ObjFunction* function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);

    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    chunk_init(&function->chunk);

    return function;
}

ObjClosure* new_closure(VM* vm, ObjFunction* function)
{
    ObjUpvalue** upvalues = ALLOCATE(vm, ObjUpvalue*, function->upvalueCount);
    for (size_t i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjUpvalue* new_upvalue(VM* vm, Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed = NIL_VAL();
    upvalue->next = NULL;
    return upvalue;
}

ObjNative* new_native(VM* vm, NativeFn function, int arity)
{
    ObjNative* native = ALLOCATE_OBJ(vm, ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    return native;
}

ObjClass* new_class(VM* vm, ObjString* name)
{
    ObjClass* loxClass = ALLOCATE_OBJ(vm, ObjClass, OBJ_CLASS);
    loxClass->name = name;
    table_init(&loxClass->methods);
    return loxClass;
}

ObjInstance* new_instance(VM* vm, ObjClass* loxClass)
{
    ObjInstance* instance = ALLOCATE_OBJ(vm, ObjInstance, OBJ_INSTANCE);
    instance->loxClass = loxClass;
    table_init(&instance->fields);
    return instance;
}

ObjBoundMethod* new_bound_method(VM* vm, Value receiver, ObjClosure* method)
{
    ObjBoundMethod* boundMethod = ALLOCATE_OBJ(vm, ObjBoundMethod, OBJ_BOUND_METHOD);
    boundMethod->receiver = receiver;
    boundMethod->method = method;
    return boundMethod;
}

static void print_function(ObjFunction* function)
{
    if (function->name == NULL) {
        printf("<script>");
    } else {
        printf("<fn %s>", function->name->chars);
    }
}

ObjString* make_string(VM* vm, size_t length)
{
    ObjString* string = (ObjString*)allocate_object(vm, sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    return string;
}

ObjString* copy_string(VM* vm, const char* chars, size_t length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString* interned = table_find_string(&vm->strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    ObjString* string = make_string(vm, length);

    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    string->hash = hash;

    vm_push(vm, OBJ_VAL(string));
    table_put(vm, &vm->strings, string, NIL_VAL());
    vm_pop(vm);

    return string;
}

ObjString* concatenate_strings(VM* vm, ObjString* a, ObjString* b)
{
    size_t length = a->length + b->length;
    ObjString* string = make_string(vm, length);

    memcpy(string->chars, a->chars, a->length);
    memcpy(string->chars + a->length, b->chars, b->length);
    string->chars[length] = '\0';
    string->hash = hash_string(string->chars, length);

    ObjString* interned = table_find_string(&vm->strings, string->chars, length, string->hash);
    if (interned != NULL) {
        vm->gc.allocatedObjects = vm->gc.allocatedObjects->next;
        deallocate(vm, string, sizeof(ObjString) + string->length + 1);

        return interned;
    } else {
        vm_push(vm, OBJ_VAL(string));
        table_put(vm, &vm->strings, string, NIL_VAL());
        vm_pop(vm);

        return string;
    }
}

uint32_t hash_string(const char* key, size_t length)
{
    uint32_t hash = 2166136261U;

    for (size_t i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}

void print_object(Value value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(value));
            break;
        }
        case OBJ_FUNCTION: {
            print_function(AS_FUNCTION(value));
            break;
        }
        case OBJ_CLOSURE: {
            print_function(AS_CLOSURE(value)->function);
            break;
        }
        case OBJ_UPVALUE: {
            printf("upvalue");
            break;
        }
        case OBJ_NATIVE: {
            printf("<native fn>");
            break;
        }
        case OBJ_CLASS: {
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        }
        case OBJ_INSTANCE: {
            printf("%s instance", AS_INSTANCE(value)->loxClass->name->chars);
            break;
        }
        case OBJ_BOUND_METHOD: {
            print_function(AS_BOUND_METHOD(value)->method->function);
            break;
        }
    }
}
