#pragma once

#include "stack.h"
#include "memory.h"

void stack_init(Stack* stack)
{
    stack->count = 0;
    stack->capacity = 0;
    stack->values = NULL;
}

void stack_push(Stack* stack, Value value)
{
    if (stack->count >= stack->capacity) {
        int oldCapacity = stack->capacity;
        stack->capacity = GROW_CAPACITY(oldCapacity);
        stack->values = GROW_ARRAY(Value, stack->values, oldCapacity, stack->capacity);
    }

    stack->values[stack->count] = value;
    stack->count++;
}

Value stack_pop(Stack* stack)
{
    stack->count--;
    return stack->values[stack->count];
}

void stack_free(Stack* stack)
{
    stack->values = FREE_ARRAY(Value, stack->values, stack->count);
    stack_init(stack);
}
