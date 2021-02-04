#include <math.h>
#include <time.h>

#include "library.h"
#include "vm.h"
#include "value.h"

#include "object.h"
#include "obj_string.h"
#include "obj_native.h"
#include "obj_function.h"
#include "obj_coroutine.h"
#include "obj_list.h"
#include "obj_map.h"
#include "obj_module.h"
#include "obj_iterator.h"
#include "obj_range.h"
#include "obj_tuple.h"

bool Library_Error(VM* vm, const char* message, Value* args)
{
    args[-1] = OBJ_VAL(String_FromCString(vm, message));
    return false;
}

static bool clock_native(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

static bool abs_native(VM* vm, Value* args)
{
    if (!IS_NUMBER(args[0])) {
        return Library_Error(vm, "Expected a numeric value.", args);
    }

    double number = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(fabs(number));
    return true;
}

static bool pow_native(VM* vm, Value* args)
{
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return Library_Error(vm, "Expected numeric values.", args);
    }

    double x = AS_NUMBER(args[0]);
    double y = AS_NUMBER(args[1]);
    args[-1] = NUMBER_VAL(pow(x, y));
    return true;
}

static bool typeof_native(VM* vm, Value* args)
{
    if (!IS_OBJ(args[0])) {
        return Library_Error(vm, "Expected an object.", args);
    }

    args[-1] = OBJ_VAL(AS_OBJ(args[0])->type);
    return true;
}

static void define_native(VM* vm, const char* name, NativeFn function, int arity)
{
    Vm_PushTemporary(vm, OBJ_VAL(String_FromCString(vm, name)));
    Vm_PushTemporary(vm, OBJ_VAL(Native_New(vm, function, arity)));
    Table_Put(vm, &vm->builtins, Vm_PeekTemporary(vm, 1), Vm_PeekTemporary(vm, 0));
    Vm_PopTemporary(vm);
    Vm_PopTemporary(vm);
}

static void define_type(VM* vm, const char* name, ObjectType* type)
{
    Vm_PushTemporary(vm, OBJ_VAL(String_FromCString(vm, name)));
    Table_Put(vm, &vm->builtins, Vm_PeekTemporary(vm, 0), OBJ_VAL(type));
    Vm_PopTemporary(vm);
}

void Library_Init(VM* vm)
{
    vm->stringType = String_NewType(vm);
    vm->nativeType = Native_NewType(vm);
    vm->functionType = Function_NewType(vm);
    vm->upvalueType = Upvalue_NewType(vm);
    vm->closureType = Closure_NewType(vm);
    vm->boundMethodType = BoundMethod_NewType(vm);
    vm->coroutineFunctionType = CoroutineFunction_NewType(vm);
    vm->coroutineType = Coroutine_NewType(vm);
    vm->listType = List_NewType(vm);
    vm->mapType = Map_NewType(vm);
    vm->moduleType = Module_NewType(vm);
    vm->iteratorType = Iterator_NewType(vm);
    vm->rangeType = Range_NewType(vm);
    vm->tupleType = Tuple_NewType(vm);

    vm->initString = String_FromCString(vm, "init");

    String_PrepareType(vm->stringType, vm);
    Native_PrepareType(vm->nativeType, vm);
    Function_PrepareType(vm->functionType, vm);
    Upvalue_PrepareType(vm->upvalueType, vm);
    Closure_PrepareType(vm->closureType, vm);
    BoundMethod_PrepareType(vm->boundMethodType, vm);
    CoroutineFunction_PrepareType(vm->coroutineFunctionType, vm);
    Coroutine_PrepareType(vm->coroutineType, vm);
    List_PrepareType(vm->listType, vm);
    Map_PrepareType(vm->mapType, vm);
    Module_PrepareType(vm->moduleType, vm);
    Iterator_PrepareType(vm->iteratorType, vm);
    Range_PrepareType(vm->rangeType, vm);
    Tuple_PrepareType(vm->tupleType, vm);

    define_type(vm, "String", vm->stringType);
    define_type(vm, "Coroutine", vm->coroutineType);

    define_native(vm, "clock", clock_native, 0);
    define_native(vm, "abs", abs_native, 1);
    define_native(vm, "pow", pow_native, 2);
    define_native(vm, "typeof", typeof_native, 1);
}

void Library_DefineTypeMethod(ObjectType* type, VM* vm, const char* name, NativeFn function, int arity)
{
    Vm_PushTemporary(vm, OBJ_VAL(String_FromCString(vm, name)));
    Vm_PushTemporary(vm, OBJ_VAL(Native_New(vm, function, arity)));
    Table_Put(vm, &type->methods, Vm_PeekTemporary(vm, 1), Vm_PeekTemporary(vm, 0));
    Vm_PopTemporary(vm);
    Vm_PopTemporary(vm);
}
