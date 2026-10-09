#pragma once

#include <stdbool.h>
#include <stdint.h>

#define HID_PARSER_FIELD_MAX 32

enum {
  HID_FIELD_ARRAY = 0x01,
  HID_FIELD_SIGNED = 0x02,
  HID_FIELD_RELATIVE = 0x04,
};

/* One Input main item, or one usage of a variable Input main item. */
typedef struct {
  uint8_t report_id;
  uint8_t flags;
  uint8_t bit_size;
  uint8_t count;
  uint16_t bit_offset;
  uint16_t usage_page;
  uint16_t usage;
  int32_t logical_min;
} hid_field_t;

typedef struct {
  bool uses_report_id;
  uint8_t field_count;
  hid_field_t fields[HID_PARSER_FIELD_MAX];
} hid_layout_t;

/* Parses the Input items of a HID report descriptor. Only fields that carry
 * keys, buttons, relative axes or consumer usages are kept. */
void hid_parser_parse(const uint8_t *desc, uint16_t desc_len, hid_layout_t *layout);
void hid_parser_boot_keyboard(hid_layout_t *layout);
void hid_parser_boot_mouse(hid_layout_t *layout);

/* Reads element `index` of a field from a report body (report ID byte already
 * stripped). Returns false when the report is too short. */
bool hid_parser_read(const hid_field_t *field, uint8_t index, const uint8_t *body, uint16_t body_len, int32_t *value);
