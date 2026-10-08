#include "http.h"
#include <string.h>
#include <stdint.h>

#include <stdbool.h>

#define HTTP_VERSION "HTTP/1.1"
#define OK 0
#define ERR_ARGS (-1)
#define ERR_BUFFER (-2)
#define ERR_SYS (-3)
#define ERR_PARSE (-4)
#define ERR_FIELD_OVERFLOW (-5)
#define CONT_LEN_KEY "content-length"
#define CONT_LEN_KEY_SIZE 14
#define LOWERCASE 32

ssize_t build_http_message(char* result, size_t result_size, const http_message_data* data) {
    if (!data || !result || result_size == 0) return ERR_ARGS;
    if (!data->method || !data->path) return ERR_ARGS;
    size_t len = 0;

    #define WRITE_TO_BUF(...) do { \
        if (len >= result_size) return ERR_BUFFER; \
        int written = snprintf(&result[len], result_size - len, __VA_ARGS__); \
        if (written < 0) return ERR_SYS;           \
        if ((size_t)written >= result_size - len) return ERR_BUFFER;          \
        len += written;            \
    } while(0)

    WRITE_TO_BUF("%s %s %s\r\n", data->method, data->path, HTTP_VERSION);
    if (data->fields != NULL) {
        for (int i = 0; i < data->field_num; i++) {
            if (!data->fields[i].value || !data->fields[i].key) {
                return ERR_ARGS;
            }
            WRITE_TO_BUF("%s: %s\r\n", data->fields[i].key, data->fields[i].value);
        }
    }
    WRITE_TO_BUF("\r\n");

    #undef WRITE_TO_BUF

    if (data->content != NULL && data->content_size > 0) {
        if (data->content_size > result_size - len) return ERR_BUFFER;
        memcpy(&result[len], data->content, data->content_size);
        len += data->content_size;
    }
    return (ssize_t)len;
}

static int is_content_length_key(const char* key, size_t size) {
    if (size != CONT_LEN_KEY_SIZE) return 0;
    const char* expected = CONT_LEN_KEY;
    for (size_t i = 0; i < CONT_LEN_KEY_SIZE; i++) {
        char c = key[i];
        if (c >= 'A' && c <= 'Z') c += LOWERCASE;
        if (c != expected[i]) return 0;
    }
    return 1;
}

static size_t parse_size_t(const char* value, size_t size) {
    size_t result = 0;
    int only_spaces = 0;
    for (size_t i = 0; i < size; i++) {
        if (value[i] >= '0' && value[i] <= '9' && !only_spaces) {
            result = result * 10 + (value[i] - '0');
            if (result > SIZE_MAX / 10) return 0;
        } else if (value[i] == ' ' || value[i] == '\t') {
            only_spaces = 1;
            continue;
        } else {
            break;
        }
    }
    return result;
}

// from: https://git.musl-libc.org/cgit/musl/tree/src/string/memmem.c
static char *twobyte_memmem(const unsigned char *h, size_t k, const unsigned char *n)
{
    if (k < 2) return NULL; // this line was not present in the original code
    uint16_t nw = n[0]<<8 | n[1], hw = h[0]<<8 | h[1];
    for (h+=2, k-=2; k; k--, hw = hw<<8 | *h++)
        if (hw == nw) return (char *)h-2;
    return hw == nw ? (char *)h-2 : 0;
}

// from: https://git.musl-libc.org/cgit/musl/tree/src/string/memmem.c
static char *threebyte_memmem(const unsigned char *h, size_t k, const unsigned char *n)
{
    if (k < 3) return NULL; // this line was not present in the original code
    uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8;
    uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8;
    for (h+=3, k-=3; k; k--, hw = (hw|*h++)<<8)
        if (hw == nw) return (char *)h-3;
    return hw == nw ? (char *)h-3 : 0;
}

// needs size_t pos, size_t buffer_size and char* buffer
#define SKIP_SPACES() do { \
    while (pos < buffer_size && (buffer[pos] == ' ' || buffer[pos] == '\t')) \
    pos++;                 \
} while (0)

// needs char* start, char* end
#define memmem(separator, separator_size, len) do { \
    char* c = separator;                            \
    size_t __len = len;                             \
    if (separator_size == 1)                        \
        end = memchr(start, c[0], __len);           \
    if (separator_size == 2)                        \
        end = twobyte_memmem((unsigned char*)start, __len, (unsigned char*)c); \
    if (separator_size == 3)                        \
        end = threebyte_memmem((unsigned char*) start, __len, (unsigned char*) c); \
} while(0)

// This is the magic that handles whole parsing.
// Well, it finds the next separator, saves in proper place what needs to be saved,
// and if not found executes the error_handling; it also skips spaces.
// Accepts only separator_size 1, 2 or 3, else will just execute error_handling.
// needs char* buffer, char* structure->[field_name],
// size_t structure->[field_name]_size, size_t pos, size_t buffer_size
#define PARSE_SIGN_SEPARATED(structure, field_name, separator, separator_size, error_handling) do { \
    const char* start = buffer + pos;                                                               \
    char* end = NULL;                                                                               \
    memmem(separator, separator_size, buffer_size - pos);                                           \
    if (!end) error_handling;                                                                       \
    else {                                                                                          \
        structure->field_name = start;                                                              \
        structure->field_name##_size = end - start;                                                 \
        pos = (end - buffer) + separator_size;                                                      \
        SKIP_SPACES();                                                                              \
    }                                                                                               \
} while(0)


// if correct, returns remaining bytes in content to be read
ssize_t parse_http_message(const char* buffer, size_t buffer_size, http_parsed_data* parsed_data,
                       http_parsed_field* fields_buf, size_t max_fields) {
    if (!buffer || !buffer_size || !parsed_data) return ERR_ARGS;
    memset(parsed_data, 0, sizeof(http_parsed_data));
    size_t pos = 0;
    int has_content_length = 0;

    // parse version
    PARSE_SIGN_SEPARATED(parsed_data, version, " ", 1, return ERR_PARSE);

    // parse code
    PARSE_SIGN_SEPARATED(parsed_data, code, " ", 1, return ERR_PARSE);

    // parse status_msg
    PARSE_SIGN_SEPARATED(parsed_data, status_msg, "\r\n", 2, return ERR_PARSE);

    parsed_data->fields = fields_buf;
    parsed_data->fields_num = 0;

    while (pos < buffer_size) {
        http_parsed_field* curr_field = &fields_buf[parsed_data->fields_num];
        if (buffer_size - pos >= 2 && buffer[pos] == '\r' && buffer[pos + 1] == '\n') {
            pos += 2;
            break;
        }
        if (parsed_data->fields_num >= max_fields) {
            return ERR_FIELD_OVERFLOW;
        }
        // parse key
        PARSE_SIGN_SEPARATED(curr_field, key, ":", 1, return ERR_PARSE);

        // parse value
        PARSE_SIGN_SEPARATED(curr_field, value, "\r\n", 2, return ERR_PARSE);

        if (!has_content_length && is_content_length_key(curr_field->key, curr_field->key_size)) {
            parsed_data->content_size = parse_size_t(curr_field->value, curr_field->value_size);
            has_content_length = 1;
        }

        parsed_data->fields_num++;
    }

    if (buffer_size - pos == 0) {
        parsed_data->content = NULL;
        if (!has_content_length) {
            parsed_data->content_size = 0;
        }
    } else {
        parsed_data->content = buffer + pos;
        if (!has_content_length) parsed_data->content_size = buffer_size - pos;
    }
    if (parsed_data->content_size > buffer_size - pos) {
        return (ssize_t)(parsed_data->content_size - (buffer_size - pos));
    }
    return 0;
}

static ssize_t parse_host_port(char* buffer, size_t buffer_size, url_parsed* parsed_data) {
    parsed_data->port = NULL;
    parsed_data->port_size = 0;
    bool has_port = true;
    size_t pos = 0;
    if (buffer[0] == '[') {
        pos++;
        PARSE_SIGN_SEPARATED(parsed_data, host, "]", 1, return ERR_PARSE);
        buffer[pos - 1] = '\0';
        if (buffer[pos] == ':') pos++;
    } else {
        PARSE_SIGN_SEPARATED(parsed_data, host, ":", 1, has_port = false);
        if (!has_port) {
            parsed_data->host = buffer;
            parsed_data->host_size = buffer_size;
            return OK;
        }
        buffer[pos - 1] = '\0';
    }
    if (pos < buffer_size) {
        parsed_data->port = buffer + pos;
        parsed_data->port_size = buffer_size - pos;
    }
    return OK;
}

// functions need to get actual strings, so I will edit the buffer and add \0 in some places
// I add optional raw_buffer which will be remembered in struct
ssize_t parse_url(char* buffer, size_t buffer_size, url_parsed* parsed_data, const char* raw_buffer) {
    if (!buffer || !buffer_size || !parsed_data) return ERR_ARGS;
    memset(parsed_data, 0, sizeof(url_parsed));
    size_t pos = 0;
    parsed_data->raw = raw_buffer;

    bool has_protocol = true;
    PARSE_SIGN_SEPARATED(parsed_data, protocol, "://", 3, has_protocol = false);
    if (!has_protocol) {
        parsed_data->protocol = NULL;
        parsed_data->protocol_size = 0;
    } else {
        buffer[pos - 3] = '\0';
    }

    // temporarily save host:port to host
    char* host_port_start = buffer + pos;
    bool has_path = true;
    PARSE_SIGN_SEPARATED(parsed_data, host, "/", 1, has_path = false);
    if (!has_path) {
        parsed_data->path = NULL;
        parsed_data->path_size = 0;
        parsed_data->host = buffer + pos;
        parsed_data->host_size = buffer_size - pos;
    } else {
        buffer[pos - 1] = '\0';
    }

    //parse path (and detach optional query string)
    parsed_data->query = NULL;
    parsed_data->query_size = 0;
    bool has_args = true;
    PARSE_SIGN_SEPARATED(parsed_data, path, "?", 1, has_args = false);
    if (!has_args && has_path) {
        parsed_data->path = buffer + pos;
        parsed_data->path_size = buffer_size - pos;
    } else if (has_args) {
        buffer[pos - 1] = '\0';
        if (!has_path) {
            parsed_data->host = parsed_data->path;
            parsed_data->host_size = parsed_data->path_size;
            parsed_data->path = NULL;
            parsed_data->path_size = 0;
        }
        // Whatever remains after '?' is the query string; keep it for the request.
        parsed_data->query = buffer + pos;
        parsed_data->query_size = buffer_size - pos;
    }

    size_t host_port_length = parsed_data->host_size;

    return parse_host_port(host_port_start, host_port_length, parsed_data);
}

ssize_t find_key_id(const http_parsed_field* parsed_fields, size_t fields_num,
                   const char* key, size_t key_size) {
    for (size_t i = 0; i < fields_num; i++) {
        if (parsed_fields[i].key_size == key_size && strncasecmp(parsed_fields[i].key, key, key_size) == 0) {
            return (ssize_t)i;
        }
    }
    return -1;
}

#undef PARSE_SIGN_SEPARATED
#undef SKIP_SPACES
