#ifndef OBJSTRING_H
#define OBJSTRING_H

#include "object.h"

#define AS_STRING(object)  ((ObjString*)object)
#define AS_CSTRING(object) (AS_STRING(object)->chars)
#define IS_STRING(object)  (OBJ_TYPE(object) == &StringType)

#define VAL_AS_STRING(value)  (AS_STRING(AS_OBJ(value)))
#define VAL_AS_CSTRING(value) (VAL_AS_STRING(value)->chars)
#define VAL_IS_STRING(value)  (value_is_object_of_type((value), &StringType))

extern ObjectType StringType;

typedef struct ObjString {
    Object base;
    uint32_t hash;
    size_t length;
    char chars[];
} ObjString;

ObjString* new_string(VM* vm, size_t length);
ObjString* copy_string(VM* vm, const char* chars, size_t length);
ObjString* concatenate_strings(VM* vm, ObjString* a, ObjString* b);

#endif
