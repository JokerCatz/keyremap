# Debug and Capture Workflow

The goal is to make the final device testable from the web page immediately
after the handle is connected.

## Before Soldering

These parts can be developed and tested without the Type-A host port:

- WebHID connection
- Firmware protocol
- Config/layer state
- Remap pipeline
- Simulated input events
- Output event state
- Linux-side event capture from the handle while it is still plugged into the PC

## Capture Current Handle Events on Linux

The handle currently appears as:

```text
event21 keyboard
event22 mouse
event23 consumer control
event24 system control
```

Record events:

```sh
tools/record_linux_input.py --list
sudo tools/record_linux_input.py --seconds 300
```

or:

```sh
make list-handle
make record-handle
make summarize-handle
```

Override the duration:

```sh
make record-handle RECORD_SECONDS=60
```

Default outputs:

```text
captures/input-events.jsonl
captures/input-summary.json
```

During capture, press each handle button once, move the stick/pointing control,
and scroll if the device has wheel input. Each line is JSON:

```json
{"device":"/dev/input/event21","type_name":"EV_KEY","code_name":"KEY_A","value":1}
```

Value meanings for key/button events:

```text
1 pressed
0 released
2 repeat
```

Relative mouse events use signed movement values.

## Firmware Debug Mode

The web UI 偵測 (Monitor) tab shows, live:

- raw HID reports exactly as the handle sent them (`0x20`)
- each parsed input event and what it was remapped to (`0x34`)

The 裝置 (Device) tab shows each handle interface and its decoded report
descriptor (`0x21`, `0x22`). Use these to check whether a problem is on the
input side (no raw report, or a raw report that decodes to nothing) or on the
remap side.

`0x40 simulate input` injects an input event into the remap pipeline.
