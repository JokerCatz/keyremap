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

void mapper_init(void);
bool mapper_process(const input_event_t *input, output_event_t *output);
