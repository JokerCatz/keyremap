#include "config.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "keyremap_protocol.h"

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#define CONFIG_MAGIC 0x4b524d50u
#define CONFIG_VERSION 3u
#define CONFIG_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

static keyremap_config_t config;

typedef struct {
  uint32_t magic;
  uint16_t version;
  uint16_t length;
  uint32_t checksum;
  keyremap_config_t config;
} persisted_config_t;

static uint8_t flash_sector[FLASH_SECTOR_SIZE];

typedef struct {
  uint8_t input;
  uint8_t output;
} key_pair_t;

static const key_pair_t base_key_passthrough[] = {
  {0x1e, 0x1e}, /* 1 */
  {0x1f, 0x1f}, /* 2 */
  {0x20, 0x20}, /* 3 */
  {0x21, 0x21}, /* 4 */
  {0x29, 0x29}, /* ESC */
  {0x2b, 0x2b}, /* TAB */
  {0x05, 0x05}, /* B */
  {0x0a, 0x0a}, /* G */
  {0x10, 0x10}, /* M */
  {0x1c, 0x1c}, /* Y */
  {0x22, 0x22}, /* 5 */
  {0x23, 0x23}, /* 6 */
  {0x24, 0x24}, /* 7 */
  {0x25, 0x25}, /* 8 */
  {0x28, 0x28}, /* ENTER */
  {0x3a, 0x3a}, /* F1 */
  {0x17, 0x17}, /* T */
  {0x0b, 0x0b}, /* H */
  {0x3b, 0x3b}, /* F2 */
  {0x11, 0x11}, /* N */
  {0x14, 0x14}, /* Q */
  {0x1a, 0x1a}, /* W */
  {0x08, 0x08}, /* E */
  {0x15, 0x15}, /* R */
  {0x04, 0x04}, /* A */
  {0x16, 0x16}, /* S */
  {0x07, 0x07}, /* D */
  {0x09, 0x09}, /* F */
  {0x1d, 0x1d}, /* Z */
  {0x1b, 0x1b}, /* X */
  {0x06, 0x06}, /* C */
  {0x19, 0x19}, /* V */
  {0x2c, 0x2c}, /* SPACE */
  {0xe0, 0xe0}, /* LEFTCTRL */
  {0xe1, 0xe1}, /* LEFTSHIFT */
  {0xe2, 0xe2}, /* LEFTALT */
};

static uint32_t checksum_bytes(const uint8_t *data, uint32_t len) {
  uint32_t hash = 2166136261u;
  for (uint32_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

static void set_binding(keyremap_layer_t *layer, uint8_t slot, uint8_t input_kind, uint8_t input_code, uint8_t output_kind, uint8_t output_code, int16_t scale) {
  if (slot >= KEYREMAP_BINDING_COUNT) {
    return;
  }

  layer->bindings[slot].input_kind = input_kind;
  layer->bindings[slot].input_code = input_code;
  layer->bindings[slot].output_kind = output_kind;
  layer->bindings[slot].output_code = output_code;
  layer->bindings[slot].scale = scale;
}

void config_reset_default(void) {
  memset(&config, 0, sizeof(config));
  config.generation = 1;
  config.active_profile = 0;
  config.active_layer = 0;

  strcpy(config.profiles[0].name, "default");
  strcpy(config.profiles[0].layers[0].name, "base");
  strcpy(config.profiles[0].layers[1].name, "nav");
  strcpy(config.profiles[0].layers[2].name, "media");
  strcpy(config.profiles[0].layers[3].name, "game");

  uint8_t slot = 0;
  for (uint8_t i = 0; i < sizeof(base_key_passthrough) / sizeof(base_key_passthrough[0]); i++) {
    set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_KEY, base_key_passthrough[i].input, OUTPUT_KIND_KEY, base_key_passthrough[i].output, 1000);
  }

  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_MOUSE_BUTTON, 1, OUTPUT_KIND_MOUSE_BUTTON, 1, 1000);
  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_MOUSE_BUTTON, 2, OUTPUT_KIND_MOUSE_BUTTON, 2, 1000);
  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_MOUSE_BUTTON, 3, OUTPUT_KIND_MOUSE_BUTTON, 3, 1000);
  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_MOUSE_BUTTON, 4, OUTPUT_KIND_MOUSE_BUTTON, 4, 1000);
  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_REL_X, 0, OUTPUT_KIND_REL_X, 0, 1000);
  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_REL_Y, 0, OUTPUT_KIND_REL_Y, 0, 1000);
  set_binding(&config.profiles[0].layers[0], slot++, INPUT_KIND_WHEEL, 0, OUTPUT_KIND_WHEEL, 0, 1000);

  set_binding(&config.profiles[0].layers[1], 0, INPUT_KIND_KEY, 0x1e, OUTPUT_KIND_KEY, 0x3a, 1000);
  set_binding(&config.profiles[0].layers[1], 1, INPUT_KIND_KEY, 0x2b, OUTPUT_KIND_KEY, 0x3b, 1000);
  set_binding(&config.profiles[0].layers[1], 2, INPUT_KIND_KEY, 0x1c, OUTPUT_KIND_KEY, 0x28, 1000);
  set_binding(&config.profiles[0].layers[1], 3, INPUT_KIND_MOUSE_BUTTON, 1, OUTPUT_KIND_KEY, 0x2c, 1000);

  set_binding(&config.profiles[0].layers[2], 0, INPUT_KIND_KEY, 0x1e, OUTPUT_KIND_KEY, 0x04, 1000);
  set_binding(&config.profiles[0].layers[2], 1, INPUT_KIND_KEY, 0x2b, OUTPUT_KIND_KEY, 0x05, 1000);
  set_binding(&config.profiles[0].layers[2], 2, INPUT_KIND_KEY, 0x1c, OUTPUT_KIND_KEY, 0x06, 1000);

  set_binding(&config.profiles[0].layers[3], 0, INPUT_KIND_KEY, 0x1e, OUTPUT_KIND_KEY, 0x1e, 1000);
  set_binding(&config.profiles[0].layers[3], 1, INPUT_KIND_KEY, 0x1f, OUTPUT_KIND_KEY, 0x1f, 1000);
  set_binding(&config.profiles[0].layers[3], 2, INPUT_KIND_KEY, 0x20, OUTPUT_KIND_KEY, 0x20, 1000);
}

static bool config_load(void) {
  const persisted_config_t *stored = (const persisted_config_t *)(XIP_BASE + CONFIG_FLASH_OFFSET);
  if (stored->magic != CONFIG_MAGIC || stored->version != CONFIG_VERSION || stored->length != sizeof(keyremap_config_t)) {
    return false;
  }

  if (stored->checksum != checksum_bytes((const uint8_t *)&stored->config, sizeof(stored->config))) {
    return false;
  }

  memcpy(&config, &stored->config, sizeof(config));
  return true;
}

void config_init(void) {
  config_reset_default();
  (void)config_load();
}

const keyremap_config_t *config_get(void) {
  return &config;
}

uint8_t config_active_layer(void) {
  return config.active_layer;
}

bool config_set_active_layer(uint8_t layer) {
  if (layer >= KEYREMAP_LAYER_COUNT) {
    return false;
  }

  config.active_layer = layer;
  config.generation++;
  return true;
}

bool config_get_binding(uint8_t layer, uint8_t slot, keyremap_binding_t *binding) {
  if (layer >= KEYREMAP_LAYER_COUNT || slot >= KEYREMAP_BINDING_COUNT || !binding) {
    return false;
  }

  *binding = config.profiles[config.active_profile].layers[layer].bindings[slot];
  return true;
}

bool config_set_binding(uint8_t layer, uint8_t slot, const keyremap_binding_t *binding) {
  if (layer >= KEYREMAP_LAYER_COUNT || slot >= KEYREMAP_BINDING_COUNT || !binding) {
    return false;
  }

  config.profiles[config.active_profile].layers[layer].bindings[slot] = *binding;
  config.generation++;
  return true;
}

bool config_save(void) {
  persisted_config_t stored = {
    .magic = CONFIG_MAGIC,
    .version = CONFIG_VERSION,
    .length = sizeof(keyremap_config_t),
    .checksum = checksum_bytes((const uint8_t *)&config, sizeof(config)),
    .config = config,
  };

  memset(flash_sector, 0xff, sizeof(flash_sector));
  memcpy(flash_sector, &stored, sizeof(stored));

  uint32_t interrupts = save_and_disable_interrupts();
  flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(CONFIG_FLASH_OFFSET, flash_sector, FLASH_SECTOR_SIZE);
  restore_interrupts(interrupts);
  return true;
}

const keyremap_binding_t *config_find_binding(uint8_t input_kind, uint8_t input_code) {
  const keyremap_layer_t *layer = &config.profiles[config.active_profile].layers[config.active_layer];

  for (uint8_t i = 0; i < KEYREMAP_BINDING_COUNT; i++) {
    const keyremap_binding_t *binding = &layer->bindings[i];
    if (binding->input_kind == input_kind && binding->input_code == input_code && binding->output_kind != OUTPUT_KIND_NONE) {
      return binding;
    }
  }

  return NULL;
}
