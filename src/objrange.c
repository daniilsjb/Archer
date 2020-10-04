#include <stdio.h>

#include "objrange.h"
#include "objiterator.h"
#include "objstring.h"
#include "objlist.h"

#include "vm.h"
#include "library.h"

static bool iterator_reached_end(ObjectIterator* iterator)
{
    ObjectRange* range = AS_RANGE(iterator->container);
    double number = AS_NUMBER(iterator->value);

    if (range->step > 0.0) {
        return number >= range->end;
    } else {
        return number <= range->end;
    }
}

static void iterator_advance(ObjectIterator* iterator)
{
    ObjectRange* range = AS_RANGE(iterator->container);
    iterator->value = NUMBER_VAL((AS_NUMBER(iterator->value) + range->step));
}

static Value iterator_get_value(VM* vm, ObjectIterator* iterator)
{
    return iterator->value;
}

static ObjectIterator* make_iterator(VM* vm, ObjectRange* range)
{
    ObjectIterator* iterator = Iterator_New(vm);
    iterator->container = (Object*)range;
    iterator->value = NUMBER_VAL(range->begin);
    iterator->ReachedEnd = iterator_reached_end;
    iterator->Advance = iterator_advance;
    iterator->GetValue = iterator_get_value;
    return iterator;
}

static ObjectString* range_to_string(Object* object, VM* vm)
{
    ObjectRange* range = AS_RANGE(object);

    char chars[155] = { 0 };
    snprintf(chars, 155, "%g..%g:%g", range->begin, range->end, range->step);

    return String_FromCString(vm, chars);
}

static void range_print(Object* object)
{
    ObjectRange* range = AS_RANGE(object);
    printf("%g..%g:%g", range->begin, range->end, range->step);
}

static bool range_get_subscript(Object* object, Value index, VM* vm, Value* result)
{
    if (!IS_NUMBER(index)) {
        runtime_error(vm, "Can only subscript ranges with numbers.");
        return false;
    }

    ObjectRange* range = AS_RANGE(object);
    int n = (int)AS_NUMBER(index);

    int totalElements = (int)((range->end - range->begin) / range->step);
    if (n < -totalElements || n >= totalElements) {
        runtime_error(vm, "Range subscript out of range.");
        return false;
    }

    double high = (n >= 0) ? range->begin : range->end;
    double number = high + (double)n * range->step;

    *result = NUMBER_VAL(number);
    return true;
}

static ObjectIterator* range_make_iterator(Object* object, VM* vm)
{
    return make_iterator(vm, AS_RANGE(object));
}

ObjectType* Range_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Range";
    type->size = sizeof(ObjectRange);
    type->flags = 0x0;
    type->ToString = range_to_string;
    type->Print = range_print;
    type->Hash = Object_GenericHash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetSubscript = range_get_subscript;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->MakeIterator = range_make_iterator;
    type->Call = NULL;
    type->Traverse = Object_GenericTraverse;
    type->Free = Object_GenericFree;
    return type;
}

void Range_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectRange* Range_New(VM* vm, double begin, double end, double step)
{
    ObjectRange* range = ALLOCATE_RANGE(vm);
    range->begin = begin;
    range->end = end;
    range->step = step;
    return range;
}
