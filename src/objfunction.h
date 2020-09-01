#ifndef OBJFUNCTION_H
#define OBJFUNCTION_H

#include "object.h"
#include "chunk.h"

#define AS_FUNCTION(object) ((ObjFunction*)object)
#define IS_FUNCTION(object) (OBJ_TYPE(object) == &FunctionType)

#define VAL_AS_FUNCTION(value) (AS_FUNCTION(AS_OBJ(value)))
#define VAL_IS_FUNCTION(value) (value_is_object_of_type(value, &FunctionType))

typedef struct ObjString ObjString;

extern ObjectType FunctionType;

typedef struct ObjFunction {
    Object base;
    Chunk chunk;
    int arity;
    size_t upvalueCount;
    ObjString* name;
} ObjFunction;

ObjFunction* new_function(VM* vm);

#define AS_UPVALUE(object) ((ObjUpvalue*)object)
#define IS_UPVALUE(object) (OBJ_TYPE(object) == &UpvalueType)

#define VAL_AS_UPVALUE(value) (AS_UPVALUE(AS_OBJ(value)))
#define VAL_IS_UPVALUE(value) (value_is_object_of_type(value, &UpvalueType))

extern ObjectType UpvalueType;

typedef struct ObjUpvalue {
    Object base;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

ObjUpvalue* new_upvalue(VM* vm, Value* slot);

#define AS_CLOSURE(object) ((ObjClosure*)object)
#define IS_CLOSURE(object) (OBJ_TYPE(object) == &ClosureType)

#define VAL_AS_CLOSURE(value) (AS_CLOSURE(AS_OBJ(value)))
#define VAL_IS_CLOSURE(value) (value_is_object_of_type(value, &ClosureType))

extern ObjectType ClosureType;

typedef struct ObjClosure {
    Object base;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    size_t upvalueCount;
} ObjClosure;

ObjClosure* new_closure(VM* vm, ObjFunction* function);

#endif
