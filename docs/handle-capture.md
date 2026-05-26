# Handle Capture

Captured target:

```text
VID:PID 1c4f:007c
Name    JD-DZ.COM USB HANDLE
Mode    keyboard + mouse + consumer/system HID composite
```

Capture files:

```text
captures/base-events.jsonl
captures/base-summary.json
captures/fn-events.jsonl
captures/fn-summary.json
```

## Summary

- Base events: 34
- Fn events: 29
- Common events: 19

The captures identify the OS-level events produced by the handle in base and Fn
modes. They do not yet label each physical button. Physical-button labeling will
be done later through the web debug view after the RP2040 host port is active.

Known physical key pairs are documented in [handle-layout.md](handle-layout.md).

## Base

| Device | Type | Code | Count |
|---|---:|---|---:|
| event21 | EV_KEY | `KEY_1` | 4 |
| event21 | EV_KEY | `KEY_2` | 4 |
| event21 | EV_KEY | `KEY_3` | 4 |
| event21 | EV_KEY | `KEY_4` | 4 |
| event21 | EV_KEY | `KEY_A` | 6 |
| event21 | EV_KEY | `KEY_B` | 4 |
| event21 | EV_KEY | `KEY_C` | 6 |
| event21 | EV_KEY | `KEY_D` | 4 |
| event21 | EV_KEY | `KEY_E` | 6 |
| event21 | EV_KEY | `KEY_ESC` | 4 |
| event21 | EV_KEY | `KEY_F` | 6 |
| event21 | EV_KEY | `KEY_G` | 4 |
| event21 | EV_KEY | `KEY_LEFTALT` | 4 |
| event21 | EV_KEY | `KEY_LEFTCTRL` | 2 |
| event21 | EV_KEY | `KEY_LEFTSHIFT` | 6 |
| event21 | EV_KEY | `KEY_M` | 4 |
| event21 | EV_KEY | `KEY_Q` | 6 |
| event21 | EV_KEY | `KEY_R` | 6 |
| event21 | EV_KEY | `KEY_S` | 4 |
| event21 | EV_KEY | `KEY_SPACE` | 6 |
| event21 | EV_KEY | `KEY_TAB` | 4 |
| event21 | EV_KEY | `KEY_V` | 6 |
| event21 | EV_KEY | `KEY_W` | 5 |
| event21 | EV_KEY | `KEY_X` | 4 |
| event21 | EV_KEY | `KEY_Y` | 6 |
| event21 | EV_KEY | `KEY_Z` | 4 |
| event22 | EV_KEY | `BTN_LEFT` | 6 |
| event22 | EV_KEY | `BTN_MIDDLE` | 6 |
| event22 | EV_KEY | `BTN_RIGHT` | 6 |
| event22 | EV_KEY | `BTN_SIDE` | 6 |
| event22 | EV_REL | `REL_WHEEL` | 15 |
| event22 | EV_REL | `REL_WHEEL_HI_RES` | 15 |
| event22 | EV_REL | `REL_X` | 213 |
| event22 | EV_REL | `REL_Y` | 225 |

## Fn

| Device | Type | Code | Count |
|---|---:|---|---:|
| event21 | EV_KEY | `KEY_5` | 2 |
| event21 | EV_KEY | `KEY_6` | 2 |
| event21 | EV_KEY | `KEY_7` | 2 |
| event21 | EV_KEY | `KEY_8` | 2 |
| event21 | EV_KEY | `KEY_A` | 6 |
| event21 | EV_KEY | `KEY_D` | 6 |
| event21 | EV_KEY | `KEY_E` | 2 |
| event21 | EV_KEY | `KEY_ENTER` | 2 |
| event21 | EV_KEY | `KEY_F1` | 2 |
| event21 | EV_KEY | `KEY_F2` | 2 |
| event21 | EV_KEY | `KEY_H` | 2 |
| event21 | EV_KEY | `KEY_LEFTALT` | 2 |
| event21 | EV_KEY | `KEY_LEFTCTRL` | 2 |
| event21 | EV_KEY | `KEY_LEFTSHIFT` | 2 |
| event21 | EV_KEY | `KEY_N` | 2 |
| event21 | EV_KEY | `KEY_Q` | 2 |
| event21 | EV_KEY | `KEY_S` | 4 |
| event21 | EV_KEY | `KEY_SPACE` | 2 |
| event21 | EV_KEY | `KEY_T` | 2 |
| event21 | EV_KEY | `KEY_W` | 7 |
| event21 | EV_KEY | `KEY_X` | 2 |
| event21 | EV_KEY | `KEY_Z` | 2 |
| event22 | EV_KEY | `BTN_LEFT` | 2 |
| event22 | EV_KEY | `BTN_RIGHT` | 2 |
| event22 | EV_KEY | `BTN_SIDE` | 2 |
| event22 | EV_REL | `REL_WHEEL` | 5 |
| event22 | EV_REL | `REL_WHEEL_HI_RES` | 5 |
| event22 | EV_REL | `REL_X` | 152 |
| event22 | EV_REL | `REL_Y` | 127 |

## Notes

- `event21` is keyboard-like input.
- `event22` is mouse-like input.
- `REL_WHEEL_HI_RES` accompanies wheel movement on Linux and can be ignored by
  the RP2040 output path unless high-resolution scrolling is explicitly added.
- Fn appears to change the source device's own emitted key set. The remapper can
  still add independent layers on top of this.
