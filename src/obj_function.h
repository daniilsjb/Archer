#ifndef OBJFUNCTION_H
#define OBJFUNCTION_H

#include "object.h"
#include "chunk.h"
#include "table.h"

#define AS_FUNCTION(object) ((ObjectFunction*)object)
#define IS_FUNCTION(object, vm) (OBJ_TYPE(object) == vm->functionType)

#define VAL_AS_FUNCTION(value) (AS_FUNCTION(AS_OBJ(value)))
#define VAL_IS_FUNCTION(value, vm) (Object_ValueHasType(value, vm->functionType))

#define ALLOCATE_FUNCTION(vm) (AS_FUNCTION(ALLOCATE_OBJ(vm, vm->functionType)))

typedef struct ObjectString ObjectString;
typedef struct ObjectModule ObjectModule;

typedef struct ObjectFunction {
    Object base;
    ObjectString* name;
    ObjectModule* mod;

    size_t upvalueCount;
    Chunk chunk;
    int arity;
} ObjectFunction;

ObjectType* Function_NewType(VM* vm);
void Function_PrepareType(ObjectType* type, VM* vm);

ObjectFunction* Function_New(VM* vm);

#define AS_UPVALUE(object) ((ObjectUpvalue*)object)
#define IS_UPVALUE(object, vm) (OBJ_TYPE(object) == vm->upvalueType)

#define VAL_AS_UPVALUE(value) (AS_UPVALUE(AS_OBJ(value)))
#define VAL_IS_UPVALUE(value, vm) (Object_ValueHasType(value, vm->upvalueType))

#define ALLOCATE_UPVALUE(vm) (AS_UPVALUE(ALLOCATE_OBJ(vm, vm->upvalueType)))

typedef struct ObjectUpvalue {
    Object base;
    Value* location;
    Value closed;
    struct ObjectUpvalue* next;
} ObjectUpvalue;

ObjectType* Upvalue_NewType(VM* vm);
void Upvalue_PrepareType(ObjectType* type, VM* vm);

ObjectUpvalue* Upvalue_New(VM* vm, Value* slot);

#define AS_CLOSURE(object) ((ObjectClosure*)object)
#define IS_CLOSURE(object, vm) (OBJ_TYPE(object) == vm->closureType)

#define VAL_AS_CLOSURE(value) (AS_CLOSURE(AS_OBJ(value)))
#define VAL_IS_CLOSURE(value, vm) (Object_ValueHasType(value, vm->closureType))

#define ALLOCATE_CLOSURE(vm) (AS_CLOSURE(ALLOCATE_OBJ(vm, vm->closureType)))

typedef struct ObjectClosure {
    Object base;
    ObjectFunction* function;
    ObjectUpvalue** upvalues;
    size_t upvalueCount;
} ObjectClosure;

ObjectType* Closure_NewType(VM* vm);
void Closure_PrepareType(ObjectType* type, VM* vm);

ObjectClosure* Closure_New(VM* vm, ObjectFunction* function);

#define AS_BOUND_METHOD(object) ((ObjectBoundMethod*)object)
#define IS_BOUND_METHOD(object, vm) (OBJ_TYPE(object) == vm->boundMethodType)

#define VAL_AS_BOUND_METHOD(value) (AS_BOUND_METHOD(AS_OBJ(value)))
#define VAL_IS_BOUND_METHOD(value, vm) (Object_ValueHasType(value, vm->boundMethodType))

#define ALLOCATE_BOUND_METHOD(vm) (AS_BOUND_METHOD(ALLOCATE_OBJ(vm, vm->boundMethodType)))

typedef struct ObjectBoundMethod {
    Object base;
    Value receiver;
    Object* method;
} ObjectBoundMethod;

ObjectType* BoundMethod_NewType(VM* vm);
void BoundMethod_PrepareType(ObjectType* type, VM* vm);

ObjectBoundMethod* BoundMethod_New(VM* vm, Value receiver, Object* method);

#endif
