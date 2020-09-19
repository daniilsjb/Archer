#include <stdio.h>

#include "objcoroutine.h"
#include "objfunction.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "library.h"

static bool method_init(VM* vm, Value* args)
{
    if (!VAL_IS_CLOSURE(args[0], vm)) {
        return Library_Error(vm, "Expected a function.", args);
    }

    args[-1] = OBJ_VAL(Coroutine_New(vm, VAL_AS_CLOSURE(args[0])));
    return true;
}

static bool method_done(VM* vm, Value* args)
{
    args[-1] = BOOL_VAL(VAL_AS_COROUTINE(args[-1])->done);
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
    frame->closure = coroutine->closure;
    frame->ip = coroutine->closure->function->chunk.code;
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
        return String_FromCString(vm, "<coroutine instance>");
    } else {
        char cstring[100];
        snprintf(cstring, 100, "<coroutine '%s' instance>", function->name->chars);
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
    if (coroutine->done) {
        runtime_error(vm, "Cannot resume coroutine that has already finished.");
        return false;
    }

    Value value = argCount == 1 ? vm_pop(vm) : NIL_VAL();

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
    type->GetField = Object_GenericGetField;
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
    Library_DefineTypeMethod(type, vm, "init", method_init, 1);
    Library_DefineTypeMethod(type, vm, "done", method_done, 0);
}

static void reset_stack(ObjectCoroutine* coroutine)
{
    coroutine->stackTop = coroutine->stack;
    coroutine->frameCount = 0;
    coroutine->openUpvalues = NULL;
}

ObjectCoroutine* Coroutine_New(VM* vm, ObjectClosure* closure)
{
    ObjectCoroutine* coroutine = ALLOCATE_COROUTINE(vm);
    coroutine->closure = closure;

    reset_stack(coroutine);
    coroutine_push(coroutine, OBJ_VAL(coroutine));
    push_call_frame(coroutine, closure, 0);

    coroutine->started = false;
    coroutine->done = false;
    return coroutine;
}

ObjectCoroutine* Coroutine_NewWithArguments(VM* vm, ObjectClosure* closure, Value* args, size_t argCount)
{
    ObjectCoroutine* coroutine = Coroutine_New(vm, closure);
    for (size_t i = 0; i < argCount; i++) {
        coroutine_push(coroutine, args[i]);
    }
    return coroutine;
}
