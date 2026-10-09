#pragma once

#include "mapper.h"

void host_input_init(void);
void host_input_task(void);
void host_input_simulate(const input_event_t *event);
uint8_t host_input_status(void);
uint16_t host_input_vid(void);
uint16_t host_input_pid(void);
uint8_t host_input_layout_flags(uint8_t instance);
