#include "macro.h"

#include <stddef.h>

#include "keyremap_protocol.h"
#include "output_hid.h"
#include "pico/time.h"

#define STEP_GAP_MS 8
#define KEY_LEFT_SHIFT 0xe1

static const char *text;
static bool key_down;
static uint8_t down_code;
static bool down_shift;
static uint32_t last_step_ms;

/* US layout: returns the HID usage for an ASCII character, or 0. */
static uint8_t ascii_to_usage(char c, bool *shift) {
  *shift = false;
  if (c >= 'a' && c <= 'z') {
    return (uint8_t)(0x04 + (c - 'a'));
  }
  if (c >= 'A' && c <= 'Z') {
    *shift = true;
    return (uint8_t)(0x04 + (c - 'A'));
  }
  if (c >= '1' && c <= '9') {
    return (uint8_t)(0x1e + (c - '1'));
  }
  switch (c) {
    case '0': return 0x27;
    case '.': return 0x37;
    case '/': return 0x38;
    case '-': return 0x2d;
    case '=': return 0x2e;
    case ':': *shift = true; return 0x33;
    case '_': *shift = true; return 0x2d;
    case '?': *shift = true; return 0x38;
    case '#': *shift = true; return 0x20;
    case '&': *shift = true; return 0x24;
    default: return 0;
  }
}

static void send_key(uint8_t code, int16_t value) {
  output_event_t event = {.kind = OUTPUT_KIND_KEY, .code = code, .value = value};
  output_hid_apply(&event);
}

void macro_init(void) {
  text = NULL;
  key_down = false;
}

void macro_type(const char *new_text) {
  text = new_text;
  key_down = false;
}

bool macro_busy(void) {
  return text != NULL;
}

void macro_task(void) {
  if (!text || output_hid_keyboard_pending()) {
    return;
  }

  uint32_t now = to_ms_since_boot(get_absolute_time());
  if (now - last_step_ms < STEP_GAP_MS) {
    return;
  }
  last_step_ms = now;

  if (key_down) {
    send_key(down_code, 0);
    if (down_shift) {
      send_key(KEY_LEFT_SHIFT, 0);
    }
    key_down = false;
    text++;
    return;
  }

  while (*text) {
    bool shift;
    uint8_t code = ascii_to_usage(*text, &shift);
    if (code) {
      if (shift) {
        send_key(KEY_LEFT_SHIFT, 1);
      }
      send_key(code, 1);
      key_down = true;
      down_code = code;
      down_shift = shift;
      return;
    }
    text++;
  }

  text = NULL;
}
