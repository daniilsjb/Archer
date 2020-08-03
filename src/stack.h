#pragma once

#include "common.h"
#include "value.h"

typedef struct {
    uint32_t count;
    uint32_t capacity;
    Value* values;
} Stack;

void stack_init(Stack* stack);
void stack_push(Stack* stack, Value value);
Value stack_pop(Stack* stack);
void stack_free(Stack* stack);