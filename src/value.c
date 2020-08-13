#include <stdio.h>

#include "value.h"
#include "object.h"
#include "memory.h"

void value_array_init(ValueArray* array)
{
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void value_array_write(ValueArray* array, Value value)
{
    if (array->count >= array->capacity) {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void value_array_free(ValueArray* array)
{
    array->values = FREE_ARRAY(Value, array->values, array->count);
    value_array_init(array);
}

bool values_equal(Value a, Value b)
{
#if NAN_BOXING
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }

    return a == b;
#else
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case VALUE_BOOL: return AS_BOOL(a) == AS_BOOL(b); break;
        case VALUE_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b); break;
        case VALUE_NIL: return true;
        case VALUE_OBJ: return AS_OBJ(a) == AS_OBJ(b);
    }

    return false;
#endif
}

void print_value(Value value)
{
#if NAN_BOXING
    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        printf("nil");
    } else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        print_object(value);
    }
#else
    switch (value.type) {
        case VALUE_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
        case VALUE_NUMBER: printf("%g", AS_NUMBER(value)); break;
        case VALUE_NIL: printf("nil"); break;
        case VALUE_OBJ: print_object(value); break;
    }
#endif
}
