#include "net_tls.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/net_sockets.h"

static int s_fd = -1;
static mbedtls_ssl_context     s_ssl;
static mbedtls_ssl_config      s_conf;
static mbedtls_x509_crt        s_cacert;
static mbedtls_ctr_drbg_context s_drbg;
static mbedtls_entropy_context  s_ent;
static bool s_inited = false;

static int tls_send(void *ctx, const unsigned char *buf, size_t len){
    (void)ctx;
    int r = send(s_fd, buf, len, 0);
    if (r < 0) return (errno==EWOULDBLOCK||errno==EAGAIN) ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_NET_SEND_FAILED;
    return r;
}

static int tls_recv(void *ctx, unsigned char *buf, size_t len){
    (void)ctx;
    int r = recv(s_fd, buf, len, 0);
    if (r < 0) return (errno==EWOULDBLOCK||errno==EAGAIN) ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_NET_RECV_FAILED;
    return r;
}

// 3DS entropy source: hardware RNG via the PS service.
static int ps_entropy(void *d, unsigned char *out, size_t len, size_t *olen){
    (void)d;
    if (R_FAILED(PS_GenerateRandomBytes(out, len))) return -1;
    *olen = len;
    return 0;
}

bool tls_connect(const char *host){
    // Clean up any previous connection
    if (s_inited) {
        mbedtls_ssl_free(&s_ssl);
        mbedtls_ssl_config_free(&s_conf);
        mbedtls_x509_crt_free(&s_cacert);
        mbedtls_ctr_drbg_free(&s_drbg);
        mbedtls_entropy_free(&s_ent);
        if (s_fd >= 0) { closesocket(s_fd); s_fd = -1; }
        s_inited = false;
    }

    // Resolve + connect
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res = NULL;
    if (getaddrinfo(host, "443", &hints, &res) != 0) return false;
    s_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s_fd < 0) { freeaddrinfo(res); return false; }
    if (connect(s_fd, res->ai_addr, res->ai_addrlen) != 0) {
        closesocket(s_fd); s_fd = -1;
        freeaddrinfo(res);
        return false;
    }
    freeaddrinfo(res);

    // Load pinned CA from romfs
    FILE *f = fopen("romfs:/isrg_root_x1.pem", "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *pem = (unsigned char*)malloc(n + 1);
    if (!pem) { fclose(f); return false; }
    size_t nr = fread(pem, 1, n, f);
    pem[nr] = 0;
    fclose(f);

    // Init mbedTLS contexts
    mbedtls_ssl_init(&s_ssl);
    mbedtls_ssl_config_init(&s_conf);
    mbedtls_x509_crt_init(&s_cacert);
    mbedtls_ctr_drbg_init(&s_drbg);
    mbedtls_entropy_init(&s_ent);
    s_inited = true;

    mbedtls_entropy_add_source(&s_ent, ps_entropy, NULL, 32, MBEDTLS_ENTROPY_SOURCE_STRONG);
    if (mbedtls_ctr_drbg_seed(&s_drbg, mbedtls_entropy_func, &s_ent, (const unsigned char*)"claudendo", 9) != 0) {
        free(pem);
        return false;
    }

    int ret = mbedtls_x509_crt_parse(&s_cacert, pem, nr + 1);
    free(pem);
    if (ret != 0) return false;

    mbedtls_ssl_config_defaults(&s_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_authmode(&s_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&s_conf, &s_cacert, NULL);
    mbedtls_ssl_conf_rng(&s_conf, mbedtls_ctr_drbg_random, &s_drbg);
    if (mbedtls_ssl_setup(&s_ssl, &s_conf) != 0) return false;
    mbedtls_ssl_set_hostname(&s_ssl, host);
    mbedtls_ssl_set_bio(&s_ssl, NULL, tls_send, tls_recv, NULL);

    // Handshake loop
    while ((ret = mbedtls_ssl_handshake(&s_ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false;
        }
    }

    return mbedtls_ssl_get_verify_result(&s_ssl) == 0;
}

int tls_write(const void *buf, size_t len){
    const unsigned char *p = (const unsigned char*)buf;
    size_t sent = 0;
    while (sent < len) {
        int r = mbedtls_ssl_write(&s_ssl, p + sent, len - sent);
        if (r == MBEDTLS_ERR_SSL_WANT_WRITE || r == MBEDTLS_ERR_SSL_WANT_READ) continue;
        if (r < 0) return r;
        sent += (size_t)r;
    }
    return (int)sent;
}

int tls_read(void *buf, size_t len){
    while (1) {
        int r = mbedtls_ssl_read(&s_ssl, (unsigned char*)buf, len);
        if (r == MBEDTLS_ERR_SSL_WANT_READ) continue;
        if (r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) return 0;
        return r;
    }
}

void tls_close(void){
    if (s_inited) {
        mbedtls_ssl_close_notify(&s_ssl);
        mbedtls_ssl_free(&s_ssl);
        mbedtls_ssl_config_free(&s_conf);
        mbedtls_x509_crt_free(&s_cacert);
        mbedtls_ctr_drbg_free(&s_drbg);
        mbedtls_entropy_free(&s_ent);
        s_inited = false;
    }
    if (s_fd >= 0) {
        closesocket(s_fd);
        s_fd = -1;
    }
}
