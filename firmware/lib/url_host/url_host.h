#pragma once
#include <stddef.h>

// Extracts the hostname from a URL that MUST start with "https://" -
// OTA downloads (this module's only current caller, ota_session.h)
// are HTTPS-only, no plain-HTTP fallback: a firmware image's download
// path integrity matters even though image_verify.h's signature check
// is the real security boundary, not this allowlist gate. Pulled into
// its own pure/gcc-testable module rather than inlined into
// ota_session.h, the same "extract everything pure out of the
// hardware-I/O orchestration" reasoning as ota_trigger.h/
// image_verify.h.
//
// Returns false (out left unspecified) if url is null, doesn't start
// with exactly "https://", the host portion is empty, or the host
// doesn't fit out_size. Deliberately NOT a general URL parser - it
// only extracts the substring between "https://" and the next '/',
// ':', '?', or end of string, with no validation that the result is a
// syntactically well-formed hostname/IP. That's fine here because the
// caller only ever uses the result for an exact-string match against
// net_allowlist's commissioning-configured entries (net_allowlist.h) -
// a malformed "host" simply won't match anything in the allowlist and
// fails closed there, same as any other unrecognized host would.
bool extractHttpsUrlHost(const char *url, char *out, size_t out_size);
