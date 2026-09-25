#pragma once

// The OTA image-verification PUBLIC key, compiled into firmware -
// deliberately NOT stored in DeviceConfig/NVS and NOT settable via
// commissioning_portal's form (unlike WireGuard's keys or MQTT's TLS
// material). See image_verify.h's header comment for the full
// reasoning; short version: if this key were commissioning-portal-
// configurable, anyone able to commission the device (an
// unauthenticated local AP) could install their own key and sign
// their own "updates" - the whole point is that changing this key
// requires a new firmware BUILD, which needs source-tree access, not
// local-AP access.
//
// *** PLACEHOLDER - NOT A REAL KEY ***
// The string below is empty on purpose, not a real (even if
// low-value) key, so image_verify's verifySignature() fails closed -
// rejects EVERY signature, i.e. refuses every OTA trigger - until
// Ozcan actually provisions a real key pair. That's deliberate: a
// silently-accepted placeholder key would be far worse than an OTA
// path that simply doesn't work yet.
//
// How to provision a real key pair (ECDSA P-256/secp256r1 - see
// image_verify.h for why this curve): generate the PRIVATE key
// offline, on a machine that never touches this device or repo, and
// keep it there - it signs each release's firmware SHA-256 hash and
// is never embedded anywhere:
//   openssl ecparam -name prime256v1 -genkey -noout -out ota_signing_private.pem
//   openssl ec -in ota_signing_private.pem -pubout -out ota_signing_public.pem
// Paste ota_signing_public.pem's contents (the "-----BEGIN PUBLIC
// KEY-----" block) in as OTA_SIGNING_PUBLIC_KEY_PEM below, replacing
// this placeholder, then to sign a release image:
//   openssl dgst -sha256 -sign ota_signing_private.pem -out fw.sig firmware.bin
//   openssl base64 -A -in fw.sig -out fw.sig.b64
// fw.sig.b64's contents are the "sig" field of the MQTT OTA-trigger
// payload (ota_trigger.h); the "sha256" field is
// `openssl dgst -sha256 firmware.bin` in hex.
#define OTA_SIGNING_PUBLIC_KEY_PEM ""
