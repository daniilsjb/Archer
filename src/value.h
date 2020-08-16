#ifndef VALUE_H
#define VALUE_H

#include "common.h"
#include "vector.h"

#define NAN_BOXING 1

struct VM;

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#if NAN_BOXING
#include <string.h>

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3

typedef uint64_t Value;

#define NIL_VAL()        ((Value)(uint64_t)(QNAN | TAG_NIL))
#define FALSE_VAL()      ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL()       ((Value)(uint64_t)(QNAN | TAG_TRUE))

#define BOOL_VAL(b)      ((b) ? TRUE_VAL() : FALSE_VAL())
#define NUMBER_VAL(num)  (num_to_value(num))
#define OBJ_VAL(obj)     ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)))

#define IS_NIL(value)    ((value) == NIL_VAL())
#define IS_BOOL(value)   (((value) | 1) == TRUE_VAL())
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define IS_OBJ(value)    (((value) & (SIGN_BIT | QNAN)) == (SIGN_BIT | QNAN))

#define AS_BOOL(value)   ((value) == TRUE_VAL())
#define AS_NUMBER(value) (value_to_num(value))
#define AS_OBJ(value)    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

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
    VALUE_BOOL,
    VALUE_NUMBER,
    VALUE_NIL,
    VALUE_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

#define BOOL_VAL(value) ((Value){ VALUE_BOOL, { .boolean = value } })
#define NUMBER_VAL(value) ((Value){ VALUE_NUMBER, { .number = value } })
#define NIL_VAL() ((Value){ VALUE_NIL, { .number = 0 } })
#define OBJ_VAL(object) ((Value){ VALUE_OBJ, { .obj = (Obj*)object } })

#define IS_BOOL(value) ((value).type == VALUE_BOOL)
#define IS_NUMBER(value) ((value).type == VALUE_NUMBER)
#define IS_NIL(value) ((value).type == VALUE_NIL)
#define IS_OBJ(object) ((object).type == VALUE_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(object) ((object).as.obj)

#endif

typedef VECTOR(Value) ValueArray;

//typedef struct {
//    size_t count;
//    size_t capacity;
//    Value* values;
//} ValueArray;
//
//void value_array_init(ValueArray* array);
//void value_array_free(struct VM* vm, ValueArray* array);
//
//void value_array_write(struct VM* vm, ValueArray* array, Value value);

bool value_is_falsey(Value value);
bool values_equal(Value a, Value b);

void print_value(Value value);

#endif
