#ifndef NOTIFIER_FILE_SAVER_H_HEADER
#define NOTIFIER_FILE_SAVER_H_HEADER

#include "main.h"

typedef struct file_saver_ctx {
    global_ctx_t* global_ctx;
    string_builder_t* log_dir;
} file_saver_ctx_t;

int file_saver_init (file_saver_ctx_t* ctx, global_ctx_t* global);
void file_saver_free (file_saver_ctx_t* ctx);
void file_saver_handle_event (global_ctx_t* ctx, eso_event_t* eso_event);
event_handler_t* file_saver_event_handler_create ();
void file_saver_event_handler_free (event_handler_t* handler);

#endif