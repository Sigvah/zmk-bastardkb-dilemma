# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_DILEMMA_OLED_SCREEN)
  zephyr_library_named(dilemma_oled_screen)
  zephyr_library_sources(status_screen.c)
endif()
