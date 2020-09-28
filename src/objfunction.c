#include <stdio.h>

#include "objfunction.h"
#include "objstring.h"
#include "objcoroutine.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static ObjectString* function_to_string(Object* object, VM* vm)
{
    ObjectFunction* function = AS_FUNCTION(object);

    if (!function->name) {
        return String_FromCString(vm, "<lambda fn>");
    } else {
        char cstring[100];
        snprintf(cstring, 100, "<fn '%s'>", AS_CSTRING(function->name));
        return String_FromCString(vm, cstring);
    }
}

static void function_print(Object* object)
{
    ObjectFunction* function = AS_FUNCTION(object);

    if (!function->name) {
        printf("<lambda fn>");
    } else {
        printf("<fn '%s'>", AS_CSTRING(function->name));
    }
}

static void function_traverse(Object* object, GC* gc)
{
    ObjectFunction* function = AS_FUNCTION(object);
    GC_MarkObject(gc, (Object*)function->mod);
    GC_MarkObject(gc, (Object*)function->name);
    GC_MarkArray(gc, &function->chunk.constants);
    Object_GenericTraverse(object, gc);
}

static void function_free(Object* object, GC* gc)
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
    type->ToString = function_to_string;
    type->Print = function_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = function_traverse;
    type->Free = function_free;
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

static ObjectString* upvalue_to_string(Object* object, VM* vm)
{
    return String_FromCString(vm, "<upvalue>");
}

static void upvalue_print(Object* object)
{
    printf("<upvalue>");
}

static void upvalue_traverse(Object* object, GC* gc)
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
    type->ToString = upvalue_to_string;
    type->Print = upvalue_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = upvalue_traverse;
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

static ObjectString* closure_to_string(Object* object, VM* vm)
{
    return function_to_string((Object*)AS_CLOSURE(object)->function, vm);
}

static void closure_print(Object* object)
{
    function_print((Object*)AS_CLOSURE(object)->function);
}

static bool closure_call(Object* callee, uint8_t argCount, VM* vm)
{
    return call(vm, AS_CLOSURE(callee), argCount);
}

static void closure_traverse(Object* object, GC* gc)
{
    ObjectClosure* closure = AS_CLOSURE(object);
    GC_MarkObject(gc, (Object*)closure->function);
    for (size_t i = 0; i < closure->upvalueCount; i++) {
        GC_MarkObject(gc, (Object*)closure->upvalues[i]);
    }

    Object_GenericTraverse(object, gc);
}

static void closure_free(Object* object, GC* gc)
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
    type->ToString = closure_to_string;
    type->Print = closure_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = closure_call;
    type->Traverse = closure_traverse;
    type->Free = closure_free;
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

static ObjectString* bound_method_to_string(Object* object, VM* vm)
{
    return Object_ToString(AS_BOUND_METHOD(object)->method, vm);
}

static void bound_method_print(Object* object)
{
    Object_Print(AS_BOUND_METHOD(object)->method);
}

static bool bound_method_call(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectBoundMethod* bound = AS_BOUND_METHOD(callee);
    vm->coroutine->stackTop[-argCount - 1] = bound->receiver;
    return Object_Call(bound->method, argCount, vm);
}

static void bound_method_traverse(Object* object, GC* gc)
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
    type->ToString = bound_method_to_string;
    type->Print = bound_method_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = bound_method_call;
    type->Traverse = bound_method_traverse;
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