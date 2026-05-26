#include "status_led.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

#define STATUS_LED_PIN 16
#define STATUS_LED_PIO pio1
#define STATUS_LED_SM 0

static bool initialized;
static bool host_waiting;
static uint32_t host_waiting_last_ms;
static bool host_waiting_on;

static inline uint32_t grb_word(uint8_t red, uint8_t green, uint8_t blue) {
  return ((uint32_t)green << 24) | ((uint32_t)red << 16) | ((uint32_t)blue << 8);
}

void status_led_init(void) {
  uint offset = pio_add_program(STATUS_LED_PIO, &ws2812_program);
  ws2812_program_init(STATUS_LED_PIO, STATUS_LED_SM, offset, STATUS_LED_PIN, 800000, false);
  initialized = true;
  status_led_set_rgb(0, 0, 16);
}

void status_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
  if (!initialized) {
    return;
  }
  pio_sm_put_blocking(STATUS_LED_PIO, STATUS_LED_SM, grb_word(red, green, blue));
}

void status_led_set_host_waiting(void) {
  host_waiting = true;
  host_waiting_last_ms = to_ms_since_boot(get_absolute_time());
  host_waiting_on = true;
  status_led_set_rgb(0, 32, 0);
}

void status_led_set_host_ok(void) {
  host_waiting = false;
  status_led_set_rgb(0, 32, 0);
}

void status_led_set_host_other(void) {
  host_waiting = false;
  status_led_set_rgb(28, 12, 0);
}

void status_led_set_host_error(void) {
  host_waiting = false;
  status_led_set_rgb(32, 0, 0);
}

void status_led_set_layer(uint8_t layer) {
  host_waiting = false;
  switch (layer) {
    case 0:
      status_led_set_rgb(0, 32, 0);
      break;
    case 1:
      status_led_set_rgb(18, 0, 28);
      break;
    case 2:
      status_led_set_rgb(0, 24, 24);
      break;
    case 3:
      status_led_set_rgb(24, 12, 0);
      break;
    default:
      status_led_set_rgb(16, 0, 0);
      break;
  }
}

void status_led_task(void) {
  if (!host_waiting) {
    return;
  }

  uint32_t now = to_ms_since_boot(get_absolute_time());
  if (now - host_waiting_last_ms < 500) {
    return;
  }

  host_waiting_last_ms = now;
  host_waiting_on = !host_waiting_on;
  if (host_waiting_on) {
    status_led_set_rgb(0, 32, 0);
  } else {
    status_led_set_rgb(0, 0, 0);
  }
}
