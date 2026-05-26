#pragma once

#include <stdbool.h>
#include <stdint.h>

#define KEYREMAP_PROFILE_COUNT 1
#define KEYREMAP_LAYER_COUNT 4
#define KEYREMAP_BINDING_COUNT 48

typedef struct {
  uint8_t input_kind;
  uint8_t input_code;
  uint8_t output_kind;
  uint8_t output_code;
  int16_t scale;
} keyremap_binding_t;

typedef struct {
  char name[12];
  keyremap_binding_t bindings[KEYREMAP_BINDING_COUNT];
} keyremap_layer_t;

typedef struct {
  char name[12];
  keyremap_layer_t layers[KEYREMAP_LAYER_COUNT];
} keyremap_profile_t;

typedef struct {
  uint8_t active_profile;
  uint8_t active_layer;
  uint32_t generation;
  keyremap_profile_t profiles[KEYREMAP_PROFILE_COUNT];
} keyremap_config_t;

void config_init(void);
const keyremap_config_t *config_get(void);
uint8_t config_active_layer(void);
bool config_set_active_layer(uint8_t layer);
bool config_get_binding(uint8_t layer, uint8_t slot, keyremap_binding_t *binding);
bool config_set_binding(uint8_t layer, uint8_t slot, const keyremap_binding_t *binding);
bool config_save(void);
void config_reset_default(void);
const keyremap_binding_t *config_find_binding(uint8_t input_kind, uint8_t input_code);
