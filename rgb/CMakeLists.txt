# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_DILEMMA_LAYER_RGB)
  zephyr_library_named(dilemma_layer_rgb)
  zephyr_library_sources(layer_rgb.c)
  # ZMK does not export its app headers to modules; reach into the west
  # workspace the way community ZMK modules do.
  zephyr_library_include_directories(${ZEPHYR_BASE}/../zmk/app/include)
endif()
