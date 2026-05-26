#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"

#define USB_VID 0xCafe
#define USB_PID 0x4020
#define USB_BCD 0x0001

enum {
  ITF_NUM_KEYBOARD,
  ITF_NUM_MOUSE,
  ITF_NUM_CONSUMER,
  ITF_NUM_CONFIG,
  ITF_NUM_TOTAL
};

enum {
  EPNUM_KEYBOARD = 0x81,
  EPNUM_MOUSE = 0x82,
  EPNUM_CONSUMER = 0x83,
  EPNUM_CONFIG_IN = 0x84,
  EPNUM_CONFIG_OUT = 0x03
};

uint8_t const desc_keyboard_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const desc_mouse_report[] = {
  TUD_HID_REPORT_DESC_MOUSE()
};

uint8_t const desc_consumer_report[] = {
  TUD_HID_REPORT_DESC_CONSUMER()
};

uint8_t const desc_config_report[] = {
  TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_EP_BUFSIZE)
};

tusb_desc_device_t const desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = 0x00,
  .bDeviceSubClass = 0x00,
  .bDeviceProtocol = 0x00,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = USB_BCD,
  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_INOUT_DESC_LEN, 100, 100),
  TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_keyboard_report), EPNUM_KEYBOARD, 16, 5),
  TUD_HID_DESCRIPTOR(ITF_NUM_MOUSE, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_mouse_report), EPNUM_MOUSE, 16, 5),
  TUD_HID_DESCRIPTOR(ITF_NUM_CONSUMER, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_consumer_report), EPNUM_CONSUMER, 16, 5),
  TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_CONFIG, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_config_report), EPNUM_CONFIG_IN, EPNUM_CONFIG_OUT, CFG_TUD_HID_EP_BUFSIZE, 5),
};

char const *string_desc_arr[] = {
  (const char[]){0x09, 0x04},
  "keyremap",
  "RP2040-Zero Keyremap Config",
  "000001",
};

static uint16_t desc_str[32];

uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  switch (instance) {
    case ITF_NUM_KEYBOARD:
      return desc_keyboard_report;
    case ITF_NUM_MOUSE:
      return desc_mouse_report;
    case ITF_NUM_CONSUMER:
      return desc_consumer_report;
    case ITF_NUM_CONFIG:
      return desc_config_report;
    default:
      return NULL;
  }
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;
  if (index == 0) {
    memcpy(&desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
      return NULL;
    }

    const char *str = string_desc_arr[index];
    chr_count = strlen(str);
    if (chr_count > 31) {
      chr_count = 31;
    }

    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str[1 + i] = str[i];
    }
  }

  desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
  return desc_str;
}
