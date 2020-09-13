#include <stdio.h>

#include "vm.h"
#include "value.h"
#include "objstring.h"
#include "object.h"
#include "memory.h"

bool value_is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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
        case VALUE_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VALUE_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VALUE_NIL: return true;
        case VALUE_OBJ: return AS_OBJ(a) == AS_OBJ(b);
    }

    return false;
#endif
}

/*
 *  Thomas Wang, Integer Hash Functions
 *  https://gist.github.com/badboy/6267743
 */
uint32_t hash_bits(uint64_t hash)
{
    hash = ~hash + (hash << 18);
    hash = hash ^ (hash >> 31);
    hash = hash * 21;
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);

    //We only want lower 32-bits
    return (uint32_t)(hash & 0x3fffffff);
}

static uint32_t hash_number(double number)
{
    uint64_t bits;
    memcpy(&bits, &number, sizeof(double));
    return hash_bits(bits);
}

uint32_t value_hash(Value value)
{
#if NAN_BOXING
    if (IS_OBJ(value)) {
        return Object_Hash(AS_OBJ(value));
    }

    return hash_bits(value);
#else
    switch (value.type) {
        case VALUE_NIL: return 1;
        case VALUE_BOOL: return AS_BOOL(value) ? 2 : 0;
        case VALUE_NUMBER: return hash_number(AS_NUMBER(value));
        case VALUE_OBJ: return Object_Hash(AS_OBJ(value));
    }

    return 0;
#endif
}

void print_value(Value value)
{
#if NAN_BOXING
    if (IS_NIL(value)) {
        printf("nil");
    } else if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        Object_Print(AS_OBJ(value));
    }
#else
    switch (value.type) {
        case VALUE_NIL: printf("nil"); break;
        case VALUE_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
        case VALUE_NUMBER: printf("%g", AS_NUMBER(value)); break;
        case VALUE_OBJ: Object_Print(AS_OBJ(value)); break;
    }
#endif
}
