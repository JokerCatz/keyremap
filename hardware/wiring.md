# Wiring Draft

Initial bench wiring for the handle-side USB Type-A female connector:

```text
Type-A pin 1 VBUS  -> RP2040-Zero 5V/VBUS
Type-A pin 2 D-    -> GPIO3
Type-A pin 3 D+    -> GPIO2
Type-A pin 4 GND   -> GND
```

The D+/D- GPIO choice is now the project default for the PIO USB host port.

Do not plug a second computer or powered hub into this Type-A connector. It is
intended only for the target `1c4f:007c` handle.

Use short D+/D- wires for the final build. Keep the two signal wires together and
avoid routing them near noisy power wiring.
