#include <stdio.h>
#include <inttypes.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>

#include "utils.h"
#include "http.h"
#include "err.h"
#include "tls.h"

#define MAX_REDIRECTS 5
#define RECONNECT_DELAY_US 500000

#define RESET_CONNECTION(connp) do { \
    conn_close(connp);               \
    is_connected = false;            \
} while(0)

#define CLEAN_EXIT(code) do {     \
    fflush(stdout);               \
    conn_close(&conn);            \
    tls_global_cleanup();         \
    SAFE_FREE(current_raw_url);   \
    SAFE_FREE(current_cookie);    \
    exit(code);                   \
} while(0)

#define FIND_HEADER(parsed_data, key_str) \
    find_key_id((parsed_data).fields, (parsed_data).fields_num, \
                (key_str), sizeof(key_str) - 1)

#define LOG(config, level, ...) do { \
    if ((config)->verbosity >= (level)) { \
        fprintf(stderr, __VA_ARGS__);\
        fprintf(stderr, "\n");       \
    }                                \
} while(0)

#define LOG_NO_NEWLINE(config, level, ...) do { \
    if ((config)->verbosity >= (level)) {       \
        fprintf(stderr, __VA_ARGS__);           \
    }                                           \
} while(0)

#define FREE_ALL(...) do { \
    void* __ptrs[] = { __VA_ARGS__ }; \
    for (size_t __i = 0; __i < sizeof(__ptrs) / sizeof(__ptrs[0]); __i++) { \
        free(__ptrs[__i]); \
    }                      \
} while(0)

#define SAFE_FREE(ptr) do { \
    if ((ptr)) free((ptr)); \
} while(0)

static bool url_is_https(const url_parsed* u) {
    return u->protocol && u->protocol_size == 5 &&
           strncasecmp(u->protocol, "https", 5) == 0;
}

static void log_headers_lf(const char* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != '\r') fputc(buf[i], stderr);
    }
}

static void set_default_config(configuration* config) {
    config->url = NULL;
    config->multiplex = false;
    config->timeout = 5000;
    config->force_ipv4 = false;
    config->force_ipv6 = false;
    config->verbosity = 2;
}

static void set_default_state(stream_state* state, char* meta_buffer) {
    state->metaint = 0;
    state->bytes_left = 0;
    state->meta_buffer = meta_buffer;
    state->meta_buffer_pos = 0;
    state->current_state = AUDIO;
}

static void parse_parameters(int argc, char* argv[], configuration* config, url_parsed* url, char** raw_url_out) {
    set_default_config(config);
    int opt;
    while ((opt = getopt(argc, argv, "u:mt:46v:q")) != -1) {
        switch (opt) {
            case 'u': {
                // strdup so we own the buffer; parse_url writes into it
                char* dup = strdup(optarg);
                if (!dup) syserr("strdup");
                if (parse_url(dup, strlen(dup), url, dup) < 0) {
                    free(dup);
                    fatal_usage("Failed to parse url!");
                }
                free(*raw_url_out);
                *raw_url_out = dup;
                config->url = url;
                break;
            }
            case 'm':
                config->multiplex = true;
                break;
            case 't': {
                errno = 0;
                char* end = NULL;
                long t = strtol(optarg, &end, 10);
                if (errno != 0 || !end || *end != '\0' || t < 100 || t > 100000) {
                    fatal_usage("Timeout out of range! Gave: %s, but needs 100-100000", optarg);
                }
                config->timeout = t;
                break;
            }
            case '4':
                config->force_ipv4 = true;
                break;
            case '6':
                config->force_ipv6 = true;
                break;
            case 'v': {
                errno = 0;
                char* end = NULL;
                long v = strtol(optarg, &end, 10);
                if (errno != 0 || !end || *end != '\0' || v < 0 || v > 4) {
                    fatal_usage("Verbosity out of range! Gave: %s, but needs 0-4", optarg);
                }
                config->verbosity = v;
                break;
            }
            case 'q':
                config->verbosity = 0;
                break;
            case '?':
            default:
                fatal_usage("Usage: %s -u url [-m] [-t timeout] [-4] [-6] [-v verbosity] [-q]", argv[0]);
        }
    }
    if (!config->url) fatal_usage("Parameter -u is required!");
    if (config->force_ipv4 && config->force_ipv6) {
        config->force_ipv4 = false;
        config->force_ipv6 = false;
    }
}

static int open_tcp(configuration* config, const url_parsed* current_url) {
    if (config->verbosity >= 1) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        if (t) {
            fprintf(stderr, "%04d.%02d.%02d %02d.%02d.%02d\n", t->tm_year + 1900,
                    t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        }
        fprintf(stderr, "resolving name %.*s\n", (int)current_url->host_size, current_url->host);
    }

    struct addrinfo hints, *res = NULL, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    if (config->force_ipv4) hints.ai_family = AF_INET;
    if (config->force_ipv6) hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    const char* default_port = url_is_https(current_url) ? "443" : "80";
    char* port = current_url->port_size > 0
                 ? strndup(current_url->port, current_url->port_size)
                 : strdup(default_port);
    char* host = strndup(current_url->host, current_url->host_size);
    if (!port || !host) syserr("strndup");

    int s = getaddrinfo(host, port, &hints, &res);
    if (s != 0) {
        LOG(config, 3, "getaddrinfo failed: %s", gai_strerror(s));
        FREE_ALL(host, port);
        return -1;
    }

    int sock = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) continue;

        if (config->verbosity >= 1) {
            char ipstr[INET6_ADDRSTRLEN];
            if (rp->ai_family == AF_INET) {
                struct sockaddr_in *ipv4 = (struct sockaddr_in*)rp->ai_addr;
                inet_ntop(rp->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
                fprintf(stderr, "connecting to server %s:%s\n", ipstr, port);
            } else {
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)rp->ai_addr;
                inet_ntop(rp->ai_family, &(ipv6->sin6_addr), ipstr, sizeof(ipstr));
                fprintf(stderr, "connecting to server [%s]:%s\n", ipstr, port);
            }
        }

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) break;
        close(sock);
        sock = -1;
    }

    freeaddrinfo(res);
    FREE_ALL(host, port);

    if (sock == -1) {
        LOG(config, 3, "Could not connect to server!");
        return -1;
    }
    return sock;
}

static void process_stream_data(const char* buf, size_t len, stream_state* state) {
    size_t pos = 0;
    while (pos < len) {
        if (state->current_state == AUDIO) {
            size_t chunk = len - pos;
            if (state->metaint > 0 && chunk > state->bytes_left) chunk = state->bytes_left;
            fwrite(buf + pos, 1, chunk, stdout);
            fflush(stdout);
            pos += chunk;
            if (state->metaint > 0) {
                state->bytes_left -= chunk;
                if (state->bytes_left == 0) state->current_state = META_LEN;
            }
        } else if (state->current_state == META_LEN) {
            int L = (unsigned char)buf[pos++];
            if (L == 0) {
                state->bytes_left = state->metaint;
                state->current_state = AUDIO;
            } else {
                state->bytes_left = L * 16;
                state->meta_buffer_pos = 0;
                state->current_state = META;
            }
        } else if (state->current_state == META) {
            size_t chunk = len - pos;
            if (chunk > state->bytes_left) chunk = state->bytes_left;
            memcpy(state->meta_buffer + state->meta_buffer_pos, buf + pos, chunk);
            state->meta_buffer_pos += chunk;
            pos += chunk;
            state->bytes_left -= chunk;
            if (state->bytes_left == 0) {
                state->meta_buffer[state->meta_buffer_pos] = '\0';
                fprintf(stderr, "%s\n", state->meta_buffer);
                state->bytes_left = state->metaint;
                state->current_state = AUDIO;
            }
        }
    }
}

static bool send_http_request(conn_t* conn, const url_parsed* current_url, const configuration* config, const char* current_cookie) {
    char req_buf[4096];
    http_header_field fields[4];
    int f_idx = 0;
    char host_header[256];
    // Examples in the task description always send Host without the port,
    // even for non-default ports.
    snprintf(host_header, sizeof(host_header), "%.*s",
             (int)current_url->host_size, current_url->host);
    fields[f_idx++] = (http_header_field){"Host", host_header};
    fields[f_idx++] = (http_header_field){"Connection", "Keep-Alive"};
    if (current_cookie) {
        fields[f_idx++] = (http_header_field){"Cookie", current_cookie};
    }
    if (config->multiplex) {
        fields[f_idx++] = (http_header_field){"Icy-MetaData", "1"};
    }

    char path_buf[2048] = "/";
    if (current_url->query_size > 0) {
        snprintf(path_buf, sizeof(path_buf), "/%.*s?%.*s",
                 (int)current_url->path_size, current_url->path ? current_url->path : "",
                 (int)current_url->query_size, current_url->query);
    } else if (current_url->path_size > 0) {
        snprintf(path_buf, sizeof(path_buf), "/%.*s",
                 (int)current_url->path_size, current_url->path);
    }
    http_message_data msg = {
            .method = "GET",
            .path = path_buf,
            .fields = fields,
            .field_num = f_idx,
            .content = NULL,
            .content_size = 0
    };

    ssize_t req_len = build_http_message(req_buf, sizeof(req_buf), &msg);
    if (req_len < 0) {
        LOG(config, 3, "Failed to build HTTP request");
        return false;
    }

    if (config->verbosity >= 1) {
        log_headers_lf(req_buf, (size_t)req_len);
        fputc('\n', stderr);
    }

    if (conn_write(conn, req_buf, (size_t)req_len) != req_len) {
        LOG(config, 3, "Failed to send HTTP request, reconnecting...");
        return false;
    }
    return true;
}

static bool read_http_headers(conn_t* conn, char* header_buf, size_t max_size, size_t* header_len, const configuration* config) {
    bool headers_done = false;
    bool timeout_occured = false;
    *header_len = 0;

    while (!headers_done && *header_len + 1 < max_size) {
        if (conn_pending(conn) == 0) {
            struct pollfd pfd = { .fd = conn->fd, .events = POLLIN };
            int pr = poll(&pfd, 1, (int)config->timeout);
            if (pr < 0) {
                if (errno == EINTR) continue;
                syserr("poll");
            }
            if (pr == 0) {
                timeout_occured = true;
                break;
            }
            if (!(pfd.revents & (POLLIN | POLLHUP | POLLERR))) {
                continue;
            }
        }

        ssize_t r = conn_read(conn, header_buf + *header_len, 1);
        if (r < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            return false;
        }
        if (r == 0) break;
        *header_len += (size_t)r;
        header_buf[*header_len] = '\0';
        if (*header_len >= 4 && memcmp(header_buf + *header_len - 4, "\r\n\r\n", 4) == 0) {
            headers_done = true;
        }
    }

    if (timeout_occured) {
        LOG(config, 1, "data receiving timeout");
        return false;
    }
    if (!headers_done) {
        LOG(config, 3, "Failed to read full headers, reconnecting...");
        return false;
    }
    return true;
}

static void update_cookie(const http_parsed_data* parsed, char** current_cookie) {
    ssize_t cookie_field_id = FIND_HEADER(*parsed, "set-cookie");
    if (cookie_field_id >= 0) {
        const char* val = parsed->fields[cookie_field_id].value;
        size_t val_len = parsed->fields[cookie_field_id].value_size;
        const char* semi = memchr(val, ';', val_len);
        if (semi) {
            val_len = semi - val;
        }
        SAFE_FREE(*current_cookie);
        *current_cookie = strndup(val, val_len);
    }
}

static long get_http_status(const http_parsed_data* parsed, configuration* config) {
    char code_buf[16];
    size_t copy_size = parsed->code_size < sizeof(code_buf) - 1 ?
                       parsed->code_size : sizeof(code_buf) - 1;
    strncpy(code_buf, parsed->code, copy_size);
    code_buf[copy_size] = '\0';
    char* end_ptr = NULL;
    errno = 0;
    long status_code = strtol(code_buf, &end_ptr, 10);

    if (errno != 0 || end_ptr == code_buf || *end_ptr != '\0' || status_code < 100 || status_code > 599) {
        LOG(config, 3, "Invalid or out of range HTTP status code: %s", code_buf);
        return -1;
    }
    return status_code;
}

static bool handle_redirect(const http_parsed_data* parsed, long status_code,
                            int* redirect_count, char** current_raw_url,
                            url_parsed* current_url) {
    if (status_code >= 300 && status_code <= 308) {
        char* redirect_location = NULL;
        ssize_t id = FIND_HEADER(*parsed, "location");
        if (id >= 0) redirect_location = strndup(parsed->fields[id].value,
                                                 parsed->fields[id].value_size);

        if (redirect_location) {
            if (*redirect_count >= MAX_REDIRECTS) {
                free(redirect_location);
                fatal("Too many redirects!");
            }
            SAFE_FREE(*current_raw_url);
            *current_raw_url = redirect_location;
            url_parsed new_url;
            if (parse_url(*current_raw_url, strlen(*current_raw_url), &new_url, *current_raw_url) < 0) {
                fatal("Failed to parse redirect URL!");
            }
            *current_url = new_url;
            (*redirect_count)++;
            return true;
        }
    }
    return false;
}

// Returns true if the line "quit" was seen (any line, ignoring trailing \r).
static bool consume_stdin_lines(char* buf, size_t* len) {
    size_t start = 0;
    for (size_t i = 0; i < *len; i++) {
        if (buf[i] == '\n') {
            size_t line_end = i;
            if (line_end > start && buf[line_end - 1] == '\r') line_end--;
            size_t line_len = line_end - start;
            if (line_len == 4 && memcmp(buf + start, "quit", 4) == 0) {
                return true;
            }
            start = i + 1;
        }
    }
    size_t leftover = *len - start;
    if (start > 0 && leftover > 0) memmove(buf, buf + start, leftover);
    *len = leftover;
    return false;
}

int main(int argc, char* argv[]) {
    configuration config_val;
    configuration* config = &config_val;
    url_parsed url;
    char* raw_url = NULL;
    parse_parameters(argc, argv, config, &url, &raw_url);
    err_set_verbosity((int)config->verbosity);

    // play(1) may close its stdin while we're writing; we want EPIPE, not SIGPIPE.
    signal(SIGPIPE, SIG_IGN);

    conn_t conn = { .fd = -1, .ssl = NULL, .is_tls = false };
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = -1;
    fds[1].events = POLLIN;
    bool is_connected = false;
    bool stdin_done = false;

    char meta_buffer[4081];
    stream_state state;
    set_default_state(&state, meta_buffer);
    char stdin_buffer[1024];
    size_t stdin_len = 0;

    int redirect_count = 0;
    url_parsed current_url = *(config->url);
    char* current_raw_url = raw_url;
    char* current_cookie = NULL;

    while (true) {
        if (!is_connected) {
            int sock = open_tcp(config, &current_url);
            if (sock == -1) {
                if (redirect_count > 0) {
                    fatal("Connection failed after redirect");
                }
                fatal("Could not connect to server");
            }

            bool want_tls = url_is_https(&current_url);
            if (want_tls) tls_global_init();
            if (conn_init(&conn, sock, want_tls,
                          (current_url.host && current_url.host_size > 0) ? current_url.host : NULL) < 0) {
                close(sock);
                LOG(config, 3, "TLS handshake failed");
                fatal("TLS handshake failed");
            }

            if (!send_http_request(&conn, &current_url, config, current_cookie)) {
                RESET_CONNECTION(&conn);
                usleep(RECONNECT_DELAY_US);
                continue;
            }

            char header_buf[8192];
            size_t header_len = 0;
            if (!read_http_headers(&conn, header_buf, sizeof(header_buf), &header_len, config)) {
                RESET_CONNECTION(&conn);
                usleep(RECONNECT_DELAY_US);
                continue;
            }

            if (config->verbosity >= 1) {
                char* headers_end = strstr(header_buf, "\r\n\r\n");
                if (headers_end) {
                    log_headers_lf(header_buf, (size_t)(headers_end - header_buf + 4));
                    // Trailing blank line separating response from stream titles.
                    fputc('\n', stderr);
                }
            }

            http_parsed_data parsed;
            http_parsed_field parsed_fields[64];
            if (parse_http_message(header_buf, header_len, &parsed, parsed_fields,
                                   sizeof(parsed_fields) / sizeof(parsed_fields[0])) < 0) {
                LOG(config, 3, "Failed to parse HTTP response!");
                RESET_CONNECTION(&conn);
                usleep(RECONNECT_DELAY_US);
                continue;
            }

            update_cookie(&parsed, &current_cookie);

            long status_code = get_http_status(&parsed, config);
            if (status_code == -1) {
                RESET_CONNECTION(&conn);
                usleep(RECONNECT_DELAY_US);
                continue;
            }

            if (handle_redirect(&parsed, status_code, &redirect_count, &current_raw_url, &current_url)) {
                RESET_CONNECTION(&conn);
                continue;
            }

            if (status_code >= 400) {
                fatal("HTTP error response: %ld", status_code);
            }

            redirect_count = 0;
            set_default_state(&state, meta_buffer);

            if (config->multiplex) {
                ssize_t metaint_id = FIND_HEADER(parsed, "icy-metaint");
                if (metaint_id >= 0) {
                    char temp_val[64];
                    size_t c_size = parsed.fields[metaint_id].value_size < sizeof(temp_val) - 1
                                    ? parsed.fields[metaint_id].value_size
                                    : sizeof(temp_val) - 1;
                    memcpy(temp_val, parsed.fields[metaint_id].value, c_size);
                    temp_val[c_size] = '\0';
                    state.metaint = strtoull(temp_val, NULL, 10);
                }
            }
            state.bytes_left = state.metaint;

            is_connected = true;
            fds[1].fd = conn.fd;
            fds[1].events = POLLIN;
            if (parsed.content_size > 0 && parsed.content) {
                process_stream_data(parsed.content, parsed.content_size, &state);
            }
        }

        // If TLS has buffered bytes, don't block in poll.
        int poll_timeout = conn_pending(&conn) > 0 ? 0 : (int)config->timeout;
        fds[0].fd = stdin_done ? -1 : STDIN_FILENO;
        int ret = poll(fds, 2, poll_timeout);
        if (ret < 0) {
            if (errno == EINTR) continue;
            syserr("poll");
        }
        if (ret == 0 && conn_pending(&conn) == 0) {
            LOG(config, 1, "data receiving timeout");
            RESET_CONNECTION(&conn);
            usleep(RECONNECT_DELAY_US);
            continue;
        }

        if (!stdin_done && (fds[0].revents & (POLLIN | POLLHUP))) {
            ssize_t r = read(STDIN_FILENO, stdin_buffer + stdin_len, sizeof(stdin_buffer) - stdin_len - 1);
            if (r > 0) {
                stdin_len += (size_t)r;
                if (consume_stdin_lines(stdin_buffer, &stdin_len)) {
                    CLEAN_EXIT(0);
                }
                if (stdin_len >= sizeof(stdin_buffer) - 1) {
                    // Pathological long line without newline; drop it.
                    stdin_len = 0;
                }
            } else if (r == 0) {
                // EOF on stdin is not a fatal error per task forum guidance.
                stdin_done = true;
                fds[0].fd = -1;
            } else {
                if (errno != EINTR && errno != EAGAIN) {
                    LOG(config, 3, "STDIN read error");
                    stdin_done = true;
                    fds[0].fd = -1;
                }
            }
        }

        bool conn_ready = conn_pending(&conn) > 0 || (fds[1].revents & (POLLIN | POLLHUP | POLLERR));
        if (conn_ready) {
            char net_buf[8192];
            ssize_t bytes_read = conn_read(&conn, net_buf, sizeof(net_buf));
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EINTR) continue;
                LOG(config, 3, "Read error, reconnecting...");
                RESET_CONNECTION(&conn);
                usleep(RECONNECT_DELAY_US);
                continue;
            }
            if (bytes_read == 0) {
                // Server closed connection - flush and exit 0 per task spec.
                CLEAN_EXIT(0);
            }
            process_stream_data(net_buf, (size_t)bytes_read, &state);
        }
    }
}
