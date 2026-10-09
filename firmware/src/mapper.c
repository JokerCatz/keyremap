#include "mapper.h"

#include <string.h>

#include "config.h"
#include "keyremap_protocol.h"
#include "output_hid.h"
#include "status_led.h"

#define HELD_MAX 16
#define LOG_SIZE 64
#define NO_LAYER 0xff

/* Digital inputs remember the output they resolved to on press, so the release
 * goes to the same output even if the layer changed in between. */
typedef struct {
  uint8_t input_kind;
  uint8_t input_code;
  output_event_t output;
} held_input_t;

static held_input_t held[HELD_MAX];
static uint8_t base_layer;
static uint8_t hold_layer;
/* Sub-unit remainders (in 1/1000) of scaled movement, per output axis, so a
 * small scale such as 10% still moves instead of rounding every step to 0. */
static int32_t motion_remainder[3];
static mapper_log_entry_t log_ring[LOG_SIZE];
static uint32_t log_seq;

static bool input_is_digital(uint8_t kind) {
  return kind == INPUT_KIND_KEY || kind == INPUT_KIND_MOUSE_BUTTON || kind == INPUT_KIND_CONSUMER;
}

static void passthrough(const input_event_t *input, output_event_t *output) {
  output->code = input->code;
  output->value = input->value;
  switch (input->kind) {
    case INPUT_KIND_KEY: output->kind = OUTPUT_KIND_KEY; break;
    case INPUT_KIND_MOUSE_BUTTON: output->kind = OUTPUT_KIND_MOUSE_BUTTON; break;
    case INPUT_KIND_REL_X: output->kind = OUTPUT_KIND_REL_X; break;
    case INPUT_KIND_REL_Y: output->kind = OUTPUT_KIND_REL_Y; break;
    case INPUT_KIND_WHEEL: output->kind = OUTPUT_KIND_WHEEL; break;
    case INPUT_KIND_CONSUMER: output->kind = OUTPUT_KIND_CONSUMER; break;
    default: output->kind = OUTPUT_KIND_NONE; break;
  }
}

static void resolve(const input_event_t *input, output_event_t *output) {
  uint8_t layer = mapper_active_layer();
  const keyremap_binding_t *binding = config_find_binding(layer, input->kind, input->code);
  if (!binding && layer != 0) {
    binding = config_find_binding(0, input->kind, input->code);
  }

  if (!binding) {
    passthrough(input, output);
    return;
  }

  int32_t value;
  bool motion_out = binding->output_kind >= OUTPUT_KIND_REL_X && binding->output_kind <= OUTPUT_KIND_WHEEL;
  if (motion_out && !input_is_digital(input->kind)) {
    int32_t *remainder = &motion_remainder[binding->output_kind - OUTPUT_KIND_REL_X];
    int32_t total = (int32_t)input->value * binding->scale + *remainder;
    value = total / 1000;
    *remainder = total - value * 1000;
  } else {
    value = ((int32_t)input->value * binding->scale) / 1000;
  }
  if (value > INT16_MAX) {
    value = INT16_MAX;
  } else if (value < INT16_MIN) {
    value = INT16_MIN;
  }

  output->kind = binding->output_kind;
  output->code = binding->output_code;
  output->value = (int16_t)value;
}

static held_input_t *find_held(uint8_t kind, uint8_t code) {
  for (uint8_t i = 0; i < HELD_MAX; i++) {
    if (held[i].input_kind == kind && held[i].input_code == code) {
      return &held[i];
    }
  }
  return NULL;
}

static void remember_press(const input_event_t *input, const output_event_t *output) {
  held_input_t *slot = find_held(input->kind, input->code);
  if (!slot) {
    slot = find_held(0, 0);
  }
  if (slot) {
    slot->input_kind = input->kind;
    slot->input_code = input->code;
    slot->output = *output;
  }
}

static void show_layer(void) {
  status_led_set_layer(mapper_active_layer());
}

static void apply_layer_action(const input_event_t *input, const output_event_t *output) {
  bool pressed = input->value != 0;

  switch (output->kind) {
    case OUTPUT_KIND_LAYER:
      if (pressed) {
        mapper_set_base_layer(output->code);
      }
      break;
    case OUTPUT_KIND_NEXT_LAYER:
      if (pressed) {
        mapper_set_base_layer((uint8_t)((base_layer + 1) % KEYREMAP_LAYER_COUNT));
      }
      break;
    case OUTPUT_KIND_LAYER_HOLD:
      if (pressed && output->code < KEYREMAP_LAYER_COUNT) {
        hold_layer = output->code;
      } else if (!pressed && hold_layer == output->code) {
        hold_layer = NO_LAYER;
      }
      show_layer();
      break;
    default:
      break;
  }
}

static void log_event(const input_event_t *input, const output_event_t *output, bool simulated) {
  log_seq++;
  mapper_log_entry_t *entry = &log_ring[log_seq % LOG_SIZE];
  entry->seq = log_seq;
  entry->input = *input;
  entry->output = *output;
  entry->layer = mapper_active_layer();
  entry->simulated = simulated;
}

void mapper_init(void) {
  memset(held, 0, sizeof(held));
  memset(log_ring, 0, sizeof(log_ring));
  memset(motion_remainder, 0, sizeof(motion_remainder));
  log_seq = 0;
  base_layer = 0;
  hold_layer = NO_LAYER;
}

void mapper_handle_input(const input_event_t *input, bool simulated) {
  output_event_t output;
  bool digital = input_is_digital(input->kind);
  held_input_t *previous = digital && input->value == 0 ? find_held(input->kind, input->code) : NULL;

  if (previous) {
    output = previous->output;
    output.value = 0;
    memset(previous, 0, sizeof(*previous));
  } else {
    resolve(input, &output);
    if (digital && input->value != 0) {
      remember_press(input, &output);
    }
  }

  apply_layer_action(input, &output);
  log_event(input, &output, simulated);
  output_hid_apply(&output);
}

void mapper_release_all(void) {
  memset(held, 0, sizeof(held));
  hold_layer = NO_LAYER;
  show_layer();
  output_hid_release_all();
}

uint8_t mapper_active_layer(void) {
  return hold_layer != NO_LAYER ? hold_layer : base_layer;
}

bool mapper_set_base_layer(uint8_t layer) {
  if (layer >= KEYREMAP_LAYER_COUNT) {
    return false;
  }
  base_layer = layer;
  show_layer();
  return true;
}

const mapper_log_entry_t *mapper_log_next(uint32_t after, bool *dropped) {
  *dropped = false;
  if (after >= log_seq) {
    return NULL;
  }

  uint32_t oldest = log_seq >= LOG_SIZE ? log_seq - LOG_SIZE + 1 : 1;
  uint32_t want = after + 1;
  if (want < oldest) {
    want = oldest;
    *dropped = true;
  }
  return &log_ring[want % LOG_SIZE];
}

uint32_t mapper_log_latest_seq(void) {
  return log_seq;
}
