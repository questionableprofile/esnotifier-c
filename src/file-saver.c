#include <errno.h>
#include <sys/stat.h>
#include "file-saver.h"
#include "telegram.h"
#include "util.h"
#include "config.h"

int file_saver_init (file_saver_ctx_t* ctx, global_ctx_t* global) {
    ctx->global_ctx = global;

    string_builder_t* log_dir;
    if (config_get_value(global->config, "logs") != NULL) {
        // ...
        // unimplemented
        return -1;
    }
    else {
        string_builder_t* log_dir_path = string_builder_copy(global->config->executable_folder_path->value);
        string_builder_append(log_dir_path, FILE_SEP_S);
        string_builder_append(log_dir_path, "logs");
        string_builder_append(log_dir_path, FILE_SEP_S);
        log_dir = log_dir_path;
    }

    const char* log_dir_s = string_builder_as_cstring(log_dir);
    struct stat fstat;
    int stat_res = stat(log_dir_s, &fstat);
    if (stat_res < 0) {
        int mkdir_res = mkdir(log_dir_s, S_IRWXU | S_IRWXG | S_IRWXO);
        if (mkdir_res < 0) {
            printf("err %d, cannot create directory \"%s\"\n", errno, log_dir_s);
            return -1;
        }
    }

    ctx->log_dir = log_dir;
    return 0;
}

void file_saver_free (file_saver_ctx_t* ctx) {
    string_builder_free(ctx->log_dir);
}

void file_saver_handle_event (global_ctx_t* ctx, eso_event_t* eso_event) {
    char* file_name = get_format_time("%d-%m-%Y");
    string_builder_t* file_path = string_builder_copy(ctx->file_saver_ctx->log_dir->value);
    string_builder_append(file_path, file_name);
    string_builder_append(file_path, ".txt");
    free(file_name);

    FILE* file = fopen(string_builder_as_cstring(file_path), "ab");
    if (file == NULL) {
        printf("err %d, cannot open or create file \"%s\"\n", errno, string_builder_as_cstring(file_path));
        string_builder_free(file_path);
        return;
    }

    char* time = get_format_time("%H:%M");
    string_builder_t* eso_format = tg_format_eso_event(eso_event);
    string_builder_t* formatted = string_builder_printf("%s %s", time, string_builder_as_cstring(eso_format));
    string_builder_append(formatted, "\n");

    size_t wrote_bytes = fwrite(string_builder_as_cstring(formatted), sizeof(char), string_builder_size(formatted), file);
    int fd = fileno(file);
    if (fd < 0) {
        printf("cannot obtain file descriptor, err %d\nwrote bytes: %ld", errno, wrote_bytes);
        string_builder_free(file_path);
        string_builder_free(formatted);
        return;
    }
    fsync(fd);
    fflush(file);
    fclose(file);
    string_builder_free(file_path);
    string_builder_free(formatted);
}

event_handler_t* file_saver_event_handler_create () {
    event_handler_t* event_handler = MALLOC_STRUCT(event_handler_t);
    event_handler->event = &file_saver_handle_event;
    return event_handler;
}

void file_saver_event_handler_free (event_handler_t* handler) {
    free(handler);
}