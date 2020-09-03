#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdbool.h>

#include "table.h"
#include "value.h"

#define OBJ_TYPE(object) ((object)->type)

#define ALLOCATE_OBJ(vm, type, objectType) (type*)allocate_object(vm, sizeof(type), objectType)

typedef struct GC GC;
typedef struct VM VM;

typedef struct ObjectType ObjectType;

typedef struct Object {
    ObjectType* type;
    struct Object* next;
    bool marked;
} Object;

typedef void (*PrintObj)(Object* object);
typedef uint32_t (*HashObj)(Object* object);
typedef Value(*GetMethodObj)(Object* object, Object* key, VM* vm);
typedef bool (*CallObj)(Object* callee, uint8_t argCount, VM* vm);
typedef void (*TraverseObj)(Object* object, GC* gc);
typedef void (*FreeObj)(Object* object, GC* gc);

typedef struct ObjectType {
    const char* name;

    PrintObj print;
    HashObj hash;
    GetMethodObj getMethod;
    CallObj call;
    TraverseObj traverse;
    FreeObj free;

    Table methods;
} ObjectType;

void print_object(Object* object);
uint32_t hash_object(Object* object);
Value get_method_object(Object* object, Object* key, VM* vm);
bool call_object(Object* callee, uint8_t argCount, VM* vm);
void traverse_object(Object* object, GC* gc);
void free_object(Object* object, GC* gc);

Object* allocate_object(VM* vm, size_t size, ObjectType* type);

bool value_is_object_of_type(Value value, ObjectType* type);

Value generic_get_method(Object* object, Object* key, VM* vm);

#endif
