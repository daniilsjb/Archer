#include <stdio.h>
#include <math.h>

#include "objarray.h"
#include "vm.h"
#include "objstring.h"
#include "objnative.h"
#include "library.h"

static bool method_init(VM* vm, Value* args)
{
    int length = (int)AS_NUMBER(args[0]);
    if (length < 0) {
        return Library_Error(vm, "Array size must be positive.", args);
    }

    //TODO: By creating a new array here we are discarding the object that was pre-allocated, find a way to fix this
    args[-1] = OBJ_VAL(Array_New(vm, (size_t)length));
    return true;
}

static bool method_length(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)VAL_AS_ARRAY(args[-1])->length);
    return true;
}

static ObjectString* array_to_string(Object* object, VM* vm)
{
    vm_push(vm, OBJ_VAL(String_FromCString(vm, "[")));
    Value* accumulator = vm->stackTop - 1;

    ObjectArray* array = AS_ARRAY(object);
    size_t length = array->length;

    for (size_t i = 0; i < length; i++) {
        vm_push(vm, OBJ_VAL(String_FromValue(vm, array->elements[i])));
        *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(vm->stackTop[-1])));
        vm_pop(vm);

        if (i != length - 1) {
            vm_push(vm, OBJ_VAL(String_FromCString(vm, ", ")));
            *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(vm->stackTop[-1])));
            vm_pop(vm);
        }
    }
    vm_push(vm, OBJ_VAL(String_FromCString(vm, "]")));
    *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(vm->stackTop[-1])));
    vm_pop(vm);

    return VAL_AS_STRING(vm_pop(vm));
}

static void array_print(Object* object)
{
    ObjectArray* array = AS_ARRAY(object);

    printf("[");
    for (size_t i = 0; i < array->length; i++) {
        print_value(array->elements[i]);

        if (i != array->length - 1) {
            printf(", ");
        }
    }
    printf("]");
}

static Value* array_at_index(ObjectArray* array, int index, VM* vm)
{
    if (index < 0) {
        if (abs(index) > array->length) {
            runtime_error(vm, "Index out of bounds.");
            return NULL;
        }

        return &array->elements[array->length + index];
    } else {
        if (index >= array->length) {
            runtime_error(vm, "Index out of bounds.");
            return NULL;
        }

        return &array->elements[index];
    }
}

static bool array_get_subscript(Object* object, Value index, VM* vm, Value* result)
{
    if (!IS_NUMBER(index)) {
        runtime_error(vm, "Can only subscript lists with numbers.");
        return false;
    }

    ObjectArray* array = AS_ARRAY(object);
    Value* element = array_at_index(array, (int)AS_NUMBER(index), vm);
    if (!element) {
        return false;
    }

    *result = *element;
    return true;
}

static bool array_set_subscript(Object* object, Value index, Value value, VM* vm)
{
    if (!IS_NUMBER(index)) {
        runtime_error(vm, "Can only subscript lists with numbers.");
        return false;
    }

    ObjectArray* array = AS_ARRAY(object);
    Value* element = array_at_index(array, (int)AS_NUMBER(index), vm);
    if (!element) {
        return false;
    }

    *element = value;
    return true;
}

static void array_traverse(Object* object, GC* gc)
{
    ObjectArray* array = AS_ARRAY(object);
    for (size_t i = 0; i < array->length; i++) {
        GC_MarkValue(gc, array->elements[i]);
    }

    Object_GenericTraverse(object, gc);
}

static void array_free(Object* object, GC* gc)
{
    ObjectArray* array = AS_ARRAY(object);
    table_free(gc, &array->base.fields);
    Mem_Deallocate(gc, array, sizeof(ObjectArray) + sizeof(Value) * array->length);
}

ObjectType* Array_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Array";
    type->size = sizeof(ObjectArray);
    type->flags = 0x0;
    type->ToString = array_to_string;
    type->Print = array_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = array_get_subscript;
    type->SetSubscript = array_set_subscript;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = array_traverse;
    type->Free = array_free;
    return type;
}

void Array_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "init", method_init, 1);
    Library_DefineTypeMethod(type, vm, "length", method_length, 0);
}

ObjectArray* Array_New(VM* vm, size_t length)
{
    ObjectArray* array = ALLOCATE_ARRAY(vm, length);
    array->length = length;
    array->base.type = vm->arrayType;

    for (size_t i = 0; i < length; i++) {
        array->elements[i] = NIL_VAL();
    }

    return array;
}
