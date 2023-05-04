#include <json.h>
#include <telebot.h>
#include <sys/socket.h>
#include <pthread.h>
#include <locale.h>

#include "main.h"
#include "server.h"
#include "eso.h"
#include "telegram.h"
#include "config.h"
#include "file-saver.h"

const char commands_uri[] = "/commands";
const char event_uri[] = "/event";

void request_handler (server_ctx_t* ctx, request_t* request) {
    string_builder_t* string = string_builder_create(4096);
    string_builder_t* response = NULL;
    /*if (request->body != NULL)
        printf("%s\n", request->body);*/
    if (STREQUAL(request->method_name, "POST") && request->body == NULL) {
        // should be bad request
        http_respond_text(string, string_builder_copy("no body"), HTTP_CONTENT_PLAIN);
    }
    else if (strcmp(request->method_name, "OPTIONS") == 0)
        http_respond_options(string, "GET, POST, OPTIONS");
    else if (request->method == HTTP_METHOD_GET && strncmp(request->uri, commands_uri, sizeof(commands_uri)) == 0) {
        list_t* commands = command_queue_retrieve(ctx->global_ctx->command_queue);
        response = eso_command_list_to_json(commands);
        eso_command_list_free(commands);
        http_respond_text(string, response, HTTP_CONTENT_PLAIN);
    }
    else if (request->method == HTTP_METHOD_POST && strncmp(request->uri, event_uri, sizeof(event_uri)) == 0) {
        if (request->body == NULL) {
            response = string_builder_copy("empty body");
            http_respond_text(string, response, HTTP_CONTENT_PLAIN);
            goto exit;
        }
        eso_event_t* event = parse_eso_event(request->body);
        if (event == NULL) {
            printf("body: \"%s\"\n", request->body);
            response = string_builder_copy("failed to to parse event");
        }
        else {
            event_handlers_eso_event(ctx->global_ctx, event);
            eso_event_free(event);
            response = string_builder_copy("done 👍");
        }
        http_respond_text(string, response, HTTP_CONTENT_PLAIN);
    }
    else
        http_not_found(string);
    exit:
    send(request->conn.client_sd, string->value, string->value_null - string->value, 0);
    if (response != NULL)
        string_builder_free(response);
    string_builder_free(string);
}

int main (int argc, char* argv[]) {
    setlocale (LC_ALL, "");

    config_t* config = config_read("config.txt");
    if (config == NULL)
        return 1;

    if (config_get_value(config, "port") == NULL)
        config_set_value(config, "port", "9673");
    config->debug = config_get_value(config, "debug") != NULL ? true : false;

    global_ctx_t* global_ctx = MALLOC_STRUCT(global_ctx_t);
    global_ctx->config = config;
    command_queue_t* command_queue = command_queue_create();
    global_ctx->command_queue = command_queue;
    global_ctx->event_handlers = list_create(event_handler_t*);
    global_ctx->tg_ctx = NULL;
    global_ctx->server_ctx = NULL;

    file_saver_ctx_t* fs_ctx = MALLOC_STRUCT(file_saver_ctx_t);
    global_ctx->file_saver_ctx = fs_ctx;
    if (file_saver_init(fs_ctx, global_ctx) != 0)
        return 1;
    list_push(global_ctx->event_handlers, file_saver_event_handler_create());

    tg_context_t* tg_ctx = NULL;
    pthread_t* telegram_thread = NULL;
    if (config_get_value(config, "token") != NULL) {
        tg_ctx = MALLOC_STRUCT(tg_context_t);
        telebot_error_e init_result = tg_init_context(global_ctx, tg_ctx, config_get_value(config, "token"));
        if (init_result != TELEBOT_ERROR_NONE) {
            printf("telegram bot error %d\n", init_result);
            return 1;
        }
        global_ctx->tg_ctx = tg_ctx;
        list_push(global_ctx->event_handlers, tg_event_handler_create());
        telegram_thread = tg_run_worker(tg_ctx);
        if (telegram_thread == NULL)
            return 1;
    }

    server_ctx_t* server_ctx = MALLOC_STRUCT(server_ctx_t);
    int cs_error = create_server(
            global_ctx,
            server_ctx,
            &request_handler,
            config_get_value(config, "port"));
    if (cs_error < 0)
        return 1;
    global_ctx->server_ctx = server_ctx;

    pthread_t server_thread = run_server(server_ctx);
    if (server_thread == 0)
        return 1;

    char input;
    while (true) {
        scanf("%c", &input);
        if (input == 'e') {
            printf("shutting down\n");
            if (tg_ctx != NULL)
                tg_pause_worker(tg_ctx);
            stop_server_loop(server_ctx);
            break;
        }
    }

    if (tg_ctx != NULL) {
        pthread_join(*telegram_thread, NULL);
        tg_free_context(tg_ctx);
    }
    if (global_ctx->file_saver_ctx != NULL) {
        file_saver_free(global_ctx->file_saver_ctx);
    }
    pthread_join(server_thread, NULL);
    command_queue_free(command_queue);

    return 0;
}

void event_handlers_eso_event (global_ctx_t* ctx, eso_event_t* eso_event) {
    for (size_t i = 0; i < list_size(ctx->event_handlers); i++) {
        event_handler_t* handler = list_get(ctx->event_handlers, i, event_handler_t*);
        handler->event(ctx, eso_event);
    }
}

command_queue_t* command_queue_create () {
    command_queue_t* queue = MALLOC_STRUCT(command_queue_t);
    queue->queue = list_create(eso_command_t*);
    queue->mutex = MALLOC_STRUCT(mutex_t);
    mutex_init(queue->mutex);
    return queue;
}

void command_queue_free (command_queue_t* commands) {
    mutex_free(commands->mutex);
    eso_command_list_free(commands->queue);
    list_free(commands->queue);
    free(commands);
}

void command_queue_add (command_queue_t* commands, eso_command_t* cmd) {
    list_push(commands->queue, cmd);
}

list_t* command_queue_retrieve (command_queue_t* commands) {
    if (list_size(commands->queue) == 0)
        return list_create_empty(eso_command_t*);

    mutex_lock(commands->mutex);
    list_t* new_queue = list_create(eso_command_t*);
    list_t* old_queue = commands->queue;
    commands->queue = new_queue;
    mutex_unlock(commands->mutex);
    return old_queue;
}