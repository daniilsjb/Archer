#include <stdio.h>

#include "object.h"
#include "objfunction.h"
#include "objnative.h"
#include "vm.h"
#include "gc.h"
#include "memory.h"
#include "common.h"

Object* Object_Allocate(VM* vm, size_t size)
{
    Object* object = (Object*)Mem_Allocate(&vm->gc, size);
    object->marked = false;
    table_init(&object->fields);

    GC_AppendObject(&vm->gc, object);

#if DEBUG_LOG_GC
    printf("%p allocated object\n", (void*)object);
#endif

    return object;
}

void Object_Deallocate(GC* gc, Object* object)
{
    table_free(gc, &object->fields);
    Mem_Deallocate(gc, object, object->type->size);
}

Object* Object_New(VM* vm, ObjectType* type)
{
    vm_push(vm, OBJ_VAL(type));
    Object* object = Object_Allocate(vm, type->size);
    object->type = type;
    vm_pop(vm);
    return object;
}

bool Object_IsType(Object* object)
{
    ObjectType* meta = object->type;
    return meta == meta->base.type;
}

bool Object_ValueHasType(Value value, ObjectType* type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

bool Object_ValueIsType(Value value)
{
    return IS_OBJ(value) && Object_IsType(AS_OBJ(value));
}

void Object_Print(Object* object)
{
    object->type->Print(object);
}

uint32_t Object_Hash(Object* object)
{
    return object->type->Hash(object);
}

Value Object_GetMethod(Object* object, Object* key, VM* vm)
{
    return object->type->GetMethod(object, key, vm);
}

bool Object_Call(Object* callee, uint8_t argCount, VM* vm)
{
    return callee->type->Call(callee, argCount, vm);
}

void Object_Traverse(Object* object, GC* gc)
{
#if DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    object->type->Traverse(object, gc);
}

void Object_Free(Object* object, GC* gc)
{
    object->type->Free(object, gc);
}

Value Object_GenericGetMethod(Object* object, Object* key, VM* vm)
{
    Value method;
    if (!table_get(&object->type->methods, (ObjectString*)key, &method)) {
        return NIL_VAL();
    }

    return OBJ_VAL(BoundMethod_New(vm, OBJ_VAL(object), AS_OBJ(method)));
}

void Object_GenericTraverse(Object* object, GC* gc)
{
    GC_MarkTable(gc, &object->fields);
    GC_MarkObject(gc, (Object*)object->type);
}

void Object_GenericFree(Object* object, GC* gc)
{
    Object_Deallocate(gc, object);
}

ObjectType* Type_Allocate(VM* vm)
{
    ObjectType* type = (ObjectType*)Object_Allocate(vm, sizeof(ObjectType));
    table_init(&type->methods);
    return type;
}

void Type_Deallocate(ObjectType* type, GC* gc)
{
    table_free(gc, &type->methods);
    Object_Deallocate(gc, (Object*)type);
}

static void type_print(Object* object)
{
    printf("<class '%s'>", AS_TYPE(object)->name);
}

static bool type_call(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectType* type = AS_TYPE(callee);
    vm->stackTop[-argCount - 1] = OBJ_VAL(Object_New(vm, type));

    Value initializer;
    if (table_get(&type->methods, vm->initString, &initializer)) {
        return Object_Call(AS_OBJ(initializer), argCount, vm);
    }

    if (argCount != 0) {
        runtime_error(vm, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    return true;
}

static ObjectType* new_meta_type(VM* vm)
{
    ObjectType* meta = Type_Allocate(vm);
    meta->name = "MetaType";
    meta->size = sizeof(ObjectType);
    meta->Print = type_print;
    meta->Hash = NULL;
    meta->GetMethod = Object_GenericGetMethod;
    meta->Call = type_call;
    meta->Traverse = Type_GenericTraverse;
    meta->Free = Type_GenericFree;
    meta->base.type = meta;
    return meta;
}

ObjectType* Type_New(VM* vm)
{
    ObjectType* meta = new_meta_type(vm);
    ObjectType* type = (ObjectType*)Object_New(vm, meta);
    table_init(&type->methods);
    return type;
}

void Type_GenericTraverse(Object* object, GC* gc)
{
    GC_MarkTable(gc, &AS_TYPE(object)->methods);
    Object_GenericTraverse(object, gc);
}

void Type_GenericFree(Object* object, GC* gc)
{
    Type_Deallocate((ObjectType*)object, gc);
}

static void instance_print(Object* object)
{
    printf("<'%s' instance>", object->type->name);
}

ObjectType* Type_NewClass(VM* vm, const char* name)
{
    ObjectType* type = Type_New(vm);
    type->name = name;
    type->size = sizeof(Object);
    type->Print = instance_print;
    type->Hash = NULL;
    type->GetMethod = Object_GenericGetMethod;
    type->Call = NULL;
    type->Traverse = Object_GenericTraverse;
    type->Free = Object_GenericFree;
    return type;
}
