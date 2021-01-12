#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

void init_table(b_table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void free_table(b_table *table) {
  FREE_ARRAY(b_entry, table->entries, table->capacity + 1);
  init_table(table);
}

static b_entry *find_entry(b_entry *entries, int capacity, b_value key) {
  uint32_t hash = hash_value(key);

#if DEBUG_MODE == 1
#if DEBUG_TABLE == 1
  printf("looking for key ");
  print_value(key);
  printf(" with hash %u in table...\n", hash);
#endif
#endif

  uint32_t index = hash % capacity;
  b_entry *tombstone = NULL;

  for (;;) {
    b_entry *entry = &entries[index];

    if (IS_EMPTY(entry->key)) {
      if (IS_NIL(entry->value)) {
        // empty entry
        return tombstone != NULL ? tombstone : entry;
      } else {
        // we found a tombstone.
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (values_equal(key, entry->key)) {
#if DEBUG_MODE == 1
#if DEBUG_TABLE == 1
      printf("found entry for key ");
      print_value(key);
      printf(" with hash %u in table as ", hash);
      print_value(entry->value);
      printf("...\n");
#endif
#endif

      return entry;
    }

    index = (index + 1) % capacity;
  }
}

bool table_get(b_table *table, b_value key, b_value *value) {
  if (table->count == 0 || table->entries == NULL)
    return false;

#if DEBUG_MODE == 1
#if DEBUG_TABLE == 1
  printf("getting entry with hash %u...\n", hash_value(key));
#endif
#endif

  b_entry *entry = find_entry(table->entries, table->capacity, key);

  if (IS_NIL(entry->key) || IS_EMPTY(entry->key))
    return false;

#ifdef DEBUG_TABLE
  printf("found entry for hash %u == ", hash_value(entry->key));
  print_value(entry->value);
  printf("\n");
#endif

  *value = entry->value;
  return true;
}

static void adjust_capacity(b_table *table, int capacity) {
  b_entry *entries = ALLOCATE(b_entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = EMPTY_VAL;
    entries[i].value = NIL_VAL;
  }

  // repopulate buckets
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (IS_EMPTY(entry->key))
      continue;

    b_entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  // free the old entries...
  FREE_ARRAY(b_entry, table->entries, table->capacity);

  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(b_table *table, b_value key, b_value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  b_entry *entry = find_entry(table->entries, table->capacity, key);

  bool is_new = IS_EMPTY(entry->key);

  if (is_new && IS_NIL(entry->value))
    table->count++;

  // overwrites exisiting entries.
  entry->key = key;
  entry->value = value;

  return is_new;
}

bool table_delete(b_table *table, b_value key) {
  if (table->count == 0)
    return false;

  // find the entry
  b_entry *entry = find_entry(table->entries, table->capacity, key);
  if (IS_EMPTY(entry->key))
    return false;

  // place a tombstone in the entry.
  entry->key = EMPTY_VAL;
  entry->value = BOOL_VAL(true);

  return true;
}

void table_add_all(b_table *from, b_table *to) {
  for (int i = 0; i <= from->capacity; i++) {
    b_entry *entry = &from->entries[i];
    if (!IS_EMPTY(entry->key)) {
      table_set(to, entry->key, entry->value);
    }
  }
}

b_obj_string *table_find_string(b_table *table, const char *chars, int length,
                                uint32_t hash) {
  if (table->count == 0)
    return NULL;

  uint32_t index = hash % table->capacity;

  for (;;) {
    b_entry *entry = &table->entries[index];

    if (IS_EMPTY(entry->key)) {
      // stop if we find an empty non-tombstone entry
      if (IS_NIL(entry->value))
        return NULL;
    }

    if (IS_STRING(entry->key)) {
      b_obj_string *string = AS_STRING(entry->key);
      if (string->length == length && string->hash == hash &&
          memcmp(string->chars, chars, length) == 0) {
        // we found it
        return string;
      }
    }

    index = (index + 1) % table->capacity;
  }
}

void table_print(b_table *table) {
  printf("<HashTable: [");
  for (int i = 0; i <= table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (!IS_NIL(entry->key) && !IS_EMPTY(entry->key)) {
      printf("{key: %u, value: ", hash_value(entry->key));
      print_value(entry->value);
      printf("}");
    }
  }
  printf("]>\n");
}