#include <stdio.h>
#include <ctype.h>

#include "objstring.h"
#include "objnative.h"
#include "vm.h"
#include "memory.h"
#include "gc.h"
#include "library.h"

static bool method_init(VM* vm, Value* args)
{
    //TODO: By creating a new string here we are discarding the object that was pre-allocated, find a way to fix this
    args[-1] = OBJ_VAL(String_FromValue(vm, args[0]));
    return true;
}

static bool method_length(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)VAL_AS_STRING(args[-1])->length);
    return true;
}

static bool method_is_empty(VM* vm, Value* args)
{
    args[-1] = BOOL_VAL(VAL_AS_STRING(args[-1])->length == 0);
    return true;
}

static bool method_to_lower(VM* vm, Value* args)
{
    ObjectString* object = VAL_AS_STRING(args[-1]);

    char* cstring = _strdup(object->chars);
    for (char* current = cstring; *current != '\0'; current++) {
        *current = tolower(*current);
    }

    args[-1] = OBJ_VAL(String_FromCString(vm, cstring));
    raw_deallocate(cstring);
    return true;
}

static bool method_to_upper(VM* vm, Value* args)
{
    ObjectString* object = VAL_AS_STRING(args[-1]);

    char* cstring = _strdup(object->chars);
    for (char* current = cstring; *current != '\0'; current++) {
        *current = toupper(*current);
    }

    args[-1] = OBJ_VAL(String_FromCString(vm, cstring));
    raw_deallocate(cstring);
    return true;
}

static bool method_starts_with(VM* vm, Value* args)
{
    if (!VAL_IS_STRING(args[0], vm)) {
        return Library_Error(vm, "Expected a string.", args);
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

static bool method_ends_with(VM* vm, Value* args)
{
    if (!VAL_IS_STRING(args[0], vm)) {
        return Library_Error(vm, "Expected a string.", args);
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

static bool method_from_number(VM* vm, Value* args)
{
    if (!IS_NUMBER(args[0])) {
        return Library_Error(vm, "Expected a number.", args);
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
    return AS_STRING(object)->hash;
}

static ObjectString* string_to_string(Object* object, VM* vm)
{
    //TODO: Should this return a copy or the string itself?
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
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = Object_GenericGetMethod;
    type->SetMethod = NULL;
    type->Call = NULL;
    type->Traverse = Object_GenericTraverse;
    type->Free = string_free;
    return type;
}

void String_PrepareType(ObjectType* type, VM* vm)
{
    Library_DefineTypeMethod(type, vm, "init", method_init, 1);
    Library_DefineTypeMethod(type, vm, "length", method_length, 0);
    Library_DefineTypeMethod(type, vm, "isEmpty", method_is_empty, 0);
    Library_DefineTypeMethod(type, vm, "toLower", method_to_lower, 0);
    Library_DefineTypeMethod(type, vm, "toUpper", method_to_upper, 0);
    Library_DefineTypeMethod(type, vm, "startsWith", method_starts_with, 1);
    Library_DefineTypeMethod(type, vm, "endsWith", method_ends_with, 1);

    Library_DefineTypeMethod(type->base.type, vm, "fromNumber", method_from_number, 1);
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

    vm_push_temporary(vm, OBJ_VAL(string));
    table_put(vm, &vm->strings, OBJ_VAL(string), NIL_VAL());
    vm_pop_temporary(vm);

    return string;
}

ObjectString* String_FromCString(VM* vm, const char* chars)
{
    return String_Copy(vm, chars, strlen(chars));
}

ObjectString* String_FromNil(VM* vm)
{
    return String_FromCString(vm, "nil");
}

ObjectString* String_FromBoolean(VM* vm, bool boolean)
{
    return String_FromCString(vm, boolean ? "true" : "false");
}

ObjectString* String_FromNumber(VM* vm, double number)
{
    char cstring[50];
    snprintf(cstring, 50, "%g", number);
    return String_FromCString(vm, cstring);
}

ObjectString* String_Concatenate(VM* vm, ObjectString* a, ObjectString* b)
{
    size_t length = a->length + b->length;
    ObjectString* string = String_New(vm, length);

    memcpy(string->chars, a->chars, a->length);
    memcpy(string->chars + a->length, b->chars, b->length);
    string->chars[length] = '\0';
    string->hash = hash_cstring(string->chars, string->length);

    ObjectString* interned = table_find_string(&vm->strings, string->chars, length, string->hash);
    if (interned != NULL) {
        vm->gc.allocatedObjects = vm->gc.allocatedObjects->next;
        Mem_Deallocate(&vm->gc, string, sizeof(ObjectString) + string->length + 1);

        return interned;
    } else {
        vm_push_temporary(vm, OBJ_VAL(string));
        table_put(vm, &vm->strings, OBJ_VAL(string), NIL_VAL());
        vm_pop_temporary(vm);

        return string;
    }
}

ObjectString* String_FromValue(VM* vm, Value value)
{
#if NAN_BOXING
    if IS_NIL(value) {
        return String_FromNil(vm);
    } else if (IS_BOOL(value)) {
        return String_FromBoolean(vm, AS_BOOL(value));
    } else if IS_NUMBER(value) {
        return String_FromNumber(vm, AS_NUMBER(value));
    } else {
        return Object_ToString(AS_OBJ(value), vm);
    }
#else
    switch (value.type) {
        case VALUE_NIL: return String_FromNil(vm);
        case VALUE_BOOL: return String_FromBoolean(vm, AS_BOOL(value));
        case VALUE_NUMBER: return String_FromNumber(vm, AS_NUMBER(value));
        case VALUE_OBJ: return Object_ToString(AS_OBJ(value), vm);
    }

    return NULL;
#endif
}
