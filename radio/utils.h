#ifndef RADIO_UTILS_H
#define RADIO_UTILS_H

#include <stdbool.h>
#include "http.h"

typedef struct {
    url_parsed* url;
    bool multiplex;
    long timeout;
    bool force_ipv4;
    bool force_ipv6;
    long verbosity;
} configuration;

typedef enum {
    AUDIO, META, META_LEN
} state_type;

typedef struct {
    size_t metaint;
    size_t bytes_left;
    char* meta_buffer;
    size_t meta_buffer_pos;
    state_type current_state;
} stream_state;

#endif //RADIO_UTILS_H
