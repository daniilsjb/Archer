#include <stdio.h>

#include "objstring.h"
#include "objnative.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static bool length_method(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)VAL_AS_STRING(args[-1])->length);
    return true;
}

static bool starts_with_method(VM* vm, Value* args)
{
    if (!VAL_IS_STRING(args[0], vm)) {
        return false;
    }

    ObjString* original = VAL_AS_STRING(args[-1]);
    ObjString* compared = VAL_AS_STRING(args[0]);
    if (compared->length > original->length) {
        args[-1] = BOOL_VAL(false);
        return true;
    }

    args[-1] = BOOL_VAL(memcmp(original->chars, compared->chars, compared->length) == 0);
    return true;
}

static bool ends_with_method(VM* vm, Value* args)
{
    if (!VAL_IS_STRING(args[0], vm)) {
        return false;
    }

    ObjString* original = VAL_AS_STRING(args[-1]);
    ObjString* compared = VAL_AS_STRING(args[0]);
    if (compared->length > original->length) {
        args[-1] = BOOL_VAL(false);
        return true;
    }

    args[-1] = BOOL_VAL(memcmp(original->chars + original->length - compared->length, compared->chars, compared->length) == 0);
    return true;
}

static void print_string(Object* object)
{
    printf("%s", AS_CSTRING(object));
}

static uint32_t hash_cstring(const char* chars, size_t length)
{
    uint32_t hash = 2166136261U;

    for (size_t i = 0; i < length; i++) {
        hash ^= chars[i];
        hash *= 16777619;
    }

    return hash;
}

static uint32_t hash_string(Object* object)
{
    ObjString* string = AS_STRING(object);
    return hash_cstring(string->chars, string->length);
}

static void traverse_string(Object* object, GC* gc)
{
    gc_mark_table(gc, &object->type->methods);
}

static void free_string(Object* object, GC* gc)
{
    ObjString* string = AS_STRING(object);
    deallocate(gc, string, string->length + 1);
}

ObjectType* new_string_type(VM* vm)
{
    ObjectType* type = raw_allocate(sizeof(ObjectType));
    if (!type) {
        return NULL;
    }

    *type = (ObjectType) {
        .name = "string",
        .print = print_string,
        .hash = hash_string,
        .getMethod = generic_get_method,
        .traverse = traverse_string,
        .free = free_string
    };

    table_init(&type->methods);

    return type;
}

void prepare_string_type(ObjectType* type, VM* vm)
{
    table_put(vm, &type->methods, copy_string(vm, "length", 6), OBJ_VAL(new_native(vm, length_method, 0)));
    table_put(vm, &type->methods, copy_string(vm, "startsWith", 10), OBJ_VAL(new_native(vm, starts_with_method, 1)));
    table_put(vm, &type->methods, copy_string(vm, "endsWith", 8), OBJ_VAL(new_native(vm, ends_with_method, 1)));
}

void free_string_type(ObjectType* type, VM* vm)
{
    table_free(&vm->gc, &type->methods);
    raw_deallocate(type);
}

ObjString* new_string(VM* vm, size_t length)
{
    ObjString* string = ALLOCATE_STRING(vm, length);
    string->length = length;
    return string;
}

ObjString* copy_string(VM* vm, const char* chars, size_t length)
{
    uint32_t hash = hash_cstring(chars, length);
    ObjString* interned = table_find_string(&vm->strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    ObjString* string = new_string(vm, length);

    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    string->hash = hash;

    vm_push(vm, OBJ_VAL(string));
    table_put(vm, &vm->strings, string, NIL_VAL());
    vm_pop(vm);

    return string;
}

ObjString* concatenate_strings(VM* vm, ObjString* a, ObjString* b)
{
    size_t length = a->length + b->length;
    ObjString* string = new_string(vm, length);

    memcpy(string->chars, a->chars, a->length);
    memcpy(string->chars + a->length, b->chars, b->length);
    string->chars[length] = '\0';
    string->hash = hash_string((Object*)string);

    ObjString* interned = table_find_string(&vm->strings, string->chars, length, string->hash);
    if (interned != NULL) {
        vm->gc.allocatedObjects = vm->gc.allocatedObjects->next;
        deallocate(&vm->gc, string, sizeof(ObjString) + string->length + 1);

        return interned;
    } else {
        vm_push(vm, OBJ_VAL(string));
        table_put(vm, &vm->strings, string, NIL_VAL());
        vm_pop(vm);

        return string;
    }
}
