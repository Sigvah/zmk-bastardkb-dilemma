/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Paint the underglow to match the active layer.
 *
 * Runs on the central half only: layer state never reaches the peripheral,
 * but the rgb_ug behavior has global locality, so invoking it here executes
 * on this half and is forwarded to the peripheral over the split link.
 * Both halves therefore change color together, and doing it as behavior
 * invocations also sidesteps the persisted-to-flash underglow state, which
 * would otherwise override static configuration on every boot.
 */

#include <zephyr/kernel.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include <dt-bindings/zmk/rgb.h>

struct layer_color {
    uint16_t h;
    uint8_t s;
    uint8_t b;
};

/* Indexed by layer: default, win, numbers, num_win, nav, fn, controls,
 * mouse, scrolly_moly. Layers beyond the table reuse the last entry. */
static const struct layer_color colors[] = {
    {180, 100, 60}, // MAC: teal
    {220, 100, 60}, // WIN: azure
    {40, 100, 60},  // NUM: amber
    {40, 100, 60},  // NUM (win): amber
    {280, 100, 60}, // NAV: purple
    {0, 100, 60},   // FN: red
    {60, 100, 60},  // CTL: yellow
    {140, 100, 60}, // MOUSE: spring green
    {320, 100, 60}, // SCROLL: magenta
};

static void invoke_rgb(uint32_t cmd, uint32_t param) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = "rgb_ug",
        .param1 = cmd,
        .param2 = param,
    };
    struct zmk_behavior_binding_event event = {
        .position = 0,
        .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    zmk_behavior_invoke_binding(&binding, event, true);
}

static void apply_layer_color(struct k_work *work) {
    static uint32_t last_sent = UINT32_MAX;
    static bool effect_sent;

    zmk_keymap_layer_index_t layer = zmk_keymap_highest_layer_active();
    const struct layer_color *c =
        &colors[MIN(layer, (zmk_keymap_layer_index_t)(ARRAY_SIZE(colors) - 1))];
    uint32_t hsb = RGB_COLOR_HSB_VAL(c->h, c->s, c->b);

    /*
     * Skip no-op updates: every rgb_ug invocation schedules a settings
     * save on both halves, and on RP2040 those saves stall the chip (see
     * the SETTINGS_SAVE_DEBOUNCE note in the defconfigs), so only send
     * when something actually changes and select the solid effect once.
     */
    if (!effect_sent) {
        invoke_rgb(RGB_EFS_CMD, 0); // solid
        effect_sent = true;
    }
    if (hsb != last_sent) {
        last_sent = hsb;
        invoke_rgb(RGB_COLOR_HSB_CMD, hsb);
    }
}

static K_WORK_DELAYABLE_DEFINE(color_work, apply_layer_color);

static int layer_rgb_listener(const zmk_event_t *eh) {
    /*
     * Debounced off the event thread. Momentary layer keys flap quickly, and
     * each invocation crosses the split link, so coalesce bursts instead of
     * spamming the UART.
     */
    k_work_reschedule(&color_work, K_MSEC(50));
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_rgb, layer_rgb_listener);
ZMK_SUBSCRIPTION(layer_rgb, zmk_layer_state_changed);

static int layer_rgb_init(void) {
    /*
     * Paint the base layer once the split link is up. The peripheral needs a
     * moment to connect before a forwarded behavior can reach it.
     */
    k_work_schedule(&color_work, K_SECONDS(2));
    return 0;
}

SYS_INIT(layer_rgb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
