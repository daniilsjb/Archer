#ifndef OBJMODULE_H
#define OBJMODULE_H

#include "object.h"

#define AS_MODULE(object) ((ObjectModule*)object)
#define IS_MODULE(object, vm) (OBJ_TYPE(object) == vm->moduleType)

#define VAL_AS_MODULE(value) (AS_MODULE(AS_OBJ(value)))
#define VAL_IS_MODULE(value, vm) (Object_ValueHasType((value), vm->moduleType))

#define ALLOCATE_MODULE(vm) (AS_MODULE(ALLOCATE_OBJ(vm, vm->moduleType)))

typedef struct ObjectString ObjectString;

typedef struct ObjectModule {
    Object base;
    ObjectString* path;
    ObjectString* name;
    bool imported;
} ObjectModule;

ObjectType* Module_NewType(VM* vm);
void Module_PrepareType(ObjectType* type, VM* vm);

ObjectModule* Module_New(VM* vm, ObjectString* path, ObjectString* name);
ObjectModule* Module_FromFullPath(VM* vm, const char* fullPath);

#endif
