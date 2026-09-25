#include "url_host.h"
#include <string.h>

bool extractHttpsUrlHost(const char *url, char *out, size_t out_size) {
    if (url == nullptr || out == nullptr || out_size == 0) return false;

    static const char PREFIX[] = "https://";
    static const size_t PREFIX_LEN = sizeof(PREFIX) - 1; // exclude '\0'
    if (strncmp(url, PREFIX, PREFIX_LEN) != 0) return false;

    const char *auth_start = url + PREFIX_LEN;
    const char *p = auth_start;
    while (*p != '\0' && *p != '/' && *p != '?') {
        p++;
    }

    const char *host_start = auth_start;
    for (const char *q = auth_start; q < p; q++) {
        if (*q == '@') {
            host_start = q + 1;
        }
    }

    const char *host_end = host_start;
    while (host_end < p && *host_end != ':') {
        host_end++;
    }

    size_t len = (size_t)(host_end - host_start);
    if (len == 0 || len + 1 > out_size) return false;

    memcpy(out, host_start, len);
    out[len] = '\0';
    return true;
}
