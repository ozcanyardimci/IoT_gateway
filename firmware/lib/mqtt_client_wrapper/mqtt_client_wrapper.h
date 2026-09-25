#pragma once
#include <stdint.h>
#include <functional>
#include <PubSubClient.h>
#include <Client.h>
#include "mqtt_outbox.h"

// Wraps knolleary/PubSubClient - a real, existing, widely-used MQTT
// library, referenced in platformio.ini's lib_deps via its GitHub URL
// (not PlatformIO's own package registry, which is network-blocked in
// this environment - see CLAUDE.md). Adds reconnect-with-backoff and a
// small store-and-forward ring buffer for when the broker is unreachable
// (docs/roadmap.md Firmware roadmap, F3), plus (added 2026-09-24, for
// F4's OTA-trigger flow) a single-topic subscribe()/onMessage() -
// PubSubClient supports subscriptions and a receive callback natively
// (confirmed against its own real header, not assumed), this wrapper
// just adds the same reconnect-survives-it handling publish() already
// gets: PubSubClient's connect() establishes a clean MQTT session every
// call, so a subscription does NOT survive a disconnect/reconnect on
// its own unless something re-subscribes after every successful
// (re)connect - see tryReconnect() in the .cpp.
//
// Takes a Client& (WiFiClient / WiFiClientSecure / EthernetClient -
// whichever network interface is active) rather than owning one, so this
// class doesn't care whether the transport is WiFi, Ethernet, or TLS.
class MqttClientWrapper {
public:
    static const int OUTBOX_CAPACITY = 16;
    static const int MAX_PAYLOAD_LEN = 256;
    static const int MAX_TOPIC_LEN = 64;
    // PubSubClient's shared send/receive buffer (both directions use
    // the same one - confirmed against its real header, not the
    // 256-byte MQTT_MAX_PACKET_SIZE default). Sized for this project's
    // one real subscriber's worst case: the OTA-trigger command
    // payload (ota_trigger.h's url/sha256/sig/version fields plus JSON
    // punctuation and MQTT's own topic+header overhead) comfortably
    // fits under 550 bytes; 700 leaves real headroom rather than
    // cutting it close.
    static const uint16_t MQTT_RX_BUFFER_SIZE = 700;

    // Payload pointer is only valid for the duration of the callback -
    // it points into PubSubClient's own internal receive buffer,
    // reused on the next message. Copy out anything needed past the
    // callback's return (main.cpp's OTA-trigger handler does this).
    typedef std::function<void(const char *topic, const uint8_t *payload,
                                unsigned int length)> MessageCallback;

    MqttClientWrapper(Client &net_client, const char *server, uint16_t port,
                       const char *client_id);

    void begin();

    // Subscribes to `topic` - immediately if already connected,
    // otherwise remembered and (re-)subscribed automatically on every
    // successful (re)connect (see this class's own header comment).
    // Only ONE topic is supported right now - this project's only
    // current subscriber is main.cpp's OTA command topic; extend this
    // to a small fixed array if a second subscriber is ever needed,
    // the same way outbox_ below is a fixed array rather than
    // something dynamic. Returns false if topic is null/empty or
    // doesn't fit MAX_TOPIC_LEN.
    bool subscribe(const char *topic);

    // Registers the callback invoked for every incoming message on the
    // subscribed topic. Replaces any previously registered callback -
    // only one is supported, matching subscribe()'s one-topic limit.
    void onMessage(MessageCallback callback);

    // Publishes immediately if connected; otherwise queues into the
    // outbox (oldest entry dropped if full - best-effort store-and-
    // forward, not a guaranteed-delivery queue). Call loop() regularly to
    // drain the outbox once reconnected.
    void publish(const char *topic, const char *payload);

    // Call frequently (e.g. every main-loop iteration). Handles
    // reconnect-with-backoff and flushes the outbox once connected.
    void loop();

    bool isConnected();

private:
    PubSubClient client_;
    const char *client_id_;
    MqttOutbox outbox_;
    unsigned long last_reconnect_attempt_ = 0;
    unsigned long reconnect_backoff_ms_ = 1000;
    static const unsigned long RECONNECT_BACKOFF_MAX_MS = 30000;

    char subscribed_topic_[MAX_TOPIC_LEN] = "";
    bool has_subscription_ = false;
    MessageCallback message_callback_;

    void flushOutbox();
    void tryReconnect();
};
