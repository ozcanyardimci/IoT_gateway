#include "image_verify.h"
#include <string.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>

Sha256Streaming::Sha256Streaming() {
    mbedtls_sha256_init(&ctx_);
    // BUG FIX (2026-09-24, L6): check the return code instead of
    // discarding it - see the "ok()" comment in image_verify.h for
    // why this matters even though it practically never fails.
    int ret = mbedtls_sha256_starts(&ctx_, 0); // is224 = 0 -> real SHA-256, not SHA-224
    ok_ = (ret == 0);
}

Sha256Streaming::~Sha256Streaming() {
    mbedtls_sha256_free(&ctx_);
}

void Sha256Streaming::update(const uint8_t *data, size_t len) {
    // BUG FIX (2026-09-24, L6): same as the constructor - check the
    // return code. Once ok_ goes false it must stay false for the
    // rest of this object's life (a later successful update() call
    // must not paper over an earlier failed one), so this only ever
    // ANDs a new result in, never sets ok_ back to true.
    int ret = mbedtls_sha256_update(&ctx_, data, len);
    ok_ = ok_ && (ret == 0);
}

void Sha256Streaming::finish(uint8_t out[IMAGE_VERIFY_SHA256_LEN]) {
    // BUG FIX (2026-09-24, L6): same reasoning - finish() has an int
    // return too, latch it the same way.
    int ret = mbedtls_sha256_finish(&ctx_, out);
    ok_ = ok_ && (ret == 0);
}

bool verifySignature(const uint8_t hash[IMAGE_VERIFY_SHA256_LEN],
                      const uint8_t *signature, size_t signature_len,
                      const char *public_key_pem) {
    // Fail closed on the placeholder/not-yet-provisioned key (see
    // ota_signing_key.h) rather than letting an empty key somehow
    // parse as "trust nothing, so anything passes."
    if (public_key_pem == nullptr || public_key_pem[0] == '\0') {
        return false;
    }
    if (hash == nullptr || signature == nullptr || signature_len == 0) {
        return false;
    }

    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    // mbedtls_pk_parse_public_key expects the PEM buffer's length to
    // include the null terminator for PEM input (its own documented
    // behavior - PEM parsing needs the terminator to find the end of
    // the "-----END...-----" line reliably) - strlen()+1, not
    // strlen().
    int parse_result = mbedtls_pk_parse_public_key(
        &pk, reinterpret_cast<const unsigned char *>(public_key_pem),
        strlen(public_key_pem) + 1);

    bool ok = false;
    if (parse_result == 0) {
        int verify_result = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash,
                                               IMAGE_VERIFY_SHA256_LEN, signature,
                                               signature_len);
        ok = (verify_result == 0); // 0 is mbedtls_pk_verify's only
                                    // "signature is valid" result -
                                    // every other return value
                                    // (including MBEDTLS_ERR_PK_SIG_LEN_MISMATCH)
                                    // is a failure, treated uniformly
                                    // as "not valid" here rather than
                                    // special-cased
    }

    mbedtls_pk_free(&pk);
    return ok;
}

int base64Decode(const char *input, uint8_t *out, size_t out_size) {
    if (input == nullptr || out == nullptr) return -1;
    size_t olen = 0;
    int result = mbedtls_base64_decode(out, out_size, &olen,
                                        reinterpret_cast<const unsigned char *>(input),
                                        strlen(input));
    if (result != 0) return -1; // buffer too small or invalid base64 -
                                 // caller doesn't need to distinguish
                                 // which, both mean "reject"
    return (int)olen;
}
