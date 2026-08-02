# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_DILEMMA_STATE_SYNC)
  zephyr_library_named(dilemma_state_sync)
  zephyr_library_sources(state_sync.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}/../zmk/app/include)
endif()
