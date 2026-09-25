# Secure boot / flash encryption

Guidance only - **nothing in this project's firmware enables either of these**, and
this doc isn't a plan to do so soon. Written up now (2026-09-23) because it's an F3
checklist item and the underlying facts (irreversible eFuse burns) matter enough that
"we'll figure it out later" isn't good enough once someone actually reaches for the
`idf.py` command that turns them on. Confirmed directly against Espressif's own
ESP32-S3 security docs (Secure Boot V2, Flash Encryption) - not from general ESP32
knowledge, since the exact mechanism differs across chip generations and getting it
wrong here isn't recoverable.

## What these two features actually are

They're related but separate, and get enabled independently:

- **Secure Boot V2** - the bootloader and app image are signed (RSA-3072 with PSS
  padding, SHA-256) and the ROM bootloader + second-stage bootloader refuse to run
  anything not signed by a key matching the digest burned into eFuse. Stops someone
  from replacing this device's firmware with their own.
- **Flash encryption** - the flash contents themselves are encrypted at rest (AES-XTS,
  256 or 512-bit key), with the key generated on-device by the hardware RNG and burned
  into an eFuse block that's read/write-protected from software. Stops someone from
  reading firmware/secrets off the flash chip directly (relevant here: `device_config`
  stores WiFi credentials and, if TLS is used, certificate/private-key PEM material in
  plain NVS - see that module's own header for the same caveat stated where it
  actually matters day to day).

Either can be enabled without the other, but for what this project would actually want
(stop both firmware tampering and secret extraction), both matter.

## Why irreversibility is the whole story here

Both features work by burning eFuses - one-time, physically permanent bits on the
chip. Specifically:

- Secure Boot: `SECURE_BOOT_EN`, the key-digest eFuses (`KEY_PURPOSE_X`/`BLOCK_KEYX`),
  and optionally `KEY_REVOKEX` to permanently disable unused key slots. Once burned,
  every future bootloader/app image needs a valid signature from a key matching what's
  in eFuse - no exceptions, no override.
- Flash encryption: there are two modes.
  - **Development Mode** - the device will transparently encrypt plaintext firmware
    you flash over UART, and (only in this mode) the `SPI_BOOT_CRYPT_CNT` eFuse bits
    stay unprotected, so encryption can in principle still be turned back off by
    burning that same eFuse counter to a disabling value. Meant for iterating during
    bring-up, not a real "reversible" mode.
  - **Release Mode** - burns `DIS_DOWNLOAD_MANUAL_ENCRYPT` and write-protects
    `SPI_BOOT_CRYPT_CNT`. From that point on, **plaintext UART reflashing is
    permanently impossible** - the only way to update firmware is a signed/encrypted
    OTA update. If OTA is ever broken, misconfigured, or the signing key is lost, the
    device cannot be recovered over USB/UART at all. This is the actual sharp edge:
    not "flash encryption" in the abstract, but "Release Mode with no working OTA
    path" - that combination can genuinely brick a device for good.

## Why deferred, concretely (not just "it's risky")

1. **F3's OTA-with-rollback item isn't done yet.** Release Mode flash encryption
   removes UART reflashing as a recovery path - OTA becomes the *only* update
   mechanism from that point on. Enabling Release Mode before OTA is written, tested,
   and proven to roll back cleanly on a bad update would mean the first OTA bug bricks
   the device with no fallback. Do OTA first, prove it on real hardware, then consider
   this.
2. **No Rev-A hardware exists yet** (`docs/roadmap.md` step 8, not started). There's
   nothing to validate eFuse-burning against yet, and no reason to be burning
   irreversible bits on hardware that doesn't exist.
3. **The build toolchain path is unverified for this project's specific setup.**
   Signed/encrypted image generation normally goes through `idf.py`
   (`idf.py secure-boot-generate-signing-key`, `idf.py encrypted-app-flash`, etc.) -
   this project builds via PlatformIO + the pioarduino platform
   (`framework = arduino`), and whether that toolchain's image-generation step
   correctly produces signed/encrypted images the way plain ESP-IDF's does hasn't been
   checked here. Same category of "needs a real build-integration pass before
   trusting it" as `wireguard_link` (see that module's header and
   `docs/roadmap.md`'s F3 section) - don't assume it just works.
4. **Key custody is a real decision, not a checkbox.** Secure Boot's signing private
   key and, if generated off-device, a flash-encryption key both need to exist
   *outside* the device in a place that survives the device being lost, damaged, or
   bricked - losing the signing key after Release Mode is burned means this specific
   device can never receive another verified update, full stop. Where that key lives
   (a password manager? an offline USB drive? something more deliberate?) is worth
   deciding on its own, calmly, not improvised at 11pm while burning eFuses.

## Recommended sequencing, when it's time

1. OTA-with-rollback implemented and proven on Rev-A hardware (deliberately kill power
   mid-update, confirm it recovers) - not just written, actually exercised.
2. Secure Boot V2 first, on its own, in whatever mode Espressif's docs call
   non-final/testable if one exists for this chip - confirm signed images boot and
   unsigned ones are rejected, on real hardware, before touching flash encryption.
3. Flash encryption in **Development Mode** first, get comfortable with the encrypted-
   reflash workflow, confirm OTA still works end to end with it on.
4. Only then Release Mode - and only after the signing/encryption key(s) have a real,
   external, durable storage plan, written down somewhere that isn't this repo.
5. Treat step 4 as one-way. Re-confirm scope explicitly before running it, the same
   way `CLAUDE.md`'s git-history rewrite note already treats a different irreversible
   operation - a past note in this project's own history existing isn't authorization
   to proceed on a new one.

Nothing above is scheduled. This doc exists so that whenever it *is* time, the actual
facts (not a vague "it's permanent, be careful") are already written down.
