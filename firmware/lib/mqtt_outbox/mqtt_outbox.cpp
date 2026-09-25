#include "mqtt_outbox.h"
#include <string.h>

MqttOutbox::MqttOutbox() {
    for (int i = 0; i < CAPACITY; i++) {
        items_[i].used = false;
    }
}

void MqttOutbox::enqueue(const char *topic, const char *payload) {
    if (topic == nullptr || payload == nullptr) return;
    int slot = -1;
    for (int i = 0; i < CAPACITY; i++) {
        if (!items_[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        for (int i = 1; i < CAPACITY; i++) {
            items_[i - 1] = items_[i];
        }
        slot = CAPACITY - 1;
    }
    strncpy(items_[slot].topic, topic, sizeof(items_[slot].topic) - 1);
    items_[slot].topic[sizeof(items_[slot].topic) - 1] = '\0';
    strncpy(items_[slot].payload, payload, sizeof(items_[slot].payload) - 1);
    items_[slot].payload[sizeof(items_[slot].payload) - 1] = '\0';
    items_[slot].used = true;
}

MqttOutboxItem* MqttOutbox::getItem(int index) {
    if (index < 0 || index >= CAPACITY) return nullptr;
    return &items_[index];
}

void MqttOutbox::compact() {
    int keep_idx = 0;
    for (int i = 0; i < CAPACITY; i++) {
        if (!items_[i].used) continue;
        if (i != keep_idx) {
            items_[keep_idx] = items_[i];
            items_[i].used = false;
        }
        keep_idx++;
    }
}
