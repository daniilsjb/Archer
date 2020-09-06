#include <stdio.h>

#include "objnative.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static void print_native(Object* object)
{
    printf("<native fn>");
}

static bool call_native(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectNative* native = AS_NATIVE(callee);
    if (native->arity != argCount) {
        runtime_error(vm, "Expected %d arguments but got %d.", native->arity, argCount);
        return false;
    }

    if (native->function(vm, vm->stackTop - argCount)) {
        vm->stackTop -= (uint64_t)argCount;
        return true;
    } else {
        runtime_error(vm, AS_CSTRING(vm->stackTop[-argCount - 1]));
        return false;
    }
}

ObjectType* Native_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Native";
    type->size = sizeof(ObjectNative);
    type->Print = print_native;
    type->Hash = NULL;
    type->Call = call_native;
    type->GetMethod = NULL;
    type->Traverse = Object_GenericTraverse;
    type->Free = Object_GenericFree;
    return type;
}

void Native_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectNative* Native_New(VM* vm, NativeFn function, int arity)
{
    ObjectNative* native = ALLOCATE_NATIVE(vm);
    native->function = function;
    native->arity = arity;
    return native;
}
