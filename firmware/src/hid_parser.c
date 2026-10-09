#include "hid_parser.h"

#include <string.h>

#define USAGE_MAX 16
#define REPORT_ID_MAX 8

enum {
  PAGE_GENERIC_DESKTOP = 0x01,
  PAGE_KEYBOARD = 0x07,
  PAGE_BUTTON = 0x09,
  PAGE_CONSUMER = 0x0c,
};

typedef struct {
  uint16_t usage_page;
  int32_t logical_min;
  uint8_t report_size;
  uint8_t report_count;
  uint8_t report_id;
} globals_t;

typedef struct {
  uint32_t usages[USAGE_MAX];
  uint8_t usage_count;
  uint32_t usage_min;
  uint32_t usage_max;
  bool has_range;
} locals_t;

typedef struct {
  uint8_t report_id;
  uint16_t bit_offset;
} report_offset_t;

static bool page_is_interesting(uint16_t page) {
  return page == PAGE_KEYBOARD || page == PAGE_BUTTON || page == PAGE_CONSUMER || page == PAGE_GENERIC_DESKTOP;
}

static bool variable_is_interesting(uint16_t page, uint16_t usage) {
  if (page == PAGE_GENERIC_DESKTOP) {
    return usage == 0x30 || usage == 0x31 || usage == 0x38;
  }
  return page == PAGE_KEYBOARD || page == PAGE_BUTTON || page == PAGE_CONSUMER;
}

static void add_field(hid_layout_t *layout, const hid_field_t *field) {
  if (layout->field_count < HID_PARSER_FIELD_MAX) {
    layout->fields[layout->field_count++] = *field;
  }
}

static uint16_t *offset_for(report_offset_t *offsets, uint8_t *offset_count, uint8_t report_id) {
  for (uint8_t i = 0; i < *offset_count; i++) {
    if (offsets[i].report_id == report_id) {
      return &offsets[i].bit_offset;
    }
  }

  if (*offset_count >= REPORT_ID_MAX) {
    return NULL;
  }

  offsets[*offset_count].report_id = report_id;
  offsets[*offset_count].bit_offset = 0;
  return &offsets[(*offset_count)++].bit_offset;
}

static void add_input(hid_layout_t *layout, const globals_t *g, const locals_t *l, uint8_t flags, uint16_t bit_offset) {
  bool constant = flags & 0x01;
  bool variable = flags & 0x02;
  if (constant) {
    return;
  }

  hid_field_t field = {
    .report_id = g->report_id,
    .bit_size = g->report_size,
    .logical_min = g->logical_min,
  };
  if (g->logical_min < 0) {
    field.flags |= HID_FIELD_SIGNED;
  }
  if (flags & 0x04) {
    field.flags |= HID_FIELD_RELATIVE;
  }

  if (!variable) {
    uint32_t first = l->has_range ? l->usage_min : (l->usage_count ? l->usages[0] : 0);
    uint16_t page = (first >> 16) ? (uint16_t)(first >> 16) : g->usage_page;
    if (!page_is_interesting(page) || page == PAGE_GENERIC_DESKTOP) {
      return;
    }
    field.flags |= HID_FIELD_ARRAY;
    field.usage_page = page;
    field.usage = (uint16_t)first;
    field.count = g->report_count;
    field.bit_offset = bit_offset;
    add_field(layout, &field);
    return;
  }

  field.count = 1;
  for (uint8_t i = 0; i < g->report_count; i++) {
    uint32_t usage;
    if (i < l->usage_count) {
      usage = l->usages[i];
    } else if (l->has_range) {
      usage = l->usage_min + i - l->usage_count;
      if (usage > l->usage_max) {
        usage = l->usage_max;
      }
    } else if (l->usage_count) {
      usage = l->usages[l->usage_count - 1];
    } else {
      continue;
    }

    uint16_t page = (usage >> 16) ? (uint16_t)(usage >> 16) : g->usage_page;
    if (!variable_is_interesting(page, (uint16_t)usage)) {
      continue;
    }

    field.usage_page = page;
    field.usage = (uint16_t)usage;
    field.bit_offset = (uint16_t)(bit_offset + i * g->report_size);
    add_field(layout, &field);
  }
}

void hid_parser_parse(const uint8_t *desc, uint16_t desc_len, hid_layout_t *layout) {
  memset(layout, 0, sizeof(*layout));

  globals_t g = {0};
  globals_t stack[2];
  uint8_t stack_depth = 0;
  locals_t l = {0};
  report_offset_t offsets[REPORT_ID_MAX];
  uint8_t offset_count = 0;
  uint16_t i = 0;

  while (i < desc_len) {
    uint8_t prefix = desc[i++];
    if (prefix == 0xfe) {
      if (i + 1 >= desc_len) {
        break;
      }
      i = (uint16_t)(i + 2 + desc[i]);
      continue;
    }

    uint8_t size = prefix & 0x03;
    if (size == 3) {
      size = 4;
    }
    if (i + size > desc_len) {
      break;
    }

    uint32_t udata = 0;
    for (uint8_t b = 0; b < size; b++) {
      udata |= (uint32_t)desc[i + b] << (8 * b);
    }
    int32_t sdata = (int32_t)udata;
    if (size == 1) {
      sdata = (int8_t)udata;
    } else if (size == 2) {
      sdata = (int16_t)udata;
    }
    i = (uint16_t)(i + size);

    uint8_t type = (prefix >> 2) & 0x03;
    uint8_t tag = prefix >> 4;

    if (type == 0) {
      if (tag == 0x8) {
        uint16_t *offset = offset_for(offsets, &offset_count, g.report_id);
        if (offset) {
          add_input(layout, &g, &l, (uint8_t)udata, *offset);
          *offset = (uint16_t)(*offset + g.report_size * g.report_count);
        }
      }
      memset(&l, 0, sizeof(l));
    } else if (type == 1) {
      switch (tag) {
        case 0x0: g.usage_page = (uint16_t)udata; break;
        case 0x1: g.logical_min = sdata; break;
        case 0x7: g.report_size = (uint8_t)udata; break;
        case 0x8:
          g.report_id = (uint8_t)udata;
          layout->uses_report_id = true;
          break;
        case 0x9: g.report_count = (uint8_t)udata; break;
        case 0xa:
          if (stack_depth < 2) {
            stack[stack_depth++] = g;
          }
          break;
        case 0xb:
          if (stack_depth) {
            g = stack[--stack_depth];
          }
          break;
        default: break;
      }
    } else if (type == 2) {
      uint32_t usage = size == 4 ? udata : (udata & 0xffff);
      switch (tag) {
        case 0x0:
          if (l.usage_count < USAGE_MAX) {
            l.usages[l.usage_count++] = usage;
          }
          break;
        case 0x1:
          l.usage_min = usage;
          l.has_range = true;
          break;
        case 0x2: l.usage_max = usage; break;
        default: break;
      }
    }
  }
}

void hid_parser_boot_keyboard(hid_layout_t *layout) {
  memset(layout, 0, sizeof(*layout));
  for (uint8_t i = 0; i < 8; i++) {
    hid_field_t mod = {.flags = 0, .bit_size = 1, .count = 1, .bit_offset = i, .usage_page = PAGE_KEYBOARD, .usage = (uint16_t)(0xe0 + i)};
    add_field(layout, &mod);
  }
  hid_field_t keys = {.flags = HID_FIELD_ARRAY, .bit_size = 8, .count = 6, .bit_offset = 16, .usage_page = PAGE_KEYBOARD, .usage = 0};
  add_field(layout, &keys);
}

void hid_parser_boot_mouse(hid_layout_t *layout) {
  memset(layout, 0, sizeof(*layout));
  for (uint8_t i = 0; i < 5; i++) {
    hid_field_t button = {.bit_size = 1, .count = 1, .bit_offset = i, .usage_page = PAGE_BUTTON, .usage = (uint16_t)(i + 1)};
    add_field(layout, &button);
  }
  static const uint16_t axes[] = {0x30, 0x31, 0x38};
  for (uint8_t i = 0; i < 3; i++) {
    hid_field_t axis = {
      .flags = HID_FIELD_SIGNED | HID_FIELD_RELATIVE,
      .bit_size = 8,
      .count = 1,
      .bit_offset = (uint16_t)(8 + 8 * i),
      .usage_page = PAGE_GENERIC_DESKTOP,
      .usage = axes[i],
      .logical_min = -127,
    };
    add_field(layout, &axis);
  }
}

bool hid_parser_read(const hid_field_t *field, uint8_t index, const uint8_t *body, uint16_t body_len, int32_t *value) {
  uint32_t start = field->bit_offset + (uint32_t)index * field->bit_size;
  uint8_t size = field->bit_size;
  if (size == 0 || size > 32 || (start + size + 7) / 8 > body_len) {
    return false;
  }

  uint32_t raw = 0;
  for (uint8_t b = 0; b < size; b++) {
    uint32_t bit = start + b;
    if (body[bit / 8] & (1u << (bit % 8))) {
      raw |= 1u << b;
    }
  }

  if ((field->flags & HID_FIELD_SIGNED) && size < 32 && (raw & (1u << (size - 1)))) {
    raw |= ~((1u << size) - 1);
  }

  *value = (int32_t)raw;
  return true;
}
