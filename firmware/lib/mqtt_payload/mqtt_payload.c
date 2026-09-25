#include "mqtt_payload.h"
#include <stdio.h>

int mqtt_payload_build_json(const mqtt_gateway_state_t *state, char *out, size_t out_size) {
    if (state == NULL || out == NULL || out_size == 0) return -1;

    size_t pos = 0;
    int n;

    n = snprintf(out + pos, out_size - pos, "{\"ts\":%u,\"di\":[", (unsigned)state->unix_timestamp);
    if (n < 0 || (size_t)n >= out_size - pos) return -1;
    pos += (size_t)n;

    for (int i = 0; i < MQTT_PAYLOAD_DI_COUNT; i++) {
        n = snprintf(out + pos, out_size - pos, "%s%u", (i == 0) ? "" : ",", (unsigned)(state->di[i] ? 1 : 0));
        if (n < 0 || (size_t)n >= out_size - pos) return -1;
        pos += (size_t)n;
    }

    n = snprintf(out + pos, out_size - pos, "],\"relay\":[");
    if (n < 0 || (size_t)n >= out_size - pos) return -1;
    pos += (size_t)n;

    for (int i = 0; i < MQTT_PAYLOAD_RELAY_COUNT; i++) {
        n = snprintf(out + pos, out_size - pos, "%s%u", (i == 0) ? "" : ",", (unsigned)(state->relay[i] ? 1 : 0));
        if (n < 0 || (size_t)n >= out_size - pos) return -1;
        pos += (size_t)n;
    }

    n = snprintf(out + pos, out_size - pos, "],\"ai\":[");
    if (n < 0 || (size_t)n >= out_size - pos) return -1;
    pos += (size_t)n;

    for (int i = 0; i < MQTT_PAYLOAD_AI_COUNT; i++) {
        n = snprintf(out + pos, out_size - pos, "%s%u", (i == 0) ? "" : ",", (unsigned)state->ai[i]);
        if (n < 0 || (size_t)n >= out_size - pos) return -1;
        pos += (size_t)n;
    }

    n = snprintf(out + pos, out_size - pos, "],\"ao\":%u}", (unsigned)state->ao);
    if (n < 0 || (size_t)n >= out_size - pos) return -1;
    pos += (size_t)n;

    return (int)pos;
}
