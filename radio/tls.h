#ifndef RADIO_TLS_H
#define RADIO_TLS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include <openssl/ssl.h>

typedef struct {
    int fd;
    SSL* ssl;
    bool is_tls;
} conn_t;

void tls_global_init(void);

void tls_global_cleanup(void);

// Wraps an already-connected socket; for plain HTTP just records fd.
// For TLS performs SSL_connect() with SNI set to hostname.
// Returns 0 on success, -1 on failure. On failure fd is NOT closed (caller's responsibility).
int conn_init(conn_t* c, int fd, bool use_tls, const char* hostname);

// SSL_pending() (or 0 for plain) - bytes immediately available without IO.
size_t conn_pending(const conn_t* c);

// Reads up to len bytes; returns >0 bytes read, 0 on clean shutdown / EOF,
// -1 on error (errno may be set for plain, or set to EIO for SSL errors).
ssize_t conn_read(conn_t* c, void* buf, size_t len);

// Writes exactly len bytes; returns len on success, -1 on error.
ssize_t conn_write(conn_t* c, const void* buf, size_t len);

// Tears down SSL (if any) and closes fd; safe to call repeatedly.
void conn_close(conn_t* c);

#endif //RADIO_TLS_H
