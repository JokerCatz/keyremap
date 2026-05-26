#pragma once

#include <stdint.h>

#include "mapper.h"

typedef struct {
  uint8_t kind;
  uint8_t code;
  int16_t value;
  uint32_t count;
} output_state_t;

void output_hid_init(void);
void output_hid_task(void);
void output_hid_release_all(void);
void output_hid_apply(const output_event_t *event);
const output_state_t *output_hid_state(void);
