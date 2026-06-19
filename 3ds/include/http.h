#pragma once
#include <3ds.h>
#include <stddef.h>
// Returns HTTP status code (e.g. 200), or <0 on transport/TLS failure.
// On success allocates *pcm_out via linearAlloc (caller linearFree); fills desc (NUL-terminated) and *rate.
int http_describe(const char *host, const char *token, const char *persona,
                  const u8 *img, int w, int h,
                  char *desc, size_t desc_cap,
                  u8 **pcm_out, size_t *pcm_len, int *rate_out);
