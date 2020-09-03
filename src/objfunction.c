#include <stdio.h>

#include "objfunction.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static void print_function(Object* object)
{
    ObjFunction* function = AS_FUNCTION(object);

    if (!function->name) {
        printf("<script>");
    } else {
        printf("<fn %s>", function->name->chars);
    }
}

static void traverse_function(Object* object, GC* gc)
{
    ObjFunction* function = AS_FUNCTION(object);
    gc_mark_object(gc, (Object*)function->name);
    gc_mark_array(gc, &function->chunk.constants);
}

static void free_function(Object* object, GC* gc)
{
    ObjFunction* function = AS_FUNCTION(object);
    chunk_free(gc, &function->chunk);
    FREE(gc, ObjFunction, function);
}

ObjectType* new_function_type(VM* vm)
{
    ObjectType* type = raw_allocate(sizeof(ObjectType));
    if (!type) {
        return NULL;
    }

    *type = (ObjectType) {
        .name = "function",
        .print = print_function,
        .traverse = traverse_function,
        .free = free_function
    };

    return type;
}

void prepare_function_type(ObjectType* type, VM* vm)
{
}

void free_function_type(ObjectType* type, VM* vm)
{
    raw_deallocate(type);
}

ObjFunction* new_function(VM* vm)
{
    ObjFunction* function = ALLOCATE_FUNCTION(vm);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    chunk_init(&function->chunk);
    return function;
}

static void print_upvalue(Object* object)
{
    printf("<upvalue>");
}

static void traverse_upvalue(Object* object, GC* gc)
{
    gc_mark_value(gc, AS_UPVALUE(object)->closed);
}

static void free_upvalue(Object* object, GC* gc)
{
    FREE(gc, ObjUpvalue, object);
}

ObjectType* new_upvalue_type(VM* vm)
{
    ObjectType* type = raw_allocate(sizeof(ObjectType));
    if (!type) {
        return NULL;
    }

    *type = (ObjectType) {
        .name = "upvalue",
        .print = print_upvalue,
        .traverse = traverse_upvalue,
        .free = free_upvalue
    };

    return type;
}

void prepare_upvalue_type(ObjectType* type, VM* vm)
{
}

void free_upvalue_type(ObjectType* type, VM* vm)
{
    raw_deallocate(type);
}

ObjUpvalue* new_upvalue(VM* vm, Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_UPVALUE(vm);
    upvalue->location = slot;
    upvalue->closed = NIL_VAL();
    upvalue->next = NULL;
    return upvalue;
}

static void print_closure(Object* object)
{
    print_function((Object*)AS_CLOSURE(object)->function);
}

static void traverse_closure(Object* object, GC* gc)
{
    ObjClosure* closure = AS_CLOSURE(object);
    gc_mark_object(gc, (Object*)closure->function);
    for (size_t i = 0; i < closure->upvalueCount; i++) {
        gc_mark_object(gc, (Object*)closure->upvalues[i]);
    }
}

static bool call_closure(Object* callee, uint8_t argCount, VM* vm)
{
    return call(vm, AS_CLOSURE(callee), argCount);
}

static void free_closure(Object* object, GC* gc)
{
    ObjClosure* closure = AS_CLOSURE(object);
    FREE_ARRAY(gc, ObjUpvalue*, closure->upvalues, closure->upvalueCount);
    FREE(gc, ObjClosure, closure);
}

ObjectType* new_closure_type(VM* vm)
{
    ObjectType* type = raw_allocate(sizeof(ObjectType));
    if (!type) {
        return NULL;
    }

    *type = (ObjectType) {
        .name = "closure",
        .print = print_closure,
        .call = call_closure,
        .traverse = traverse_closure,
        .free = free_closure
    };

    return type;
}

void prepare_closure_type(ObjectType* type, VM* vm)
{
}

void free_closure_type(ObjectType* type, VM* vm)
{
    raw_deallocate(type);
}

ObjClosure* new_closure(VM* vm, ObjFunction* function)
{
    ObjUpvalue** upvalues = ALLOCATE(&vm->gc, ObjUpvalue*, function->upvalueCount);
    for (size_t i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_CLOSURE(vm);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}