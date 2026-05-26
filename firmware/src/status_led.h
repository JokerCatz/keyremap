#pragma once

#include <stdint.h>

void status_led_init(void);
void status_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);
void status_led_set_layer(uint8_t layer);
void status_led_set_host_waiting(void);
void status_led_set_host_ok(void);
void status_led_set_host_other(void);
void status_led_set_host_error(void);
void status_led_task(void);
