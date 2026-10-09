# keyremap

RP2040-Zero based USB HID remapper for one known keyboard/mouse hybrid handle.

Target input device currently observed on Linux:

- `1c4f:007c` SiGma Micro / JD-DZ.COM `USB HANDLE`
- Interfaces: boot keyboard, mouse, consumer control, system control

Target hardware:

- Waveshare-style RP2040-Zero
- USB-C port connects to the computer
- A soldered USB Type-A female port connects to the handle
- On-board WS2812 RGB LED on GPIO16 is used as status/layer indicator

## Current State

The current firmware is usable for the target handle:

- RP2040-Zero acts as a USB HID device to the computer
- PIO USB host on GPIO2/GPIO3 reads the handle
- Keyboard, mouse (stick/buttons/wheel) and consumer inputs are parsed from the
  handle's HID report descriptors and can be remapped
- Four layers: unbound inputs fall through to the base layer, and unbound base
  inputs pass through unchanged
- Layer switching is a binding: hold-to-switch, switch-to, or next-layer
- Config is edited from a static WebHID page and saved to RP2040 flash
- The web page shows raw HID reports and every input → output event live
- The on-board WS2812 indicates host/layer status

## Layout

- `firmware/` - RP2040 firmware using Pico SDK and TinyUSB
- `web/` - static WebHID configuration page, suitable for GitHub Pages
- `docs/` - hardware, firmware, and protocol notes
- `hardware/` - wiring notes and future board/case assets
- `tools/` - Linux-side capture/debug helpers

## Quick Commands

The Makefile defaults to `$(HOME)/sdk/pico-sdk`.

```sh
make doctor
make build
make flash
make test-firmware
```

Hosted Web UI: https://jokercatz.github.io/keyremap/

## Switching Layers

The default config has no layer key. In the web UI 綁定 tab, pick a key with
「按手把新增」 and set its output to e.g. 「按住時切到 Layer 1」. Then build the
Layer 1 bindings; anything not bound there keeps working as in Base.

Local Web UI:

```sh
make web-start
make web-open
make web-stop
```

The host-only hardware probe can be built with:

```sh
make build-host-probe
make flash-host-probe
```

`make flash` expects the RP2040-Zero to be in BOOTSEL mode as an `RPI-RP2`
USB mass-storage device.

On Linux, install the udev rule before using the WebHID page:

```sh
sudo cp udev/60-keyremap.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then unplug and replug the RP2040. See [docs/linux.md](docs/linux.md).

To record the currently connected handle before soldering the RP2040 host port:

```sh
sudo tools/record_linux_input.py --seconds 30
```

See [docs/debug.md](docs/debug.md).

Current base/Fn capture notes are in [docs/handle-capture.md](docs/handle-capture.md).
The current physical key pair map is in [docs/handle-layout.md](docs/handle-layout.md).
