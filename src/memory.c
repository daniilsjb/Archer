#include "memory.h"
#include "memlib.h"
#include "gc.h"

void* allocate(GC* gc, size_t size)
{
    gc_allocate_bytes(gc, size);
    gc_attempt_collection(gc);

    return raw_allocate(size);
}

void deallocate(GC* gc, void* pointer, size_t size)
{
    gc_deallocate_bytes(gc, size);
    raw_deallocate(pointer);
}

void* reallocate(GC* gc, void* pointer, size_t oldSize, size_t newSize)
{
    if (newSize > oldSize) {
        gc_allocate_bytes(gc, newSize - oldSize);
        gc_attempt_collection(gc);
    } else {
        gc_deallocate_bytes(gc, oldSize - newSize);
    }

    return raw_reallocate(pointer, newSize);
}
