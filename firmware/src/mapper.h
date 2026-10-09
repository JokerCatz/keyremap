#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t kind;
  uint8_t code;
  int16_t value;
} input_event_t;

typedef struct {
  uint8_t kind;
  uint8_t code;
  int16_t value;
} output_event_t;

typedef struct {
  uint32_t seq;
  input_event_t input;
  output_event_t output;
  uint8_t layer;
  bool simulated;
} mapper_log_entry_t;

void mapper_init(void);

/* Resolves an input through the active layer (falling back to the base layer,
 * then to identity passthrough), runs layer actions and emits HID output. */
void mapper_handle_input(const input_event_t *input, bool simulated);
void mapper_release_all(void);

uint8_t mapper_active_layer(void);
bool mapper_set_base_layer(uint8_t layer);

const mapper_log_entry_t *mapper_log_next(uint32_t after, bool *dropped);
uint32_t mapper_log_latest_seq(void);
