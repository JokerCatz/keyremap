#include "config.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "keyremap_protocol.h"

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#define CONFIG_MAGIC 0x4b524d50u
#define CONFIG_VERSION 4u
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

static uint32_t checksum_bytes(const uint8_t *data, uint32_t len) {
  uint32_t hash = 2166136261u;
  for (uint32_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

/* Every layer starts empty: unbound inputs fall through to the base layer,
 * and unbound base-layer inputs pass through unchanged. */
void config_reset_default(void) {
  memset(&config, 0, sizeof(config));
  config.generation = 1;
  config.active_profile = 0;

  strcpy(config.profiles[0].name, "default");
  strcpy(config.profiles[0].layers[0].name, "base");
  strcpy(config.profiles[0].layers[1].name, "layer1");
  strcpy(config.profiles[0].layers[2].name, "layer2");
  strcpy(config.profiles[0].layers[3].name, "layer3");
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

const keyremap_binding_t *config_find_binding(uint8_t layer_index, uint8_t input_kind, uint8_t input_code) {
  if (layer_index >= KEYREMAP_LAYER_COUNT) {
    return NULL;
  }

  const keyremap_layer_t *layer = &config.profiles[config.active_profile].layers[layer_index];

  for (uint8_t i = 0; i < KEYREMAP_BINDING_COUNT; i++) {
    const keyremap_binding_t *binding = &layer->bindings[i];
    if (binding->input_kind == input_kind && binding->input_code == input_code && binding->output_kind != OUTPUT_KIND_NONE) {
      return binding;
    }
  }

  return NULL;
}
