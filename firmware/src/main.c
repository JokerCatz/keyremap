#include <string.h>

#include "bsp/board_api.h"
#include "config.h"
#include "host_input.h"
#include "keyremap_protocol.h"
#include "macro.h"
#include "mapper.h"
#include "output_hid.h"
#include "raw_capture.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "status_led.h"
#include "tusb.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"

static uint8_t last_red;
static uint8_t last_green;
static uint8_t last_blue = 16;

static void send_response(uint8_t command, uint8_t sequence, uint8_t status, const uint8_t *payload, uint8_t len) {
  uint8_t report[KEYREMAP_REPORT_SIZE] = {0};
  report[0] = command;
  report[1] = sequence;
  report[2] = len;
  report[3] = status;

  if (payload && len) {
    if (len > KEYREMAP_REPORT_SIZE - 4) {
      len = KEYREMAP_REPORT_SIZE - 4;
      report[2] = len;
    }
    memcpy(&report[4], payload, len);
  }

  if (tud_hid_n_ready(3)) {
    tud_hid_n_report(3, 0, report, sizeof(report));
  }
}

static void put_u16(uint8_t *dst, uint16_t value) {
  dst[0] = (uint8_t)(value & 0xff);
  dst[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *dst, uint32_t value) {
  for (uint8_t i = 0; i < 4; i++) {
    dst[i] = (uint8_t)(value >> (8 * i));
  }
}

static uint32_t get_u32(const uint8_t *src) {
  return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static void handle_get_raw_reports(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 4) {
    send_response(CMD_GET_RAW_REPORTS, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  uint8_t response[KEYREMAP_REPORT_SIZE - 4] = {0};
  uint8_t pos = 6;
  uint8_t count = 0;
  uint32_t after = get_u32(payload);

  while (true) {
    bool dropped;
    const raw_report_t *entry = raw_capture_next(after, &dropped);
    if (!entry || pos + 6 + entry->len > sizeof(response)) {
      break;
    }
    if (dropped) {
      response[1] |= 0x01;
    }

    put_u32(&response[pos], entry->seq);
    response[pos + 4] = (uint8_t)(entry->instance | (entry->truncated ? 0x80 : 0));
    response[pos + 5] = entry->len;
    memcpy(&response[pos + 6], entry->data, entry->len);
    pos = (uint8_t)(pos + 6 + entry->len);
    after = entry->seq;
    count++;
  }

  response[0] = count;
  put_u32(&response[2], raw_capture_latest_seq());
  send_response(CMD_GET_RAW_REPORTS, sequence, STATUS_OK, response, pos);
}

static void handle_get_hid_interfaces(uint8_t sequence) {
  uint8_t response[1 + RAW_CAPTURE_INSTANCE_COUNT * 6] = {0};
  response[0] = RAW_CAPTURE_INSTANCE_COUNT;

  for (uint8_t i = 0; i < RAW_CAPTURE_INSTANCE_COUNT; i++) {
    const raw_interface_t *itf = raw_capture_interface(i);
    uint8_t *dst = &response[1 + i * 6];
    dst[0] = itf->mounted ? 1 : 0;
    dst[1] = itf->itf_protocol;
    dst[2] = itf->protocol_mode;
    put_u16(&dst[3], itf->desc_len);
    dst[5] = host_input_layout_flags(i);
  }

  send_response(CMD_GET_HID_INTERFACES, sequence, STATUS_OK, response, sizeof(response));
}

static void handle_get_report_descriptor(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 3 || payload[0] >= RAW_CAPTURE_INSTANCE_COUNT) {
    send_response(CMD_GET_REPORT_DESCRIPTOR, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  const raw_interface_t *itf = raw_capture_interface(payload[0]);
  uint16_t offset = (uint16_t)(payload[1] | (payload[2] << 8));
  uint8_t response[KEYREMAP_REPORT_SIZE - 4] = {0};
  uint8_t chunk = 0;

  if (offset < itf->desc_len) {
    uint16_t remaining = (uint16_t)(itf->desc_len - offset);
    chunk = (uint8_t)(remaining > sizeof(response) - 5 ? sizeof(response) - 5 : remaining);
    memcpy(&response[5], &itf->desc[offset], chunk);
  }

  response[0] = payload[0];
  put_u16(&response[1], offset);
  put_u16(&response[3], itf->desc_len);
  send_response(CMD_GET_REPORT_DESCRIPTOR, sequence, STATUS_OK, response, (uint8_t)(5 + chunk));
}

static void handle_get_info(uint8_t sequence) {
  uint8_t payload[60] = {0};
  const char board_name[] = "rp2040-zero";

  payload[0] = KEYREMAP_PROTOCOL_MAJOR;
  payload[1] = KEYREMAP_PROTOCOL_MINOR;
  payload[2] = 0;
  payload[3] = 4;
  payload[4] = 0;
  payload[5] = mapper_active_layer();
  payload[6] = host_input_status();
  memcpy(&payload[7], board_name, sizeof(board_name));

  send_response(CMD_GET_INFO, sequence, STATUS_OK, payload, 7 + sizeof(board_name));
}

static void handle_get_config_summary(uint8_t sequence) {
  uint8_t payload[60] = {0};
  const keyremap_config_t *cfg = config_get();

  payload[0] = KEYREMAP_PROFILE_COUNT;
  payload[1] = KEYREMAP_LAYER_COUNT;
  payload[2] = KEYREMAP_BINDING_COUNT;
  payload[3] = cfg->active_profile;
  payload[4] = mapper_active_layer();
  payload[5] = (uint8_t)(cfg->generation & 0xff);
  payload[6] = (uint8_t)((cfg->generation >> 8) & 0xff);
  payload[7] = (uint8_t)((cfg->generation >> 16) & 0xff);
  payload[8] = (uint8_t)((cfg->generation >> 24) & 0xff);

  send_response(CMD_GET_CONFIG_SUMMARY, sequence, STATUS_OK, payload, 9);
}

static void encode_binding(uint8_t *payload, uint8_t layer, uint8_t slot, const keyremap_binding_t *binding) {
  payload[0] = layer;
  payload[1] = slot;
  payload[2] = binding->input_kind;
  payload[3] = binding->input_code;
  payload[4] = binding->output_kind;
  payload[5] = binding->output_code;
  payload[6] = (uint8_t)(binding->scale & 0xff);
  payload[7] = (uint8_t)((binding->scale >> 8) & 0xff);
}

static bool decode_binding(const uint8_t *payload, keyremap_binding_t *binding) {
  if (!binding) {
    return false;
  }

  binding->input_kind = payload[2];
  binding->input_code = payload[3];
  binding->output_kind = payload[4];
  binding->output_code = payload[5];
  binding->scale = (int16_t)((uint16_t)payload[6] | ((uint16_t)payload[7] << 8));
  return true;
}

static void handle_get_binding(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 2) {
    send_response(CMD_GET_BINDING, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  keyremap_binding_t binding;
  if (!config_get_binding(payload[0], payload[1], &binding)) {
    send_response(CMD_GET_BINDING, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  uint8_t response[8];
  encode_binding(response, payload[0], payload[1], &binding);
  send_response(CMD_GET_BINDING, sequence, STATUS_OK, response, sizeof(response));
}

static void handle_set_binding(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 8) {
    send_response(CMD_SET_BINDING, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  keyremap_binding_t binding;
  if (!decode_binding(payload, &binding) || !config_set_binding(payload[0], payload[1], &binding)) {
    send_response(CMD_SET_BINDING, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  send_response(CMD_SET_BINDING, sequence, STATUS_OK, payload, len);
}

static void handle_save_config(uint8_t sequence) {
  if (!config_save()) {
    send_response(CMD_SAVE_CONFIG, sequence, STATUS_INTERNAL_ERROR, NULL, 0);
    return;
  }

  send_response(CMD_SAVE_CONFIG, sequence, STATUS_OK, NULL, 0);
}

static void handle_reset_config(uint8_t sequence) {
  config_reset_default();
  (void)config_save();
  send_response(CMD_RESET_CONFIG, sequence, STATUS_OK, NULL, 0);
}

static void handle_set_led(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 3) {
    send_response(CMD_SET_LED, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  last_red = payload[0];
  last_green = payload[1];
  last_blue = payload[2];
  status_led_set_rgb(last_red, last_green, last_blue);
  send_response(CMD_SET_LED, sequence, STATUS_OK, NULL, 0);
}

static void handle_set_active_layer(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 1) {
    send_response(CMD_SET_ACTIVE_LAYER, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  if (!mapper_set_base_layer(payload[0])) {
    send_response(CMD_SET_ACTIVE_LAYER, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  send_response(CMD_SET_ACTIVE_LAYER, sequence, STATUS_OK, &payload[0], 1);
}

static void handle_get_layer_state(uint8_t sequence) {
  uint8_t payload[] = {mapper_active_layer()};
  send_response(CMD_GET_LAYER_STATE, sequence, STATUS_OK, payload, sizeof(payload));
}

static void handle_get_host_status(uint8_t sequence) {
  root_port_t *root = PIO_USB_ROOT_PORT(0);
  uint8_t payload[] = {
    host_input_status(),
    (uint8_t)pio_usb_bus_get_line_state(root),
    root->is_fullspeed ? 1 : 0,
    root->connected ? 1 : 0,
    root->suspended ? 1 : 0,
    (uint8_t)(root->ints & 0xff),
    (uint8_t)(host_input_vid() & 0xff),
    (uint8_t)((host_input_vid() >> 8) & 0xff),
    (uint8_t)(host_input_pid() & 0xff),
    (uint8_t)((host_input_pid() >> 8) & 0xff),
  };

  send_response(CMD_GET_HOST_STATUS, sequence, STATUS_OK, payload, sizeof(payload));
}

static void handle_get_event_log(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 4) {
    send_response(CMD_GET_EVENT_LOG, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  uint8_t response[KEYREMAP_REPORT_SIZE - 4] = {0};
  uint8_t pos = 6;
  uint8_t count = 0;
  uint32_t after = get_u32(payload);

  while (pos + 13 <= sizeof(response)) {
    bool dropped;
    const mapper_log_entry_t *entry = mapper_log_next(after, &dropped);
    if (!entry) {
      break;
    }
    if (dropped) {
      response[1] |= 0x01;
    }

    uint8_t *dst = &response[pos];
    put_u32(dst, entry->seq);
    dst[4] = entry->input.kind;
    dst[5] = entry->input.code;
    put_u16(&dst[6], (uint16_t)entry->input.value);
    dst[8] = entry->output.kind;
    dst[9] = entry->output.code;
    put_u16(&dst[10], (uint16_t)entry->output.value);
    dst[12] = (uint8_t)(entry->layer | (entry->simulated ? 0x80 : 0));
    pos = (uint8_t)(pos + 13);
    after = entry->seq;
    count++;
  }

  response[0] = count;
  put_u32(&response[2], mapper_log_latest_seq());
  send_response(CMD_GET_EVENT_LOG, sequence, STATUS_OK, response, pos);
}

static void handle_simulate_input(uint8_t sequence, const uint8_t *payload, uint8_t len) {
  if (len != 4) {
    send_response(CMD_SIMULATE_INPUT, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  input_event_t event = {
    .kind = payload[0],
    .code = payload[1],
    .value = (int16_t)((uint16_t)payload[2] | ((uint16_t)payload[3] << 8)),
  };

  host_input_simulate(&event);
  send_response(CMD_SIMULATE_INPUT, sequence, STATUS_OK, NULL, 0);
}

static void handle_release_all(uint8_t sequence) {
  mapper_release_all();
  send_response(CMD_RELEASE_ALL, sequence, STATUS_OK, NULL, 0);
}

static void handle_report(uint8_t const *buffer, uint16_t bufsize) {
  if (bufsize < 4) {
    return;
  }

  uint8_t command = buffer[0];
  uint8_t sequence = buffer[1];
  uint8_t len = buffer[2];
  const uint8_t *payload = &buffer[4];

  if (len > bufsize - 4) {
    send_response(command, sequence, STATUS_INVALID_LENGTH, NULL, 0);
    return;
  }

  switch (command) {
    case CMD_GET_INFO:
      handle_get_info(sequence);
      break;
    case CMD_SET_LED:
      handle_set_led(sequence, payload, len);
      break;
    case CMD_GET_CONFIG_SUMMARY:
      handle_get_config_summary(sequence);
      break;
    case CMD_GET_BINDING:
      handle_get_binding(sequence, payload, len);
      break;
    case CMD_SET_BINDING:
      handle_set_binding(sequence, payload, len);
      break;
    case CMD_SAVE_CONFIG:
      handle_save_config(sequence);
      break;
    case CMD_RESET_CONFIG:
      handle_reset_config(sequence);
      break;
    case CMD_GET_RAW_REPORTS:
      handle_get_raw_reports(sequence, payload, len);
      break;
    case CMD_GET_HID_INTERFACES:
      handle_get_hid_interfaces(sequence);
      break;
    case CMD_GET_REPORT_DESCRIPTOR:
      handle_get_report_descriptor(sequence, payload, len);
      break;
    case CMD_SET_ACTIVE_LAYER:
      handle_set_active_layer(sequence, payload, len);
      break;
    case CMD_GET_LAYER_STATE:
      handle_get_layer_state(sequence);
      break;
    case CMD_GET_HOST_STATUS:
      handle_get_host_status(sequence);
      break;
    case CMD_GET_EVENT_LOG:
      handle_get_event_log(sequence, payload, len);
      break;
    case CMD_SIMULATE_INPUT:
      handle_simulate_input(sequence, payload, len);
      break;
    case CMD_RELEASE_ALL:
      handle_release_all(sequence);
      break;
    case CMD_REBOOT_BOOTSEL:
      send_response(CMD_REBOOT_BOOTSEL, sequence, STATUS_OK, NULL, 0);
      sleep_ms(50);
      reset_usb_boot(0, 0);
      break;
    default:
      send_response(command, sequence, STATUS_UNKNOWN_COMMAND, NULL, 0);
      break;
  }
}

int main(void) {
  board_init();
  config_init();
  mapper_init();
  macro_init();
  output_hid_init();
  host_input_init();
  status_led_init();
  status_led_set_rgb(0, 32, 0);
  sleep_ms(180);
  status_led_set_rgb(0, 0, 0);
  sleep_ms(180);
  status_led_set_rgb(0, 32, 0);
  sleep_ms(180);
  status_led_set_rgb(0, 0, 0);
  sleep_ms(180);

  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  tusb_rhport_init_t host_init = {
    .role = TUSB_ROLE_HOST,
    .speed = TUSB_SPEED_AUTO,
  };
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = 2;
  pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
  tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
  tusb_init(BOARD_TUH_RHPORT, &host_init);
  status_led_set_host_waiting();

  while (true) {
    tud_task();
    tuh_task_ext(0, false);
    host_input_task();
    mapper_task();
    macro_task();
    output_hid_task();
    status_led_task();
  }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  (void)report_id;

  if (instance == 3 && report_type == HID_REPORT_TYPE_OUTPUT) {
    handle_report(buffer, bufsize);
  }
}

void tud_mount_cb(void) {
}

void tud_umount_cb(void) {
}

void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
}

void tud_resume_cb(void) {
}
