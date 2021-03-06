#include <stdio.h>
#include <math.h>

#include "vm.h"
#include "library.h"

#include "obj_list.h"
#include "obj_string.h"
#include "obj_native.h"
#include "obj_coroutine.h"
#include "obj_iterator.h"

static bool iterator_reached_end(ObjectIterator* iterator)
{
    ObjectList* list = AS_LIST(iterator->container);
    return (uintptr_t)((Value*)iterator->ptr - list->elements.data) >= list->elements.count;
}

static void iterator_advance(ObjectIterator* iterator)
{
    ((Value*)iterator->ptr)++;
}

static Value iterator_get_value(VM* vm, ObjectIterator* iterator)
{
    return *((Value*)iterator->ptr);
}

static ObjectIterator* make_iterator(VM* vm, ObjectList* list)
{
    ObjectIterator* iterator = Iterator_New(vm);
    iterator->container = (Object*)list;
    iterator->ptr = list->elements.data;
    iterator->ReachedEnd = iterator_reached_end;
    iterator->Advance = iterator_advance;
    iterator->GetValue = iterator_get_value;
    return iterator;
}

static bool method_append(VM* vm, Value* args)
{
    ObjectList* list = VAL_AS_LIST(args[-1]);
    VECTOR_PUSH(&vm->gc, ValueArray, &list->elements, Value, args[0]);
    args[-1] = NIL_VAL();
    return true;
}

static bool method_pop(VM* vm, Value* args)
{
    ObjectList* list = VAL_AS_LIST(args[-1]);
    if (list->elements.count == 0) {
        return Library_Error(vm, "Cannot pop an empty list.", args);
    }

    VECTOR_POP(&list->elements);
    args[-1] = NIL_VAL();
    return true;
}

static bool method_length(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)VAL_AS_LIST(args[-1])->elements.count);
    return true;
}

static ObjectString* list_to_string(Object* object, VM* vm)
{
    Vm_Push(vm, OBJ_VAL(String_FromCString(vm, "[")));
    Value* accumulator = vm->coroutine->stackTop - 1;

    ObjectList* list = AS_LIST(object);
    size_t count = list->elements.count;

    for (size_t i = 0; i < count; i++) {
        Vm_Push(vm, OBJ_VAL(String_FromValue(vm, list->elements.data[i])));
        *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(Vm_Peek(vm, 0))));
        Vm_Pop(vm);

        if (i != count - 1) {
            Vm_Push(vm, OBJ_VAL(String_FromCString(vm, ", ")));
            *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(Vm_Peek(vm, 0))));
            Vm_Pop(vm);
        }
    }
    Vm_Push(vm, OBJ_VAL(String_FromCString(vm, "]")));
    *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(Vm_Peek(vm, 0))));
    Vm_Pop(vm);

    return VAL_AS_STRING(Vm_Pop(vm));
}

static void list_print(Object* object)
{
    ObjectList* list = AS_LIST(object);
    size_t count = list->elements.count;

    printf("[");
    for (size_t i = 0; i < count; i++) {
        Value_Print(list->elements.data[i]);

        if (i != count - 1) {
            printf(", ");
        }
    }
    printf("]");
}

static Value* list_at_index(ObjectList* list, int index, VM* vm)
{
    if (index < 0) {
        if (abs(index) > list->elements.count) {
            Vm_RuntimeError(vm, "Index out of bounds.");
            return NULL;
        }

        return &list->elements.data[list->elements.count + index];
    } else {
        if (index >= list->elements.count) {
            Vm_RuntimeError(vm, "Index out of bounds.");
            return NULL;
        }

        return &list->elements.data[index];
    }
}

static bool list_get_subscript(Object* object, Value index, VM* vm, Value* result)
{
    if (!IS_NUMBER(index)) {
        Vm_RuntimeError(vm, "Can only subscript lists with numbers.");
        return false;
    }

    ObjectList* list = AS_LIST(object);
    Value* element = list_at_index(list, (int)AS_NUMBER(index), vm);
    if (!element) {
        return false;
    }

    *result = *element;
    return true;
}

static bool list_set_subscript(Object* object, Value index, Value value, VM* vm)
{
    if (!IS_NUMBER(index)) {
        Vm_RuntimeError(vm, "Can only subscript lists with numbers.");
        return false;
    }

    ObjectList* list = AS_LIST(object);
    Value* element = list_at_index(list, (int)AS_NUMBER(index), vm);
    if (!element) {
        return false;
    }

    *element = value;
    return true;
}

static ObjectIterator* list_make_iterator(Object* object, VM* vm)
{
    return make_iterator(vm, AS_LIST(object));
}

static void list_traverse(Object* object, GC* gc)
{
    GC_MarkArray(gc, &AS_LIST(object)->elements);
    Object_GenericTraverse(object, gc);
}

static void list_free(Object* object, GC* gc)
{
    VECTOR_FREE(gc, ValueArray, &AS_LIST(object)->elements, Value);
    Object_GenericFree(object, gc);
}

ObjectType* List_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "List";
    type->size = sizeof(ObjectList);
    type->flags = 0x0;
    type->ToString = list_to_string;
    type->Print = list_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = list_get_subscript;
    type->SetSubscript = list_set_subscript;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->MakeIterator = list_make_iterator;
    type->Call = NULL;
    type->Traverse = list_traverse;
    type->Free = list_free;
    return type;
}

void List_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "append", method_append, 1);
    Library_DefineTypeMethod(type, vm, "pop", method_pop, 0);
    Library_DefineTypeMethod(type, vm, "length", method_length, 0);
}

ObjectList* List_New(VM* vm)
{
    ObjectList* list = ALLOCATE_LIST(vm);
    VECTOR_INIT(ValueArray, &list->elements);
    return list;
}

void List_Append(ObjectList* list, Value value, VM* vm)
{
    VECTOR_PUSH(&vm->gc, ValueArray, &list->elements, Value, value);
}
