#include "host_input.h"

#include <string.h>

#include "config.h"
#include "keyremap_protocol.h"
#include "output_hid.h"
#include "pico/stdlib.h"
#include "pio_usb_ll.h"
#include "status_led.h"
#include "tusb.h"
#include "host/hcd.h"

#define TARGET_VID 0x1c4f
#define TARGET_PID 0x007c

static hid_keyboard_report_t previous_keyboard_report;
static uint8_t previous_mouse_buttons;
static uint8_t host_status;
static uint16_t mounted_vid;
static uint16_t mounted_pid;
static uint32_t last_attach_kick_ms;
static uint32_t last_reconnect_kick_ms;
static input_event_t last_input_event;
static uint32_t input_event_count;

static void dispatch_event(uint8_t kind, uint8_t code, int16_t value) {
  input_event_t input = {
    .kind = kind,
    .code = code,
    .value = value,
  };
  last_input_event = input;
  input_event_count++;
  host_input_simulate(&input);
}

static bool keyboard_report_has_key(const hid_keyboard_report_t *report, uint8_t keycode) {
  for (uint8_t i = 0; i < 6; i++) {
    if (report->keycode[i] == keycode) {
      return true;
    }
  }

  return false;
}

static void process_modifier(uint8_t old_mod, uint8_t new_mod, uint8_t mask, uint8_t keycode) {
  if ((old_mod & mask) != (new_mod & mask)) {
    dispatch_event(INPUT_KIND_KEY, keycode, (new_mod & mask) ? 1 : 0);
  }
}

static void process_keyboard_report(const hid_keyboard_report_t *report) {
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_LEFTCTRL, 0xe0);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_LEFTSHIFT, 0xe1);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_LEFTALT, 0xe2);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_LEFTGUI, 0xe3);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_RIGHTCTRL, 0xe4);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_RIGHTSHIFT, 0xe5);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_RIGHTALT, 0xe6);
  process_modifier(previous_keyboard_report.modifier, report->modifier, KEYBOARD_MODIFIER_RIGHTGUI, 0xe7);

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t keycode = report->keycode[i];
    if (keycode && !keyboard_report_has_key(&previous_keyboard_report, keycode)) {
      dispatch_event(INPUT_KIND_KEY, keycode, 1);
    }
  }

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t keycode = previous_keyboard_report.keycode[i];
    if (keycode && !keyboard_report_has_key(report, keycode)) {
      dispatch_event(INPUT_KIND_KEY, keycode, 0);
    }
  }

  previous_keyboard_report = *report;
}

static void process_mouse_report(const hid_mouse_report_t *report) {
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t mask = (uint8_t)(1u << i);
    if ((previous_mouse_buttons & mask) != (report->buttons & mask)) {
      dispatch_event(INPUT_KIND_MOUSE_BUTTON, (uint8_t)(i + 1), (report->buttons & mask) ? 1 : 0);
    }
  }

  if (report->x) {
    dispatch_event(INPUT_KIND_REL_X, 0, report->x);
  }
  if (report->y) {
    dispatch_event(INPUT_KIND_REL_Y, 0, report->y);
  }
  if (report->wheel) {
    dispatch_event(INPUT_KIND_WHEEL, 0, report->wheel);
  }

  previous_mouse_buttons = report->buttons;
}

void host_input_init(void) {
  memset(&previous_keyboard_report, 0, sizeof(previous_keyboard_report));
  previous_mouse_buttons = 0;
  host_status = 0;
  mounted_vid = 0;
  mounted_pid = 0;
  last_attach_kick_ms = 0;
  last_reconnect_kick_ms = 0;
  memset(&last_input_event, 0, sizeof(last_input_event));
  input_event_count = 0;
}

void host_input_task(void) {
  if (host_status != 0) {
    return;
  }

  root_port_t *root = PIO_USB_ROOT_PORT(0);
  if (!root->connected || pio_usb_bus_get_line_state(root) == PORT_PIN_SE0) {
    return;
  }

  uint32_t now = to_ms_since_boot(get_absolute_time());
  if (now - last_attach_kick_ms < 1500) {
    return;
  }

  last_attach_kick_ms = now;
  hcd_event_device_attach(BOARD_TUH_RHPORT, false);

  if (now - last_reconnect_kick_ms >= 3000) {
    last_reconnect_kick_ms = now;
    hcd_event_device_remove(BOARD_TUH_RHPORT, false);
    root->connected = false;
    root->suspended = false;
  }
}

void host_input_simulate(const input_event_t *event) {
  output_event_t output;

  if (mapper_process(event, &output)) {
    if (output.kind == OUTPUT_KIND_LAYER) {
      if (event->value && config_set_active_layer(output.code)) {
        status_led_set_layer(output.code);
      }
      output_hid_apply(&output);
      return;
    }

    if (output.kind == OUTPUT_KIND_NEXT_LAYER) {
      if (event->value) {
        uint8_t layer = (uint8_t)((config_active_layer() + 1) % KEYREMAP_LAYER_COUNT);
        if (config_set_active_layer(layer)) {
          status_led_set_layer(layer);
        }
      }
      output_hid_apply(&output);
      return;
    }

    output_hid_apply(&output);
  }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  (void)desc_report;
  (void)desc_len;

  uint16_t vid;
  uint16_t pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  mounted_vid = vid;
  mounted_pid = pid;

  if (vid == TARGET_VID && pid == TARGET_PID) {
    host_status = 1;
    status_led_set_host_ok();
  } else {
    host_status = 2;
    status_led_set_host_other();
  }

  if (!tuh_hid_receive_report(dev_addr, instance)) {
    status_led_set_host_error();
  }
}

void tuh_mount_cb(uint8_t dev_addr) {
  uint16_t vid;
  uint16_t pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  mounted_vid = vid;
  mounted_pid = pid;

  if (host_status == 0) {
    host_status = 5;
    status_led_set_host_other();
  }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  (void)dev_addr;
  (void)instance;
  output_hid_release_all();
  host_status = 3;
  mounted_vid = 0;
  mounted_pid = 0;
  status_led_set_host_error();
}

void tuh_umount_cb(uint8_t dev_addr) {
  (void)dev_addr;
  output_hid_release_all();
  host_status = 3;
  mounted_vid = 0;
  mounted_pid = 0;
  status_led_set_host_error();
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
  uint8_t protocol = tuh_hid_interface_protocol(dev_addr, instance);

  if (protocol == HID_ITF_PROTOCOL_KEYBOARD && len >= sizeof(hid_keyboard_report_t)) {
    process_keyboard_report((const hid_keyboard_report_t *)report);
  } else if (protocol == HID_ITF_PROTOCOL_MOUSE && len >= sizeof(hid_mouse_report_t)) {
    process_mouse_report((const hid_mouse_report_t *)report);
  }

  if (!tuh_hid_receive_report(dev_addr, instance)) {
    host_status = 4;
    status_led_set_host_error();
  }
}

uint8_t host_input_status(void) {
  return host_status;
}

uint16_t host_input_vid(void) {
  return mounted_vid;
}

uint16_t host_input_pid(void) {
  return mounted_pid;
}

const input_event_t *host_input_last_event(void) {
  return &last_input_event;
}

uint32_t host_input_event_count(void) {
  return input_event_count;
}
