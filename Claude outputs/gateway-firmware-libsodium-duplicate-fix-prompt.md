# Fix: `pio run -e esp32-s3-devkitc-1` fails with a `port` build-directory collision

## Ground rules — read before touching anything

- Touch only the two exact edits specified below, in the two files named. Do not modify anything else in either file, and do not touch any other file.
- Never write the device's real commercial name into any file, comment, or commit message. Refer to it only as "a commercial industrial IoT gateway reference device" if it needs mentioning at all.
- You cannot compile or run `pio`. Do not claim anything is "tested," "verified," "working," or "fixed." State exactly what you changed. A human will run `pio run -e esp32-s3-devkitc-1` afterward to confirm.
- If either file doesn't look the way it's described below, stop and report the discrepancy instead of improvising.

## Background (context only, don't act on this part)

`pio run -e esp32-s3-devkitc-1` fails during the Arduino-sketch build stage (after the ESP-IDF libs precompile stage now succeeds cleanly) with:

```
*** '.../firmware/.pio/build/esp32-s3-devkitc-1/port' already has a source directory:
'.../firmware/.pio/libdeps/esp32-s3-devkitc-1/libsodium/port'.
```

This was confirmed, via direct inspection of the checked-out library folders, to be caused by PlatformIO installing **two separate copies of the `libsodium` library**:

- `.pio/libdeps/esp32-s3-devkitc-1/libsodium/` — installed because `firmware/platformio.ini`'s `lib_deps` explicitly pins it: `https://github.com/esphome-libs/libsodium.git#1.10021.11`
- `.pio/libdeps/esp32-s3-devkitc-1/libsodium@src-<hash>/` — installed automatically because `wireguard`'s own `library.json` already declares `libsodium ^1.10018.1` as its own dependency, resolved via PlatformIO's package registry

A `diff -rq` between the two copies' `port/` folders came back with zero differences — they are byte-identical. Both copies define their `port/*.c` sources via a `srcFilter` that reaches outside their own `srcDir` (`+<../../../port/version.c>` etc.), and SCons maps that escaped path to the same unqualified `.pio/build/esp32-s3-devkitc-1/port` build directory for both copies, so the second one collides with the first.

The explicit `libsodium` pin in `lib_deps` was added deliberately when `wireguard` was integrated — `platformio.ini`'s own header comment explains that all `lib_deps` entries were pinned via direct git URL (rather than PlatformIO's registry package names) because the registry was network-blocked in the environment this project was originally developed in. That restriction does not apply in the environment this is actually being built in — a real `pio run` on the real build machine shows PlatformIO reaching the registry successfully (installing `Unity` via the registry, and auto-resolving `wireguard`'s own `libsodium` dependency via the registry too). So the explicit pin is no longer needed and is what's causing the duplicate-copy collision.

A repo-wide grep of `lib/`, `src/`, and `include/` confirmed nothing in this project's own code calls any libsodium API directly (`sodium_*`, `crypto_sign`, `crypto_box`, `crypto_secretbox`, `crypto_generichash`, `randombytes_buf`, etc. — zero matches). The only reference to `libsodium` anywhere in project code is a comment in `lib/wireguard_link/wireguard_link.h` noting it's pinned in `platformio.ini` "the same way" as wireguard — libsodium is purely an internal implementation detail of the `wireguard` library, never touched directly.

## Fix — two edits

### Edit 1: `firmware/platformio.ini`

In the `[env:esp32-s3-devkitc-1]` section, find this `lib_deps` block:

```ini
lib_deps =
    https://github.com/knolleary/pubsubclient.git
    https://github.com/esphome-libs/wireguard.git#v0.4.6
    https://github.com/esphome-libs/libsodium.git#1.10021.11
```

Remove the third line so it becomes:

```ini
lib_deps =
    https://github.com/knolleary/pubsubclient.git
    https://github.com/esphome-libs/wireguard.git#v0.4.6
```

Do not touch the header comment above `lib_deps` (the one explaining why entries are pinned via git URL) — it's still accurate for the two remaining entries. Do not touch anything else in this section (`board_build.flash_size`, `board_upload.flash_size`, `board_build.partitions`, `custom_sdkconfig`, etc. all stay exactly as they are).

### Edit 2: `firmware/lib/wireguard_link/wireguard_link.h`

Find this sentence, inside the larger comment block near the top of the file:

```
// i.e. `framework = espidf`). platformio.ini's lib_deps now pulls it
// (and its own libsodium dependency, pinned the same way - see that
// file's comment) directly; this is the real, confirmed integration
// shape, not an unverified guess. What's still genuinely unverified is
```

Replace just that sentence (keep everything else in the comment block, before and after, exactly as it is) with:

```
// i.e. `framework = espidf`). platformio.ini's lib_deps now pulls it
// directly. libsodium is deliberately NOT pinned separately there -
// wireguard's own library.json already declares a libsodium dependency,
// which PlatformIO resolves automatically; pinning it a second time via
// an explicit git URL caused PlatformIO to install two separate copies
// of it, which collided during the SCons build (both copies' port/
// source folder mapped to the same build directory). This is the real,
// confirmed integration shape, not an unverified guess. What's still
// genuinely unverified is
```

Only change this one sentence. Leave the rest of the comment block (before this point and after "What's still genuinely unverified is") exactly as it is.

## After both edits

List both files you edited with a one-line note of what changed in each. Don't run or claim to have run `pio`. A human will delete `.pio/libdeps/esp32-s3-devkitc-1/` (to force a clean re-resolution of dependencies against the new `lib_deps`) and then run `pio run -e esp32-s3-devkitc-1` to confirm.
