# Hardware Notes

## Board

Assumed board: Waveshare RP2040-Zero or a compatible RP2040-Zero clone.

Observed/expected features:

- RP2040 MCU
- USB-C connector for the computer side
- BOOT and RESET buttons
- UF2 bootloader mass-storage flashing
- On-board WS2812 RGB LED on GPIO16

Sources checked:

- Waveshare RP2040-Zero product page: https://www.waveshare.com/product/rp2040-zero.htm
- Waveshare RP2040-Zero wiki: https://www.waveshare.com/wiki/RP2040-Zero
- NuttX board notes for RP2040-Zero LED GPIO16: https://nuttx.apache.org/docs/latest/platforms/arm/rp2040/boards/waveshare-rp2040-zero/index.html

## USB Topology

The intended topology is:

```text
JD-DZ.COM USB HANDLE
        |
  soldered USB Type-A female port
        |
 RP2040-Zero USB host side
        |
 remap/layer/config firmware
        |
 RP2040-Zero USB-C device side
        |
      computer
```

This is reasonable, but it requires two USB roles at once:

- Device role to the computer
- Host role to the handle

RP2040 has one native USB controller. For this board, the practical plan is:

- Native USB-C: device role to the computer
- PIO USB on GPIO pins: host role for the soldered Type-A port

## Type-A Host Port Wiring

The Type-A female connector for the handle needs:

```text
Type-A VBUS  -> board 5V/VBUS supply, with current awareness
Type-A GND   -> board GND
Type-A D+    -> selected PIO USB D+ GPIO
Type-A D-    -> selected PIO USB D- GPIO
```

Do not connect the Type-A D+/D- lines to the same USB-C D+/D- traces unless the
design intentionally shares the native USB port. This project needs a separate
PIO USB host pair so the computer connection can remain active.

Recommended pin choice:

```text
Type-A D+ -> RP2040-Zero GPIO2
Type-A D- -> RP2040-Zero GPIO3
```

Keep D+/D- short, routed together, and avoid long jumper wires for the final
build. For bench testing, short wires are acceptable.

The selected pair is intentionally separate from the board's USB-C connector.
The USB-C native USB remains the computer-side device port. GPIO2/GPIO3 are
reserved for the handle-side PIO USB host port.

## Power

The handle is bus-powered. The computer supplies power through the RP2040-Zero
USB-C port, and the Type-A VBUS line powers the handle.

Check current before final enclosure wiring. The RP2040-Zero product page lists
an on-board LDO, but the exact safe total budget depends on the clone and USB
source. The observed handle reports `MaxPower 100mA`, so it should be modest.

## LED

The firmware treats the on-board WS2812 as a status/layer indicator:

```text
off      booting or fault before LED init
green    host input active / base layer
purple   layer 1
cyan     layer 2
orange   layer 3
red      fault
```

M0 only implements basic boot/config colors. Layer colors are reserved for the
remapper milestone.
