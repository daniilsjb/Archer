#ifndef OBJCLASS_H
#define OBJCLASS_H

#include "object.h"
#include "table.h"

typedef struct ObjString ObjString;
typedef struct ObjClosure ObjClosure;
typedef struct ObjClass ObjClass;

#define AS_INSTANCE(object) ((ObjInstance*)object)
#define IS_INSTANCE(object, vm) (OBJ_TYPE(object) == vm->instanceType || OBJ_TYPE(object) == vm->classType)

#define VAL_AS_INSTANCE(value) (AS_INSTANCE(AS_OBJ(value)))
#define VAL_IS_INSTANCE(value, vm) (value_is_object_of_type(value, vm->instanceType) || value_is_object_of_type(value, vm->classType))

#define ALLOCATE_INSTANCE(vm) (ALLOCATE_OBJ(vm, ObjInstance, vm->instanceType))

typedef struct {
    Object base;
    ObjClass* clazz;
    Table fields;
} ObjInstance;

ObjectType* new_instance_type(VM* vm);
void free_instance_type(ObjectType* type, VM* vm);

ObjInstance* new_instance(VM* vm, ObjClass* clazz);

#define AS_CLASS(object) ((ObjClass*)object)
#define IS_CLASS(object, vm) (OBJ_TYPE(object) == vm->classType)

#define VAL_AS_CLASS(value) (AS_CLASS(AS_OBJ(value)))
#define VAL_IS_CLASS(value, vm) (value_is_object_of_type(value, vm->classType))

#define ALLOCATE_CLASS(vm) (ALLOCATE_OBJ(vm, ObjClass, vm->classType))

typedef struct ObjClass {
    ObjInstance base;
    ObjString* name;
    Table methods;
} ObjClass;

ObjectType* new_class_type(VM* vm);
void free_class_type(ObjectType* type, VM* vm);

ObjClass* new_class(VM* vm, ObjString* name);

#define AS_BOUND_METHOD(object) ((ObjBoundMethod*)object)
#define IS_BOUND_METHOD(object, vm) (OBJ_TYPE(object) == vm->boundMethodType)

#define VAL_AS_BOUND_METHOD(value) (AS_BOUND_METHOD(AS_OBJ(value)))
#define VAL_IS_BOUND_METHOD(value, vm) (value_is_object_of_type(value, vm->boundMethodType))

#define ALLOCATE_BOUND_METHOD(vm) (ALLOCATE_OBJ(vm, ObjBoundMethod, vm->boundMethodType))

typedef struct ObjBoundMethod {
    Object base;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

ObjectType* new_bound_method_type(VM* vm);
void free_bound_method_type(ObjectType* type, VM* vm);

ObjBoundMethod* new_bound_method(VM* vm, Value receiver, ObjClosure* method);

#endif
