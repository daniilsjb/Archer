#include <stdio.h>
#include <math.h>

#include "vm.h"
#include "library.h"

#include "obj_map.h"
#include "obj_string.h"
#include "obj_native.h"

static bool method_length(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)Table_Size(&VAL_AS_MAP(args[-1])->table));
    return true;
}

static bool method_is_empty(VM* vm, Value* args)
{
    args[-1] = BOOL_VAL(Table_Size(&VAL_AS_MAP(args[-1])->table) == 0);
    return true;
}

static bool method_contains_key(VM* vm, Value* args)
{
    ObjectMap* map = VAL_AS_MAP(args[-1]);
    Value key = args[0];

    Value dump;
    args[-1] = BOOL_VAL(Table_Get(&map->table, key, &dump));
    return true;
}

static bool method_get_or_default(VM* vm, Value* args)
{
    ObjectMap* map = VAL_AS_MAP(args[-1]);
    Value key = args[0];
    Value value = args[1];

    Value result;
    if (Table_Get(&map->table, key, &result)) {
        args[-1] = result;
    } else {
        args[-1] = value;
    }

    return true;
}

static bool method_put_if_absent(VM* vm, Value* args)
{
    ObjectMap* map = VAL_AS_MAP(args[-1]);
    Value result;
    if (Table_Get(&map->table, args[0], &result)) {
        args[-1] = result;
    } else {
        Table_Put(vm, &map->table, args[0], args[1]);
        args[-1] = NIL_VAL();
    }

    return true;
}

static bool method_put_all(VM* vm, Value* args)
{
    if (!VAL_IS_MAP(args[0])) {
        return Library_Error(vm, "Expected a map.", args);
    }

    ObjectMap* map = VAL_AS_MAP(args[-1]);
    ObjectMap* other = VAL_AS_MAP(args[0]);
    Table_PutFrom(vm, &other->table, &map->table);

    args[-1] = NIL_VAL();
    return true;
}

static bool method_remove(VM* vm, Value* args)
{
    Table_Remove(&VAL_AS_MAP(args[-1])->table, args[0]);
    args[-1] = NIL_VAL();
    return true;
}

static bool method_clear(VM* vm, Value* args)
{
    Table_Free(&vm->gc, &VAL_AS_MAP(args[-1])->table);
    args[-1] = NIL_VAL();
    return true;
}

static ObjectString* map_to_string(Object* object, VM* vm)
{
    return String_FromCString(vm, "<map>");
}

static void map_print(Object* object)
{
    printf("<map>");
}

static bool map_get_subscript(Object* object, Value index, VM* vm, Value* result)
{
    if (!Table_Get(&AS_MAP(object)->table, index, result)) {
        Vm_RuntimeError(vm, "Key not found.");
        return false;
    }
    
    return true;
}

static bool map_set_subscript(Object* object, Value index, Value value, VM* vm)
{
    return Table_Put(vm, &AS_MAP(object)->table, index, value);
}

static void map_traverse(Object* object, GC* gc)
{
    GC_MarkTable(gc, &AS_MAP(object)->table);
    Object_GenericTraverse(object, gc);
}

static void map_free(Object* object, GC* gc)
{
    Table_Free(gc, &AS_MAP(object)->table);
    Object_GenericFree(object, gc);
}

ObjectType* Map_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Map";
    type->size = sizeof(ObjectMap);
    type->flags = 0x0;
    type->ToString = map_to_string;
    type->Print = map_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = NULL;
    type->GetSubscript = map_get_subscript;
    type->SetSubscript = map_set_subscript;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->MakeIterator = NULL;
    type->Call = NULL;
    type->Traverse = map_traverse;
    type->Free = map_free;
    return type;
}

void Map_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "length", method_length, 0);
    Library_DefineTypeMethod(type, vm, "isEmpty", method_is_empty, 0);
    Library_DefineTypeMethod(type, vm, "containsKey", method_contains_key, 1);
    Library_DefineTypeMethod(type, vm, "getOrDefault", method_get_or_default, 2);
    Library_DefineTypeMethod(type, vm, "putIfAbsent", method_put_if_absent, 2);
    Library_DefineTypeMethod(type, vm, "putAll", method_put_all, 1);
    Library_DefineTypeMethod(type, vm, "remove", method_remove, 1);
    Library_DefineTypeMethod(type, vm, "clear", method_clear, 0);
}

ObjectMap* Map_New(VM* vm)
{
    ObjectMap* map = ALLOCATE_MAP(vm);
    Table_Init(&map->table);
    return map;
}

void Map_Insert(ObjectMap* map, Value key, Value value, VM* vm)
{
    Table_Put(vm, &map->table, key, value);
}
