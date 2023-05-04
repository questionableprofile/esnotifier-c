#ifndef NOTIFIER_HTTP_H_HEADER
#define NOTIFIER_HTTP_H_HEADER

#include "util.h"

#define HTTP_CRLF "\r\n"
#define HTTP_VERSION "HTTP/1.1"
#define HTTP_CORS_ORIGIN "*"

#define HTTP_OK "200 OK"
#define HTTP_NOT_FOUND "404 NOT FOUND"

#define HTTP_CONTENT_PLAIN "text/plain"
#define HTTP_CONTENT_HTML "text/html"
#define HTTP_CONTENT_JSON "application/json"

void add_http_version (string_builder_t* builder);
void add_http_ok (string_builder_t* builder);
void add_http_header (string_builder_t* builder, char* name, char* value);

void http_server_header (string_builder_t* builder);
void http_cors_header (string_builder_t* builder);

void http_respond_options (string_builder_t* builder, char* method_list_string);
void http_respond_text (string_builder_t* builder, string_builder_t* response, char* content_type);
void http_not_found (string_builder_t* builder);

#endif