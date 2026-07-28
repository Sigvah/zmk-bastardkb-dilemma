# maXTouch input driver

Vendored from
[void-veritas/maxtouch-zephyr-module](https://github.com/void-veritas/maxtouch-zephyr-module)
at commit `2f17a39b46b3dca382e385b68723540b842b1945`, a fork of
[george-norton/maxtouch-zephyr-module](https://github.com/george-norton/maxtouch-zephyr-module)
updated to report relative motion so it works with mainline ZMK pointing.

Vendored (rather than pulled in via `west.yml`) because the upstream module
also ships a stale `peacock_vik_module` shield whose Kconfig references
symbols that only exist on the old `feat/pointers-move-scroll-ptp` ZMK
branch, which breaks the Kconfig stage on ZMK main.
