#include <stdint.h>

#include "bsp/board_api.h"
#include "pico/stdlib.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"
#include "status_led.h"
#include "tusb.h"

#define TARGET_VID 0x1c4f
#define TARGET_PID 0x007c

typedef enum {
  PROBE_NO_LINE = 0,
  PROBE_LINE_ONLY,
  PROBE_USB_MOUNTED,
  PROBE_TARGET_HID,
  PROBE_ERROR,
} probe_state_t;

static probe_state_t probe_state = PROBE_NO_LINE;
static uint32_t last_tick_ms;
static uint8_t blink_phase;

static void set_probe_state(probe_state_t state) {
  if (probe_state == state) {
    return;
  }

  probe_state = state;
  blink_phase = 0;
  last_tick_ms = to_ms_since_boot(get_absolute_time());
}

static void show_boot_marker(void) {
  status_led_set_rgb(0, 0, 32);
  sleep_ms(160);
  status_led_set_rgb(0, 0, 0);
  sleep_ms(160);
  status_led_set_rgb(0, 32, 0);
  sleep_ms(160);
  status_led_set_rgb(0, 0, 0);
  sleep_ms(160);
}

static void show_repeating_blinks(uint8_t count, uint8_t red, uint8_t green, uint8_t blue) {
  uint32_t now = to_ms_since_boot(get_absolute_time());
  if (now - last_tick_ms < 180) {
    return;
  }

  last_tick_ms = now;
  blink_phase = (uint8_t)((blink_phase + 1) % ((count * 2) + 4));
  if (blink_phase < count * 2 && (blink_phase % 2) == 0) {
    status_led_set_rgb(red, green, blue);
  } else {
    status_led_set_rgb(0, 0, 0);
  }
}

static void update_line_state(void) {
  if (probe_state == PROBE_USB_MOUNTED || probe_state == PROBE_TARGET_HID || probe_state == PROBE_ERROR) {
    return;
  }

  root_port_t *root = PIO_USB_ROOT_PORT(0);
  port_pin_status_t line_state = pio_usb_bus_get_line_state(root);
  set_probe_state(line_state == PORT_PIN_SE0 ? PROBE_NO_LINE : PROBE_LINE_ONLY);
}

static void led_task(void) {
  switch (probe_state) {
    case PROBE_NO_LINE:
      show_repeating_blinks(1, 0, 0, 28);
      break;
    case PROBE_LINE_ONLY:
      show_repeating_blinks(2, 0, 28, 0);
      break;
    case PROBE_USB_MOUNTED:
      show_repeating_blinks(3, 24, 12, 0);
      break;
    case PROBE_TARGET_HID:
      status_led_set_rgb(0, 32, 0);
      break;
    case PROBE_ERROR:
    default:
      show_repeating_blinks(4, 32, 0, 0);
      break;
  }
}

int main(void) {
  board_init();
  status_led_init();
  show_boot_marker();

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = 2;
  pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
  tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

  tusb_rhport_init_t host_init = {
    .role = TUSB_ROLE_HOST,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(BOARD_TUH_RHPORT, &host_init);

  while (true) {
    tuh_task_ext(0, false);
    update_line_state();
    led_task();
  }
}

void tuh_mount_cb(uint8_t dev_addr) {
  uint16_t vid = 0;
  uint16_t pid = 0;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  if (probe_state != PROBE_TARGET_HID) {
    set_probe_state(PROBE_USB_MOUNTED);
  }
}

void tuh_umount_cb(uint8_t dev_addr) {
  (void)dev_addr;
  set_probe_state(PROBE_NO_LINE);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  (void)instance;
  (void)desc_report;
  (void)desc_len;

  uint16_t vid = 0;
  uint16_t pid = 0;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  if (vid == TARGET_VID && pid == TARGET_PID) {
    set_probe_state(PROBE_TARGET_HID);
  } else {
    set_probe_state(PROBE_USB_MOUNTED);
  }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  (void)dev_addr;
  (void)instance;
  set_probe_state(PROBE_NO_LINE);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
  (void)report;
  (void)len;
  (void)tuh_hid_receive_report(dev_addr, instance);
}
