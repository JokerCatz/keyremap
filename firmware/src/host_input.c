#include "host_input.h"

#include <string.h>

#include "hid_parser.h"
#include "keyremap_protocol.h"
#include "raw_capture.h"
#include "pico/stdlib.h"
#include "pio_usb_ll.h"
#include "status_led.h"
#include "tusb.h"
#include "host/hcd.h"

#define TARGET_VID 0x1c4f
#define TARGET_PID 0x007c

#define PRESSED_MAX 24

typedef struct {
  uint8_t report_id;
  uint8_t kind;
  uint8_t code;
} pressed_input_t;

typedef struct {
  bool active;
  bool boot;
  hid_layout_t layout;
  pressed_input_t pressed[PRESSED_MAX];
  uint8_t pressed_count;
} instance_state_t;

static instance_state_t instances[CFG_TUH_HID];
static uint8_t host_status;
static uint16_t mounted_vid;
static uint16_t mounted_pid;
static uint32_t last_attach_kick_ms;
static uint32_t last_reconnect_kick_ms;

static void dispatch_event(uint8_t kind, uint8_t code, int16_t value) {
  input_event_t input = {
    .kind = kind,
    .code = code,
    .value = value,
  };
  mapper_handle_input(&input, false);
}

static bool add_pressed(pressed_input_t *set, uint8_t *count, uint8_t report_id, uint8_t kind, uint8_t code) {
  for (uint8_t i = 0; i < *count; i++) {
    if (set[i].report_id == report_id && set[i].kind == kind && set[i].code == code) {
      return true;
    }
  }
  if (*count >= PRESSED_MAX) {
    return false;
  }
  set[*count].report_id = report_id;
  set[*count].kind = kind;
  set[*count].code = code;
  (*count)++;
  return true;
}

static bool set_contains(const pressed_input_t *set, uint8_t count, const pressed_input_t *item) {
  for (uint8_t i = 0; i < count; i++) {
    if (set[i].report_id == item->report_id && set[i].kind == item->kind && set[i].code == item->code) {
      return true;
    }
  }
  return false;
}

/* Maps a pressed usage to a digital input kind/code, or returns false. */
static bool usage_to_digital(uint16_t page, uint32_t usage, uint8_t *kind, uint8_t *code) {
  if (page == 0x07 && usage >= 0x04 && usage <= 0xe7) {
    *kind = INPUT_KIND_KEY;
  } else if (page == 0x09 && usage >= 1 && usage <= 8) {
    *kind = INPUT_KIND_MOUSE_BUTTON;
  } else if (page == 0x0c && usage >= 1 && usage <= 0xff) {
    *kind = INPUT_KIND_CONSUMER;
  } else {
    return false;
  }
  *code = (uint8_t)usage;
  return true;
}

static int16_t clamp16(int32_t value) {
  if (value > INT16_MAX) {
    return INT16_MAX;
  }
  if (value < INT16_MIN) {
    return INT16_MIN;
  }
  return (int16_t)value;
}

static void process_report(instance_state_t *inst, const uint8_t *report, uint16_t len) {
  uint8_t report_id = 0;
  if (inst->layout.uses_report_id && !inst->boot) {
    if (len < 1) {
      return;
    }
    report_id = report[0];
    report++;
    len--;
  }

  pressed_input_t current[PRESSED_MAX];
  uint8_t current_count = 0;
  bool matched = false;
  int32_t axes[3] = {0, 0, 0};

  for (uint8_t f = 0; f < inst->layout.field_count; f++) {
    const hid_field_t *field = &inst->layout.fields[f];
    if (field->report_id != report_id) {
      continue;
    }

    for (uint8_t n = 0; n < field->count; n++) {
      int32_t value;
      if (!hid_parser_read(field, n, report, len, &value)) {
        break;
      }
      matched = true;

      if (field->flags & HID_FIELD_ARRAY) {
        if (value < field->logical_min || value == 0) {
          continue;
        }
        uint32_t usage = field->usage + (uint32_t)(value - field->logical_min);
        uint8_t kind;
        uint8_t code;
        if (usage_to_digital(field->usage_page, usage, &kind, &code)) {
          add_pressed(current, &current_count, report_id, kind, code);
        }
      } else if (field->usage_page == 0x01) {
        uint8_t axis = field->usage == 0x30 ? 0 : field->usage == 0x31 ? 1 : 2;
        axes[axis] += value;
      } else if (!(field->flags & HID_FIELD_RELATIVE) && value) {
        uint8_t kind;
        uint8_t code;
        if (usage_to_digital(field->usage_page, field->usage, &kind, &code)) {
          add_pressed(current, &current_count, report_id, kind, code);
        }
      }
    }
  }

  if (!matched) {
    return;
  }

  /* Releases first, then presses, then movement. */
  for (uint8_t i = 0; i < inst->pressed_count; i++) {
    const pressed_input_t *old = &inst->pressed[i];
    if (old->report_id == report_id && !set_contains(current, current_count, old)) {
      dispatch_event(old->kind, old->code, 0);
    }
  }

  pressed_input_t next[PRESSED_MAX];
  uint8_t next_count = 0;
  for (uint8_t i = 0; i < inst->pressed_count; i++) {
    if (inst->pressed[i].report_id != report_id) {
      next[next_count++] = inst->pressed[i];
    }
  }
  for (uint8_t i = 0; i < current_count; i++) {
    if (!set_contains(inst->pressed, inst->pressed_count, &current[i])) {
      dispatch_event(current[i].kind, current[i].code, 1);
    }
    add_pressed(next, &next_count, current[i].report_id, current[i].kind, current[i].code);
  }
  memcpy(inst->pressed, next, sizeof(next[0]) * next_count);
  inst->pressed_count = next_count;

  if (axes[0]) {
    dispatch_event(INPUT_KIND_REL_X, 0, clamp16(axes[0]));
  }
  if (axes[1]) {
    dispatch_event(INPUT_KIND_REL_Y, 0, clamp16(axes[1]));
  }
  if (axes[2]) {
    dispatch_event(INPUT_KIND_WHEEL, 0, clamp16(axes[2]));
  }
}

static void release_instance(instance_state_t *inst) {
  for (uint8_t i = 0; i < inst->pressed_count; i++) {
    dispatch_event(inst->pressed[i].kind, inst->pressed[i].code, 0);
  }
  memset(inst, 0, sizeof(*inst));
}

void host_input_init(void) {
  memset(instances, 0, sizeof(instances));
  host_status = 0;
  mounted_vid = 0;
  mounted_pid = 0;
  last_attach_kick_ms = 0;
  last_reconnect_kick_ms = 0;
  raw_capture_init();
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
  mapper_handle_input(event, true);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  uint8_t protocol_mode = tuh_hid_get_protocol(dev_addr, instance);
  raw_capture_mount(instance, itf_protocol, protocol_mode, desc_report, desc_len);

  if (instance < CFG_TUH_HID) {
    instance_state_t *inst = &instances[instance];
    memset(inst, 0, sizeof(*inst));
    inst->active = true;
    /* TinyUSB only switches boot-subclass interfaces to boot protocol; their
     * reports then use the fixed boot layout instead of the descriptor. */
    inst->boot = itf_protocol != HID_ITF_PROTOCOL_NONE && protocol_mode == HID_PROTOCOL_BOOT;
    if (inst->boot && itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
      hid_parser_boot_keyboard(&inst->layout);
    } else if (inst->boot && itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
      hid_parser_boot_mouse(&inst->layout);
    } else {
      hid_parser_parse(desc_report, desc_len, &inst->layout);
    }
  }

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
  raw_capture_unmount(instance);
  if (instance < CFG_TUH_HID) {
    release_instance(&instances[instance]);
  }
  mapper_release_all();
  host_status = 3;
  mounted_vid = 0;
  mounted_pid = 0;
  status_led_set_host_error();
}

void tuh_umount_cb(uint8_t dev_addr) {
  (void)dev_addr;
  raw_capture_unmount_all();
  for (uint8_t i = 0; i < CFG_TUH_HID; i++) {
    release_instance(&instances[i]);
  }
  mapper_release_all();
  host_status = 3;
  mounted_vid = 0;
  mounted_pid = 0;
  status_led_set_host_error();
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
  raw_capture_report(instance, report, len);

  if (instance < CFG_TUH_HID && instances[instance].active) {
    process_report(&instances[instance], report, len);
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

uint8_t host_input_layout_flags(uint8_t instance) {
  if (instance >= CFG_TUH_HID || !instances[instance].active) {
    return 0;
  }

  const instance_state_t *inst = &instances[instance];
  uint8_t flags = inst->boot ? 0x80 : 0;
  for (uint8_t i = 0; i < inst->layout.field_count; i++) {
    const hid_field_t *field = &inst->layout.fields[i];
    switch (field->usage_page) {
      case 0x07: flags |= 0x01; break;
      case 0x09: flags |= 0x02; break;
      case 0x01: flags |= 0x04; break;
      case 0x0c: flags |= (field->flags & HID_FIELD_RELATIVE) ? 0 : 0x08; break;
      default: break;
    }
  }
  return flags;
}
