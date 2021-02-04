#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "value.h"

typedef struct ObjectString ObjectString;
typedef struct VM VM;
typedef struct GC GC;

typedef struct {
    Value key;
    Value value;
} Entry;

typedef struct Table {
    size_t size;
    int count;
    int capacityMask;
    Entry* entries;
} Table;

void Table_Init(Table* table);
void Table_Free(GC* gc, Table* table);

size_t Table_Size(Table* table);

bool Table_Get(Table* table, Value key, Value* value);

bool Table_Put(VM* vm, Table* table, Value key, Value value);
void Table_PutFrom(VM* vm, Table* source, Table* destination);

bool Table_Remove(Table* table, Value key);

ObjectString* Table_FindString(Table* table, const char* chars, size_t length, uint32_t hash);

#endif
