#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "value.h"

typedef struct ObjString ObjString;
typedef struct VM VM;
typedef struct GC GC;

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct Table {
    int count;
    int capacityMask;
    Entry* entries;
} Table;

void table_init(Table* table);
void table_free(GC* gc, Table* table);

bool table_get(Table* table, ObjString* key, Value* value);

bool table_put(VM* vm, Table* table, ObjString* key, Value value);
void table_put_from(VM* vm, Table* source, Table* destination);

bool table_remove(Table* table, ObjString* key);

ObjString* table_find_string(Table* table, const char* chars, size_t length, uint32_t hash);

#endif
