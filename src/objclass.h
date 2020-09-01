#ifndef OBJCLASS_H
#define OBJCLASS_H

#include "object.h"
#include "table.h"

typedef struct ObjString ObjString;
typedef struct ObjClosure ObjClosure;
typedef struct ObjClass ObjClass;

#define AS_INSTANCE(object) ((ObjInstance*)object)
#define IS_INSTANCE(object) (OBJ_TYPE(object) == &InstanceType || OBJ_TYPE(object) == &ClassType)

#define VAL_AS_INSTANCE(value) (AS_INSTANCE(AS_OBJ(value)))
#define VAL_IS_INSTANCE(value) (value_is_object_of_type(value, &InstanceType) || value_is_object_of_type(value, &ClassType))

extern ObjectType InstanceType;

typedef struct {
    Object base;
    ObjClass* clazz;
    Table fields;
} ObjInstance;

ObjInstance* new_instance(VM* vm, ObjClass* clazz);

#define AS_CLASS(object) ((ObjClass*)object)
#define IS_CLASS(object) (OBJ_TYPE(object) == &ClassType)

#define VAL_AS_CLASS(value) (AS_CLASS(AS_OBJ(value)))
#define VAL_IS_CLASS(value) (value_is_object_of_type(value, &ClassType))

extern ObjectType ClassType;

typedef struct ObjClass {
    ObjInstance base;
    ObjString* name;
    Table methods;
} ObjClass;

ObjClass* new_class(VM* vm, ObjString* name);

#define AS_BOUND_METHOD(object) ((ObjBoundMethod*)object)
#define IS_BOUND_METHOD(object) (OBJ_TYPE(object) == &BoundMethodType)

#define VAL_AS_BOUND_METHOD(value) (AS_BOUND_METHOD(AS_OBJ(value)))
#define VAL_IS_BOUND_METHOD(value) (value_is_object_of_type(value, &BoundMethodType))

extern ObjectType BoundMethodType;

typedef struct ObjBoundMethod {
    Object base;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* new_bound_method(VM* vm, Value receiver, ObjClosure* method);

#endif
