#include "io_state.h"
#include <string.h>

#if IO_STATE_DI_COUNT != MQTT_PAYLOAD_DI_COUNT || \
    IO_STATE_RELAY_COUNT != MQTT_PAYLOAD_RELAY_COUNT || \
    IO_STATE_AI_COUNT != MQTT_PAYLOAD_AI_COUNT
#error "io_state and mqtt_payload channel counts must match"
#endif

void io_state_init(io_state_t *s) {
    if (s == NULL) return;
    memset(s, 0, sizeof(*s));
}

int io_state_set_di(io_state_t *s, int index, int value) {
    if (s == NULL || index < 0 || index >= IO_STATE_DI_COUNT) return -1;
    s->di[index] = value ? 1 : 0;
    return 0;
}

int io_state_get_di(const io_state_t *s, int index, uint8_t *out_value) {
    if (s == NULL || out_value == NULL || index < 0 || index >= IO_STATE_DI_COUNT) return -1;
    *out_value = s->di[index];
    return 0;
}

int io_state_set_relay(io_state_t *s, int index, int value) {
    if (s == NULL || index < 0 || index >= IO_STATE_RELAY_COUNT) return -1;
    s->relay[index] = value ? 1 : 0;
    return 0;
}

int io_state_get_relay(const io_state_t *s, int index, uint8_t *out_value) {
    if (s == NULL || out_value == NULL || index < 0 || index >= IO_STATE_RELAY_COUNT) return -1;
    *out_value = s->relay[index];
    return 0;
}

int io_state_set_ai(io_state_t *s, int index, uint16_t raw_value) {
    if (s == NULL || index < 0 || index >= IO_STATE_AI_COUNT) return -1;
    s->ai[index] = raw_value;
    return 0;
}

int io_state_get_ai(const io_state_t *s, int index, uint16_t *out_value) {
    if (s == NULL || out_value == NULL || index < 0 || index >= IO_STATE_AI_COUNT) return -1;
    *out_value = s->ai[index];
    return 0;
}

void io_state_set_ao(io_state_t *s, uint16_t raw_value) {
    if (s == NULL) return;
    s->ao = raw_value;
}

uint16_t io_state_get_ao(const io_state_t *s) {
    if (s == NULL) return 0;
    return s->ao;
}

void io_state_to_mqtt_state(const io_state_t *s, uint32_t unix_timestamp, mqtt_gateway_state_t *out) {
    if (s == NULL || out == NULL) return;
    out->unix_timestamp = unix_timestamp;
    memcpy(out->di, s->di, sizeof(out->di));
    memcpy(out->relay, s->relay, sizeof(out->relay));
    memcpy(out->ai, s->ai, sizeof(out->ai));
    out->ao = s->ao;
}
