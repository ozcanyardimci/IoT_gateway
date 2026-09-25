#include "net_allowlist.h"
#include <string.h>

void NetAllowlist::clear() {
    count_ = 0;
}

bool NetAllowlist::add(const char *host) {
    if (host == nullptr || host[0] == '\0') return false;
    if (strlen(host) >= (size_t)MAX_HOST_LEN) return false;
    if (count_ >= MAX_ENTRIES) return false;
    strncpy(entries_[count_], host, MAX_HOST_LEN - 1);
    entries_[count_][MAX_HOST_LEN - 1] = '\0';
    count_++;
    return true;
}

bool NetAllowlist::isAllowed(const char *host) const {
    if (host == nullptr || host[0] == '\0') return false;
    for (int i = 0; i < count_; i++) {
        if (strcmp(entries_[i], host) == 0) return true;
    }
    return false;
}
