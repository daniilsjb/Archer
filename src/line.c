#include "line.h"
#include "memory.h"

void line_array_init(LineArray* array)
{
    array->count = 0;
    array->capacity = 0;
    array->lines = NULL;
}

void line_array_free(LineArray* array)
{
    FREE_ARRAY(Line, array->lines, array->capacity);
    line_array_init(array);
}

void line_array_write(LineArray* array, int line)
{
    if (array->count > 0) {
        Line* currentLine = &array->lines[array->count - 1];
        if (currentLine->number == line) {
            currentLine->count++;
            return;
        }
    }

    if (array->count >= array->capacity) {
        size_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->lines = GROW_ARRAY(Line, array->lines, oldCapacity, array->capacity);
    }

    array->lines[array->count] = (Line){ .number = line, .count = 1 };
    array->count++;
}
