#include <stdarg.h>
#include <time.h>
#include "util.h"

ref_counter_t* ref_counter_create () {
    ref_counter_t* rc = MALLOC_STRUCT(ref_counter_t);
    list_t* list = list_create(void*);
    rc->ref_list = list;
    return rc;
}

void ref_counter_add (ref_counter_t* rc, const void* ref) {
    for (size_t i = 0; i < list_size(rc->ref_list); i++)
        if (ref == list_get(rc->ref_list, i, void*))
            return;
    list_push(rc->ref_list, ref);
}

void ref_counter_free_ref (ref_counter_t* rc, void* ref) {
    void* elem;
    for (size_t i = 0; i < list_size(rc->ref_list); i++) {
        elem = list_get(rc->ref_list, i, void*);
        if (ref == elem) {
            free(elem);
            list_remove_index(rc->ref_list, i);
            break;
        }
    }
}

void ref_counter_free_all (ref_counter_t* rc) {
    for (size_t i = list_size(rc->ref_list); i > 0; i--) {
        void* ref = list_get(rc->ref_list, i - 1, void*);
        free(ref);
    }
    list_clear(rc->ref_list);
}

void ref_counter_free (ref_counter_t* rc) {
    ref_counter_free_all(rc);
    free(rc->ref_list);
    free(rc);
}


list_t* list_create_with_size (size_t value_size, size_t initial_length) {
    list_t* list = MALLOC_STRUCT(list_t);
    void* values = malloc(value_size * initial_length);
    list->values = (char*)values;
    list->size = 0;
    list->value_size = value_size;
    list->length = initial_length;
    return list;
}

list_t* list_create_empty_list (size_t value_size) {
    list_t* list = MALLOC_STRUCT(list_t);
    list->values = NULL;
    list->size = 0;
    list->value_size = value_size;
    list->length = 0;
    return list;
}

void list_clear (list_t* list) {
    list->size = 0;
}

void list_free(list_t* list) {
    free(list->values);
    free(list);
}

void list_ensure_size (list_t* list, size_t position) {
    if (list->length < position + 1) {
        size_t old_length = list->length;
        size_t new_length = (old_length == 0 ? 1 : old_length) * 2;
        list_grow(list, new_length);
    }
}

void list_grow (list_t* list, size_t new_length) {
    size_t new_bytes_size = list->value_size * new_length;
    void* memory;
    if (list->values == NULL)
        memory = malloc(new_bytes_size);
    else
        memory = realloc(list->values, new_bytes_size);
    list->values = (char*)memory;
    list->length = new_length;
}

/*bool list_check_index (list_t* list, size_t position) {
    return position >= 0 && position <= list->length;
}*/

void list_set_index_value (list_t* list, size_t position, const void* value) {
    list_ensure_size(list, position);
    memcpy(list->values + position * list->value_size, value, list->value_size);
}

void list_set_index_ptr (list_t* list, size_t position, const void* value) {
    list_ensure_size(list, position);
    memcpy(list->values + position * list->value_size, &value, list->value_size);
}

void list_push_value (list_t* list, const void* value) {
    list_set_index_value(list, list->size++, value);
}

void list_push_ptr (list_t* list, const void* value) {
    list_set_index_ptr(list, list->size++, value);
}

void* list_remove_index (list_t* list, size_t index) {
    if (index == list->size - 1);
    else {
        size_t overwrite_start = index * list->value_size;
        memcpy(list->values + overwrite_start, list->values + overwrite_start + list->value_size, list->value_size * (list->size - index + 1));
    }
    list->size--;
    return NULL;
}

inline size_t list_size (list_t* list) {
    return list->size;
}

void* list_get_index (list_t* list, size_t position) {
    return list->values + position * list->value_size;
}


string_builder_t* string_builder_create (size_t length) {
    string_builder_t* builder = MALLOC_STRUCT(string_builder_t);
    char* value = (char*) malloc(sizeof(char) * length);
    builder->value = value;
    builder->value_null = value;
    builder->length = length;
    builder->size = 0;
    value[0] = '\0';
//	value[length - 1] = '\0';
    return builder;
}

string_builder_t* string_builder_copy (const char* string) {
    string_builder_t* builder = MALLOC_STRUCT(string_builder_t);
    size_t length = strlen(string);
    size_t memory_length = length + 1;
    char* value = (char*) malloc(sizeof(char) * memory_length);
    memcpy(value, string, memory_length);

    builder->value = value;
    builder->value_null = value + length;
    builder->length = memory_length;
    builder->size = length;
    return builder;
}

string_builder_t* string_builder_printf (const char* format, ...) {
    va_list args;
    va_start(args, format);
    size_t would_write = vsnprintf(NULL, 0, format, args);
    va_end(args);
    string_builder_t* builder = string_builder_create(would_write + 1);
    va_start(args, format);
    vsnprintf(builder->value, would_write + 1, format, args);
    va_end(args);
    builder->size = would_write;
    builder->value_null = builder->value + would_write;
//    *builder->value_null = '\0';
    return builder;
}

void string_builder_free(string_builder_t* builder) {
    free(builder->value);
    free(builder);
}

void string_builder_append_string (string_builder_t* builder, const char* string, size_t string_size) {
    if (builder->length - 1 - builder->size < string_size) {
        size_t new_size = builder->size + string_size + 1;
        char* new_value = (char*) realloc(builder->value, new_size);
        builder->value = new_value;
        builder->value_null = new_value + builder->size;
        builder->length = new_size;
    }
    memcpy(builder->value_null, string, string_size);
    builder->value_null += string_size;
    builder->size += string_size;
    builder->value[builder->size] = '\0';
}

void string_builder_append (string_builder_t* builder, const char* string) {
    size_t len = strlen(string);
    if (len == 0)
        return;
    if (string[len - 1] == '\0')
        len -= 1;
    string_builder_append_string(builder, string, len);
}

const char* string_builder_as_cstring (string_builder_t* builder) {
    return builder->value;
}

const char* string_builder_get_cstring (string_builder_t* builder) {
    const char* value = builder->value;
    free(builder);
    return value;
}

size_t string_builder_size (string_builder_t* s) {
    return s->size;
}


int read_file (file_t* file, const char* file_name) {
    FILE* fd = fopen(file_name, "r");
    if (fd == NULL)
        return -1;
    int seek = fseek(fd, 0, SEEK_END);
    if (seek != 0)
        return -2;
    size_t file_size = ftell(fd);
    if (file_size == -1)
        return -3;
    char* mem = malloc(sizeof(char) * (file_size + 2));
    if (mem == NULL)
        return -4;
    mem[0] = '\0';
    seek = fseek(fd, 0, SEEK_SET);
    if (seek != 0)
        return -5;
    size_t new_len = fread(mem, sizeof(char), file_size, fd);
    if (ferror(fd) != 0)
        return -6;
    // should be moved to config.c
    if (mem[new_len] != '\n') {
        mem[new_len] = '\n';
        new_len++;
    }
    mem[new_len] = '\0';
    fclose(fd);
    file->data = mem;
    file->length = new_len;
    return 0;
}


char* get_format_time (const char* format) {
    time_t epoch = time(NULL);
    struct tm ltm;
    localtime_r(&epoch, &ltm);
    char* buf = malloc(sizeof(char) * 256);
    strftime(buf, 256, format, &ltm);
    return buf;
}