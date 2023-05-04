#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <pthread.h>

#include "server.h"
#include "util.h"
#include "config.h"

#define ERRNO98 "Address already in use"

int create_server (global_ctx_t* global_ctx, server_ctx_t *ctx, request_callback_fun request_callback, const char* port) {
    int server_sd = -1;
    struct addrinfo* addrinfo_result;
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int addrinfo_res_code = getaddrinfo(NULL, port, &hints, &addrinfo_result);
    if (addrinfo_res_code != 0) {
        printf("addrinfo error %d, errno %d\n", addrinfo_res_code, errno);
        return -1;
    }

    int bind_err = -1;
    for (struct addrinfo* curaddr = addrinfo_result; curaddr != NULL; curaddr = curaddr->ai_next) {
        server_sd = socket(curaddr->ai_family, curaddr->ai_socktype, curaddr->ai_protocol);
        if (server_sd < 0)
            break;
        int reuseaddr_val = 1;
        setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_val, sizeof(reuseaddr_val));

        struct sockaddr_in* sin = (struct sockaddr_in*) curaddr->ai_addr;
        printf("binding to %s:%d\n", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));

        bind_err = bind(server_sd, curaddr->ai_addr, curaddr->ai_addrlen);
        if (bind_err == 0)
            break;
        else
            close(server_sd);
    }
    freeaddrinfo(addrinfo_result);

    if (bind_err < 0 || server_sd < 0) {
        char* errname = "";
        if (errno == 98)
            errname = ERRNO98;
        printf("error binding: %d %s\n", errno, errname);
        return -2;
    }

    if (listen(server_sd, 1000) != 0) {
        printf("listen() error: %d\n", errno);
        return -3;
    }
    ctx->request_callback = request_callback;
    ctx->server_sd = server_sd;
    ctx->state = 0;
    ctx->global_ctx = global_ctx;

    return 0;
}

void stop_server_loop (server_ctx_t* ctx) {
    ctx->state = 2;
}

pthread_t run_server (server_ctx_t* ctx) {
    pthread_t thread;
    ctx->state = 1;
    int result = pthread_create(&thread, NULL, &server_listener, (void*)ctx);
    if (result != 0) {
        ctx->state = 0;
        return 0;
    }
    ctx->worker = thread;

    return thread;
}

void sig_child_handler (int pid) {
    wait(NULL);
}

void* server_listener (void* _ctx) {
    server_ctx_t* ctx = (server_ctx_t*)_ctx;
//    bool debug = ctx->global_ctx->config->debug;
    bool debug = true;
    signal(SIGCHLD, sig_child_handler);
    while(ctx->state == 1) {
        int client_sd = accept(ctx->server_sd, NULL, NULL);
        int pid;
        if (!debug)
            pid = fork();
        else
            pid = 0;
        if (pid < 0) {
            printf("couldn't create process\n");
            shutdown(client_sd, SHUT_RDWR);
            close(client_sd);
        }
        else if (pid > 0);
        else {
            if (client_sd < 0) {
                printf("accept() error %d, client %d\n", errno, client_sd);
                break;
            }
            serve_client(ctx, client_sd);
            shutdown(client_sd, SHUT_RDWR);
            close(client_sd);
            if (!debug) {
                fflush(stdout);
                shutdown(STDOUT_FILENO, SHUT_WR);
                close(STDOUT_FILENO);
                exit(0);
            }
        }

    }
    close(ctx->server_sd);
    free(ctx);
    return NULL;
}

void serve_client (server_ctx_t* ctx, int client_sd) {
    char buf[CLIENT_SOCKET_BUF_SIZE];
    buf[CLIENT_SOCKET_BUF_SIZE - 1] = 0;
    size_t buf_length = CLIENT_SOCKET_BUF_SIZE;
    ssize_t bytes_read;

    struct pollfd fd = {.fd = client_sd, .events = POLLIN};
    struct pollfd fds[] = {fd};
    request_t* req = NULL;
    int poll_retries = 0;
    bool flag = false;

    do {
        bytes_read = recv(client_sd, buf, buf_length, MSG_DONTWAIT);
//        printf("client %d bytes: %ld, errno: %d\n", client_sd, bytes_read, errno);
        if (bytes_read < 0) {
            if (errno == EAGAIN) {
                poll(fds, sizeof(fds) / sizeof(fds[0]), 1000);
                poll_retries++;
            }
            else {
                printf("read error %d\n", errno);
                break;
            }
        }
        else if (bytes_read == 0)
            break;
        else {
            if (req == NULL)
                req = create_request();
            req->flag = flag;
            req->p.buf_pos = 0;
            int req_status = parse_request(req, buf, bytes_read);
            if (req_status != ok)
                goto cleanup;
            if (req->p.crlf2 && req->method != HTTP_METHOD_POST)
                break;
            if (req->p.body && req->p.word_pos >= req->body_length)
                break;
        }
    } while (bytes_read > 0 || poll_retries < 5);

    if (req == NULL)
        return;

    req->conn.client_sd = client_sd;
    if (ctx->request_callback != NULL)
        ctx->request_callback(ctx, req);
    cleanup:
    if (req != NULL)
        free_request(req);
}

void write_param (struct parser* p, char* buf, char** param) {
//    if (p->word_pos == 0)
    *param = malloc(sizeof(char) * (p->word_pos + 1));
    memcpy(*param, buf + p->buf_pos - p->word_pos, p->word_pos);
    (*param)[p->word_pos] = '\0';
    p->word_pos = 0;
}

enum http_parse_error parse_request (request_t* req, char* buf, size_t buf_len) {
    struct parser* p = &req->p;

    while (p->buf_pos < buf_len) {
        char c = buf[p->buf_pos];

        if (p->body) {
            if (req->body == NULL)
                return body_not_allocated;
            if (p->crlf2) {
                p->crlf2 = false;
                p->word_pos = 0;
                continue;
            }
            if (p->word_pos > req->body_length)
                return body_overflow;
            (req->body)[p->word_pos++] = c;
            p->buf_pos++;
            if (c == '\0')
                break;
            else
                continue;
        }

        if (c == p->expected_breakpoint) {
            if (p->current_param == HTTP_N_METHOD) {
                p->current_param = HTTP_N_URI;
                write_param(p, buf, &req->method_name);
            }
            else if (p->current_param == HTTP_N_URI) {
                req->method = match_method_name(req->method_name);
                p->current_param = HTTP_N_VERSION;
                p->expected_breakpoint = '\r';
                write_param(p, buf, &req->uri);
            }
            else if (p->current_param == HTTP_N_VERSION) {
                p->expected_breakpoint = '\n';
                p->current_param = HTTP_N_HEADERS;
                write_param(p, buf, &req->http_version);
            }
            else if (p->current_param == HTTP_N_HEADERS) {
                if (c == '\n') {
                    p->expected_breakpoint = ':';
                    goto final;
                }
                else if (c == ':') {
                    header_t* next = create_header();
                    if (p->last_header != NULL) {
                        // doesn't check last header
                        if (p->last_header->name == NULL || p->last_header->value == NULL)
                            return incomplete_header;
                        p->last_header->next = next;
                    }
                    else
                        req->header = next;
                    p->last_header = next;
                    write_param(p, buf, &p->last_header->name);
                    p->expected_breakpoint = ' ';
                }
                else if (c == ' ') {
                    p->expected_breakpoint = '\r';
                    goto final;
                }
                else {
                    if (p->last_header == NULL)
                        return header_value_without_name;
                    write_param(p, buf, &p->last_header->value);
                    p->expected_breakpoint = '\n';
                    static char cnt_length_h_name[] = "Content-Length";
                    if (
                            req->misc.content_length_header == NULL &&
                            strncasecmp(p->last_header->name, cnt_length_h_name, sizeof(cnt_length_h_name)) == 0
                    ) {
                        size_t content_length = strtoll(p->last_header->value, NULL, 10);
                        if (content_length > HTTP_MAX_CONTENT_LENGTH)
                            return large_content_length;
                        req->misc.content_length_header = p->last_header;
                        req->body_length = content_length;
                        req->misc.content_length = content_length;
                        if (content_length > 0) {
                            req->body = malloc(sizeof(char) * (content_length + 1));
                            req->body[content_length] = '\0';
                        }
                    }
                }

                if (p->buf_pos < buf_len - 3 && c == '\r' && buf[p->buf_pos + 1] == '\n' && buf[p->buf_pos + 2] == '\r' && buf[p->buf_pos + 3] == '\n') {
                    p->body = true;
                    p->crlf2 = true;
                }
            }
            else {
                printf("%s\n", "unknown param");
                goto final;
            }

            if (p->body)
                p->buf_pos += 3;
        }
        else if (c == '\0') {
            break;
        }
        else
            p->word_pos++;

        final:
        if (p->word_pos >= HTTP_WORD_MAX_LENGTH)
            return large_word;
        p->buf_pos++;
    }
    return ok;
}

request_t* create_request () {
    request_t* req = MALLOC_STRUCT(request_t);
    req->conn.client_sd = -1;

    req->p.buf_pos = 0;
    req->p.word_pos = 0;
    req->p.body = false;
    req->p.crlf2 = false;
    req->p.expected_breakpoint = ' ';
    req->p.current_param = HTTP_N_METHOD;
    req->p.last_header = NULL;

    req->misc.content_length_header = NULL;
    req->misc.content_length = 0;

    req->method = HTTP_METHOD_UNSET;
    req->method_name = NULL;
    req->uri = NULL;
    req->http_version = NULL;
    req->body = NULL;
    req->body_length = 0;
    return req;
}

header_t* create_header () {
    header_t* header = MALLOC_STRUCT(header_t);
    header->name = NULL;
    header->value = NULL;
    header->next = NULL;
    return header;
}

void free_request (request_t* req) {
    FREE_IF_NOTNULL(req, method_name);
    FREE_IF_NOTNULL(req, uri);
    FREE_IF_NOTNULL(req, http_version);
    FREE_IF_NOTNULL(req, body);
    header_t* next = req->header;
    while (next != NULL) {
        header_t* current = next;
        next = current->next;
        free(current);
    }
    free(req);
}

int match_method_name (char* method_name) {
    if (STREQUAL(method_name, "GET"))
        return HTTP_METHOD_GET;
    else if (STREQUAL(method_name, "POST"))
        return HTTP_METHOD_POST;
    else if (STREQUAL(method_name, "OPTIONS"))
        return HTTP_METHOD_OPTIONS;
    else if (STREQUAL(method_name, "HEAD"))
        return HTTP_METHOD_HEAD;
    else
        return HTTP_METHOD_UNSET;
}

/*void read_body (request_t* req, int timeout) {
    char buf[CLIENT_SOCKET_BUF_SIZE];
    size_t buf_size = CLIENT_SOCKET_BUF_SIZE;
    size_t bytes_read = -1;
    int poll_retries = 0;
    struct pollfd file = {.file = req->conn.client_sd, .events = POLLIN};
    struct pollfd fds[] = {file};

    do {
        bytes_read = recv(req->conn.client_sd, buf, buf_size, MSG_DONTWAIT);
    } while (bytes_read > 0);
}*/