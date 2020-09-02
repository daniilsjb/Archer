#ifndef OBJSTRING_H
#define OBJSTRING_H

#include "object.h"

#define AS_STRING(object)  ((ObjString*)object)
#define AS_CSTRING(object) (AS_STRING(object)->chars)
#define IS_STRING(object, vm)  (OBJ_TYPE(object) == vm->stringType)

#define VAL_AS_STRING(value)  (AS_STRING(AS_OBJ(value)))
#define VAL_AS_CSTRING(value) (VAL_AS_STRING(value)->chars)
#define VAL_IS_STRING(value, vm)  (value_is_object_of_type((value), vm->stringType))

#define ALLOCATE_STRING(vm, length) (AS_STRING(allocate_object((vm), sizeof(ObjString) + (length) + 1, vm->stringType)))

typedef struct ObjString {
    Object base;
    uint32_t hash;
    size_t length;
    char chars[];
} ObjString;

ObjectType* new_string_type(VM* vm);
void free_string_type(ObjectType* type, VM* vm);

ObjString* new_string(VM* vm, size_t length);
ObjString* copy_string(VM* vm, const char* chars, size_t length);
ObjString* concatenate_strings(VM* vm, ObjString* a, ObjString* b);

#endif
