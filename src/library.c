#include <math.h>
#include <time.h>

#include "library.h"
#include "vm.h"
#include "value.h"
#include "object.h"
#include "objstring.h"
#include "objnative.h"
#include "objfunction.h"

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
    vm_push(vm, OBJ_VAL(String_FromCString(vm, name)));
    vm_push(vm, OBJ_VAL(Native_New(vm, function, arity)));
    table_put(vm, &vm->globals, VAL_AS_STRING(vm->stack[0]), vm->stack[1]);
    vm_pop(vm);
    vm_pop(vm);
}

void Library_Init(VM* vm)
{
    vm->stringType = String_NewType(vm);
    vm->nativeType = Native_NewType(vm);
    vm->functionType = Function_NewType(vm);
    vm->upvalueType = Upvalue_NewType(vm);
    vm->closureType = Closure_NewType(vm);
    vm->boundMethodType = BoundMethod_NewType(vm);

    vm->initString = String_FromCString(vm, "init");

    String_PrepareType(vm->stringType, vm);
    Native_PrepareType(vm->nativeType, vm);
    Function_PrepareType(vm->functionType, vm);
    Upvalue_PrepareType(vm->upvalueType, vm);
    Closure_PrepareType(vm->closureType, vm);
    BoundMethod_PrepareType(vm->boundMethodType, vm);

    vm_push(vm, OBJ_VAL(String_FromCString(vm, "String")));
    table_put(vm, &vm->globals, String_FromCString(vm, "String"), OBJ_VAL(vm->stringType));
    vm_pop(vm);

    define_native(vm, "clock", clock_native, 0);
    define_native(vm, "abs", abs_native, 1);
    define_native(vm, "pow", pow_native, 2);
    define_native(vm, "typeOf", typeof_native, 1);
}
