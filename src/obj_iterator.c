#include <stdio.h>

#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "library.h"

#include "obj_iterator.h"
#include "obj_string.h"
#include "obj_native.h"

static ObjectString* iterator_to_string(Object* object, VM* vm)
{
    return String_FromCString(vm, "<iterator>");
}

static void iterator_print(Object* object)
{
    printf("<iterator>");
}

static void iterator_traverse(Object* object, GC* gc)
{
    ObjectIterator* iterator = AS_ITERATOR(object);
    GC_MarkObject(gc, iterator->container);
    Object_GenericTraverse(object, gc);
}

ObjectType* Iterator_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Iterator";
    type->size = sizeof(ObjectIterator);
    type->flags = 0x0;
    type->ToString = iterator_to_string;
    type->Print = iterator_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->MakeIterator = NULL;
    type->Call = NULL;
    type->Traverse = iterator_traverse;
    type->Free = Object_GenericFree;
    return type;
}

void Iterator_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectIterator* Iterator_New(VM* vm)
{
    ObjectIterator* iterator = ALLOCATE_ITERATOR(vm);
    return iterator;
}

bool Iterator_ReachedEnd(ObjectIterator* iterator)
{
    return iterator->ReachedEnd(iterator);
}

void Iterator_Advance(ObjectIterator* iterator)
{
    iterator->Advance(iterator);
}

Value Iterator_GetValue(VM* vm, ObjectIterator* iterator)
{
    return iterator->GetValue(vm, iterator);
}
