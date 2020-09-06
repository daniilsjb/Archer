#include "memory.h"
#include "memlib.h"
#include "gc.h"

void* Mem_Allocate(GC* gc, size_t size)
{
    GC_AllocateBytes(gc, size);
    GC_AttemptCollection(gc);

    return raw_allocate(size);
}

void Mem_Deallocate(GC* gc, void* pointer, size_t size)
{
    GC_DeallocateBytes(gc, size);
    raw_deallocate(pointer);
}

void* Mem_Reallocate(GC* gc, void* pointer, size_t oldSize, size_t newSize)
{
    if (newSize > oldSize) {
        GC_AllocateBytes(gc, newSize - oldSize);
        GC_AttemptCollection(gc);
    } else {
        GC_DeallocateBytes(gc, oldSize - newSize);
    }

    return raw_reallocate(pointer, newSize);
}
