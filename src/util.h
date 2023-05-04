#ifndef NOTIFIER_UTIL_H_HEADER
#define NOTIFIER_UTIL_H_HEADER

typedef void(*destructor_t)(void*);
typedef struct list list_t;
typedef struct string_builder string_builder_t;

#include "main.h"

#define LIST_DEFAULT_SIZE 10

typedef struct ref_counter {
    list_t* ref_list;
} ref_counter_t;

typedef struct list {
    char* values;
    size_t size;
    size_t value_size;
    size_t length;
} list_t;

typedef struct string_builder {
    char* value;
    char* value_null;
    size_t length;
    size_t size;
} string_builder_t;

typedef struct file {
    char* data;
    size_t length;
} file_t;

ref_counter_t* ref_counter_create ();
void ref_counter_add (ref_counter_t* rc, const void* ref);
void ref_counter_free_ref (ref_counter_t* rc, void* ref);
void ref_counter_free_all (ref_counter_t* rc);
string_builder_t* string_builder_printf (const char* format, ...);
void ref_counter_free (ref_counter_t* rc);

void string_builder_free(string_builder_t* builder);
string_builder_t* string_builder_create (size_t length);
string_builder_t* string_builder_copy (const char* string);
void string_builder_append_string (string_builder_t* builder, const char* string, size_t string_size);
void string_builder_append (string_builder_t* builder, const char* string);
const char* string_builder_as_cstring (string_builder_t* builder);
//const char* string_builder_get_cstring (string_builder_t* builder);
size_t string_builder_size (string_builder_t* s);

list_t* list_create_with_size (size_t value_size, size_t initial_length);
list_t* list_create_empty_list (size_t value_size);
#define list_create_sized(type, size) list_create_with_size(sizeof(type), size)
#define list_create(type) list_create_with_size(sizeof(type), LIST_DEFAULT_SIZE)
#define list_create_empty(type) list_create_empty_list(sizeof(type))
void list_clear (list_t* list);
void list_free(list_t* list);
void list_ensure_size (list_t* list, size_t position);
void list_grow (list_t* list, size_t new_length);
void list_set_index_value (list_t* list, size_t position, const void* value);
#define list_set_val(list, position, value) list_set_index_value(list, position, value)
void list_set_index_ptr (list_t* list, size_t position, const void* value);
#define list_set(list, position, value) list_set_index_ptr(list, position, value)
void list_push_value (list_t* list, const void* value);
void list_push_ptr (list_t* list, const void* value);
#define list_push(list, value) list_push_ptr(list, value)
void* list_remove_index (list_t* list, size_t index);
size_t list_size (list_t* list);
void* list_get_index (list_t* list, size_t position);
#define list_get(list, position, type) *((type*)list_get_index(list, position))

int read_file (file_t* file, const char* file_name);

char* get_format_time (const char* format);

#endif