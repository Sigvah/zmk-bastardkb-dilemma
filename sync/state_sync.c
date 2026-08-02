/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Push modifier state from the central to the peripheral.
 *
 * The split protocol carries no modifier state, but it does forward
 * behavior invocations by name. This file is both ends of that channel:
 * on the central, a listener watches the HID modifier byte and invokes the
 * state_sync behavior at the peripheral with the mods as its parameter; on
 * the peripheral, the behavior's handler stores them for the screen to
 * read. The behavior driver is compiled on both halves so the name
 * resolves everywhere.
 */

#define DT_DRV_COMPAT dilemma_behavior_state_sync

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

/* Written by the behavior handler on the peripheral, read by the screen. */
volatile uint8_t dilemma_synced_mods;

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    dilemma_synced_mods = (uint8_t)binding->param1;
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api state_sync_api = {
    .binding_pressed = on_pressed,
    .binding_released = on_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &state_sync_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY */

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/split/central.h>

static uint8_t last_sent_mods = 0xFF; /* force one send at startup */

static void send_mods(struct k_work *work) {
    uint8_t mods = zmk_hid_get_explicit_mods();

    if (mods == last_sent_mods) {
        return;
    }
    last_sent_mods = mods;

    struct zmk_behavior_binding binding = {
        .behavior_dev = "state_sync",
        .param1 = mods,
        .param2 = 0,
    };
    struct zmk_behavior_binding_event event = {
        .position = 0,
        .timestamp = k_uptime_get(),
        .source = 0,
    };

    zmk_split_central_invoke_behavior(0, &binding, event, true);
}

static K_WORK_DELAYABLE_DEFINE(mods_work, send_mods);

static int mod_sync_listener(const zmk_event_t *eh) {
    /*
     * The HID report is updated in the same event chain; a short debounce
     * lets it settle and coalesces rolls into one UART message.
     */
    k_work_reschedule(&mods_work, K_MSEC(15));
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(mod_sync, mod_sync_listener);
ZMK_SUBSCRIPTION(mod_sync, zmk_keycode_state_changed);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
