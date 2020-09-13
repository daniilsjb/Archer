#ifndef OBJMAP_H
#define OBJMAP_H

#include "object.h"
#include "table.h"

#define AS_MAP(object) ((ObjectMap*)object)
#define IS_MAP(object) (OBJ_TYPE(object) == vm->mapType)

#define VAL_AS_MAP(value) (AS_MAP(AS_OBJ(value)))
#define VAL_IS_MAP(value) (Object_ValueHasType((value), vm->mapType))

#define ALLOCATE_MAP(vm) (AS_MAP(ALLOCATE_OBJ(vm, vm->mapType)))

typedef struct ObjectMap {
    Object base;
    Table table;
} ObjectMap;

ObjectType* Map_NewType(VM* vm);
void Map_PrepareType(ObjectType* type, VM* vm);

ObjectMap* Map_New(VM* vm);
void Map_Insert(ObjectMap* map, Value key, Value value, VM* vm);

#endif
