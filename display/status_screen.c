/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Status screen for the left half's 128x128 OLED.
 *
 * Top row: four modifier indicators (shift, cmd, alt, ctrl). Modifier
 * state lives on the central; sync/state_sync.c forwards it here over the
 * split link and this screen just reads the result.
 *
 * Below: one bar per left-hand column that jumps when a key in that column
 * is pressed and decays back down. The peripheral raises position events
 * for its own matrix locally, so this needs no data from the central.
 *
 * Built from plain rectangles only: no canvas, no fonts, no theme-supplied
 * styles, and nothing allocated after the screen is created. Every LVGL
 * object is touched exclusively from the display thread's timer.
 */

#include <zephyr/kernel.h>

#include <lvgl.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

/*
 * Declared rather than included: zmk/display/status_screen.h lives under the
 * ZMK app's include directory, which is not on an external module's include
 * path. This must stay in step with that header.
 */
lv_obj_t *zmk_display_status_screen(void);

extern volatile uint8_t dilemma_synced_mods;

#define TICK_MS 50

/* Modifier indicators: left to right shift, cmd, alt, ctrl. Each mask
 * covers the left and right variant of the modifier. */
#define MOD_COUNT 4
#define MOD_W 24
#define MOD_GAP 8
#define MOD_H_ACTIVE 26
#define MOD_H_IDLE 3
#define MOD_Y 8
static const uint8_t mod_masks[MOD_COUNT] = {
    0x22, // shift: LSFT | RSFT
    0x88, // cmd:   LGUI | RGUI
    0x44, // alt:   LALT | RALT
    0x11, // ctrl:  LCTL | RCTL
};
static lv_obj_t *mod_rects[MOD_COUNT];

/* One bar per left-hand column. */
#define BAR_COUNT 5
#define BAR_W 18
#define BAR_GAP 6
#define BAR_MIN 4
#define BAR_MAX 74
#define BAR_BOTTOM_MARGIN 6
#define BAR_DECAY 5 /* pixels per tick */
static lv_obj_t *bar_rects[BAR_COUNT];

/* Written from the position-event listener, consumed by the LVGL timer. */
static volatile uint8_t bar_levels[BAR_COUNT];

static int position_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Rows are 10 wide across both halves; this half owns columns 0-4 of
     * rows 0-2 and thumb positions 30-32, which map onto the inner
     * columns. */
    uint32_t col;
    if (ev->position < 30) {
        col = ev->position % 10;
        if (col > 4) {
            return ZMK_EV_EVENT_BUBBLE;
        }
    } else if (ev->position <= 32) {
        col = ev->position - 30 + 2;
    } else {
        return ZMK_EV_EVENT_BUBBLE;
    }

    bar_levels[col] = BAR_MAX;
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(oled_bars, position_listener);
ZMK_SUBSCRIPTION(oled_bars, zmk_position_state_changed);

static lv_obj_t *solid_rect(lv_obj_t *parent, int32_t w, int32_t h) {
    lv_obj_t *rect = lv_obj_create(parent);

    lv_obj_remove_style_all(rect);
    lv_obj_set_style_bg_color(rect, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(rect, w, h);

    return rect;
}

static void screen_tick(lv_timer_t *timer) {
    uint8_t mods = dilemma_synced_mods;

    for (int i = 0; i < MOD_COUNT; i++) {
        lv_obj_set_height(mod_rects[i], (mods & mod_masks[i]) ? MOD_H_ACTIVE : MOD_H_IDLE);
    }

    for (int i = 0; i < BAR_COUNT; i++) {
        uint8_t level = bar_levels[i];
        if (level > BAR_MIN) {
            level = level > BAR_MIN + BAR_DECAY ? level - BAR_DECAY : BAR_MIN;
            bar_levels[i] = level;
        }
        lv_obj_set_height(bar_rects[i], MAX(level, BAR_MIN));
    }
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    int mod_x0 = (128 - (MOD_COUNT * MOD_W + (MOD_COUNT - 1) * MOD_GAP)) / 2;
    for (int i = 0; i < MOD_COUNT; i++) {
        mod_rects[i] = solid_rect(screen, MOD_W, MOD_H_IDLE);
        lv_obj_set_pos(mod_rects[i], mod_x0 + i * (MOD_W + MOD_GAP), MOD_Y);
    }

    int bar_x0 = (128 - (BAR_COUNT * BAR_W + (BAR_COUNT - 1) * BAR_GAP)) / 2;
    for (int i = 0; i < BAR_COUNT; i++) {
        bar_rects[i] = solid_rect(screen, BAR_W, BAR_MIN);
        bar_levels[i] = BAR_MIN;
        /* Bottom-left alignment keeps the bottom edge pinned while the
         * timer changes the height, so bars grow upward. */
        lv_obj_set_align(bar_rects[i], LV_ALIGN_BOTTOM_LEFT);
        lv_obj_set_pos(bar_rects[i], bar_x0 + i * (BAR_W + BAR_GAP), -BAR_BOTTOM_MARGIN);
    }

    lv_timer_create(screen_tick, TICK_MS, NULL);

    return screen;
}
