#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdbool.h>

#include "table.h"
#include "value.h"

#define OBJ_TYPE(object) ((object)->type)

#define ALLOCATE_OBJ(vm, objectType) Object_New(vm, objectType)

typedef struct GC GC;
typedef struct VM VM;

typedef struct ObjectType ObjectType;

typedef struct Object {
    ObjectType* type;
    Table fields;
    struct Object* next;
    bool marked;
} Object;

Object* Object_Allocate(VM* vm, size_t size);
void Object_Deallocate(GC* gc, Object* object);

Object* Object_New(VM* vm, ObjectType* type);

bool Object_IsType(Object* object);

bool Object_ValueHasType(Value value, ObjectType* type);
bool Object_ValueIsType(Value value);

void Object_Print(Object* object);
uint32_t Object_Hash(Object* object);
Value Object_GetMethod(Object* object, Object* key, VM* vm);
bool Object_Call(Object* callee, uint8_t argCount, VM* vm);
void Object_Traverse(Object* object, GC* gc);
void Object_Free(Object* object, GC* gc);

Value Object_GenericGetMethod(Object* object, Object* key, VM* vm);
void Object_GenericTraverse(Object* object, GC* gc);
void Object_GenericFree(Object* object, GC* gc);

#define AS_TYPE(object) ((ObjectType*)object)
#define IS_TYPE(object) (Object_IsType(object))

#define VAL_AS_TYPE(value) (AS_TYPE(AS_OBJ(value)))
#define VAL_IS_TYPE(value) (Object_ValueIsType(value))

typedef void (*PrintFn)(Object* object);
typedef uint32_t (*HashFn)(Object* object);
typedef Value (*GetMethodFn)(Object* object, Object* key, VM* vm);
typedef bool (*CallFn)(Object* callee, uint8_t argCount, VM* vm);
typedef void (*TraverseFn)(Object* object, GC* gc);
typedef void (*FreeFn)(Object* object, GC* gc);

typedef struct ObjectType {
    Object base;
    const char* name;
    size_t size;
    Table methods;

    PrintFn Print;
    HashFn Hash;
    GetMethodFn GetMethod;
    CallFn Call;
    TraverseFn Traverse;
    FreeFn Free;
} ObjectType;

ObjectType* Type_Allocate(VM* vm);
void Type_Deallocate(ObjectType* type, GC* gc);

ObjectType* Type_New(VM* vm);

void Type_GenericTraverse(Object* object, GC* gc);
void Type_GenericFree(Object* object, GC* gc);

ObjectType* Type_NewClass(VM* vm, const char* name);

#endif
