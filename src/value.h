#ifndef VALUE_H
#define VALUE_H

#include <string.h>

#include "common.h"
#include "vector.h"

#define NAN_BOXING 1

typedef struct Object Object;

#if NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_UNDEFINED 0
#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3

typedef uint64_t Value;

#define UNDEFINED_VAL()   ((Value)(uint64_t)(QNAN | TAG_UNDEFINED))
#define NIL_VAL()         ((Value)(uint64_t)(QNAN | TAG_NIL))
#define FALSE_VAL()       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL()        ((Value)(uint64_t)(QNAN | TAG_TRUE))

#define BOOL_VAL(value)   ((value) ? TRUE_VAL() : FALSE_VAL())
#define NUMBER_VAL(value) (num_to_value(value))
#define OBJ_VAL(value)    ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(value)))

#define IS_UNDEFINED(value) ((value) == UNDEFINED_VAL())
#define IS_NIL(value)       ((value) == NIL_VAL())
#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL())
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJ(value)       (((value) & (SIGN_BIT | QNAN)) == (SIGN_BIT | QNAN))

#define AS_BOOL(value)   ((value) == TRUE_VAL())
#define AS_NUMBER(value) (value_to_num(value))
#define AS_OBJ(value)    ((Object*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

static inline double value_to_num(Value value)
{
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Value num_to_value(double num)
{
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else

typedef enum {
    VALUE_UNDEFINED,
    VALUE_NIL,
    VALUE_BOOL,
    VALUE_NUMBER,
    VALUE_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Object* object;
    } as;
} Value;

#define UNDEFINED_VAL() ((Value){ VALUE_UNDEFINED, { .number = 0 } })
#define NIL_VAL() ((Value){ VALUE_NIL, { .number = 0 } })
#define BOOL_VAL(value) ((Value){ VALUE_BOOL, { .boolean = value } })
#define NUMBER_VAL(value) ((Value){ VALUE_NUMBER, { .number = value } })
#define OBJ_VAL(value) ((Value){ VALUE_OBJ, { .object = (Object*)value } })

#define IS_UNDEFINED(value) ((value).type == VALUE_UNDEFINED)
#define IS_NIL(value) ((value).type == VALUE_NIL)
#define IS_BOOL(value) ((value).type == VALUE_BOOL)
#define IS_NUMBER(value) ((value).type == VALUE_NUMBER)
#define IS_OBJ(value) ((value).type == VALUE_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.object)

#endif

typedef VECTOR(Value) ValueArray;

bool Value_IsFalsey(Value value);
bool Value_Equal(Value a, Value b);

uint32_t Value_HashBits(uint64_t hash);
uint32_t Value_Hash(Value value);

void Value_Print(Value value);

#endif
