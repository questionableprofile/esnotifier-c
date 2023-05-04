#ifndef NOTIFIER_SERVER_H_HEADER
#define NOTIFIER_SERVER_H_HEADER

#include "main.h"

#define HTTP_MAX_CONTENT_LENGTH 8388608 // 8MiB
#define CLIENT_SOCKET_BUF_SIZE 8192 // 8KiB
#define HTTP_WORD_MAX_LENGTH 2048
#define HTTP_N_METHOD   1
#define HTTP_N_URI      2
#define HTTP_N_VERSION  3
#define HTTP_N_HEADERS  4
#define HTTP_N_BODY     5

#define HTTP_METHOD_UNSET (-1)
#define HTTP_METHOD_GET     1
#define HTTP_METHOD_POST    2
#define HTTP_METHOD_OPTIONS 3
#define HTTP_METHOD_HEAD    4

enum http_parse_error {
    ok, body_not_allocated, body_overflow, incomplete_header, header_value_without_name, large_word, large_content_length
};

typedef struct request_data request_t;
typedef struct server_ctx server_ctx_t;

typedef void(*request_callback_fun)(server_ctx_t* ctx, request_t*);

typedef struct header {
    char* name;
    char* value;
    struct header* next;
} header_t;

struct parser {
    size_t word_pos;
    size_t buf_pos;
    bool body;
    bool crlf2;
    char expected_breakpoint;
    int current_param;
    header_t* last_header;
};

typedef struct request_data {
    bool flag;
    struct {
        int client_sd;
    } conn;
    struct {
        header_t* content_length_header;
        size_t content_length;
    } misc;
    struct parser p;
    int method;
    char* method_name;
    char* uri;
    char* http_version;
    char* body;
    size_t body_length;
    header_t* header;
} request_t;

typedef struct server_ctx {
    global_ctx_t* global_ctx;
    int server_sd;
    int state;
    request_callback_fun request_callback;
    pthread_t worker;
} server_ctx_t;

int create_server (global_ctx_t* global_ctx, server_ctx_t *ctx, request_callback_fun req_callback, const char* ip, const char* port);
void stop_server_loop (server_ctx_t* ctx);
pthread_t run_server (server_ctx_t* ctx);
void* server_listener (void* ctx);
void serve_client (server_ctx_t* ctx, int client_sd);
enum http_parse_error parse_request (request_t* req, char* buf, size_t buf_len);
request_t* create_request ();
header_t* create_header ();
void free_request (request_t* req);
int match_method_name (char* method_name);

#endif