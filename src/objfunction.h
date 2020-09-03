#ifndef OBJFUNCTION_H
#define OBJFUNCTION_H

#include "object.h"
#include "chunk.h"

#define AS_FUNCTION(object) ((ObjFunction*)object)
#define IS_FUNCTION(object, vm) (OBJ_TYPE(object) == vm->functionType)

#define VAL_AS_FUNCTION(value) (AS_FUNCTION(AS_OBJ(value)))
#define VAL_IS_FUNCTION(value, vm) (value_is_object_of_type(value, vm->functionType))

#define ALLOCATE_FUNCTION(vm) (ALLOCATE_OBJ(vm, ObjFunction, vm->functionType))

typedef struct ObjString ObjString;

typedef struct ObjFunction {
    Object base;
    Chunk chunk;
    int arity;
    size_t upvalueCount;
    ObjString* name;
} ObjFunction;

ObjectType* new_function_type(VM* vm);
void prepare_function_type(ObjectType* type, VM* vm);
void free_function_type(ObjectType* type, VM* vm);

ObjFunction* new_function(VM* vm);

#define AS_UPVALUE(object) ((ObjUpvalue*)object)
#define IS_UPVALUE(object, vm) (OBJ_TYPE(object) == vm->upvalueType)

#define VAL_AS_UPVALUE(value) (AS_UPVALUE(AS_OBJ(value)))
#define VAL_IS_UPVALUE(value, vm) (value_is_object_of_type(value, vm->upvalueType))

#define ALLOCATE_UPVALUE(vm) (ALLOCATE_OBJ(vm, ObjUpvalue, vm->upvalueType))

typedef struct ObjUpvalue {
    Object base;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

ObjectType* new_upvalue_type(VM* vm);
void prepare_upvalue_type(ObjectType* type, VM* vm);
void free_upvalue_type(ObjectType* type, VM* vm);

ObjUpvalue* new_upvalue(VM* vm, Value* slot);

#define AS_CLOSURE(object) ((ObjClosure*)object)
#define IS_CLOSURE(object, vm) (OBJ_TYPE(object) == vm->closureType)

#define VAL_AS_CLOSURE(value) (AS_CLOSURE(AS_OBJ(value)))
#define VAL_IS_CLOSURE(value, vm) (value_is_object_of_type(value, vm->closureType))

#define ALLOCATE_CLOSURE(vm) (ALLOCATE_OBJ(vm, ObjClosure, vm->closureType))

typedef struct ObjClosure {
    Object base;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    size_t upvalueCount;
} ObjClosure;

ObjectType* new_closure_type(VM* vm);
void prepare_closure_type(ObjectType* type, VM* vm);
void free_closure_type(ObjectType* type, VM* vm);

ObjClosure* new_closure(VM* vm, ObjFunction* function);

#endif
