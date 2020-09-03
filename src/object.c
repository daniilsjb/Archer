#include "object.h"
#include "objclass.h"
#include "objnative.h"
#include "vm.h"
#include "gc.h"
#include "memory.h"
#include "common.h"

void print_object(Object* object)
{
    object->type->print(object);
}

uint32_t hash_object(Object* object)
{
    return object->type->hash(object);
}

Value get_method_object(Object* object, Object* key, VM* vm)
{
    return object->type->getMethod(object, key, vm);
}

bool call_object(Object* callee, uint8_t argCount, VM* vm)
{
    return callee->type->call(callee, argCount, vm);
}

void traverse_object(Object* object, GC* gc)
{
#if DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    object->type->traverse(object, gc);
}

void free_object(Object* object, GC* gc)
{
    object->type->free(object, gc);
}

Object* allocate_object(VM* vm, size_t size, ObjectType* type)
{
    Object* object = (Object*)reallocate(&vm->gc, NULL, 0, size);
    object->type = type;
    object->marked = false;

    gc_append_object(&vm->gc, object);

#if DEBUG_LOG_GC
    printf("%p allocated object of type %d\n", (void*)object, object->type);
#endif

    return object;
}

bool value_is_object_of_type(Value value, ObjectType* type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

Value generic_get_method(Object* object, Object* key, VM* vm)
{
    Value method;
    if (!table_get(&object->type->methods, (ObjString*)key, &method)) {
        return NIL_VAL();
    }

    return OBJ_VAL(new_bound_method(vm, OBJ_VAL(object), AS_OBJ(method)));
}
