#pragma once

#include "mapper.h"

void host_input_init(void);
void host_input_task(void);
void host_input_simulate(const input_event_t *event);
uint8_t host_input_status(void);
uint16_t host_input_vid(void);
uint16_t host_input_pid(void);
const input_event_t *host_input_last_event(void);
uint32_t host_input_event_count(void);
