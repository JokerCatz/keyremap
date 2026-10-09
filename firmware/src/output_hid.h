#pragma once

#include <stdint.h>

#include "mapper.h"

void output_hid_init(void);
void output_hid_task(void);
void output_hid_release_all(void);
void output_hid_apply(const output_event_t *event);
