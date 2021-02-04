#include <stdio.h>

#include "vm.h"
#include "memory.h"
#include "gc.h"

#include "obj_module.h"
#include "obj_function.h"
#include "obj_string.h"

static ObjectString* module_to_string(Object* object, VM* vm)
{
    ObjectModule* mod = AS_MODULE(object);

    char cstring[100];
    snprintf(cstring, 100, "<module '%s'>", AS_CSTRING(mod->name));
    return String_FromCString(vm, cstring);
}

static void module_print(Object* object)
{
    ObjectModule* mod = AS_MODULE(object);
    printf("<module '%s'>", AS_CSTRING(mod->name));
}

static void module_traverse(Object* object, GC* gc)
{
    ObjectModule* mod = AS_MODULE(object);
    GC_MarkObject(gc, (Object*)mod->path);
    GC_MarkObject(gc, (Object*)mod->name);
    Object_GenericTraverse(object, gc);
}

ObjectType* Module_NewType(VM* vm)
{
    ObjectType* type = Type_New(vm);
    type->name = "Module";
    type->size = sizeof(ObjectModule);
    type->flags = 0x0;
    type->ToString = module_to_string;
    type->Print = module_print;
    type->Hash = Object_GenericHash;
    type->GetField = Object_GenericGetField;
    type->SetField = Object_GenericSetField;
    type->GetSubscript = NULL;
    type->SetSubscript = NULL;
    type->GetMethod = NULL;
    type->SetMethod = NULL;
    type->MakeIterator = NULL;
    type->Call = NULL;
    type->Traverse = module_traverse;
    type->Free = Object_GenericFree;
    return type;
}

void Module_PrepareType(ObjectType* type, VM* vm)
{
}

ObjectModule* Module_New(VM* vm, ObjectString* path, ObjectString* name)
{
    ObjectModule* mod = ALLOCATE_MODULE(vm);
    mod->path = path;
    mod->name = name;
    mod->imported = false;
    return mod;
}

ObjectModule* Module_FromFullPath(VM* vm, const char* fullPath)
{
    ObjectString* path = NULL;
    ObjectString* name = NULL;

    char* dest = strrchr(fullPath, '/');
    if (!dest) {
        path = String_MakeEmpty(vm);
        Vm_PushTemporary(vm, OBJ_VAL(path));
        name = String_Copy(vm, fullPath, strlen(fullPath));
        Vm_PushTemporary(vm, OBJ_VAL(name));
    } else {
        path = String_Copy(vm, fullPath, strlen(fullPath) - strlen(dest) + 1);
        Vm_PushTemporary(vm, OBJ_VAL(path));
        name = String_Copy(vm, dest + 1, strlen(dest + 1));
        Vm_PushTemporary(vm, OBJ_VAL(name));
    }

    ObjectModule* mod = Module_New(vm, path, name);
    Vm_PopTemporary(vm);
    Vm_PopTemporary(vm);
    return mod;
}
