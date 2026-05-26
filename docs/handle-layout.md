# Handle Layout

User-provided physical key pairs:

| Physical ID | Base event | Fn event |
|---|---|---|
| k01 | `KEY_1` | `KEY_5` |
| k05 | `KEY_2` | `KEY_6` |
| k06 | `KEY_3` | `KEY_7` |
| k04 | `KEY_4` | `KEY_8` |
| k09 | `KEY_ESC` | `KEY_ENTER` |
| k02 | `KEY_TAB` | `KEY_F1` |
| k07 | `KEY_B` | `KEY_T` |
| k08 | `KEY_G` | `KEY_H` |
| k10 | `KEY_M` | `KEY_F2` |
| k03 | `KEY_Y` | `KEY_N` |

Machine-readable source:

```text
config/handle-layout.json
```

These pairs identify the same physical key in the handle's built-in base and Fn
modes. The project can expose them in the web UI as one physical key with two
observed source events.
