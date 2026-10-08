#ifndef RADIO_HTTP_H
#define RADIO_HTTP_H

#include <stdio.h>

typedef struct {
    const char* key;
    const char* value;
} http_header_field;

typedef struct {
    const char* key;
    size_t key_size;
    const char* value;
    size_t value_size;
} http_parsed_field;

typedef struct {
    const char* method;
    const char* path;
    const http_header_field* fields;
    int field_num;
    const char* content;
    size_t content_size;
} http_message_data;

typedef struct {
    const char* version;
    size_t version_size;
    const char* code;
    size_t code_size;
    const char* status_msg;
    size_t status_msg_size;
    const http_parsed_field* fields;
    size_t fields_num;
    const char* content;
    size_t content_size;
} http_parsed_data;

typedef struct {
    const char* raw;
    const char* protocol;
    size_t protocol_size;
    const char* host;
    size_t host_size;
    const char* port;
    size_t port_size;
    const char* path;
    size_t path_size;
    const char* query;
    size_t query_size;
} url_parsed;

// Builds http message from given struct
ssize_t build_http_message(char* result, size_t result_size, const http_message_data* data);

// Parses http message into given struct
ssize_t parse_http_message(const char* buffer, size_t buffer_size, http_parsed_data* parsed_data,
                       http_parsed_field* fields_buf, size_t max_fields);

// Parses url into given struct
ssize_t parse_url(char* buffer, size_t buffer_size, url_parsed* parsed_data,
                  const char* raw_buffer);

// Finds key among given http fields
ssize_t find_key_id(const http_parsed_field* parsed_fields, size_t fields_num,
                    const char* key, size_t key_size);

#endif //RADIO_HTTP_H
