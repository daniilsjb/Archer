#include <stdio.h>

#include "objnative.h"
#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static ObjectString* native_to_string(Object* object, VM* vm)
{
    return String_FromCString(vm, "<native fn>");
}

static void native_print(Object* object)
{
    printf("<native fn>");
}

static bool native_call(Object* callee, uint8_t argCount, VM* vm)
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
        runtime_error(vm, VAL_AS_CSTRING(vm->stackTop[-argCount - 1]));
        return false;
    }
}

ObjectType* Native_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Native";
    type->size = sizeof(ObjectNative);
    type->flags = 0x0;
    type->ToString = native_to_string;
    type->Print = native_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->Call = native_call;
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
