#include "memory.h"
#include "gc.h"

#include <stdio.h>

void* xmalloc(size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Could not allocate memory!\n");
        abort();
    }

    return ptr;
}

void* Mem_Allocate(GC* gc, size_t size)
{
    GC_AllocateBytes(gc, size);
    GC_AttemptCollection(gc);

    return xmalloc(size);
}

void Mem_Deallocate(GC* gc, void* pointer, size_t size)
{
    GC_DeallocateBytes(gc, size);
    free(pointer);
}

void* Mem_Reallocate(GC* gc, void* pointer, size_t oldSize, size_t newSize)
{
    if (newSize > oldSize) {
        GC_AllocateBytes(gc, newSize - oldSize);
        GC_AttemptCollection(gc);
    } else {
        GC_DeallocateBytes(gc, oldSize - newSize);
    }

    return realloc(pointer, newSize);
}
