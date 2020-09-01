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
    ObjNative* native = AS_NATIVE(callee);
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

static void traverse_native(Object* object, GC* gc)
{
}

static void free_native(Object* object, GC* gc)
{
    FREE(gc, ObjNative, object);
}

ObjectType NativeType = {
    .print = print_native,
    .call = call_native,
    .traverse = traverse_native,
    .free = free_native
};

ObjNative* new_native(VM* vm, NativeFn function, int arity)
{
    ObjNative* native = ALLOCATE_NATIVE(vm);
    native->function = function;
    native->arity = arity;
    return native;
}
