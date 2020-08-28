#ifndef OBJECT_H
#define OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"

struct VM;

#define OBJ_TYPE(object)        (AS_OBJ(object)->type)

#define IS_STRING(object)       isObjType(object, OBJ_STRING)
#define IS_FUNCTION(object)     isObjType(object, OBJ_FUNCTION)
#define IS_CLOSURE(object)      isObjType(object, OBJ_CLOSURE)
#define IS_NATIVE(object)       isObjType(object, OBJ_NATIVE)
#define IS_INSTANCE(object)     (isObjType(object, OBJ_INSTANCE) || isObjType(object, OBJ_CLASS))
#define IS_CLASS(object)        isObjType(object, OBJ_CLASS)
#define IS_BOUND_METHOD(object) isObjType(object, OBJ_BOUND_METHOD)

#define AS_STRING(object)       ((ObjString*)AS_OBJ(object))
#define AS_CSTRING(object)      (((ObjString*)AS_OBJ(object))->chars)
#define AS_FUNCTION(object)     ((ObjFunction*)AS_OBJ(object))
#define AS_CLOSURE(object)      ((ObjClosure*)AS_OBJ(object))
#define AS_NATIVE(object)       ((ObjNative*)AS_OBJ(object))
#define AS_INSTANCE(object)     ((ObjInstance*)AS_OBJ(object))
#define AS_CLASS(object)        ((ObjClass*)AS_OBJ(object))
#define AS_BOUND_METHOD(object) ((ObjBoundMethod*)AS_OBJ(object))

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_NATIVE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD
} ObjType;

typedef struct Obj {
    struct Obj* next;
    ObjType type;
    bool marked;
} Obj;

typedef struct {
    Obj obj;
    Chunk chunk;
    int arity;
    size_t upvalueCount;
    ObjString* name;
} ObjFunction;

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    size_t upvalueCount;
} ObjClosure;

typedef struct ObjClass ObjClass;

typedef struct {
    Obj obj;
    ObjClass* loxClass;
    Table fields;
} ObjInstance;

typedef struct ObjClass {
    ObjInstance obj;
    ObjString* name;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

typedef bool (*NativeFn)(struct VM* vm, size_t argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
    int arity;
} ObjNative;

typedef struct ObjString {
    Obj obj;
    uint32_t hash;
    size_t length;
    char chars[];
} ObjString;

Obj* allocate_object(struct VM* vm, size_t size, ObjType type);

ObjFunction* new_function(struct VM* vm);
ObjClosure* new_closure(struct VM* vm, ObjFunction* function);
ObjUpvalue* new_upvalue(struct VM* vm, Value* slot);
ObjNative* new_native(struct VM* vm, NativeFn function, int arity);
ObjInstance* new_instance(struct VM* vm, ObjClass* loxClass);
ObjClass* new_class(struct VM* vm, ObjString* name);
ObjBoundMethod* new_bound_method(struct VM* vm, Value receiver, ObjClosure* method);

ObjString* make_string(struct VM* vm, size_t length);
ObjString* copy_string(struct VM* vm, const char* chars, size_t length);
ObjString* concatenate_strings(struct VM* vm, ObjString* a, ObjString* b);

uint32_t hash_string(const char* key, size_t length);

void print_object(Value value);

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
