#ifndef NOTIFIER_CONFIG_H_HEADER
#define NOTIFIER_CONFIG_H_HEADER

#include "main.h"
#include "util.h"

typedef struct config_pair {
    char* name;
    char* value;
} config_pair_t;

typedef struct config {
    list_t* list;
    string_builder_t* file_path;
    string_builder_t* executable_path;
    string_builder_t* executable_folder_path;
    bool debug;
} config_t;

config_t* config_read (const char* file_name);
void config_rewrite (config_t* config);
const char* config_get_value (config_t* config, const char* value_name);
void config_set_value (config_t* config, char* name, char* value);
void config_print_all (config_t* config);

void config_read_executable_path (config_t* config);
void config_read_executable_folder_path (config_t* config);

#endif