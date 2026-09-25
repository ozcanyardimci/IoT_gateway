#include "mqtt_client_wrapper.h"
#include <Arduino.h>
#include <string.h>

MqttClientWrapper::MqttClientWrapper(Client &net_client, const char *server, uint16_t port,
                                      const char *client_id)
    : client_(net_client), client_id_(client_id) {
    client_.setServer(server, port);
    client_.setBufferSize(MQTT_RX_BUFFER_SIZE);
    client_.setCallback([this](char *topic, uint8_t *payload, unsigned int length) {
        if (message_callback_) {
            message_callback_(topic, payload, length);
        }
    });
}

bool MqttClientWrapper::subscribe(const char *topic) {
    if (topic == nullptr || topic[0] == '\0') return false;
    if (strlen(topic) >= sizeof(subscribed_topic_)) return false;

    strncpy(subscribed_topic_, topic, sizeof(subscribed_topic_) - 1);
    subscribed_topic_[sizeof(subscribed_topic_) - 1] = '\0';
    has_subscription_ = true;

    if (isConnected()) {
        return client_.subscribe(subscribed_topic_);
    }
    return true; // remembered - actual subscribe happens on the next
                 // successful (re)connect, see tryReconnect() below
}

void MqttClientWrapper::onMessage(MessageCallback callback) {
    message_callback_ = callback;
}

void MqttClientWrapper::begin() {
    // outbox constructor initializes itself
}

bool MqttClientWrapper::isConnected() {
    return client_.connected();
}



void MqttClientWrapper::publish(const char *topic, const char *payload) {
    if (topic == nullptr || payload == nullptr) return;
    if (isConnected() && client_.publish(topic, payload)) {
        return;
    }
    // Either not connected, or publish() failed anyway (e.g. payload too
    // large for PubSubClient's internal buffer) - queue instead of
    // silently dropping it.
    outbox_.enqueue(topic, payload);
}

void MqttClientWrapper::flushOutbox() {
    bool failed = false;
    for (int i = 0; i < MqttOutbox::CAPACITY; i++) {
        MqttOutboxItem* item = outbox_.getItem(i);
        if (item == nullptr || !item->used) break;

        if (!failed) {
            if (client_.publish(item->topic, item->payload)) {
                item->used = false;
            } else {
                failed = true;
            }
        }
    }
    outbox_.compact();
}

void MqttClientWrapper::tryReconnect() {
    unsigned long now = millis();
    if (now - last_reconnect_attempt_ < reconnect_backoff_ms_) return;
    last_reconnect_attempt_ = now;

    if (client_.connect(client_id_)) {
        reconnect_backoff_ms_ = 1000; // reset backoff on success
        if (has_subscription_) {
            // PubSubClient's connect() above just established a fresh
            // (clean) MQTT session - the broker does not remember any
            // prior subscription from before this reconnect, so it has
            // to be redone every time, not just the first time. See
            // this class's own header comment for why.
            client_.subscribe(subscribed_topic_);
        }
        flushOutbox();
    } else {
        reconnect_backoff_ms_ = reconnect_backoff_ms_ * 2;
        if (reconnect_backoff_ms_ > RECONNECT_BACKOFF_MAX_MS) {
            reconnect_backoff_ms_ = RECONNECT_BACKOFF_MAX_MS;
        }
    }
}

void MqttClientWrapper::loop() {
    client_.loop();
    if (!isConnected()) {
        tryReconnect();
    } else {
        // No "anything queued?" shortcut here on purpose: enqueue() fills
        // the first free slot (not always slot 0) and flushOutbox() stops
        // at the first send failure without compacting, so a single-slot
        // check (e.g. outbox_[0].used) can miss a message stuck in a later
        // slot and leave it stalled until some unrelated publish() happens
        // to touch slot 0 again. flushOutbox() is cheap even when the
        // outbox is empty (just a scan of unset flags), so just call it.
        flushOutbox();
    }
}
