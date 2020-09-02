#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdbool.h>

#include "value.h"

#define MAX_TYPE_METHODS 16

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
typedef bool (*CallObj)(Object* callee, uint8_t argCount, VM* vm);
typedef void (*TraverseObj)(Object* object, GC* gc);
typedef void (*FreeObj)(Object* object, GC* gc);

typedef bool (*TypeMethod)(Object* object, uint8_t argCount, VM* vm);

typedef struct {
    const char* name;
    uint32_t hash;
    size_t length;

    uint8_t arity;
    TypeMethod method;
} TypeMethodDef;

typedef struct ObjectType {
    const char* name;

    PrintObj print;
    HashObj hash;
    CallObj call;
    TraverseObj traverse;
    FreeObj free;

    TypeMethodDef methods[MAX_TYPE_METHODS];
} ObjectType;

void print_object(Object* object);
uint32_t hash_object(Object* object);
bool call_object(Object* callee, uint8_t argCount, VM* vm);
void traverse_object(Object* object, GC* gc);
void free_object(Object* object, GC* gc);

Object* allocate_object(VM* vm, size_t size, ObjectType* type);

bool value_is_object_of_type(Value value, ObjectType* type);

#endif
