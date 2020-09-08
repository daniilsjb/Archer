#ifndef OBJLIST_H
#define OBJLIST_H

#include "object.h"

#define AS_LIST(object) ((ObjectList*)object)
#define IS_LIST(object) (OBJ_TYPE(object) == vm->listType)

#define VAL_AS_LIST(value) (AS_LIST(AS_OBJ(value)))
#define VAL_IS_LIST(value) (Object_ValueHasType((value), vm->listType))

#define ALLOCATE_LIST(vm) (AS_LIST(ALLOCATE_OBJ(vm, vm->listType)))

typedef struct ObjectList {
    Object base;
    ValueArray elements;
} ObjectList;

ObjectType* List_NewType(VM* vm);
void List_PrepareType(ObjectType* type, VM* vm);

ObjectList* List_New(VM* vm);
void List_Append(ObjectList* list, Value value, VM* vm);

#endif