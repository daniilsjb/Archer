#ifndef OBJSTRING_H
#define OBJSTRING_H

#include "object.h"

#define AS_STRING(object)  ((ObjectString*)object)
#define AS_CSTRING(object) (AS_STRING(object)->chars)
#define IS_STRING(object, vm) (OBJ_TYPE(object) == vm->stringType)

#define VAL_AS_STRING(value)  (AS_STRING(AS_OBJ(value)))
#define VAL_AS_CSTRING(value) (VAL_AS_STRING(value)->chars)
#define VAL_IS_STRING(value, vm) (Object_ValueHasType((value), vm->stringType))

#define ALLOCATE_STRING(vm, length)                                         \
    (AS_STRING(Object_Allocate((vm), sizeof(ObjectString) + (length) + 1))) \

typedef struct ObjectString {
    Object base;
    uint32_t hash;
    size_t length;
    char chars[];
} ObjectString;

ObjectType* String_NewType(VM* vm);
void String_PrepareType(ObjectType* type, VM* vm);

ObjectString* String_New(VM* vm, size_t length);
ObjectString* String_Copy(VM* vm, const char* chars, size_t length);
ObjectString* String_FromCString(VM* vm, const char* chars);
ObjectString* String_FromNil(VM* vm);
ObjectString* String_FromBoolean(VM* vm, bool boolean);
ObjectString* String_FromNumber(VM* vm, double number);
ObjectString* String_Concatenate(VM* vm, ObjectString* a, ObjectString* b);
ObjectString* String_FromValue(VM* vm, Value value);

#endif
