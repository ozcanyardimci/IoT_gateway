#pragma once
#include <stdint.h>
#include "mqtt_payload.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IO_STATE_DI_COUNT    8
#define IO_STATE_RELAY_COUNT 4
#define IO_STATE_AI_COUNT    2

// Canonical in-memory model of this gateway's own onboard I/O. Drivers
// (F2) write into it, mqtt_payload/rule-engine (F1/F3) read from it - it's
// the shared state both sides work through instead of talking directly.
typedef struct {
    uint8_t  di[IO_STATE_DI_COUNT];
    uint8_t  relay[IO_STATE_RELAY_COUNT];
    uint16_t ai[IO_STATE_AI_COUNT];
    uint16_t ao;
} io_state_t;

void io_state_init(io_state_t *s);

// All setters/getters are bounds-checked: return 0 on success, -1 on an
// out-of-range index (NULL state also returns -1).
int io_state_set_di(io_state_t *s, int index, int value);
int io_state_get_di(const io_state_t *s, int index, uint8_t *out_value);
int io_state_set_relay(io_state_t *s, int index, int value);
int io_state_get_relay(const io_state_t *s, int index, uint8_t *out_value);
int io_state_set_ai(io_state_t *s, int index, uint16_t raw_value);
int io_state_get_ai(const io_state_t *s, int index, uint16_t *out_value);
void io_state_set_ao(io_state_t *s, uint16_t raw_value);
uint16_t io_state_get_ao(const io_state_t *s);

// Copies io_state into an mqtt_gateway_state_t, attaching the given
// timestamp (channel counts match by construction - see the #error check
// in io_state.c).
void io_state_to_mqtt_state(const io_state_t *s, uint32_t unix_timestamp, mqtt_gateway_state_t *out);

#ifdef __cplusplus
}
#endif
