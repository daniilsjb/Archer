#include <stdio.h>

#include "objstring.h"
#include "objnative.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"

static bool init_method(VM* vm, Value* args)
{
    args[-1] = OBJ_VAL(String_FromValue(vm, args[0]));
    return true;
}

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

    ObjectString* original = VAL_AS_STRING(args[-1]);
    ObjectString* compared = VAL_AS_STRING(args[0]);
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

    ObjectString* original = VAL_AS_STRING(args[-1]);
    ObjectString* compared = VAL_AS_STRING(args[0]);
    if (compared->length > original->length) {
        args[-1] = BOOL_VAL(false);
        return true;
    }

    args[-1] = BOOL_VAL(memcmp(original->chars + original->length - compared->length, compared->chars, compared->length) == 0);
    return true;
}

static bool from_number_method(VM* vm, Value* args)
{
    if (!IS_NUMBER(args[0])) {
        return false;
    }

    char result[50];
    sprintf(result, "%g", AS_NUMBER(args[0]));

    args[-1] = OBJ_VAL(String_FromCString(vm, result));
    return true;
}

static void string_print(Object* object)
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

static uint32_t string_hash(Object* object)
{
    ObjectString* string = AS_STRING(object);
    return hash_cstring(string->chars, string->length);
}

static ObjectString* string_to_string(Object* object, VM* vm)
{
    return AS_STRING(object);
}

static void string_free(Object* object, GC* gc)
{
    ObjectString* string = AS_STRING(object);
    table_free(gc, &string->base.fields);
    Mem_Deallocate(gc, string, sizeof(ObjectString) + string->length + 1);
}

ObjectType* String_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "String";
    type->size = sizeof(ObjectString);
    type->flags = 0x0;
    type->ToString = string_to_string;
    type->Print = string_print;
    type->Hash = string_hash;
    type->GetField = NULL;
    type->SetField = NULL;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = Object_GenericTraverse;
    type->Free = string_free;
    return type;
}

static void define_string_method(ObjectType* type, VM* vm, const char* name, NativeFn function, int arity)
{
    vm_push(vm, OBJ_VAL(String_FromCString(vm, name)));
    vm_push(vm, OBJ_VAL(Native_New(vm, function, arity)));
    table_put(vm, &type->methods, VAL_AS_STRING(vm->stack[0]), vm->stack[1]);
    vm_pop(vm);
    vm_pop(vm);
}

void String_PrepareType(ObjectType* type, VM* vm)
{
    define_string_method(type, vm, "init", init_method, 1);
    define_string_method(type, vm, "length", length_method, 0);
    define_string_method(type, vm, "startsWith", starts_with_method, 1);
    define_string_method(type, vm, "endsWith", ends_with_method, 1);

    define_string_method(type->base.type, vm, "fromNumber", from_number_method, 1);
}

ObjectString* String_New(VM* vm, size_t length)
{
    ObjectString* string = ALLOCATE_STRING(vm, length);
    string->length = length;
    string->base.type = vm->stringType;
    return string;
}

ObjectString* String_Copy(VM* vm, const char* chars, size_t length)
{
    uint32_t hash = hash_cstring(chars, length);
    ObjectString* interned = table_find_string(&vm->strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    ObjectString* string = String_New(vm, length);

    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    string->hash = hash;

    vm_push(vm, OBJ_VAL(string));
    table_put(vm, &vm->strings, string, NIL_VAL());
    vm_pop(vm);

    return string;
}

ObjectString* String_FromCString(VM* vm, const char* chars)
{
    return String_Copy(vm, chars, strlen(chars));
}

ObjectString* String_Concatenate(VM* vm, ObjectString* a, ObjectString* b)
{
    size_t length = a->length + b->length;
    ObjectString* string = String_New(vm, length);

    memcpy(string->chars, a->chars, a->length);
    memcpy(string->chars + a->length, b->chars, b->length);
    string->chars[length] = '\0';
    string->hash = string_hash((Object*)string);

    ObjectString* interned = table_find_string(&vm->strings, string->chars, length, string->hash);
    if (interned != NULL) {
        vm->gc.allocatedObjects = vm->gc.allocatedObjects->next;
        Mem_Deallocate(&vm->gc, string, sizeof(ObjectString) + string->length + 1);

        return interned;
    } else {
        vm_push(vm, OBJ_VAL(string));
        table_put(vm, &vm->strings, string, NIL_VAL());
        vm_pop(vm);

        return string;
    }
}

ObjectString* String_FromValue(VM* vm, Value value)
{
#if NAN_BOXING
    if (IS_BOOL(value)) {
        return String_FromCString(vm, AS_BOOL(value) ? "true" : "false");
    } else if IS_NUMBER(value) {
        char cstring[50];
        snprintf(cstring, 50, "%g", AS_NUMBER(value));
        return String_FromCString(vm, cstring);
    } else if IS_NIL(value) {
        return String_FromCString(vm, "nil");
    } else {
        return Object_ToString(AS_OBJ(value), vm);
    }
#else
    switch (value.type) {
        case VALUE_BOOL: return String_FromCString(vm, AS_BOOL(value) ? "true" : "false");
        case VALUE_NUMBER: {
            char cstring[50];
            snprintf(cstring, 50, "%g", AS_NUMBER(value));
            return String_FromCString(vm, cstring);
        }
        case VALUE_NIL: return String_FromCString("nil");
        case VALUE_OBJ: Object_ToString(AS_OBJ(value), vm);
    }
#endif
}
