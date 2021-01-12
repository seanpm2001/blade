#include <stdlib.h>

#include "blob.h"
#include "memory.h"

void init_blob(b_blob *blob) {
  blob->count = 0;
  blob->capacity = 0;
  blob->code = NULL;
  blob->lines = NULL;
  init_value_arr(&blob->constants);
}

void write_blob(b_blob *blob, uint8_t byte, int line) {
  if (blob->capacity < blob->count + 1) {
    int old_capacity = blob->capacity;
    blob->capacity = GROW_CAPACITY(old_capacity);
    blob->code = GROW_ARRAY(uint8_t, blob->code, old_capacity, blob->capacity);
    blob->lines = GROW_ARRAY(int, blob->lines, old_capacity, blob->capacity);
  }

  blob->code[blob->count] = byte;
  blob->lines[blob->count] = line;
  blob->count++;
}

void free_blob(b_blob *blob) {
  FREE_ARRAY(uint8_t, blob->code, blob->capacity);
  FREE_ARRAY(int, blob->lines, blob->capacity);
  init_value_arr(&blob->constants);
  init_blob(blob);
}

int add_constant(b_blob *blob, b_value value) {
  write_value_arr(&blob->constants, value);
  return blob->constants.count - 1;
}