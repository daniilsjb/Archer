#include <stdio.h>

#include "objstring.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

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
}

static void free_string(Object* object, GC* gc)
{
    ObjString* string = AS_STRING(object);
    deallocate(gc, string, string->length + 1);
}

ObjectType StringType = {
    .print = print_string,
    .hash = hash_string,
    .traverse = traverse_string,
    .free = free_string
};

ObjString* new_string(VM* vm, size_t length)
{
    ObjString* string = (ObjString*)allocate_object(vm, sizeof(ObjString) + length + 1, &StringType);
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
