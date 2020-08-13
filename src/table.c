#include <string.h>

#include "table.h"
#include "memory.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

void table_init(Table* table)
{
    table->count = 0;
    table->capacityMask = -1;
    table->entries = NULL;
}

void table_free(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacityMask + 1);
    table_init(table);
}

static Entry* find_entry(Entry* entries, int capacityMask, ObjString* key)
{
    uint32_t index = key->hash & capacityMask;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else if (tombstone == NULL) {
                tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) & capacityMask;
    }
}

static void adjust_capacity(Table* table, int capacityMask)
{
    Entry* entries = ALLOCATE(Entry, capacityMask + 1);
    for (int i = 0; i <= capacityMask; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL();
    }

    table->count = 0;
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        Entry* dest = find_entry(entries, capacityMask, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;

        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacityMask + 1);

    table->entries = entries;
    table->capacityMask = capacityMask;
}

bool table_get(Table* table, ObjString* key, Value* value)
{
    if (table->count == 0) {
        return false;
    }

    Entry* entry = find_entry(table->entries, table->capacityMask, key);
    if (entry->key == NULL) {
        return false;
    }

    *value = entry->value;
    return true;
}

bool table_put(Table* table, ObjString* key, Value value)
{
    if ((double)table->count + 1 > ((double)table->capacityMask + 1) * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacityMask + 1) - 1;
        adjust_capacity(table, capacity);
    }

    Entry* entry = find_entry(table->entries, table->capacityMask, key);

    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void table_put_from(Table* source, Table* destination)
{
    for (int i = 0; i <= source->capacityMask; i++) {
        Entry* entry = &source->entries[i];
        if (entry->key != NULL) {
            table_put(destination, entry->key, entry->value);
        }
    }
}

bool table_remove(Table* table, ObjString* key)
{
    if (table->count == 0) {
        return false;
    }

    Entry* entry = find_entry(table->entries, table->capacityMask, key);
    if (entry == NULL) {
        return false;
    }

    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

ObjString* table_find_string(Table* table, const char* chars, size_t length, uint32_t hash)
{
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash & table->capacityMask;
    while (true) {
        Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return NULL;
            }
        } else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) & table->capacityMask;
    }

    return NULL;
}

void table_remove_white(Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.marked) {
            table_remove(table, entry->key);
        }
    }
}

void mark_table(Table* table)
{
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        mark_object((Obj*)entry->key);
        mark_value(entry->value);
    }
}
