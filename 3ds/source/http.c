#include "http.h"
#include "net_tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

// Case-insensitive substring search (header region is NUL-terminated)
static char *strichr(char *s, const char *needle){
    size_t nlen = strlen(needle);
    size_t slen = strlen(s);
    if (nlen > slen) return NULL;
    for (size_t i = 0; i <= slen - nlen; i++) {
        size_t j;
        for (j = 0; j < nlen; j++) {
            if (tolower((unsigned char)s[i+j]) != tolower((unsigned char)needle[j])) break;
        }
        if (j == nlen) return s + i;
    }
    return NULL;
}

int http_describe(const char *host, const char *token, const char *persona,
                  const u8 *img, int w, int h,
                  char *desc, size_t desc_cap,
                  u8 **pcm_out, size_t *pcm_len, int *rate_out)
{
    if (!persona || !*persona) persona = "marvin";
    *pcm_out  = NULL;
    *pcm_len  = 0;
    *rate_out = 22050;
    if (desc_cap > 0) desc[0] = '\0';

    // 1. Connect
    if (!tls_connect(host)) return -1;

    // 2. Build and send request header
    char hdr[1024];
    int body_len = w * h * 2; // RGB565 = 2 bytes/pixel
    int hdr_len = snprintf(hdr, sizeof(hdr),
        "POST /describe HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Authorization: Bearer %s\r\n"
        "X-Image-Width: %d\r\n"
        "X-Image-Height: %d\r\n"
        "X-Image-Format: rgb565\r\n"
        "X-Persona: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        host, token, w, h, persona, body_len);

    if (hdr_len < 0 || hdr_len >= (int)sizeof(hdr)) { tls_close(); return -2; }
    if (tls_write(hdr, (size_t)hdr_len) < 0) { tls_close(); return -2; }

    // Send image body in chunks
    const u8 *p = img;
    int remaining = body_len;
    while (remaining > 0) {
        int chunk = remaining < 4096 ? remaining : 4096;
        int r = tls_write(p, (size_t)chunk);
        if (r < 0) { tls_close(); return -3; }
        p += r;
        remaining -= r;
    }

    // 3. Read full response into a heap buffer (grow as needed)
    size_t cap = 8192;
    size_t total = 0;
    char *resp = (char*)malloc(cap);
    if (!resp) { tls_close(); return -4; }

    while (1) {
        if (total + 4096 > cap) {
            cap *= 2;
            char *tmp = (char*)realloc(resp, cap);
            if (!tmp) { free(resp); tls_close(); return -5; }
            resp = tmp;
        }
        int r = tls_read(resp + total, cap - total - 1);
        if (r < 0) break; // treat read error as EOF (Connection: close server side)
        if (r == 0) break;
        total += (size_t)r;
    }

    tls_close();

    if (total == 0) { free(resp); return -6; }
    resp[total] = '\0'; // safe: we kept cap-1 space

    // 4. Find header/body split
    const char *split = NULL;
    // Search for \r\n\r\n in the raw bytes
    for (size_t i = 0; i + 3 < total; i++) {
        if (resp[i]=='\r' && resp[i+1]=='\n' && resp[i+2]=='\r' && resp[i+3]=='\n') {
            split = resp + i + 4;
            break;
        }
    }
    if (!split) { free(resp); return -7; }

    size_t header_len = (size_t)(split - resp);
    size_t body_off   = header_len;
    size_t body_size  = total - body_off;

    // NUL-terminate the header region for string ops
    // We already NUL-terminated all of resp, so resp[header_len-1] = the last header byte
    // Make a header copy to safely NUL-terminate just the header
    char *hdrbuf = (char*)malloc(header_len + 1);
    if (!hdrbuf) { free(resp); return -8; }
    memcpy(hdrbuf, resp, header_len);
    hdrbuf[header_len] = '\0';

    // Parse status code from first line: "HTTP/1.1 NNN ..."
    int status = 0;
    if (strncmp(hdrbuf, "HTTP/", 5) == 0) {
        char *sp = strchr(hdrbuf, ' ');
        if (sp) status = atoi(sp + 1);
    }

    // Find X-Description: header
    char *xd = strichr(hdrbuf, "\r\nX-Description:");
    if (xd) {
        xd += strlen("\r\nX-Description:");
        while (*xd == ' ') xd++;
        char *eol = strchr(xd, '\r');
        if (!eol) eol = strchr(xd, '\n');
        size_t vlen = eol ? (size_t)(eol - xd) : strlen(xd);
        if (vlen >= desc_cap) vlen = desc_cap - 1;
        memcpy(desc, xd, vlen);
        desc[vlen] = '\0';
    }

    // Find X-Sample-Rate: header
    char *xr = strichr(hdrbuf, "\r\nX-Sample-Rate:");
    if (xr) {
        xr += strlen("\r\nX-Sample-Rate:");
        while (*xr == ' ') xr++;
        *rate_out = atoi(xr);
        if (*rate_out <= 0) *rate_out = 22050;
    }

    free(hdrbuf);

    // 5. Copy PCM body into linearAlloc buffer (binary-safe)
    if (body_size > 0) {
        *pcm_out = (u8*)linearAlloc(body_size);
        if (*pcm_out) {
            memcpy(*pcm_out, resp + body_off, body_size);
            *pcm_len = body_size;
        }
    }

    free(resp);
    return status;
}
