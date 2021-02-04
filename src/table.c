#include <string.h>

#include "table.h"
#include "obj_string.h"
#include "vm.h"
#include "gc.h"
#include "memory.h"

#define TABLE_MAX_LOAD 0.75

void table_init(Table* table)
{
    table->size = 0;
    table->count = 0;
    table->capacityMask = -1;
    table->entries = NULL;
}

void table_free(GC* gc, Table* table)
{
    FREE_ARRAY(gc, Entry, table->entries, table->capacityMask + 1);
    table_init(table);
}

size_t table_size(Table* table)
{
    return table->size;
}

static Entry* find_entry(Entry* entries, int capacityMask, Value key)
{
    uint32_t index = value_hash(key) & capacityMask;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        if (IS_UNDEFINED(entry->key)) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else if (tombstone == NULL) {
                tombstone = entry;
            }
        } else if (values_equal(entry->key, key)) {
            return entry;
        }

        index = (index + 1) & capacityMask;
    }
}

static void adjust_capacity(VM* vm, Table* table, int capacityMask)
{
    Entry* entries = ALLOCATE(&vm->gc, Entry, (size_t)capacityMask + 1);
    for (int i = 0; i <= capacityMask; i++) {
        entries[i].key = UNDEFINED_VAL();
        entries[i].value = NIL_VAL();
    }

    table->count = 0;
    table->size = 0;
    for (int i = 0; i <= table->capacityMask; i++) {
        Entry* entry = &table->entries[i];
        if (IS_UNDEFINED(entry->key)) {
            continue;
        }

        Entry* dest = find_entry(entries, capacityMask, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;

        table->count++;
    }

    FREE_ARRAY(&vm->gc, Entry, table->entries, table->capacityMask + 1);

    table->entries = entries;
    table->capacityMask = capacityMask;
}

bool table_get(Table* table, Value key, Value* value)
{
    if (table->count == 0) {
        return false;
    }

    Entry* entry = find_entry(table->entries, table->capacityMask, key);
    if (IS_UNDEFINED(entry->key)) {
        return false;
    }

    *value = entry->value;
    return true;
}

bool table_put(VM* vm, Table* table, Value key, Value value)
{
    if ((double)table->count + 1 > ((double)table->capacityMask + 1) * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacityMask + 1) - 1;
        adjust_capacity(vm, table, capacity);
    }

    Entry* entry = find_entry(table->entries, table->capacityMask, key);

    bool isNewKey = IS_UNDEFINED(entry->key);
    if (isNewKey && IS_NIL(entry->value)) {
        table->count++;
        table->size++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void table_put_from(VM* vm, Table* source, Table* destination)
{
    for (int i = 0; i <= source->capacityMask; i++) {
        Entry* entry = &source->entries[i];
        if (!IS_UNDEFINED(entry->key)) {
            table_put(vm, destination, entry->key, entry->value);
        }
    }
}

bool table_remove(Table* table, Value key)
{
    if (table->count == 0) {
        return false;
    }

    Entry* entry = find_entry(table->entries, table->capacityMask, key);
    if (entry == NULL) {
        return false;
    }

    entry->key = UNDEFINED_VAL();
    entry->value = BOOL_VAL(true);
    table->size--;

    return true;
}

static bool strings_equal(const char* chars, size_t length, uint32_t hash, ObjectString* string)
{
    return string->length == length && string->hash == hash && memcmp(string->chars, chars, length) == 0;
}

ObjectString* table_find_string(Table* table, const char* chars, size_t length, uint32_t hash)
{
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash & table->capacityMask;
    while (true) {
        Entry* entry = &table->entries[index];
        if (IS_UNDEFINED(entry->key)) {
            if (IS_NIL(entry->value)) {
                return NULL;
            }
        } else if (strings_equal(chars, length, hash, VAL_AS_STRING(entry->key))) {
            return VAL_AS_STRING(entry->key);
        }

        index = (index + 1) & table->capacityMask;
    }

    return NULL;
}
