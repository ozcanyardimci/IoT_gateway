#pragma once
#include <stddef.h>

struct MqttOutboxItem {
    char topic[96];
    char payload[256];
    bool used;
};

class MqttOutbox {
public:
    static const int CAPACITY = 10;
    
    MqttOutbox();
    
    // Enqueues a message, dropping the oldest if full.
    void enqueue(const char *topic, const char *payload);
    
    // Gets the item at the specified index.
    MqttOutboxItem* getItem(int index);
    
    // compacts the array, removing items that have been marked as used = false
    void compact();

private:
    MqttOutboxItem items_[CAPACITY];
};
