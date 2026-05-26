#include "mapper.h"

#include "config.h"
#include "keyremap_protocol.h"

void mapper_init(void) {
}

bool mapper_process(const input_event_t *input, output_event_t *output) {
  const keyremap_binding_t *binding = config_find_binding(input->kind, input->code);
  if (!binding) {
    output->kind = OUTPUT_KIND_NONE;
    output->code = 0;
    output->value = 0;
    return false;
  }

  output->kind = binding->output_kind;
  output->code = binding->output_code;
  output->value = (int16_t)((input->value * binding->scale) / 1000);
  return true;
}
