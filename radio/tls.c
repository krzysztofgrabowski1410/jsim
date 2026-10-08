#include "tls.h"
#include "err.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

static SSL_CTX* g_ctx = NULL;

void tls_global_init(void) {
    if (g_ctx) return;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    g_ctx = SSL_CTX_new(TLS_client_method());
    if (!g_ctx) fatal("SSL_CTX_new failed");
    SSL_CTX_set_min_proto_version(g_ctx, TLS1_2_VERSION);
    // Best-effort trust store; we still don't verify (radio servers' chains vary).
    SSL_CTX_set_default_verify_paths(g_ctx);
    SSL_CTX_set_verify(g_ctx, SSL_VERIFY_NONE, NULL);
}

void tls_global_cleanup(void) {
    if (g_ctx) {
        SSL_CTX_free(g_ctx);
        g_ctx = NULL;
    }
}

int conn_init(conn_t* c, int fd, bool use_tls, const char* hostname) {
    if (!c) return -1;
    c->fd = fd;
    c->ssl = NULL;
    c->is_tls = use_tls;

    if (!use_tls) return 0;

    if (!g_ctx) tls_global_init();
    c->ssl = SSL_new(g_ctx);
    if (!c->ssl) return -1;

    if (SSL_set_fd(c->ssl, fd) != 1) {
        SSL_free(c->ssl);
        c->ssl = NULL;
        return -1;
    }
    if (hostname && *hostname) {
        // SNI - many shared TLS hosts require it.
        SSL_set_tlsext_host_name(c->ssl, hostname);
    }

    int r = SSL_connect(c->ssl);
    if (r != 1) {
        SSL_free(c->ssl);
        c->ssl = NULL;
        return -1;
    }
    return 0;
}

size_t conn_pending(const conn_t* c) {
    if (!c || !c->is_tls || !c->ssl) return 0;
    int p = SSL_pending(c->ssl);
    return p > 0 ? (size_t)p : 0;
}

ssize_t conn_read(conn_t* c, void* buf, size_t len) {
    if (!c) { errno = EINVAL; return -1; }
    if (!c->is_tls) {
        return read(c->fd, buf, len);
    }
    int n = SSL_read(c->ssl, buf, (int)len);
    if (n > 0) return n;
    int err = SSL_get_error(c->ssl, n);
    if (err == SSL_ERROR_ZERO_RETURN) return 0;
    if (err == SSL_ERROR_SYSCALL && n == 0) return 0; // peer closed
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        errno = EAGAIN;
        return -1;
    }
    errno = EIO;
    return -1;
}

ssize_t conn_write(conn_t* c, const void* buf, size_t len) {
    if (!c) { errno = EINVAL; return -1; }
    if (!c->is_tls) {
        // Send all bytes; tolerate short writes.
        const char* p = buf;
        size_t left = len;
        while (left > 0) {
            ssize_t w = write(c->fd, p, left);
            if (w < 0) {
                if (errno == EINTR) continue;
                return -1;
            }
            if (w == 0) return -1;
            p += w; left -= w;
        }
        return (ssize_t)len;
    }
    int w = SSL_write(c->ssl, buf, (int)len);
    if (w > 0) return w;
    errno = EIO;
    return -1;
}

void conn_close(conn_t* c) {
    if (!c) return;
    if (c->ssl) {
        // Best effort shutdown; ignore errors.
        SSL_shutdown(c->ssl);
        SSL_free(c->ssl);
        c->ssl = NULL;
    }
    if (c->fd != -1) {
        close(c->fd);
        c->fd = -1;
    }
    c->is_tls = false;
}
