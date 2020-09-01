#include <stdio.h>

#include "objclass.h"
#include "objfunction.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static void print_instance(Object* object)
{
    printf("%s instance", AS_INSTANCE(object)->clazz->name->chars);
}

static void traverse_instance(Object* object, GC* gc)
{
    ObjInstance* instance = AS_INSTANCE(object);
    gc_mark_object(gc, (Object*)instance->clazz);
    gc_mark_table(gc, &instance->fields);
}

static void free_instance(Object* object, GC* gc)
{
    table_free(gc, &AS_INSTANCE(object)->fields);
    FREE(gc, ObjInstance, object);
}

ObjectType InstanceType = {
    .print = print_instance,
    .traverse = traverse_instance,
    .free = free_instance
};

ObjInstance* new_instance(VM* vm, ObjClass* clazz)
{
    ObjInstance* instance = ALLOCATE_OBJ(vm, ObjInstance, &InstanceType);
    instance->clazz = clazz;
    table_init(&instance->fields);
    return instance;
}

static void print_class(Object* object)
{
    printf("%s", AS_CLASS(object)->name->chars);
}

static bool call_class(Object* callee, uint8_t argCount, VM* vm)
{
    ObjClass* clazz = AS_CLASS(callee);
    vm->stackTop[-argCount - 1] = OBJ_VAL(new_instance(vm, clazz));

    Value initializer;
    if (table_get(&clazz->methods, vm->initString, &initializer)) {
        return call(vm, VAL_AS_CLOSURE(initializer), argCount);
    }
    
    if (argCount != 0) {
        runtime_error(vm, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    return true;
}

static void traverse_class(Object* object, GC* gc)
{
    ObjClass* clazz = AS_CLASS(object);
    gc_mark_object(gc, (Object*)clazz->name);
    gc_mark_table(gc, &clazz->methods);

    ObjInstance* instance = AS_INSTANCE(&clazz->base);
    gc_mark_object(gc, (Object*)instance->clazz);
    gc_mark_table(gc, &instance->fields);
}

static void free_class(Object* object, GC* gc)
{
    table_free(gc, &AS_CLASS(object)->methods);
    FREE(gc, ObjClass, object);
}

ObjectType ClassType = {
    .print = print_class,
    .call = call_class,
    .traverse = traverse_class,
    .free = free_class
};

ObjClass* new_class(VM* vm, ObjString* name)
{
    ObjClass* metaclass = ALLOCATE_OBJ(vm, ObjClass, &ClassType);
    metaclass->name = concatenate_strings(vm, name, copy_string(vm, " meta", 5));
    metaclass->base.clazz = NULL;
    table_init(&metaclass->methods);
    table_init(&metaclass->base.fields);

    ObjClass* clazz = ALLOCATE_OBJ(vm, ObjClass, &ClassType);
    clazz->name = name;
    clazz->base.clazz = metaclass;
    table_init(&clazz->methods);
    table_init(&clazz->base.fields);

    return clazz;
}

static void print_bound_method(Object* object)
{
    print_object((Object*)AS_BOUND_METHOD(object)->method->function);
}

static bool call_bound_method(Object* callee, uint8_t argCount, VM* vm)
{
    ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
    vm->stackTop[-argCount - 1] = bound->receiver;
    return call(vm, bound->method, argCount);
}

static void traverse_bound_method(Object* object, GC* gc)
{
    ObjBoundMethod* boundMethod = AS_BOUND_METHOD(object);
    gc_mark_value(gc, boundMethod->receiver);
    gc_mark_object(gc, (Object*)boundMethod->method);
}

static void free_bound_method(Object* object, GC* gc)
{
    FREE(gc, ObjBoundMethod, object);
}

ObjectType BoundMethodType = {
    .print = print_bound_method,
    .call = call_bound_method,
    .traverse = traverse_bound_method,
    .free = free_bound_method
};

ObjBoundMethod* new_bound_method(VM* vm, Value receiver, ObjClosure* method)
{
    ObjBoundMethod* boundMethod = ALLOCATE_OBJ(vm, ObjBoundMethod, &BoundMethodType);
    boundMethod->receiver = receiver;
    boundMethod->method = method;
    return boundMethod;
}