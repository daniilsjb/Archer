#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "value.h"

typedef struct ObjectString ObjectString;
typedef struct VM VM;
typedef struct GC GC;

typedef struct {
    ObjectString* key;
    Value value;
} Entry;

typedef struct Table {
    int count;
    int capacityMask;
    Entry* entries;
} Table;

void table_init(Table* table);
void table_free(GC* gc, Table* table);

bool table_get(Table* table, ObjectString* key, Value* value);

bool table_put(VM* vm, Table* table, ObjectString* key, Value value);
void table_put_from(VM* vm, Table* source, Table* destination);

bool table_remove(Table* table, ObjectString* key);

ObjectString* table_find_string(Table* table, const char* chars, size_t length, uint32_t hash);

#endif
