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

Restores default config and writes it to flash.

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

### 0x33 Get Input Event

Request payload: empty.

Response payload:

```text
byte 0      input kind
byte 1      input code
byte 2..3   signed int16 value
byte 4..7   input event count
```

This reports the most recent raw input event observed from the USB handle before
remapping. Web UI polling uses the event count to detect changes.

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
```

Consumer-control output currently covers media/volume usages such as volume
up/down, mute, play/pause, next track, and previous track.

Layer output switches directly to output code `0..3` on key press. Next-layer
output cycles `0 -> 1 -> 2 -> 3 -> 0`.

### 0x41 Get Output State

Request payload: empty.

Response payload:

```text
byte 0      output kind
byte 1      output code
byte 2..3   signed int16 value
byte 4..7   output event count
```

### 0x42 Release All

Request payload: empty.

Clears all currently pressed keyboard keys, modifiers, and mouse buttons, then
sends empty keyboard and mouse reports. The web debug UI uses this as an
emergency release command.

## Future Commands

Reserved:

```text
0x10 read config chunk
0x11 write config chunk
0x12 commit config
0x13 reset config
0x20 get raw input report
0x30 set active layer
0x31 get layer state
0x7f reboot to UF2 bootloader
```
