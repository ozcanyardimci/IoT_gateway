#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_PAYLOAD_DI_COUNT    8
#define MQTT_PAYLOAD_RELAY_COUNT 4
#define MQTT_PAYLOAD_AI_COUNT    2

// Raw values only - unit conversion (e.g. ADC counts to volts/mA) is a
// mapping-layer concern, not this module's.
typedef struct {
    uint32_t unix_timestamp;
    uint8_t  di[MQTT_PAYLOAD_DI_COUNT];       // 0 or 1
    uint8_t  relay[MQTT_PAYLOAD_RELAY_COUNT]; // 0 or 1
    uint16_t ai[MQTT_PAYLOAD_AI_COUNT];       // raw ADC counts
    uint16_t ao;                              // raw DAC counts
} mqtt_gateway_state_t;

// Serializes state into a compact JSON object:
// {"ts":<u32>,"di":[0,1,...],"relay":[0,1,...],"ai":[n,n],"ao":n}
// Returns bytes written (excluding the null terminator), or a negative value
// if out_size was too small. Always null-terminates on success. This is a
// starting schema, not a fixed spec - revise freely once real payload
// requirements (topic structure, field names) are settled.
int mqtt_payload_build_json(const mqtt_gateway_state_t *state, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
