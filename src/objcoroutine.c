#include <stdio.h>

#include "objcoroutine.h"
#include "objfunction.h"
#include "objstring.h"
#include "objmodule.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "library.h"

static ObjectString* coroutine_function_to_string(Object* object, VM* vm)
{
    ObjectFunction* function = AS_COROUTINE(object)->closure->function;

    if (!function->name) {
        return String_FromCString(vm, "<coroutine function>");
    } else {
        char cstring[100];
        snprintf(cstring, 100, "<coroutine '%s' function>", function->name->chars);
        return String_FromCString(vm, cstring);
    }
}

static void coroutine_function_print(Object* object)
{
    ObjectFunction* function = AS_COROUTINE(object)->closure->function;

    if (!function->name) {
        printf("<coroutine function>");
    } else {
        printf("<coroutine '%s' function>", function->name->chars);
    }
}

static bool coroutine_function_call(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectClosure* closure = AS_COROUTINE_FUNCTION(callee)->closure;
    if (argCount != closure->function->arity) {
        runtime_error(vm, "Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    vm->coroutine->stackTop[-argCount - 1] = OBJ_VAL(Coroutine_NewFromStack(vm, closure, vm->coroutine->stackTop - argCount - 1, argCount));
    vm->coroutine->stackTop -= argCount;
    return true;
}

static void coroutine_function_traverse(Object* object, GC* gc)
{
    GC_MarkObject(gc, (Object*)AS_COROUTINE_FUNCTION(object)->closure);
    Object_GenericTraverse(object, gc);
}

ObjectType* CoroutineFunction_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "CoroutineFunction";
    type->size = sizeof(ObjectCoroutineFunction);
    type->flags = 0x0;
    type->ToString = coroutine_function_to_string;
    type->Print = coroutine_function_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = coroutine_function_call;
    type->Traverse = coroutine_function_traverse;
    type->Free = Object_GenericFree;
    return type;
}

void CoroutineFunction_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectCoroutineFunction* CoroutineFunction_New(VM* vm, ObjectClosure* closure)
{
    ObjectCoroutineFunction* coroutine = ALLOCATE_COROUTINE_FUNCTION(vm);
    coroutine->closure = closure;
    return coroutine;
}

static bool method_done(VM* vm, Value* args)
{
    args[-1] = BOOL_VAL(Coroutine_IsDone(VAL_AS_COROUTINE(args[-1])));
    return true;
}

static void coroutine_push(ObjectCoroutine* coroutine, Value value)
{
    *coroutine->stackTop = value;
    coroutine->stackTop++;
}

static Value coroutine_pop(ObjectCoroutine* coroutine)
{
    coroutine->stackTop--;
    return *coroutine->stackTop;
}

static void push_call_frame(ObjectCoroutine* coroutine, ObjectClosure* closure, uint8_t argCount)
{
    CallFrame* frame = &coroutine->frames[coroutine->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = coroutine->stackTop - argCount - 1;
}

static bool call_routine(VM* vm, ObjectCoroutine* coroutine, ObjectClosure* closure, uint8_t argCount)
{
    if (argCount != closure->function->arity) {
        runtime_error(vm, "Expected %d arguments but got %d", closure->function->arity, argCount);
        return false;
    }

    if (coroutine->frameCount == FRAMES_MAX) {
        runtime_error(vm, "Stack overflow.");
        return false;
    }

    push_call_frame(coroutine, closure, argCount);
    return true;
}

static ObjectString* coroutine_to_string(Object* object, VM* vm)
{
    ObjectFunction* function = AS_COROUTINE(object)->closure->function;

    if (!function->name) {
        return String_FromCString(vm, "<coroutine>");
    } else {
        char cstring[100];
        snprintf(cstring, 100, "<coroutine '%s'>", function->name->chars);
        return String_FromCString(vm, cstring);
    }
}

static void coroutine_print(Object* object)
{
    ObjectFunction* function = AS_COROUTINE(object)->closure->function;

    if (!function->name) {
        printf("<coroutine>");
    } else {
        printf("<coroutine '%s'>", function->name->chars);
    }
}

static bool coroutine_call(Object* callee, uint8_t argCount, VM* vm)
{
    if (argCount > 1) {
        runtime_error(vm, "Expected 0 or 1 argument but got %d.", argCount);
        return false;
    }

    ObjectCoroutine* coroutine = AS_COROUTINE(callee);
    if (Coroutine_IsDone(coroutine)) {
        runtime_error(vm, "Cannot resume coroutine that has already finished.");
        return false;
    }

    Value value = argCount == 1 ? vm_pop(vm) : NIL_VAL();
    vm_pop(vm);

    coroutine->transfer = vm->coroutine;
    vm->coroutine = coroutine;

    if (coroutine->started) {
        vm_push(vm, value);
    }

    coroutine->started = true;
    return true;
}

static void coroutine_traverse(Object* object, GC* gc)
{
    ObjectCoroutine* coroutine = AS_COROUTINE(object);

    for (Value* slot = coroutine->stack; slot < coroutine->stackTop; slot++) {
        GC_MarkValue(gc, *slot);
    }

    for (size_t i = 0; i < coroutine->frameCount; i++) {
        GC_MarkObject(gc, (Object*)coroutine->frames[i].closure);
    }

    for (ObjectUpvalue* upvalue = coroutine->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        GC_MarkObject(gc, (Object*)upvalue);
    }

    GC_MarkObject(gc, (Object*)coroutine->closure);
    GC_MarkObject(gc, (Object*)coroutine->transfer);
    Object_GenericTraverse(object, gc);
}

ObjectType* Coroutine_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Coroutine";
    type->size = sizeof(ObjectCoroutine);
    type->flags = 0x0;
    type->ToString = coroutine_to_string;
    type->Print = coroutine_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->Call = coroutine_call;
    type->Traverse = coroutine_traverse;
    type->Free = Object_GenericFree;
    return type;
}

void Coroutine_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "done", method_done, 0);
}

static void reset_stack(ObjectCoroutine* coroutine)
{
    coroutine->stackTop = coroutine->stack;
    coroutine->frameCount = 0;
    coroutine->openUpvalues = NULL;
}

static ObjectCoroutine* coroutine_new(VM* vm, ObjectClosure* closure)
{
    ObjectCoroutine* coroutine = ALLOCATE_COROUTINE(vm);
    coroutine->closure = closure;
    coroutine->transfer = NULL;
    coroutine->started = false;
    reset_stack(coroutine);
    return coroutine;
}

ObjectCoroutine* Coroutine_New(VM* vm, ObjectClosure* closure)
{
    ObjectCoroutine* coroutine = coroutine_new(vm, closure);
    coroutine_push(coroutine, OBJ_VAL(coroutine));
    push_call_frame(coroutine, closure, 0);
    return coroutine;
}

ObjectCoroutine* Coroutine_NewFromStack(VM* vm, ObjectClosure* closure, Value* slot, uint8_t argCount)
{
    ObjectCoroutine* coroutine = coroutine_new(vm, closure);
    for (Value* value = slot; value < vm->coroutine->stackTop; value++) {
        coroutine_push(coroutine, *value);
    }
    push_call_frame(coroutine, closure, argCount);
    return coroutine;
}

void Coroutine_Error(ObjectCoroutine* coroutine)
{
    reset_stack(coroutine);
}

bool Coroutine_Call(VM* vm, ObjectCoroutine* coroutine, ObjectClosure* callee, uint8_t argCount)
{
    if (argCount != callee->function->arity) {
        runtime_error(vm, "Expected %d arguments but got %d", callee->function->arity, argCount);
        return false;
    }

    if (coroutine->frameCount == FRAMES_MAX) {
        runtime_error(vm, "Stack overflow.");
        return false;
    }

    push_call_frame(coroutine, callee, argCount);
    return true;
}

bool Coroutine_CallValue(VM* vm, ObjectCoroutine* coroutine, Value callee, uint8_t argCount)
{
    if (!IS_OBJ(callee)) {
        runtime_error(vm, "Can only call objects.");
        return false;
    }

    Object* object = AS_OBJ(callee);
    if (!OBJ_TYPE(object)->Call) {
        runtime_error(vm, "Objects of type '%s' are not callable.", OBJ_TYPE(object)->name);
        return false;
    }

    return Object_Call(object, argCount, vm);
}

bool Coroutine_IsDone(ObjectCoroutine* coroutine)
{
    return coroutine->frameCount == 0;
}

void Coroutine_Run(VM* vm, ObjectCoroutine* coroutine)
{
    coroutine->transfer = vm->coroutine;
    coroutine->started = true;
    vm->coroutine = coroutine;
}