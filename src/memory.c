#include <stdlib.h>

#include "memory.h"
#include "vm.h"
#include "gc.h"
#include "memlib.h"

void* allocate(VM* vm, size_t size)
{
    gc_allocate_bytes(&vm->gc, size);
    gc_attempt_collection(&vm->gc, vm);

    void* allocated = raw_allocate(size);
    if (!allocated) {
        //TODO: Throw runtime error in the VM
        exit(1);
    }

    return allocated;
}

void deallocate(VM* vm, void* pointer, size_t size)
{
    gc_deallocate_bytes(&vm->gc, size);
    raw_deallocate(pointer);
}

void* reallocate(VM* vm, void* pointer, size_t oldSize, size_t newSize)
{
    if (newSize > oldSize) {
        gc_allocate_bytes(&vm->gc, newSize - oldSize);
        gc_attempt_collection(&vm->gc, vm);
    } else {
        gc_deallocate_bytes(&vm->gc, oldSize - newSize);
    }

    void* reallocated = raw_reallocate(pointer, newSize);
    if (!reallocated) {
        //TODO: Throw runtime error in the VM
        exit(1);
    }

    return reallocated;
}
