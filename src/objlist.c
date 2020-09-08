#include <stdio.h>
#include <math.h>

#include "objlist.h"
#include "vm.h"
#include "objstring.h"
#include "objnative.h"
#include "library.h"

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
    //TODO: Figure out a string builder protocol to efficiently convert arrays to strings
    return String_FromCString(vm, "<list>");
}

static void list_print(Object* object)
{
    ObjectList* list = AS_LIST(object);
    size_t count = list->elements.count;

    printf("[");
    for (size_t i = 0; i < count; i++) {
        print_value(list->elements.data[i]);

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
            runtime_error(vm, "Index out of bounds.");
            return NULL;
        }

        return &list->elements.data[list->elements.count + index];
    } else {
        if (index >= list->elements.count) {
            runtime_error(vm, "Index out of bounds.");
            return NULL;
        }

        return &list->elements.data[index];
    }
}

static bool list_get_subscript(Object* object, Value index, VM* vm, Value* result)
{
    if (!IS_NUMBER(index)) {
        runtime_error(vm, "Can only subscript lists with numbers.");
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
        runtime_error(vm, "Can only subscript lists with numbers.");
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
    type->Hash = NULL;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = list_get_subscript;
    type->SetSubscript = list_set_subscript;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = list_traverse;
    type->Free = list_free;
    return type;
}

static void define_list_method(ObjectType* type, VM* vm, const char* name, NativeFn function, int arity)
{
    vm_push(vm, OBJ_VAL(String_FromCString(vm, name)));
    vm_push(vm, OBJ_VAL(Native_New(vm, function, arity)));
    table_put(vm, &type->methods, VAL_AS_STRING(vm->stack[0]), vm->stack[1]);
    vm_pop(vm);
    vm_pop(vm);
}

void List_PrepareType(ObjectType* type, VM* vm)
{
    define_list_method(type, vm, "append", method_append, 1);
    define_list_method(type, vm, "pop", method_pop, 0);
    define_list_method(type, vm, "length", method_length, 0);
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
