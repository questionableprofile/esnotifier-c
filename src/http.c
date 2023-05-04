#include "http.h"
//#include "main.h"

void add_http_version (string_builder_t* builder) {
	string_builder_append(builder, "HTTP/1.0 ");
}

void add_http_ok (string_builder_t* builder) {
	string_builder_append(builder, "200 OK\r\n");
}

void add_http_header (string_builder_t* builder, char* name, char* value) {
	string_builder_append(builder, name);
	string_builder_append(builder, ": ");
	string_builder_append(builder, value);
	string_builder_append(builder, "\r\n");
}

void http_not_found (string_builder_t* builder) {
	// length 146
	char result[] = "<html><head><title>404 Not Found</title></head>\n<body>\n<center><h1>404 Not Found</h1></center>\n<hr><center>cringinx/3.22</center>\n</body>\n</html>";
	add_http_version(builder);
	string_builder_append(builder, HTTP_NOT_FOUND);
	string_builder_append(builder, HTTP_CRLF);
	http_server_header(builder);
	http_cors_header(builder);
	add_http_header(builder, "content-length", "146");
	add_http_header(builder, "content-type", HTTP_CONTENT_HTML);
	string_builder_append(builder, HTTP_CRLF);
	string_builder_append_string(builder, result, 146);
}

void http_respond_text (string_builder_t* builder, string_builder_t* response, char* content_type) {
	size_t response_length = response->value_null - response->value;
	char response_length_string[256];
	sprintf(response_length_string, "%ld", response_length);

	add_http_version(builder);
	string_builder_append(builder, HTTP_OK);
	string_builder_append(builder, HTTP_CRLF);
	http_server_header(builder);
	http_cors_header(builder);
	add_http_header(builder, "content-length", response_length_string);
	add_http_header(builder, "content-type", content_type);
	string_builder_append(builder, HTTP_CRLF);
	string_builder_append_string(builder, response->value, response_length);
}

void http_respond_options (string_builder_t* builder, char* method_list_string) {
	add_http_version(builder);
	string_builder_append(builder, HTTP_OK);
	string_builder_append(builder, HTTP_CRLF);
	add_http_header(builder, "allow", method_list_string);
	http_server_header(builder);
	http_cors_header(builder);
	string_builder_append(builder, HTTP_CRLF);
}

void http_server_header (string_builder_t* builder) {
	add_http_header(builder, "server", "cringinx");
}

void http_cors_header (string_builder_t* builder) {
	add_http_header(builder, "access-control-allow-origin", HTTP_CORS_ORIGIN);
	add_http_header(builder, "access-control-allow-headers", "*");
}