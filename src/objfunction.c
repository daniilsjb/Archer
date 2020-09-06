#include <stdio.h>

#include "objfunction.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static void print_function(Object* object)
{
    ObjectFunction* function = AS_FUNCTION(object);

    if (!function->name) {
        printf("<script>");
    } else {
        printf("<fn '%s'>", function->name->chars);
    }
}

static void traverse_function(Object* object, GC* gc)
{
    ObjectFunction* function = AS_FUNCTION(object);
    GC_MarkObject(gc, (Object*)function->name);
    GC_MarkArray(gc, &function->chunk.constants);
    Object_GenericTraverse(object, gc);
}

static void free_function(Object* object, GC* gc)
{
    chunk_free(gc, &AS_FUNCTION(object)->chunk);
    Object_Deallocate(gc, object);
}

ObjectType* Function_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Function";
    type->size = sizeof(ObjectFunction);
    type->flags = 0x0;
    type->Print = print_function;
    type->Hash = NULL;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = traverse_function;
    type->Free = free_function;
    return type;
}

void Function_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectFunction* Function_New(VM* vm)
{
    ObjectFunction* function = ALLOCATE_FUNCTION(vm);
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
    GC_MarkValue(gc, AS_UPVALUE(object)->closed);
    Object_GenericTraverse(object, gc);
}

ObjectType* Upvalue_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Upvalue";
    type->size = sizeof(ObjectUpvalue);
    type->flags = 0x0;
    type->Print = print_upvalue;
    type->Hash = NULL;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = traverse_upvalue;
    type->Free = Object_GenericFree;
    return type;
}

void Upvalue_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectUpvalue* Upvalue_New(VM* vm, Value* slot)
{
    ObjectUpvalue* upvalue = ALLOCATE_UPVALUE(vm);
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
    ObjectClosure* closure = AS_CLOSURE(object);
    GC_MarkObject(gc, (Object*)closure->function);
    for (size_t i = 0; i < closure->upvalueCount; i++) {
        GC_MarkObject(gc, (Object*)closure->upvalues[i]);
    }

    Object_GenericTraverse(object, gc);
}

static bool call_closure(Object* callee, uint8_t argCount, VM* vm)
{
    return call(vm, AS_CLOSURE(callee), argCount);
}

static void free_closure(Object* object, GC* gc)
{
    ObjectClosure* closure = AS_CLOSURE(object);
    FREE_ARRAY(gc, ObjectUpvalue*, closure->upvalues, closure->upvalueCount);
    Object_Deallocate(gc, object);
}

ObjectType* Closure_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Closure";
    type->size = sizeof(ObjectClosure);
    type->flags = 0x0;
    type->Print = print_closure;
    type->Hash = NULL;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = call_closure;
    type->Traverse = traverse_closure;
    type->Free = free_closure;
    return type;
}

void Closure_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectClosure* Closure_New(VM* vm, ObjectFunction* function)
{
    ObjectUpvalue** upvalues = ALLOCATE(&vm->gc, ObjectUpvalue*, function->upvalueCount);
    for (size_t i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjectClosure* closure = ALLOCATE_CLOSURE(vm);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

static void print_bound_method(Object* object)
{
    Object_Print(AS_BOUND_METHOD(object)->method);
}

static bool call_bound_method(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectBoundMethod* bound = AS_BOUND_METHOD(callee);
    vm->stackTop[-argCount - 1] = bound->receiver;
    return Object_Call(bound->method, argCount, vm);
}

static void traverse_bound_method(Object* object, GC* gc)
{
    ObjectBoundMethod* boundMethod = AS_BOUND_METHOD(object);
    GC_MarkValue(gc, boundMethod->receiver);
    GC_MarkObject(gc, boundMethod->method);
    Object_GenericTraverse(object, gc);
}

ObjectType* BoundMethod_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "BoundMethod";
    type->size = sizeof(ObjectBoundMethod);
    type->flags = 0x0;
    type->Print = print_bound_method;
    type->Hash = NULL;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = call_bound_method;
    type->Traverse = traverse_bound_method;
    type->Free = Object_GenericFree;
    return type;
}

void BoundMethod_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectBoundMethod* BoundMethod_New(VM* vm, Value receiver, Object* method)
{
    ObjectBoundMethod* boundMethod = ALLOCATE_BOUND_METHOD(vm);
    boundMethod->receiver = receiver;
    boundMethod->method = method;
    return boundMethod;
}