# Configuration Protocol

Transport: vendor-defined HID interface.

USB device shape:

```text
interface 0 keyboard HID output
interface 1 mouse HID output
interface 2 consumer-control HID output
interface 3 vendor HID config channel
```

Report size: 64 bytes.

Report ID: 0.

Byte order: little-endian.

Protocol version: 2.0. Version 2 removed `0x33 Get Input Event` and
`0x41 Get Output State` in favour of the raw report and event logs below; the
web UI refuses to talk to protocol 1 firmware.

## Frame

```text
byte 0      command
byte 1      sequence
byte 2      payload length N
byte 3      status for responses, reserved for requests
byte 4..63  payload
```

Status values:

```text
0x00 ok
0x01 unknown command
0x02 invalid length
0x03 busy
0x04 internal error
```

## Commands

### 0x01 Get Info

Request payload: empty.

Response payload:

```text
byte 0      protocol major
byte 1      protocol minor
byte 2      firmware major
byte 3      firmware minor
byte 4      firmware patch
byte 5      active layer
byte 6      host status
byte 7..    NUL-terminated board name
```

Current board name:

```text
rp2040-zero
```

### 0x02 Set LED

Temporary debug command for bring-up.

Request payload:

```text
byte 0 red
byte 1 green
byte 2 blue
```

Response payload: empty.

This command is for bench testing. Layer/status colors will be firmware-owned in
later milestones.

### 0x10 Get Config Summary

Request payload: empty.

Response payload:

```text
byte 0      profile count
byte 1      layer count
byte 2      binding slots per layer
byte 3      active profile
byte 4      active layer
byte 5..8   config generation
```

### 0x11 Get Binding

Request payload:

```text
byte 0 layer
byte 1 slot
```

Response payload:

```text
byte 0      layer
byte 1      slot
byte 2      input kind
byte 3      input code
byte 4      output kind
byte 5      output code
byte 6..7   signed int16 scale
```

### 0x12 Set Binding

Request payload matches the `Get Binding` response payload. The change is
applied to RAM immediately. It becomes persistent only after `Save Config`.

### 0x13 Save Config

Request payload: empty.

Writes the current config to the final flash sector.

### 0x14 Reset Config

Request payload: empty.

Restores default config (every layer empty) and writes it to flash.

## Remap Rules

For each input event the firmware looks up a binding in this order:

1. the active layer
2. the base layer (layer 0), when the active layer has no binding
3. identity passthrough (key stays key, mouse stays mouse, ...)

A binding whose output kind is `none` is treated as "no binding" and falls
through. Use output kind `block` to disable an input.

Digital inputs (keys, mouse buttons, consumer usages) remember the output they
resolved to when pressed; the release always goes to that same output, even if
the layer changed in between.

The active layer is the held layer while a `layer hold` key is down, otherwise
the base layer chosen by `layer`, `next layer`, or command `0x30`. It always
starts at layer 0 after reset.

### 0x20 Get Raw Reports

Returns raw HID input reports received from the handle, before any parsing.
The firmware keeps the last 64 reports, each truncated to 32 bytes.

Request payload:

```text
byte 0..3   return reports with sequence > this value
```

Response payload:

```text
byte 0      entry count
byte 1      flags, bit 0 = reports were overwritten before they were read
byte 2..5   latest report sequence
byte 6..    entries
```

Entry:

```text
byte 0..3   sequence
byte 4      host HID instance, bit 7 = data truncated
byte 5      data length N
byte 6..    N data bytes (includes the report ID byte when the interface uses IDs)
```

### 0x21 Get HID Interfaces

Response payload:

```text
byte 0      instance count (4)
then 6 bytes per instance:
byte 0      mounted
byte 1      interface protocol (0 none, 1 boot keyboard, 2 boot mouse)
byte 2      TinyUSB protocol mode
byte 3..4   report descriptor length
byte 5      parsed layout flags:
            bit 0 keyboard keys, bit 1 mouse buttons, bit 2 relative axes,
            bit 3 consumer usages, bit 7 fixed boot layout in use
```

Boot-subclass interfaces are switched to boot protocol and parsed with the fixed
boot layout. All other interfaces are parsed from their report descriptor.

### 0x22 Get Report Descriptor

Request payload:

```text
byte 0      instance
byte 1..2   offset
```

Response payload:

```text
byte 0      instance
byte 1..2   offset
byte 3..4   total descriptor length
byte 5..    up to 55 descriptor bytes
```

### 0x30 Set Active Layer

Request payload:

```text
byte 0 layer index
```

Response payload:

```text
byte 0 active layer
```

### 0x31 Get Layer State

Request payload: empty.

Response payload:

```text
byte 0 active layer
```

### 0x32 Get Host Status

Request payload: empty.

Response payload:

```text
byte 0 host status
byte 1 PIO USB line state
byte 2 full-speed flag
byte 3 PIO root connected flag
byte 4 PIO root suspended flag
byte 5 low byte of pending PIO root interrupts
byte 6..7 mounted VID, little-endian
byte 8..9 mounted PID, little-endian
```

Host status values:

```text
0 waiting/not mounted
1 target handle mounted
2 non-target HID mounted
3 device unmounted
4 report receive error
5 USB device mounted, but no target HID interface mounted
```

PIO USB line state values:

```text
0 SE0
1 full-speed idle
2 low-speed idle
3 SE1
```

### 0x34 Get Event Log

Returns parsed input events and the output each one was remapped to. The
firmware keeps the last 64 events.

Request payload:

```text
byte 0..3   return events with sequence > this value
```

Response payload:

```text
byte 0      entry count (at most 4)
byte 1      flags, bit 0 = events were overwritten before they were read
byte 2..5   latest event sequence
byte 6..    13-byte entries
```

Entry:

```text
byte 0..3   sequence
byte 4      input kind
byte 5      input code
byte 6..7   signed int16 input value
byte 8      output kind
byte 9      output code
byte 10..11 signed int16 output value
byte 12     active layer after the event, bit 7 = simulated input
```

### 0x40 Simulate Input

Request payload:

```text
byte 0      input kind
byte 1      input code
byte 2..3   signed int16 value
```

Input kinds:

```text
0x01 key
0x02 mouse button
0x03 relative x
0x04 relative y
0x05 wheel
0x06 consumer usage (code = usage, 0x01..0xff)
```

Output kinds:

```text
0x00 none
0x01 key
0x02 mouse button
0x03 relative x
0x04 relative y
0x05 wheel
0x06 layer
0x07 consumer control
0x08 next layer
0x09 layer hold
0x0a block
```

Consumer-control output currently covers media/volume usages such as volume
up/down, mute, play/pause, next track, and previous track.

Layer output switches the base layer to output code `0..3` on key press.
Next-layer output cycles the base layer `0 -> 1 -> 2 -> 3 -> 0`. Layer-hold
output makes layer `code` active while the key is held.

### 0x42 Release All

Request payload: empty.

Clears all currently pressed keyboard keys, modifiers, mouse buttons and any
held layer, then sends empty keyboard and mouse reports. The web debug UI uses this as an
emergency release command.

### 0x7f Reboot To BOOTSEL

Request payload: empty. Reboots into the UF2 bootloader.
