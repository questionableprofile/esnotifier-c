#include <errno.h>
#include "config.h"

config_pair_t* config_get_pair (config_t* config, const char* value_name) {
    for (int i = 0; i < list_size(config->list); i++) {
        config_pair_t* pair = list_get(config->list, i, config_pair_t*);
        if (STREQUAL(pair->name, value_name))
            return pair;
    }
    return NULL;
}

const char* config_get_value (config_t* config, const char* value_name) {
    config_pair_t* pair = config_get_pair(config, value_name);
    if (pair == NULL)
        return NULL;
    else
        return pair->value;
}

void config_set_value (config_t* config, char* name, char* value) {
    config_pair_t* pair = config_get_pair(config, name);
    if (pair != NULL) {
        free(pair->value);
        pair->value = strdup(value);
    }
    else {
        pair = MALLOC_STRUCT(config_pair_t);
        pair->name = name;
        pair->value = strdup(value);
        list_push(config->list, pair);
    }
}

void config_print_all (config_t* config) {
    for (int i = 0; i < list_size(config->list); i++) {
        config_pair_t* pair = list_get(config->list, i, config_pair_t*);
        printf("%s: \"%s\"\n", pair->name, pair->value);
    }
}

void config_parse (const char* file, size_t file_length, list_t* list) {
    size_t file_pos = 0, word_pos = 0;
    config_pair_t* pair;
    char** current_field;
    char next_breakpoint = '=';

    while (file_pos < file_length) {
        char c = file[file_pos];
        if (c == next_breakpoint) {
            if (c == '=') {
                pair = MALLOC_STRUCT(config_pair_t);
                pair->name = NULL;
                pair->value = NULL;
                list_push(list, pair);
                current_field = &pair->name;
                next_breakpoint = '\n';
            }
            else if (c == '\n') {
                next_breakpoint = '=';
                current_field = &pair->value;
            }
            *current_field = malloc(sizeof(char) * (word_pos + 1));
            memcpy(*current_field, file + file_pos - word_pos, word_pos);
            (*current_field)[word_pos] = '\0';
            word_pos = 0;
        }
        else if (c == '\0')
            break;
        else
            word_pos++;
        file_pos++;
    }
}

void config_rewrite (config_t* config) {
    FILE* file = fopen(string_builder_as_cstring(config->file_path), "w");
    if (file == NULL) {
        printf("err %d, cannot save config file \"%s\"\n", errno, string_builder_as_cstring(config->file_path));
        return;
    }
    string_builder_t* s = string_builder_create(2048);
    for (size_t i = 0; i < list_size(config->list); i++) {
        config_pair_t* pair = list_get(config->list, i, config_pair_t*);
        string_builder_append(s, pair->name);
        string_builder_append(s, "=");
        string_builder_append(s, pair->value);
        string_builder_append(s, "\n");
    }
    fwrite(string_builder_as_cstring(s), sizeof(char), string_builder_size(s), file);
    fflush(file);
    fclose(file);
}

config_t* config_read (const char* file_name) {
    file_t* file = MALLOC_STRUCT(file_t);
    config_t* config = MALLOC_STRUCT(config_t);
    config->list = list_create(config_pair_t*);
    config->file_path = NULL;

    config_read_executable_path(config);
    config_read_executable_folder_path(config);

    if (config->executable_folder_path == NULL)
        return NULL;
    string_builder_t* path = string_builder_copy(config->executable_folder_path->value);
    string_builder_append(path, "/");
    string_builder_append(path, file_name);
    config->file_path = path;

    int read_res = read_file(file, string_builder_as_cstring(path));
    if (read_res < 0) {
        printf("Cannot read config file \"%s\", err %d, errno %d\n", string_builder_as_cstring(path), read_res, errno);
    }
    else {
        config_parse(file->data, file->length, config->list);
    }

    return config;
}

void config_read_executable_path (config_t* config) {
    char link[256] = {0};
    ssize_t bytes_read = readlink("/proc/self/exe", link, 256);
    if (bytes_read < 0) {
        printf("couldn't read /proc/self/exe, err %d\n", errno);
        config->executable_path = NULL;
    }
    link[bytes_read] = '\0';
    config->executable_path = string_builder_copy(link);
}

void config_read_executable_folder_path (config_t* config) {
    string_builder_t *exe = config->executable_path;
    if (exe == NULL) {
        config->executable_folder_path = NULL;
        return;
    }

    char* c;
    size_t diff_char_count = 0;
    for (c = exe->value_null - 1; *c != FILE_SEP && c >= exe->value; c--, diff_char_count++);

    string_builder_t* res = string_builder_create(diff_char_count);
    string_builder_append_string(res, exe->value, c - exe->value);
    config->executable_folder_path = res;
}