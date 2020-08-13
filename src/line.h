#ifndef LINE_H
#define LINE_H

#include "common.h"

typedef struct {
    int number;
    int count;
} Line;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    Line* lines;
} LineArray;

void line_array_init(LineArray* array);
void line_array_write(LineArray* array, int line);
void line_array_free(LineArray* array);

#endif
