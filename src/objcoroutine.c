#include <stdio.h>

#include "objcoroutine.h"
#include "objfunction.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "library.h"

static ObjectString* coroutine_to_string(Object* object, VM* vm)
{
    ObjectFunction* function = AS_COROUTINE(object)->closure->function;

    if (!function->name) {
        return String_FromCString(vm, "<coroutine fn>");
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
        printf("<coroutine fn>");
    } else {
        printf("<coroutine '%s'>", function->name->chars);
    }
}

static void set_coroutine_args(VM* vm, ObjectCoroutineInstance* coroutine, Value* args, size_t argCount)
{
    coroutine->stackSize = argCount;
    coroutine->stackCopy = Mem_Allocate(&vm->gc, sizeof(Value) * argCount);
    memcpy(coroutine->stackCopy, args, sizeof(Value) * argCount);
}

static bool coroutine_call(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectClosure* closure = AS_COROUTINE(callee)->closure;
    if (argCount != closure->function->arity) {
        runtime_error(vm, "Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    ObjectCoroutineInstance* instance = CoroutineInstance_New(vm, closure);
    vm->stackTop[-argCount - 1] = OBJ_VAL(instance);
    set_coroutine_args(vm, instance, vm->stackTop - argCount, argCount);
    vm->stackTop -= argCount;
    return true;
}

static void coroutine_traverse(Object* object, GC* gc)
{
    GC_MarkObject(gc, (Object*)AS_COROUTINE(object)->closure);
    Object_GenericTraverse(object, gc);
}

static void coroutine_free(Object* object, GC* gc)
{
    Object_Deallocate(gc, object);
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
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = coroutine_call;
    type->Traverse = coroutine_traverse;
    type->Free = coroutine_free;
    return type;
}

void Coroutine_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectCoroutine* Coroutine_New(VM* vm, ObjectClosure* closure)
{
    ObjectCoroutine* coroutine = ALLOCATE_COROUTINE(vm);
    coroutine->closure = closure;
    return coroutine;
}

static bool method_done(VM* vm, Value* args)
{
    args[-1] = BOOL_VAL(VAL_AS_COROUTINE_INSTANCE(args[-1])->done);
    return true;
}

static ObjectString* coroutine_instance_to_string(Object* object, VM* vm)
{
    ObjectFunction* function = AS_COROUTINE_INSTANCE(object)->closure->function;

    if (!function->name) {
        return String_FromCString(vm, "<coroutine instance>");
    } else {
        char cstring[100];
        snprintf(cstring, 100, "<coroutine '%s' instance>", function->name->chars);
        return String_FromCString(vm, cstring);
    }
}

static void coroutine_instance_print(Object* object)
{
    ObjectFunction* function = AS_COROUTINE_INSTANCE(object)->closure->function;

    if (!function->name) {
        printf("<coroutine instance>");
    } else {
        printf("<coroutine '%s' instance>", function->name->chars);
    }
}

static bool coroutine_instance_call(Object* callee, uint8_t argCount, VM* vm)
{
    if (argCount > 1) {
        runtime_error(vm, "Expected 0 or 1 argument but got %d.", argCount);
        return false;
    }

    ObjectCoroutineInstance* coroutine = AS_COROUTINE_INSTANCE(callee);
    if (coroutine->done) {
        runtime_error(vm, "Cannot resume coroutine that has already finished.");
        return false;
    }

    Value value = argCount == 1 ? vm_pop(vm) : NIL_VAL();

    call(vm, coroutine->closure, coroutine->closure->function->arity);

    CallFrame* frame = &vm->frames[vm->frameCount - 1];
    frame->ip = coroutine->ip;
    frame->slots = vm->stackTop - 1;

    memcpy(frame->slots + 1, coroutine->stackCopy, sizeof(Value) * coroutine->stackSize);
    vm->stackTop += coroutine->stackSize;

    if (coroutine->started) {
        vm_push(vm, value);
    }

    coroutine->started = true;
    return true;
}

static void coroutine_instance_traverse(Object* object, GC* gc)
{
    ObjectCoroutineInstance* coroutine = AS_COROUTINE_INSTANCE(object);

    Value* value = coroutine->stackCopy;
    for (size_t i = 0; i < coroutine->stackSize; i++) {
        GC_MarkValue(gc, *value);
        value++;
    }

    GC_MarkObject(gc, (Object*)coroutine->closure);
    Object_GenericTraverse(object, gc);
}

static void coroutine_instance_free(Object* object, GC* gc)
{
    ObjectCoroutineInstance* coroutine = AS_COROUTINE_INSTANCE(object);
    Mem_Deallocate(gc, coroutine->stackCopy, sizeof(Value) * coroutine->stackSize);
    Object_Deallocate(gc, object);
}

ObjectType* CoroutineInstance_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Coroutine";
    type->size = sizeof(ObjectCoroutineInstance);
    type->flags = 0x0;
    type->ToString = coroutine_instance_to_string;
    type->Print = coroutine_instance_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->Call = coroutine_instance_call;
    type->Traverse = coroutine_instance_traverse;
    type->Free = coroutine_instance_free;
    return type;
}

void CoroutineInstance_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "done", method_done, 0);
}

ObjectCoroutineInstance* CoroutineInstance_New(VM* vm, ObjectClosure* closure)
{
    ObjectCoroutineInstance* coroutine = ALLOCATE_COROUTINE_INSTANCE(vm);
    coroutine->closure = closure;
    coroutine->ip = closure->function->chunk.code;

    coroutine->stackSize = 0;
    coroutine->stackCopy = NULL;

    coroutine->started = false;
    coroutine->done = false;
    return coroutine;
}
