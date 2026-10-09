# Firmware

## Build Stack

- Pico SDK
- TinyUSB device and host stacks
- PIO program for the on-board WS2812 status LED
- Pico-PIO-USB host stack for the handle side

## Implemented

- Buildable RP2040-Zero firmware targets
- WS2812 status LED on GPIO16
- USB keyboard, mouse, and vendor HID configuration interfaces
- Static web UI can connect and request firmware info
- PIO USB host port on GPIO2/GPIO3
- Known handle VID/PID match: `1c4f:007c`
- Input reports parsed from each interface's HID report descriptor (boot
  keyboard/mouse interfaces use the fixed boot layout)
- Raw report and event logs readable over the config channel
- Keyboard, mouse, and consumer-control output reports
- Flash-backed config storage
- Four remap layers
- Host-only probe firmware for hardware diagnosis

Current flashed firmware enumerates as:

```text
cafe:4020 keyremap RP2040-Zero Keyremap Config
```

Linux exposes four HID interfaces: keyboard, mouse, consumer control, and vendor
config. The WebHID page filters for the vendor-defined config interface.

The debug UI includes a release-all command. Use it if a simulated key or mouse
button remains held during testing.

Flash config is stored in the last 4096-byte flash sector. Web edits update RAM
first; the `Save` button commits the current config to flash.

Layer count is fixed at four. Unbound inputs fall through to the base layer,
and unbound base inputs pass through unchanged, so an empty config behaves like
a plain USB pass-through. Layer actions: hold-to-switch, switch to layer
`0..3`, and next-layer cycling. The on-board WS2812 indicates the active layer.
See [protocol.md](protocol.md#remap-rules).

Host-side unit tests for the HID descriptor parser:

```sh
make test-firmware
```

## UF2 Flashing

To enter the ROM bootloader:

```text
hold BOOT
plug in USB-C, or tap RESET while holding BOOT
```

The computer should show a mass-storage device named like `RPI-RP2`. Copy the
generated `.uf2` file to it. The board reboots into the new firmware.

## Build

Install or clone Pico SDK, then build:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S firmware -B build/firmware
cmake --build build/firmware
```

Output:

```text
build/firmware/keyremap.uf2
```

The repository also has a Makefile wrapper:

```sh
make doctor
make build
make flash
```

The static WebHID UI can be served locally with:

```sh
make web-start
make web-open
make web-stop
```

Host-only probe firmware:

```sh
make build-host-probe
make flash-host-probe
```

`host_probe.uf2` disables the normal WebHID/remap device and only tests the
PIO USB host port on GPIO2/GPIO3. After flashing it, use BOOTSEL to return to
the normal firmware.

Host probe LED patterns:

```text
blue then green once   boot marker
1 blue blink           no USB line state
2 green blinks         USB line detected, not mounted
3 orange blinks        USB device mounted, not target HID yet
solid green            target handle HID mounted
4 red blinks           probe error
```

Override the SDK path if needed:

```sh
make PICO_SDK_PATH=$HOME/other/path/pico-sdk build
```

## Design Choice: WebHID First

The web UI is static and can run on GitHub Pages. WebHID is used first because
the config channel is small, request/response oriented, and naturally fits a HID
remapper. WebUSB can still be added later if bulk transfers become useful.

Browser target:

- Chrome / Edge first
- Firefox / Safari are not primary targets for this project

## Host Input Status

The current firmware can remap simulated input from the WebHID debug UI and send
real keyboard, mouse, and consumer-control output to the computer. It does not
yet read the handle through the soldered Type-A port.

The planned handle-side wiring is:

```text
PIO USB host D+ -> GPIO2
PIO USB host D- -> GPIO3
```

After soldering, the next firmware milestone is to integrate PIO USB host input
and translate the handle reports into the existing normalized input events.
