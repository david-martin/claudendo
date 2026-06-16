#pragma once
#include <stddef.h>
#include <stdbool.h>
bool tls_connect(const char *host);          // TCP + verified TLS handshake to host:443
int  tls_write(const void *buf, size_t len);  // returns bytes written or <0
int  tls_read(void *buf, size_t len);         // returns bytes read, 0 on close, <0 on error
void tls_close(void);
