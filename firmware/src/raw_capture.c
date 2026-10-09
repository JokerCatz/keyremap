#include "raw_capture.h"

#include <string.h>

#define RAW_RING_SIZE 64

static raw_report_t ring[RAW_RING_SIZE];
static uint32_t latest_seq;
static raw_interface_t interfaces[RAW_CAPTURE_INSTANCE_COUNT];

void raw_capture_init(void) {
  memset(ring, 0, sizeof(ring));
  memset(interfaces, 0, sizeof(interfaces));
  latest_seq = 0;
}

void raw_capture_mount(uint8_t instance, uint8_t itf_protocol, uint8_t protocol_mode, const uint8_t *desc, uint16_t desc_len) {
  if (instance >= RAW_CAPTURE_INSTANCE_COUNT) {
    return;
  }

  raw_interface_t *itf = &interfaces[instance];
  itf->mounted = true;
  itf->itf_protocol = itf_protocol;
  itf->protocol_mode = protocol_mode;
  itf->desc_len = desc_len > RAW_CAPTURE_DESC_MAX ? RAW_CAPTURE_DESC_MAX : desc_len;
  if (desc && itf->desc_len) {
    memcpy(itf->desc, desc, itf->desc_len);
  }
}

void raw_capture_unmount(uint8_t instance) {
  if (instance < RAW_CAPTURE_INSTANCE_COUNT) {
    memset(&interfaces[instance], 0, sizeof(interfaces[instance]));
  }
}

void raw_capture_unmount_all(void) {
  memset(interfaces, 0, sizeof(interfaces));
}

void raw_capture_report(uint8_t instance, const uint8_t *data, uint16_t len) {
  latest_seq++;
  raw_report_t *entry = &ring[latest_seq % RAW_RING_SIZE];
  entry->seq = latest_seq;
  entry->instance = instance;
  entry->truncated = len > RAW_CAPTURE_DATA_MAX;
  entry->len = (uint8_t)(entry->truncated ? RAW_CAPTURE_DATA_MAX : len);
  memcpy(entry->data, data, entry->len);
}

const raw_interface_t *raw_capture_interface(uint8_t instance) {
  return instance < RAW_CAPTURE_INSTANCE_COUNT ? &interfaces[instance] : NULL;
}

const raw_report_t *raw_capture_next(uint32_t after, bool *dropped) {
  *dropped = false;
  if (after >= latest_seq) {
    return NULL;
  }

  uint32_t oldest = latest_seq >= RAW_RING_SIZE ? latest_seq - RAW_RING_SIZE + 1 : 1;
  uint32_t want = after + 1;
  if (want < oldest) {
    want = oldest;
    *dropped = true;
  }

  return &ring[want % RAW_RING_SIZE];
}

uint32_t raw_capture_latest_seq(void) {
  return latest_seq;
}
