#pragma once

#include <stdbool.h>
#include <stdint.h>

#define RAW_CAPTURE_INSTANCE_COUNT 4
#define RAW_CAPTURE_DATA_MAX 32
#define RAW_CAPTURE_DESC_MAX 512

typedef struct {
  uint32_t seq;
  uint8_t instance;
  uint8_t len;
  bool truncated;
  uint8_t data[RAW_CAPTURE_DATA_MAX];
} raw_report_t;

typedef struct {
  bool mounted;
  uint8_t itf_protocol;
  uint8_t protocol_mode;
  uint16_t desc_len;
  uint8_t desc[RAW_CAPTURE_DESC_MAX];
} raw_interface_t;

void raw_capture_init(void);
void raw_capture_mount(uint8_t instance, uint8_t itf_protocol, uint8_t protocol_mode, const uint8_t *desc, uint16_t desc_len);
void raw_capture_unmount(uint8_t instance);
void raw_capture_unmount_all(void);
void raw_capture_report(uint8_t instance, const uint8_t *data, uint16_t len);
const raw_interface_t *raw_capture_interface(uint8_t instance);

/* Returns the oldest stored report with seq > after, or NULL. Sets *dropped
 * when reports between `after` and the returned one were overwritten. */
const raw_report_t *raw_capture_next(uint32_t after, bool *dropped);
uint32_t raw_capture_latest_seq(void);
