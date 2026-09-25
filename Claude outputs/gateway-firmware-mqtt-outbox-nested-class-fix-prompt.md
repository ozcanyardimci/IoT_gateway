# Fix: `pio run -e esp32-s3-devkitc-1` fails at link with undefined references to `MqttClientWrapper::MqttOutbox::*`

## Ground rules — read before touching anything

- Touch only `firmware/lib/mqtt_client_wrapper/mqtt_client_wrapper.h`, and only the two exact edits specified below (moving one line). Do not touch `firmware/lib/mqtt_client_wrapper/mqtt_client_wrapper.cpp`, `firmware/lib/mqtt_outbox/mqtt_outbox.h`, `firmware/lib/mqtt_outbox/mqtt_outbox.cpp`, or `firmware/test/test_mqtt_outbox/test_mqtt_outbox.cpp` — all four were read and confirmed correct as-is; this is not a bug in any of them.
- Never write the device's real commercial name into any file, comment, or commit message. Refer to it only as "a commercial industrial IoT gateway reference device" if it needs mentioning at all.
- You cannot compile or run `pio`. Do not claim anything is "tested," "verified," "working," or "fixed." State exactly what you changed. A human will run `pio run -e esp32-s3-devkitc-1` afterward to confirm.
- If `mqtt_client_wrapper.h` doesn't look the way it's described below, stop and report the discrepancy instead of improvising.

## Background (context only, don't act on this part)

`pio run -e esp32-s3-devkitc-1` now compiles the entire codebase cleanly and reaches the linking stage for the first time, then fails with:

```
ld: .../mqtt_client_wrapper.cpp.o: undefined reference to `MqttClientWrapper::MqttOutbox::enqueue(char const*, char const*)'
ld: .../mqtt_client_wrapper.cpp.o: undefined reference to `MqttClientWrapper::MqttOutbox::getItem(int)'
ld: .../mqtt_client_wrapper.cpp.o: undefined reference to `MqttClientWrapper::MqttOutbox::compact()'
ld: .../mqtt_client_wrapper.cpp.o: undefined reference to `MqttClientWrapper::MqttOutbox::MqttOutbox()'
collect2: error: ld returned 1 exit status
```

This was root-caused by reading all four relevant files directly (not guessed from the error text alone):

`firmware/lib/mqtt_outbox/mqtt_outbox.h` declares `MqttOutbox` as an ordinary, self-contained, **top-level** class (own `#pragma once`, no enclosing class). `firmware/lib/mqtt_outbox/mqtt_outbox.cpp` implements its four methods the same way, as plain top-level definitions (`MqttOutbox::enqueue(...)`, etc.) — these compile to symbols like `MqttOutbox::enqueue(...)`, with no enclosing class in the mangled name. `firmware/test/test_mqtt_outbox/test_mqtt_outbox.cpp` also uses it this way, as a plain top-level class — which is why the native unit tests never caught this bug: they exercise `MqttOutbox` directly and never touch `mqtt_client_wrapper.h` at all.

The bug is in `firmware/lib/mqtt_client_wrapper/mqtt_client_wrapper.h`. Its `private:` section currently contains this line:

```cpp
private:
    #include "mqtt_outbox.h"
    PubSubClient client_;
    const char *client_id_;
    MqttOutbox outbox_;
```

`#include "mqtt_outbox.h"` sits *inside the body of `class MqttClientWrapper`*. The preprocessor pastes `mqtt_outbox.h`'s full text — including `class MqttOutbox { ... };` — directly into that spot, which makes the compiler define `MqttOutbox` as a **class nested inside `MqttClientWrapper`**, i.e. `MqttClientWrapper::MqttOutbox`, only in this one file. Every reference to `MqttOutbox` inside `mqtt_client_wrapper.h`/`.cpp` (the `outbox_` member, `MqttOutbox::CAPACITY`, `outbox_.enqueue(...)`, etc.) therefore resolves to the nested type `MqttClientWrapper::MqttOutbox`, and the compiler emits calls to `MqttClientWrapper::MqttOutbox::enqueue(...)` and friends. But the only compiled implementation of those methods, in `mqtt_outbox.cpp`, is the plain top-level `MqttOutbox::enqueue(...)` — a different, unrelated symbol. Two different C++ types end up sharing one name depending on which file the header text landed in, and the linker can't match the nested-type calls to the top-level-type definitions.

This is leftover from the project's earlier split of `mqtt_outbox` out of `mqtt_client_wrapper` (done specifically so native tests wouldn't need to fake `PubSubClient`): the class was evidently written inline as a nested class first, then extracted into its own file — but the `#include` was left positioned inside the class body instead of being moved up to the top of the file with the other includes. Real hardware builds are the first thing that actually links `mqtt_client_wrapper.cpp` against `mqtt_outbox.cpp`, which is why this only surfaces now.

## The fix — one line moved, in `firmware/lib/mqtt_client_wrapper/mqtt_client_wrapper.h`

### Edit 1: add the include to the top of the file, with the other includes

Find:

```cpp
#pragma once
#include <stdint.h>
#include <functional>
#include <PubSubClient.h>
#include <Client.h>
```

Replace with:

```cpp
#pragma once
#include <stdint.h>
#include <functional>
#include <PubSubClient.h>
#include <Client.h>
#include "mqtt_outbox.h"
```

### Edit 2: remove the include from inside the class body

Find:

```cpp
private:
    #include "mqtt_outbox.h"
    PubSubClient client_;
    const char *client_id_;
    MqttOutbox outbox_;
```

Replace with:

```cpp
private:
    PubSubClient client_;
    const char *client_id_;
    MqttOutbox outbox_;
```

Nothing else in the `private:` section (the backoff fields, `subscribed_topic_`, `message_callback_`, `flushOutbox()`, `tryReconnect()`) changes — only the one `#include` line moves. After this change, `MqttOutbox` is the same top-level class everywhere it's used — inside `mqtt_client_wrapper.h`/`.cpp`, in `mqtt_outbox.cpp`, and in the native test — matching the mangled symbols the linker is looking for.

Do not change anything else in this file, and do not touch `mqtt_client_wrapper.cpp`, `mqtt_outbox.h`, `mqtt_outbox.cpp`, or the test file — all four are correct as they are; only the include's position in the header was wrong.

## After the change

Confirm the one line you moved (removed from inside the `private:` block, added to the top-of-file include list) and show both resulting snippets. Don't run or claim to have run `pio`. A human will run `pio run -e esp32-s3-devkitc-1` afterward. If this is right, linking should succeed and produce `firmware.elf`/the final flashable image — but that hasn't been confirmed by an actual run yet, so don't assume it's done until that comes back clean.
