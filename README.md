# ZMK BastardKB Dilemma v3 Firmware

> [!IMPORTANT]
> This config builds against mainline ZMK, which supports full duplex wired split comms (half-duplex support is still experimental).
> To satisfy the full duplex requirement, you will have to bodge a wire from `QP_RST` to the unconnected TRRS leg.

> [!CAUTION]
> Please refer to the [schematics of the BastardKB Dilemma v3](https://github.com/Bastardkb/Dilemma/tree/5f852b3897f45f490570cc9028ca9680d5b83355/3x5_3) to correctly identify the unconnected TRRS leg and `QP_RST` through-hole first before connecting them to avoid causing any electrical shorts.

## Procyon trackpad

The `procyon_vik` shield adds support for a [Procyon](https://github.com/george-norton/procyon)
trackpad connected to the VIK port of the right (central) half. The
`dilemma_right_procyon_*` artifacts from `build.yaml` include it.

- The maXTouch driver is vendored in `drivers/input/` (see the README there);
  it reports relative motion, so it works with mainline ZMK pointing. Multitouch
  gestures/absolute mode are not supported on mainline — clicks come from the
  existing `&mkp` keymap bindings and combos.
- The shield defaults to the 42x50 Procyon. For the 57x80 variant, adjust
  `sensor-width`/`sensor-height` in `boards/shields/procyon_vik/procyon_vik.overlay`.
- If the cursor axes are wrong, use the `swap-xy`, `invert-x` and `invert-y`
  devicetree flags on the trackpad node. Touch sensitivity can be tuned with the
  `touch_threshold` and `gain` properties (see
  `dts/bindings/input/microchip,maxtouch.yaml` for the full list).
