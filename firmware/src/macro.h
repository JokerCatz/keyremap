#pragma once

#include <stdbool.h>

/* Types ASCII text through the keyboard output, one key per HID report pair.
 * Assumes a US keyboard layout on the computer. */
void macro_init(void);
void macro_type(const char *text);
bool macro_busy(void);
void macro_task(void);
