#include <stdlib.h>

#include "memlib.h"

void* raw_allocate(size_t size)
{
    if (size == 0) {
        size = 1;
    }

    return malloc(size);
}

void raw_deallocate(void* pointer)
{
    free(pointer);
}

void* raw_reallocate(void* pointer, size_t size)
{
    if (size == 0) {
        size = 1;
    }

    return realloc(pointer, size);
}
