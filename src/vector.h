#ifndef VECTOR_H
#define VECTOR_H

#include "memory.h"

#define VECTOR(type)                                                                       \
    struct {                                                                               \
        size_t count;                                                                      \
        size_t capacity;                                                                   \
        type* data;                                                                        \
    }                                                                                      \

#define VECTOR_INIT(vecType, vec)                                                          \
    do {                                                                                   \
        vecType* vector = (vec);                                                           \
        vector->count = 0;                                                                 \
        vector->capacity = 0;                                                              \
        vector->data = NULL;                                                               \
    } while (0)                                                                            \

#define VECTOR_FREE(gc, vecType, vec, type)                                                \
    do {                                                                                   \
        vecType* vector = (vec);                                                           \
        FREE_ARRAY(gc, type, vector->data, vector->capacity);                              \
        VECTOR_INIT(vecType, vec);                                                         \
    } while (0)                                                                            \

#define VECTOR_PUSH(gc, vecType, vec, type, value)                                         \
    do {                                                                                   \
        vecType* vector = (vec);                                                           \
        if (vector->count >= vector->capacity) {                                           \
            size_t capacity = vector->capacity;                                            \
            vector->capacity = GROW_CAPACITY(capacity);                                    \
            vector->data = GROW_ARRAY(gc, type, vector->data, capacity, vector->capacity); \
        }                                                                                  \
                                                                                           \
        vector->data[vector->count] = (value);                                             \
        vector->count++;                                                                   \
    } while (0)                                                                            \

#define VECTOR_POP(vec) (vec)->count--

#endif
