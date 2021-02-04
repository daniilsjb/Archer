#include <stdio.h>
#include <math.h>

#include "vm.h"
#include "library.h"

#include "obj_tuple.h"
#include "obj_string.h"
#include "obj_native.h"
#include "obj_coroutine.h"
#include "obj_iterator.h"

static bool iterator_reached_end(ObjectIterator* iterator)
{
    ObjectTuple* tuple = AS_TUPLE(iterator->container);
    return (uintptr_t)((Value*)iterator->ptr - tuple->elements) >= tuple->length;
}

static void iterator_advance(ObjectIterator* iterator)
{
    ((Value*)iterator->ptr)++;
}

static Value iterator_get_value(VM* vm, ObjectIterator* iterator)
{
    return *((Value*)iterator->ptr);
}

static ObjectIterator* make_iterator(VM* vm, ObjectTuple* tuple)
{
    ObjectIterator* iterator = Iterator_New(vm);
    iterator->container = (Object*)tuple;
    iterator->ptr = tuple->elements;
    iterator->ReachedEnd = iterator_reached_end;
    iterator->Advance = iterator_advance;
    iterator->GetValue = iterator_get_value;
    return iterator;
}

static bool method_length(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)VAL_AS_TUPLE(args[-1])->length);
    return true;
}

static ObjectString* tuple_to_string(Object* object, VM* vm)
{
    Vm_Push(vm, OBJ_VAL(String_FromCString(vm, "(")));
    Value* accumulator = vm->coroutine->stackTop - 1;

    ObjectTuple* tuple = AS_TUPLE(object);
    size_t length = tuple->length;

    for (size_t i = 0; i < length; i++) {
        Vm_Push(vm, OBJ_VAL(String_FromValue(vm, tuple->elements[i])));
        *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(Vm_Peek(vm, 0))));
        Vm_Pop(vm);

        if (i != length - 1) {
            Vm_Push(vm, OBJ_VAL(String_FromCString(vm, ", ")));
            *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(Vm_Peek(vm, 0))));
            Vm_Pop(vm);
        }
    }
    Vm_Push(vm, OBJ_VAL(String_FromCString(vm, ")")));
    *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(Vm_Peek(vm, 0))));
    Vm_Pop(vm);

    return VAL_AS_STRING(Vm_Pop(vm));
}

static void tuple_print(Object* object)
{
    ObjectTuple* tuple = AS_TUPLE(object);

    printf("(");
    for (size_t i = 0; i < tuple->length; i++) {
        Value_Print(tuple->elements[i]);

        if (i != tuple->length - 1) {
            printf(", ");
        }
    }
    printf(")");
}

static Value* tuple_at_index(ObjectTuple* tuple, int index, VM* vm)
{
    if (index < 0) {
        if (abs(index) > tuple->length) {
            Vm_RuntimeError(vm, "Index out of bounds.");
            return NULL;
        }

        return &tuple->elements[tuple->length + index];
    } else {
        if (index >= tuple->length) {
            Vm_RuntimeError(vm, "Index out of bounds.");
            return NULL;
        }

        return &tuple->elements[index];
    }
}

static bool tuple_get_subscript(Object* object, Value index, VM* vm, Value* result)
{
    if (!IS_NUMBER(index)) {
        Vm_RuntimeError(vm, "Can only subscript lists with numbers.");
        return false;
    }

    ObjectTuple* tuple = AS_TUPLE(object);
    Value* element = tuple_at_index(tuple, (int)AS_NUMBER(index), vm);
    if (!element) {
        return false;
    }

    *result = *element;
    return true;
}

static bool tuple_set_subscript(Object* object, Value index, Value value, VM* vm)
{
    if (!IS_NUMBER(index)) {
        Vm_RuntimeError(vm, "Can only subscript lists with numbers.");
        return false;
    }

    ObjectTuple* tuple = AS_TUPLE(object);
    Value* element = tuple_at_index(tuple, (int)AS_NUMBER(index), vm);
    if (!element) {
        return false;
    }

    *element = value;
    return true;
}

static ObjectIterator* tuple_make_iterator(Object* object, VM* vm)
{
    return make_iterator(vm, AS_TUPLE(object));
}

static void tuple_traverse(Object* object, GC* gc)
{
    ObjectTuple* tuple = AS_TUPLE(object);
    for (size_t i = 0; i < tuple->length; i++) {
        GC_MarkValue(gc, tuple->elements[i]);
    }

    Object_GenericTraverse(object, gc);
}

static void tuple_free(Object* object, GC* gc)
{
    ObjectTuple* tuple = AS_TUPLE(object);
    Table_Free(gc, &tuple->base.fields);
    Mem_Deallocate(gc, tuple, sizeof(ObjectTuple) + sizeof(Value) * tuple->length);
}

ObjectType* Tuple_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Tuple";
    type->size = sizeof(ObjectTuple);
    type->flags = 0x0;
    type->ToString = tuple_to_string;
    type->Print = tuple_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = tuple_get_subscript;
    type->SetSubscript = tuple_set_subscript;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->MakeIterator = tuple_make_iterator;
    type->Call = NULL;
    type->Traverse = tuple_traverse;
    type->Free = tuple_free;
    return type;
}

void Tuple_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "length", method_length, 0);
}

ObjectTuple* Tuple_New(VM* vm, size_t length)
{
    ObjectTuple* tuple = ALLOCATE_TUPLE(vm, length);
    tuple->length = length;
    tuple->base.type = vm->tupleType;

    for (size_t i = 0; i < length; i++) {
        tuple->elements[i] = NIL_VAL();
    }

    return tuple;
}

void Tuple_SetElement(ObjectTuple* tuple, size_t index, Value value)
{
    tuple->elements[index] = value;
}
