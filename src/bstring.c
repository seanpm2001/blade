#include "bstring.h"
#include "util.h"
#include "native.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_STRSEP
#include <strsep.h>
#endif /* ifndef HAVE_STRSEP */

/**
 * a Blade regex must always start and end with the same delimiter e.g. /
 *
 * e.g.
 * /\d+/
 *
 * it can be followed by one or more matching fine tuning constants
 *
 * e.g.
 *
 * /\d+.+[a-z]+/sim -> '.' matches all, it's case insensitive and multiline
 * (see the function for list of available options)
 *
 * returns:
 * -1 -> false
 * 0 -> true
 * negative value -> invalid delimiter where abs(value) is the character
 * positive value > 0 ? for compiled delimiters
 */
uint32_t is_regex(b_obj_string *string) {
  char start = string->chars[0];
  bool match_found = false;

  uint32_t c_options = 0; // pcre2 options

  for (int i = 1; i < string->length; i++) {
    if (string->chars[i] == start) {
      match_found = i > 0 && string->chars[i - 1] == '\\' ? false : true;
      continue;
    }

    if (match_found) {
      // compile the delimiters
      switch (string->chars[i]) {
        /* Perl compatible options */
        case 'i':
          c_options |= PCRE2_CASELESS;
          break;
        case 'm':
          c_options |= PCRE2_MULTILINE;
          break;
        case 's':
          c_options |= PCRE2_DOTALL;
          break;
        case 'x':
          c_options |= PCRE2_EXTENDED;
          break;

          /* PCRE specific options */
        case 'A':
          c_options |= PCRE2_ANCHORED;
          break;
        case 'D':
          c_options |= PCRE2_DOLLAR_ENDONLY;
          break;
        case 'U':
          c_options |= PCRE2_UNGREEDY;
          break;
        case 'u':
          c_options |= PCRE2_UTF;
          /* In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only
         ASCII characters, even in UTF-8 mode. However, this can be changed by
         setting the PCRE2_UCP option. */
#ifdef PCRE2_UCP
          c_options |= PCRE2_UCP;
#endif
          break;
        case 'J':
          c_options |= PCRE2_DUPNAMES;
          break;

        case ' ':
        case '\n':
        case '\r':
          break;

        default:
          return c_options = (uint32_t) string->chars[i] + 1000000;
      }
    }
  }

  if (!match_found)
    return -1;
  else
    return c_options;
}

char *remove_regex_delimiter(b_vm *vm, b_obj_string *string) {
  if (string->length == 0)
    return string->chars;

  char start = string->chars[0];
  int i = string->length - 1;
  for (; i > 0; i--) {
    if (string->chars[i] == start)
      break;
  }

  char *str = ALLOCATE(char, i);
  memcpy(str, string->chars + 1, (size_t) i - 1);
  str[i - 1] = '\0';

  return str;
}

DECLARE_STRING_METHOD(length) {
  ENFORCE_ARG_COUNT(length, 0);
  b_obj_string* string = AS_STRING(METHOD_OBJECT);
  RETURN_NUMBER(string->is_ascii ? string->length : string->utf8_length);
}

DECLARE_STRING_METHOD(upper) {
  ENFORCE_ARG_COUNT(upper, 0);
  char *string = (char *) strdup(AS_C_STRING(METHOD_OBJECT));
  for (char *p = string; *p; p++)
    *p = toupper(*p);
  RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(lower) {
  ENFORCE_ARG_COUNT(lower, 0);
  char *string = (char *) strdup(AS_C_STRING(METHOD_OBJECT));
  for (char *p = string; *p; p++)
    *p = tolower(*p);
  RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(is_alpha) {
  ENFORCE_ARG_COUNT(is_alpha, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isalpha((unsigned char) string->chars[i])) {
      RETURN_FALSE;
    }
  }
  RETURN_BOOL(string->length != 0);
}

DECLARE_STRING_METHOD(is_alnum) {
  ENFORCE_ARG_COUNT(is_alnum, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isalnum((unsigned char) string->chars[i])) {
      RETURN_FALSE;
    }
  }
  RETURN_BOOL(string->length != 0);
}

DECLARE_STRING_METHOD(is_number) {
  ENFORCE_ARG_COUNT(is_number, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isdigit((unsigned char) string->chars[i])) {
      RETURN_FALSE;
    }
  }
  RETURN_BOOL(string->length != 0);
}

DECLARE_STRING_METHOD(is_lower) {
  ENFORCE_ARG_COUNT(is_lower, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  bool has_alpha;
  for (int i = 0; i < string->length; i++) {
    bool is_alpha = isalpha((unsigned char) string->chars[i]);
    if (!has_alpha) {
      has_alpha = is_alpha;
    }
    if (is_alpha && !islower((unsigned char) string->chars[i])) {
      RETURN_FALSE;
    }
  }
  RETURN_BOOL(string->length != 0 && has_alpha);
}

DECLARE_STRING_METHOD(is_upper) {
  ENFORCE_ARG_COUNT(is_upper, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  bool has_alpha;
  for (int i = 0; i < string->length; i++) {
    bool is_alpha = isalpha((unsigned char) string->chars[i]);
    if (!has_alpha) {
      has_alpha = is_alpha;
    }
    if (is_alpha && !isupper((unsigned char) string->chars[i])) {
      RETURN_FALSE;
    }
  }
  RETURN_BOOL(string->length != 0 && has_alpha);
}

DECLARE_STRING_METHOD(is_space) {
  ENFORCE_ARG_COUNT(is_space, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isspace((unsigned char) string->chars[i])) {
      RETURN_FALSE;
    }
  }
  RETURN_BOOL(string->length != 0);
}

DECLARE_STRING_METHOD(trim) {
  ENFORCE_ARG_RANGE(trim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(trim, 0, IS_CHAR);
    trimmer = (char) AS_STRING(args[0])->chars[0];
  }

  char *string = AS_C_STRING(METHOD_OBJECT);

  char *end = NULL;

  // Trim leading space
  if (trimmer == '\0') {
    while (isspace((unsigned char) *string))
      string++;
  } else {
    while (trimmer == *string)
      string++;
  }

  if (*string == 0) { // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));
  }

  // Trim trailing space
  end = string + strlen(string) - 1;
  if (trimmer == '\0') {
    while (end > string && isspace((unsigned char) *end))
      end--;
  } else {
    while (end > string && trimmer == *end)
      end--;
  }

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(ltrim) {
  ENFORCE_ARG_RANGE(ltrim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(ltrim, 0, IS_CHAR);
    trimmer = (char) AS_STRING(args[0])->chars[0];
  }

  char *string = AS_C_STRING(METHOD_OBJECT);

  char *end = NULL;

  // Trim leading space
  if (trimmer == '\0') {
    while (isspace((unsigned char) *string))
      string++;
  } else {
    while (trimmer == *string)
      string++;
  }

  if (*string == 0) { // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));
  }

  end = string + strlen(string) - 1;

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(rtrim) {
  ENFORCE_ARG_RANGE(rtrim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(rtrim, 0, IS_CHAR);
    trimmer = (char) AS_STRING(args[0])->chars[0];
  }

  char *string = AS_C_STRING(METHOD_OBJECT);

  char *end = NULL;

  if (*string == 0) { // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));
  }

  end = string + strlen(string) - 1;
  if (trimmer == '\0') {
    while (end > string && isspace((unsigned char) *end))
      end--;
  } else {
    while (end > string && trimmer == *end)
      end--;
  }

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(join) {
  ENFORCE_ARG_COUNT(join, 1);
  ENFORCE_ARG_TYPE(join, 0, IS_OBJ);

  b_obj_string *method_obj = AS_STRING(METHOD_OBJECT);
  b_value argument = args[0];
  int length = 0;
  char **array = NULL;

  if (IS_STRING(argument)) {
    // empty argument
    if (method_obj->length == 0) {
      RETURN_VALUE(argument);
    } else if (AS_STRING(argument)->length == 0) {
      RETURN_VALUE(argument);
    }

    b_obj_string *string = AS_STRING(argument);

    char *result = ALLOCATE(char, 2);
    result[0] = string->chars[0];
    result[1] = '\0';

    for (int i = 1; i < string->length; i++) {
      if (method_obj->length > 0) {
        result = append_strings(result, method_obj->chars);
      }

      char *chr = (char *) calloc(2, sizeof(char));
      chr[0] = string->chars[i];
      chr[1] = '\0';

      result = append_strings(result, chr);
      free(chr);
    }

    RETURN_TT_STRING(result);
  } else if (IS_LIST(argument) || IS_DICT(argument)) {
    b_value *list;
    int count = 0;
    if (IS_DICT(argument)) {
      list = AS_DICT(argument)->names.values;
      count = AS_DICT(argument)->names.count;
    } else {
      list = AS_LIST(argument)->items.values;
      count = AS_LIST(argument)->items.count;
    }

    if (count == 0) {
      RETURN_STRING("");
    }

    char *result = value_to_string(vm, list[0]);

    for (int i = 1; i < count; i++) {
      if (method_obj->length > 0) {
        result = append_strings(result, method_obj->chars);
      }

      char *str = value_to_string(vm, list[i]);
      result = append_strings(result, str);
      free(str);
    }

    RETURN_TT_STRING(result);
  }

  RETURN_ERROR("join() does not support object of type %s",
               value_type(argument));
}

DECLARE_STRING_METHOD(split) {
  ENFORCE_ARG_COUNT(split, 1);
  ENFORCE_ARG_TYPE(split, 0, IS_STRING);

  b_obj_string *object = AS_STRING(METHOD_OBJECT);
  b_obj_string *delimeter = AS_STRING(args[0]);

  if (object->length == 0 || delimeter->length > object->length) RETURN_OBJ(new_list(vm));

  b_obj_list *list = (b_obj_list *) GC(new_list(vm));

  // main work here...
  if (delimeter->length > 0) {
    int start = 0;
    for(int i = 0; i <= object->length; i++) {
      // match found.
      if(memcmp(object->chars + i, delimeter->chars, delimeter->length) == 0 || i == object->length) {
        write_list(vm, list, GC_L_STRING(object->chars + start, i - start));
        i += delimeter->length - 1;
        start = i + 1;
      }
    }
  } else {
    int length = object->is_ascii ? object->length : object->utf8_length;
    for (int i = 0; i < length; i++) {

      int start = i, end = i + 1;
      if(!object->is_ascii) {
        utf8slice(object->chars, &start, &end);
      }

      write_list(vm, list, GC_L_STRING(object->chars + start, (int) (end - start)));
    }
  }

  RETURN_OBJ(list);
}

DECLARE_STRING_METHOD(index_of) {
  ENFORCE_ARG_COUNT(index_of, 1);
  ENFORCE_ARG_TYPE(index_of, 0, IS_STRING);

  char *str = AS_C_STRING(METHOD_OBJECT);
  char *result = strstr(str, AS_C_STRING(args[0]));

  if (result != NULL) RETURN_NUMBER((int) (result - str));
  RETURN_NUMBER(-1);
}

DECLARE_STRING_METHOD(starts_with) {
  ENFORCE_ARG_COUNT(starts_with, 1);
  ENFORCE_ARG_TYPE(starts_with, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 || substr->length == 0 ||
      substr->length > string->length) RETURN_FALSE;

  RETURN_BOOL(memcmp(substr->chars, string->chars, substr->length) == 0);
}

DECLARE_STRING_METHOD(ends_with) {
  ENFORCE_ARG_COUNT(ends_with, 1);
  ENFORCE_ARG_TYPE(ends_with, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 || substr->length == 0 ||
      substr->length > string->length) RETURN_FALSE;

  int difference = string->length - substr->length;

  RETURN_BOOL(memcmp(substr->chars, string->chars + difference, substr->length) == 0);
}

DECLARE_STRING_METHOD(count) {
  ENFORCE_ARG_COUNT(count, 1);
  ENFORCE_ARG_TYPE(count, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (substr->length == 0 || string->length == 0) RETURN_NUMBER(0);

  int count = 0;
  const char *tmp = string->chars;
  while ((tmp = strstr(tmp, substr->chars))) {
    count++;
    tmp++;
  }

  RETURN_NUMBER(count);
}

DECLARE_STRING_METHOD(to_number) {
  ENFORCE_ARG_COUNT(to_number, 0);
  RETURN_NUMBER(strtod(AS_C_STRING(METHOD_OBJECT), NULL));
}

DECLARE_STRING_METHOD(ascii) {
  ENFORCE_ARG_COUNT(ascii, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  string->is_ascii = true;
  RETURN_OBJ(string);
}

DECLARE_STRING_METHOD(to_list) {
  ENFORCE_ARG_COUNT(to_list, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_list *list = (b_obj_list *) GC(new_list(vm));
  int length = string->is_ascii ? string->length : string->utf8_length;

  if (length > 0) {

    for (int i = 0; i < length; i++) {
      int start = i, end = i + 1;
      if(!string->is_ascii) {
        utf8slice(string->chars, &start, &end);
      }
      write_list(vm, list, GC_L_STRING(string->chars + start, (int) (end - start)));
    }
  }

  RETURN_OBJ(list);
}

DECLARE_STRING_METHOD(lpad) {
  ENFORCE_ARG_RANGE(lpad, 1, 2);
  ENFORCE_ARG_TYPE(lpad, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int width = AS_NUMBER(args[0]);
  char fill_char = ' ';

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(lpad, 1, IS_CHAR);
    fill_char = AS_C_STRING(args[1])[0];
  }

  if (width <= string->utf8_length) RETURN_VALUE(METHOD_OBJECT);

  int fill_size = width - string->utf8_length;
  char *fill = ALLOCATE(char, (size_t) fill_size + 1);

  int final_size = string->length + fill_size;
  int final_utf8_size = string->utf8_length + fill_size;

  for (int i = 0; i < fill_size; i++)
    fill[i] = fill_char;

  char *str = ALLOCATE(char, (size_t) string->length + (size_t) fill_size + 1);
  memcpy(str, fill, fill_size);
  memcpy(str + fill_size, string->chars, string->length);
  str[final_size] = '\0';

  b_obj_string *result = take_string(vm, str, final_size);
  result->utf8_length = final_utf8_size;
  result->length = final_size;
  RETURN_OBJ(result);
}

DECLARE_STRING_METHOD(rpad) {
  ENFORCE_ARG_RANGE(rpad, 1, 2);
  ENFORCE_ARG_TYPE(rpad, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int width = AS_NUMBER(args[0]);
  char fill_char = ' ';

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(rpad, 1, IS_CHAR);
    fill_char = AS_C_STRING(args[1])[0];
  }

  if (width <= string->utf8_length) RETURN_VALUE(METHOD_OBJECT);

  int fill_size = width - string->utf8_length;
  char *fill = ALLOCATE(char, (size_t) fill_size + 1);

  int final_size = string->length + fill_size;
  int final_utf8_size = string->utf8_length + fill_size;

  for (int i = 0; i < fill_size; i++)
    fill[i] = fill_char;

  char *str = ALLOCATE(char, (size_t) string->length + (size_t) fill_size + 1);
  memcpy(str, string->chars, string->length);
  memcpy(str + string->length, fill, fill_size);
  str[final_size] = '\0';

  b_obj_string *result = take_string(vm, str, final_size);
  result->utf8_length = final_utf8_size;
  result->length = final_size;
  RETURN_OBJ(result);
}

DECLARE_STRING_METHOD(match) {
  ENFORCE_ARG_COUNT(match, 1);
  ENFORCE_ARG_TYPE(match, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 && substr->length == 0) {
    RETURN_TRUE;
  } else if (string->length == 0 || substr->length == 0) {
    RETURN_FALSE;
  }

  GET_REGEX_COMPILE_OPTIONS(substr, false);

  if ((int) compile_options < 0) {
    RETURN_BOOL(strstr(string->chars, substr->chars) - string->chars > -1);
  }

  char *real_regex = remove_regex_delimiter(vm, substr);

  int error_number;
  PCRE2_SIZE error_offset;

  PCRE2_SPTR pattern = (PCRE2_SPTR) real_regex;
  PCRE2_SPTR subject = (PCRE2_SPTR) string->chars;
  PCRE2_SIZE subject_length = (PCRE2_SIZE) string->length;

  pcre2_code *re =
      pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compile_options,
                    &error_number, &error_offset, NULL);
  free(real_regex);

  REGEX_COMPILATION_ERROR(re, error_number, error_offset);

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  int rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

  if (rc < 0) {
    if (rc == PCRE2_ERROR_NOMATCH) {
      RETURN_FALSE;
    } else {
      REGEX_RC_ERROR();
    }
  }

  PCRE2_SIZE *o_vector = pcre2_get_ovector_pointer(match_data);
  uint32_t name_count;

  b_obj_dict *result = (b_obj_dict *) GC(new_dict(vm));
  (void) pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);

  for (int i = 0; i < rc; i++) {
    PCRE2_SIZE substring_length = o_vector[2 * i + 1] - o_vector[2 * i];
    PCRE2_SPTR substring_start = subject + o_vector[2 * i];
    dict_set_entry(vm, result, NUMBER_VAL(i), GC_L_STRING((char *) substring_start, (int) substring_length));
  }

  if (name_count > 0) {
    uint32_t name_entry_size;
    PCRE2_SPTR name_table;
    PCRE2_SPTR tab_ptr;
    (void) pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
    (void) pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

    tab_ptr = name_table;

    for (int i = 0; i < (int) name_count; i++) {
      int n = (tab_ptr[0] << 8) | tab_ptr[1];

      int value_length = (int) (o_vector[2 * n + 1] - o_vector[2 * n]);
      int key_length = (int) name_entry_size - 3;

      char *_key = ALLOCATE(char, key_length + 1);
      char *_val = ALLOCATE(char, value_length + 1);

      sprintf(_key, "%*s", key_length, tab_ptr + 2);
      sprintf(_val, "%*s", value_length, subject + o_vector[2 * n]);

      while (isspace((unsigned char) *_key))
        _key++;

      dict_set_entry(vm, result, OBJ_VAL(GC(take_string(vm, _key, key_length))),
                     OBJ_VAL(GC(take_string(vm, _val, value_length))));

      tab_ptr += name_entry_size;
    }
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  RETURN_OBJ(result);
}

DECLARE_STRING_METHOD(matches) {
  ENFORCE_ARG_COUNT(matches, 1);
  ENFORCE_ARG_TYPE(matches, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 && substr->length == 0) {
    RETURN_OBJ(new_list(vm)); // empty string matches empty string to empty list
  } else if (string->length == 0 || substr->length == 0) {
    RETURN_FALSE; // if either string or str is empty, return false
  }

  GET_REGEX_COMPILE_OPTIONS(substr, true);

  char *real_regex = remove_regex_delimiter(vm, substr);

  int error_number;
  PCRE2_SIZE error_offset;
  uint32_t option_bits;
  uint32_t newline;
  uint32_t name_count, group_count;
  uint32_t name_entry_size;
  PCRE2_SPTR name_table;

  PCRE2_SPTR pattern = (PCRE2_SPTR) real_regex;
  PCRE2_SPTR subject = (PCRE2_SPTR) string->chars;
  PCRE2_SIZE subject_length = (PCRE2_SIZE) string->length;

  pcre2_code *re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compile_options,
                                 &error_number, &error_offset, NULL);
  free(real_regex);

  REGEX_COMPILATION_ERROR(re, error_number, error_offset);

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  int rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

  if (rc < 0) {
    if (rc == PCRE2_ERROR_NOMATCH) {
      RETURN_FALSE;
    } else {
      REGEX_RC_ERROR();
    }
  }

  PCRE2_SIZE *o_vector = pcre2_get_ovector_pointer(match_data);

//   REGEX_VECTOR_SIZE_WARNING();

  // handle edge cases such as /(?=.\K)/
  REGEX_ASSERTION_ERROR(re, match_data, o_vector);

  (void) pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);
  (void) pcre2_pattern_info(re, PCRE2_INFO_CAPTURECOUNT, &group_count);

  b_obj_dict *result = (b_obj_dict *) GC(new_dict(vm));

  for (int i = 0; i < rc; i++) {
    dict_set_entry(vm, result, NUMBER_VAL(0), NIL_VAL);
  }

  // add first set of matches to response
  for (int i = 0; i < rc; i++) {
    b_obj_list *list = (b_obj_list *) GC(new_list(vm));
    PCRE2_SIZE substring_length = o_vector[2 * i + 1] - o_vector[2 * i];
    PCRE2_SPTR substring_start = subject + o_vector[2 * i];
    write_list(vm, list, GC_L_STRING((char *) substring_start, (int) substring_length));
    dict_set_entry(vm, result, NUMBER_VAL(i), OBJ_VAL(list));
  }

  if (name_count > 0) {

    PCRE2_SPTR tab_ptr;
    (void) pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
    (void) pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

    tab_ptr = name_table;

    for (int i = 0; i < (int) name_count; i++) {
      int n = (tab_ptr[0] << 8) | tab_ptr[1];

      int value_length = (int) (o_vector[2 * n + 1] - o_vector[2 * n]);
      int key_length = (int) name_entry_size - 3;

      char *_key = ALLOCATE(char, key_length + 1);
      char *_val = ALLOCATE(char, value_length + 1);

      sprintf(_key, "%*s", key_length, tab_ptr + 2);
      sprintf(_val, "%*s", value_length, subject + o_vector[2 * n]);

      while (isspace((unsigned char) *_key))
        _key++;

      b_obj_list *list = (b_obj_list *) GC(new_list(vm));
      write_list(vm, list, OBJ_VAL(GC(take_string(vm, _val, value_length))));

      dict_add_entry(vm, result, OBJ_VAL(GC(take_string(vm, _key, key_length))), OBJ_VAL(list));

      tab_ptr += name_entry_size;
    }
  }

  (void) pcre2_pattern_info(re, PCRE2_INFO_ALLOPTIONS, &option_bits);
  int utf8 = (option_bits & PCRE2_UTF) != 0;

  (void) pcre2_pattern_info(re, PCRE2_INFO_NEWLINE, &newline);
  int crlf_is_newline = newline == PCRE2_NEWLINE_ANY ||
                        newline == PCRE2_NEWLINE_CRLF ||
                        newline == PCRE2_NEWLINE_ANYCRLF;

  // find the other matches
  for (;;) {
    uint32_t options = 0;
    PCRE2_SIZE start_offset = o_vector[1];

    // if the previous match was for an empty string
    if (o_vector[0] == o_vector[1]) {
      if (o_vector[0] == subject_length)
        break;
      options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
    } else {
      PCRE2_SIZE start_char = pcre2_get_startchar(match_data);
      if (start_offset > subject_length - 1) {
        break;
      }
      if (start_offset <= start_char) {
        if (start_char >= subject_length - 1) {
          break;
        }
        start_offset = start_char + 1;
        if (utf8) {
          for (; start_offset < subject_length; start_offset++)
            if ((subject[start_offset] & 0xc0) != 0x80)
              break;
        }
      }
    }

    rc = pcre2_match(re, subject, subject_length, start_offset, options,
                     match_data, NULL);

    if (rc == PCRE2_ERROR_NOMATCH) {
      if (options == 0)
        break;
      o_vector[1] = start_offset + 1;
      if (crlf_is_newline && start_offset < subject_length - 1 &&
          subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
        o_vector[1] += 1;
      else if (utf8) {
        while (o_vector[1] < subject_length) {
          if ((subject[o_vector[1]] & 0xc0) != 0x80)
            break;
          o_vector[1] += 1;
        }
      }
      continue;
    }

    if (rc < 0 && rc != PCRE2_ERROR_PARTIAL) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      REGEX_ERR("regular expression error %d", rc);
    }

    // REGEX_VECTOR_SIZE_WARNING();
    REGEX_ASSERTION_ERROR(re, match_data, o_vector);

    for (int i = 0; i < rc; i++) {
      PCRE2_SIZE substring_length = o_vector[2 * i + 1] - o_vector[2 * i];
      PCRE2_SPTR substring_start = subject + o_vector[2 * i];

      b_value vlist;
      if (dict_get_entry(result, NUMBER_VAL(i), &vlist)) {
        write_list(vm, AS_LIST(vlist), GC_L_STRING((char *) substring_start, (int) substring_length));
      } else {
        b_obj_list *list = (b_obj_list *) GC(new_list(vm));
        write_list(vm, list, GC_L_STRING((char *) substring_start, (int) substring_length));
        dict_set_entry(vm, result, NUMBER_VAL(i), OBJ_VAL(list));
      }
    }

    if (name_count > 0) {

      PCRE2_SPTR tab_ptr;
      (void) pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
      (void) pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

      tab_ptr = name_table;

      for (int i = 0; i < (int) name_count; i++) {
        int n = (tab_ptr[0] << 8) | tab_ptr[1];

        int value_length = (int) (o_vector[2 * n + 1] - o_vector[2 * n]);
        int key_length = (int) name_entry_size - 3;

        char *_key = ALLOCATE(char, key_length + 1);
        char *_val = ALLOCATE(char, value_length + 1);

        sprintf(_key, "%*s", key_length, tab_ptr + 2);
        sprintf(_val, "%*s", value_length, subject + o_vector[2 * n]);

        while (isspace((unsigned char) *_key))
          _key++;

        b_obj_string *name = (b_obj_string *) GC(take_string(vm, _key, key_length));
        b_obj_string *value = (b_obj_string *) GC(take_string(vm, _val, value_length));

        b_value nlist;
        if (dict_get_entry(result, OBJ_VAL(name), &nlist)) {
          write_list(vm, AS_LIST(nlist), OBJ_VAL(value));
        } else {
          b_obj_list *list = (b_obj_list *) GC(new_list(vm));
          write_list(vm, list, OBJ_VAL(value));
          dict_set_entry(vm, result, OBJ_VAL(name), OBJ_VAL(list));
        }

        tab_ptr += name_entry_size;
      }
    }
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  RETURN_OBJ(result);
}

DECLARE_STRING_METHOD(replace) {
  ENFORCE_ARG_COUNT(replace, 2);
  ENFORCE_ARG_TYPE(replace, 0, IS_STRING);
  ENFORCE_ARG_TYPE(replace, 1, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);
  b_obj_string *rep_substr = AS_STRING(args[1]);

  if (string->length == 0 && substr->length == 0) {
    RETURN_TRUE;
  } else if (string->length == 0 || substr->length == 0) {
    RETURN_FALSE;
  }

  GET_REGEX_COMPILE_OPTIONS(substr, false);
  char *real_regex = remove_regex_delimiter(vm, substr);

  PCRE2_SPTR input = (PCRE2_SPTR) string->chars;
  PCRE2_SPTR pattern = (PCRE2_SPTR) real_regex;
  PCRE2_SPTR replacement = (PCRE2_SPTR) rep_substr->chars;

  int result, error_number;
  PCRE2_SIZE error_offset;

  pcre2_code *re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED,
                                 compile_options & PCRE2_MULTILINE,
                                 &error_number, &error_offset, 0);
  free(real_regex);

  REGEX_COMPILATION_ERROR(re, error_number, error_offset);

  pcre2_match_context *match_context = pcre2_match_context_create(0);

  PCRE2_SIZE output_length = 0;
  result = pcre2_substitute(
      re, input, PCRE2_ZERO_TERMINATED, 0,
      PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
      0, match_context, replacement, PCRE2_ZERO_TERMINATED, 0, &output_length);

  if (result < 0 && result != PCRE2_ERROR_NOMEMORY) {
    REGEX_ERR("regular expression post-compilation failed for replacement",
              result);
  }

  PCRE2_UCHAR *output_buffer = ALLOCATE(PCRE2_UCHAR, output_length);

  result = pcre2_substitute(
      re, input, PCRE2_ZERO_TERMINATED, 0,
      PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_UNSET_EMPTY, 0, match_context,
      replacement, PCRE2_ZERO_TERMINATED, output_buffer, &output_length);

  if (result < 0 && result != PCRE2_ERROR_NOMEMORY) {
    REGEX_ERR("regular expression error at replacement time", result);
  }

  b_obj_string *response =
      take_string(vm, (char *) output_buffer, (int) output_length);

  pcre2_match_context_free(match_context);
  pcre2_code_free(re);

  RETURN_OBJ(response);
}

DECLARE_STRING_METHOD(to_bytes) {
  ENFORCE_ARG_COUNT(to_bytes, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  RETURN_OBJ(copy_bytes(vm, (unsigned char *) string->chars, string->length));
}

DECLARE_STRING_METHOD(__iter__) {
  ENFORCE_ARG_COUNT(__iter__, 1);
  ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int length = string->is_ascii ? string->length : string->utf8_length;
  int index = AS_NUMBER(args[0]);

  if (index > -1 && index < length) {
    int start = index, end = index + 1;
    if(!string->is_ascii) {
      utf8slice(string->chars, &start, &end);
    }

    RETURN_L_STRING(string->chars + start, (int) (end - start));
  }

  RETURN_NIL;
}

DECLARE_STRING_METHOD(__itern__) {
  ENFORCE_ARG_COUNT(__itern__, 1);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int length = string->is_ascii ? string->length : string->utf8_length;

  if (IS_NIL(args[0])) {
    if (length == 0) {
      RETURN_FALSE;
    }
    RETURN_NUMBER(0);
  }

  if (!IS_NUMBER(args[0])) {
    RETURN_ERROR("bytes are numerically indexed");
  }

  int index = AS_NUMBER(args[0]);
  if (index < length - 1) {
    RETURN_NUMBER((double) index + 1);
  }

  RETURN_NIL;
}