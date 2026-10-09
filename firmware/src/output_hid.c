#include "output_hid.h"

#include <stdbool.h>
#include <string.h>

#include "keyremap_protocol.h"
#include "tusb.h"

enum {
  HID_ITF_KEYBOARD = 0,
  HID_ITF_MOUSE = 1,
  HID_ITF_CONSUMER = 2,
  HID_ITF_CONFIG = 3,
};

static uint8_t pressed_keys[6];
static uint8_t keyboard_modifiers;
static uint8_t mouse_buttons;
/* Movement accumulates here and is drained in int8 steps, so large deltas
 * (scaled or high-resolution input) are not truncated. */
static int32_t pending_mouse_x;
static int32_t pending_mouse_y;
static int32_t pending_mouse_wheel;
static uint16_t pending_consumer;
static bool keyboard_pending;
static bool mouse_pending;
static bool consumer_pending;
static bool consumer_release_pending;

static void keyboard_set_key(uint8_t keycode, bool pressed) {
  if (keycode >= 0xe0 && keycode <= 0xe7) {
    uint8_t mask = (uint8_t)(1u << (keycode - 0xe0));
    if (pressed) {
      keyboard_modifiers |= mask;
    } else {
      keyboard_modifiers &= (uint8_t)~mask;
    }
    return;
  }

  for (uint8_t i = 0; i < sizeof(pressed_keys); i++) {
    if (pressed_keys[i] == keycode) {
      if (!pressed) {
        pressed_keys[i] = 0;
      }
      return;
    }
  }

  if (!pressed) {
    return;
  }

  for (uint8_t i = 0; i < sizeof(pressed_keys); i++) {
    if (pressed_keys[i] == 0) {
      pressed_keys[i] = keycode;
      return;
    }
  }
}

static uint8_t keyboard_modifier_mask(void) {
  return keyboard_modifiers;
}

static bool send_keyboard_report(void) {
  if (!tud_hid_n_ready(HID_ITF_KEYBOARD)) {
    return false;
  }

  return tud_hid_n_keyboard_report(HID_ITF_KEYBOARD, 0, keyboard_modifier_mask(), pressed_keys);
}

static bool send_mouse_report(int8_t x, int8_t y, int8_t wheel) {
  if (!tud_hid_n_ready(HID_ITF_MOUSE)) {
    return false;
  }

  return tud_hid_n_mouse_report(HID_ITF_MOUSE, 0, mouse_buttons, x, y, wheel, 0);
}

static bool send_consumer_report(uint16_t usage) {
  if (!tud_hid_n_ready(HID_ITF_CONSUMER)) {
    return false;
  }

  return tud_hid_n_report(HID_ITF_CONSUMER, 0, &usage, sizeof(usage));
}

static void queue_keyboard_report(void) {
  keyboard_pending = true;
}

static int8_t take_step(int32_t *pending) {
  int32_t step = *pending;
  if (step > 127) {
    step = 127;
  } else if (step < -127) {
    step = -127;
  }
  *pending -= step;
  return (int8_t)step;
}

static void queue_mouse_report(int32_t x, int32_t y, int32_t wheel) {
  pending_mouse_x += x;
  pending_mouse_y += y;
  pending_mouse_wheel += wheel;
  mouse_pending = true;
}

void output_hid_init(void) {
  memset(pressed_keys, 0, sizeof(pressed_keys));
  keyboard_modifiers = 0;
  mouse_buttons = 0;
  pending_mouse_x = 0;
  pending_mouse_y = 0;
  pending_mouse_wheel = 0;
  pending_consumer = 0;
  keyboard_pending = false;
  mouse_pending = false;
  consumer_pending = false;
  consumer_release_pending = false;
}

void output_hid_task(void) {
  if (keyboard_pending && send_keyboard_report()) {
    keyboard_pending = false;
  }

  if (mouse_pending && tud_hid_n_ready(HID_ITF_MOUSE)) {
    int8_t x = take_step(&pending_mouse_x);
    int8_t y = take_step(&pending_mouse_y);
    int8_t wheel = take_step(&pending_mouse_wheel);
    if (send_mouse_report(x, y, wheel)) {
      mouse_pending = pending_mouse_x || pending_mouse_y || pending_mouse_wheel;
    } else {
      pending_mouse_x += x;
      pending_mouse_y += y;
      pending_mouse_wheel += wheel;
    }
  }

  if (consumer_pending && send_consumer_report(pending_consumer)) {
    consumer_pending = false;
    consumer_release_pending = true;
  } else if (consumer_release_pending && send_consumer_report(0)) {
    consumer_release_pending = false;
  }
}

void output_hid_release_all(void) {
  memset(pressed_keys, 0, sizeof(pressed_keys));
  keyboard_modifiers = 0;
  mouse_buttons = 0;
  pending_mouse_x = 0;
  pending_mouse_y = 0;
  pending_mouse_wheel = 0;
  pending_consumer = 0;
  queue_keyboard_report();
  queue_mouse_report(0, 0, 0);
  consumer_pending = true;
  consumer_release_pending = false;
}

void output_hid_apply(const output_event_t *event) {
  switch (event->kind) {
    case OUTPUT_KIND_KEY:
      keyboard_set_key(event->code, event->value != 0);
      queue_keyboard_report();
      break;
    case OUTPUT_KIND_MOUSE_BUTTON:
      if (event->code >= 1 && event->code <= 8) {
        uint8_t mask = (uint8_t)(1u << (event->code - 1));
        if (event->value) {
          mouse_buttons |= mask;
        } else {
          mouse_buttons &= (uint8_t)~mask;
        }
      }
      queue_mouse_report(0, 0, 0);
      break;
    case OUTPUT_KIND_REL_X:
      queue_mouse_report(event->value, 0, 0);
      break;
    case OUTPUT_KIND_REL_Y:
      queue_mouse_report(0, event->value, 0);
      break;
    case OUTPUT_KIND_WHEEL:
      queue_mouse_report(0, 0, event->value);
      break;
    case OUTPUT_KIND_CONSUMER:
      if (event->value) {
        pending_consumer = event->code;
        consumer_pending = true;
        consumer_release_pending = false;
      }
      break;
    default:
      break;
  }
}
