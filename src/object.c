#include <stdio.h>

#include "object.h"
#include "obj_function.h"
#include "obj_native.h"
#include "obj_string.h"
#include "obj_coroutine.h"
#include "obj_iterator.h"

#include "vm.h"
#include "gc.h"
#include "memory.h"
#include "common.h"

Object* Object_Allocate(VM* vm, size_t size)
{
    Object* object = (Object*)Mem_Allocate(&vm->gc, size);
    object->marked = false;
    Table_Init(&object->fields);

    GC_AppendObject(&vm->gc, object);

#if DEBUG_LOG_GC
    printf("%p allocated object\n", (void*)object);
#endif

    return object;
}

void Object_Deallocate(GC* gc, Object* object)
{
    Table_Free(gc, &object->fields);
    Mem_Deallocate(gc, object, object->type->size);
}

Object* Object_New(VM* vm, ObjectType* type)
{
    Vm_PushTemporary(vm, OBJ_VAL(type));
    Object* object = Object_Allocate(vm, type->size);
    object->type = type;
    Vm_PopTemporary(vm);
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

ObjectString* Object_ToString(Object* object, VM* vm)
{
    return object->type->ToString(object, vm);
}

void Object_Print(Object* object)
{
    object->type->Print(object);
}

uint32_t Object_Hash(Object* object)
{
    return object->type->Hash(object);
}

bool Object_GetField(Object* object, Value key, VM* vm, Value* result)
{
    return object->type->GetField(object, key, vm, result);
}

bool Object_SetField(Object* object, Value key, Value value, VM* vm)
{
    return object->type->SetField(object, key, value, vm);
}

bool Object_GetSubscript(Object* object, Value index, VM* vm, Value* result)
{
    return object->type->GetSubscript(object, index, vm, result);
}

bool Object_SetSubscript(Object* object, Value index, Value value, VM* vm)
{
    return object->type->SetSubscript(object, index, value, vm);
}

bool Object_GetMethod(Object* object, Value key, VM* vm, Value* result)
{
    if (!object->type->GetMethod(object->type, key, vm, result)) {
        return false;
    }

    *result = OBJ_VAL(BoundMethod_New(vm, OBJ_VAL(object), AS_OBJ(*result)));
    return true;
}

bool Object_GetMethodDirectly(Object* object, Value key, VM* vm, Value* result)
{
    return AS_TYPE(object)->GetMethod(AS_TYPE(object), key, vm, result);
}

bool Object_SetMethod(Object* object, Value key, Value value, VM* vm)
{
    return object->type->SetMethod(object->type, key, value, vm);
}

bool Object_SetMethodDirectly(Object* object, Value key, Value value, VM* vm)
{
    return AS_TYPE(object)->SetMethod(AS_TYPE(object), key, value, vm);
}

ObjectIterator* Object_MakeIterator(Object* object, VM* vm)
{
    return object->type->MakeIterator(object, vm);
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

uint32_t Object_GenericHash(Object* object)
{
    return Value_HashBits((uint64_t)(uintptr_t)object);
}

bool Object_GenericGetField(Object* object, Value key, VM* vm, Value* result)
{
    return Table_Get(&object->fields, key, result);
}

bool Object_GenericSetField(Object* object, Value key, Value value, VM* vm)
{
    return Table_Put(vm, &object->fields, key, value);
}

bool Object_GenericGetMethod(ObjectType* type, Value key, VM* vm, Value* result)
{
    return Table_Get(&type->methods, key, result);
}

bool Object_GenericSetMethod(ObjectType* type, Value key, Value value, VM* vm)
{
    return Table_Put(vm, &type->methods, key, value);
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
    Table_Init(&type->methods);
    return type;
}

void Type_Deallocate(ObjectType* type, GC* gc)
{
    Table_Free(gc, &type->methods);
    Object_Deallocate(gc, (Object*)type);
}

static ObjectString* type_to_string(Object* object, VM* vm)
{
    char cstring[100];
    snprintf(cstring, 100, "<class '%s'>", AS_TYPE(object)->name);
    return String_FromCString(vm, cstring);
}

static void type_print(Object* object)
{
    printf("<class '%s'>", AS_TYPE(object)->name);
}

static bool type_call(Object* callee, uint8_t argCount, VM* vm)
{
    ObjectType* type = AS_TYPE(callee);
    vm->coroutine->stackTop[-argCount - 1] = OBJ_VAL(Object_New(vm, type));

    Value initializer;
    if (Table_Get(&type->methods, OBJ_VAL(vm->initString), &initializer)) {
        return Object_Call(AS_OBJ(initializer), argCount, vm);
    }

    if (argCount != 0) {
        Vm_RuntimeError(vm, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    return true;
}

static ObjectType* new_meta_type(VM* vm)
{
    ObjectType* meta = Type_Allocate(vm);
    meta->name = "MetaType";
    meta->size = sizeof(ObjectType);
    meta->flags = 0x0,
    meta->ToString = type_to_string;
    meta->Print = type_print;
    meta->Hash = Object_GenericHash;
    meta->GetField = Object_GenericGetField;
    meta->SetField = NULL;
    meta->GetMethod = Object_GenericGetMethod;
    meta->SetMethod = NULL;
    meta->MakeIterator = NULL;
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
    Table_Init(&type->methods);
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

static ObjectString* instance_to_string(Object* object, VM* vm)
{
    char cstring[100];
    snprintf(cstring, 100, "<'%s' instance>", object->type->name);
    return String_FromCString(vm, cstring);
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
    type->flags = TF_DEFAULT,
    type->ToString = instance_to_string;
    type->Print = instance_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = Object_GenericSetField;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = Object_GenericSetMethod;
    type->MakeIterator = NULL;
    type->Call = NULL;
    type->Traverse = Object_GenericTraverse;
    type->Free = Object_GenericFree;

    type->base.type->SetField = Object_GenericSetField;
    type->base.type->SetMethod = Object_GenericSetMethod;
    return type;
}
