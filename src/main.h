#ifndef MAIN_H_HEADER
#define MAIN_H_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

/* Pre-defined from other files */
typedef struct config config_t;

typedef struct eso_command eso_command_t;
typedef struct eso_event eso_event_t;

typedef struct server_ctx server_ctx_t;
typedef struct tg_context tg_context_t;
typedef struct file_saver_ctx file_saver_ctx_t;
typedef struct global_ctx global_ctx_t;
typedef struct command_queue command_queue_t;

#include "util.h"
#include "http.h"

#ifdef WIN32
typedef CRITICAL_SECTION mutex_t;
#define mutex_init(mutex_ptr) 0;
#define FILE_SEP '\\'
#define FILE_SEP_S "\\"
#else
typedef pthread_mutex_t mutex_t;
#define mutex_init(mutex_ptr) pthread_mutex_init((pthread_mutex_t*)(mutex_ptr), NULL)
#define mutex_lock(mutex_ptr) pthread_mutex_lock((pthread_mutex_t*)(mutex_ptr))
#define mutex_unlock(mutex_ptr) pthread_mutex_unlock((pthread_mutex_t*)(mutex_ptr))
#define mutex_free(mutex_ptr) pthread_mutex_destroy((pthread_mutex_t*)(mutex_ptr))
#define FILE_SEP '/'
#define FILE_SEP_S "/"
#endif

#define MALLOC_STRUCT(s) (s*)malloc(sizeof(s))
#define FREE_IF_NOTNULL(obj, field) if (obj->field != NULL) \
                                        free(obj->field)
#define BUF_SIZE(buf) (sizeof(buf) / sizeof(buf[0]))
#define REQ_NON_NULL(val, error_val) if (val == NULL) \
                                        return error_val
#define STREQUAL(s1, s2) (strcmp(s1, s2) == 0)

#define REQUIRE_CONFIG_PARAM(config, name) if (config_get_value(config, name) == NULL) { \
                                                printf("missing config parameter \"%s\"\n", name); \
                                                return -1; }

typedef void(*event_handler_fun)(global_ctx_t*, eso_event_t*);

typedef struct event_handler {
    event_handler_fun event;
} event_handler_t;

typedef struct global_ctx {
    server_ctx_t* server_ctx;
    tg_context_t* tg_ctx;
    file_saver_ctx_t* file_saver_ctx;
    command_queue_t* command_queue;
    list_t* event_handlers;
    config_t* config;
} global_ctx_t;

typedef struct command_queue {
    list_t* queue;
    mutex_t* mutex;
} command_queue_t;

command_queue_t* command_queue_create ();
void command_queue_free (command_queue_t* commands);
void command_queue_add (command_queue_t* commands, eso_command_t* cmd);
list_t* command_queue_retrieve (command_queue_t* commands);

void event_handlers_eso_event (global_ctx_t* ctx, eso_event_t* eso_event);

#endif