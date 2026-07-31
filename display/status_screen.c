/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Custom ZMK status screen for the 128x128 OLED on the left half.
 *
 * The left half is the split peripheral, so the state the stock status
 * screen shows (active layer, output/endpoint, HID indicators) is only
 * known on the central and never reaches this display. This screen is
 * therefore self-contained and needs no keyboard state at all.
 *
 * Deliberately built from plain rectangles only. No labels, so no font is
 * involved in rendering, and nothing here depends on a theme having
 * supplied a default style.
 */

#include <lvgl.h>

/*
 * Declared rather than included: zmk/display/status_screen.h lives under the
 * ZMK app's include directory, which is not on an external module's include
 * path. This must stay in step with that header.
 */
lv_obj_t *zmk_display_status_screen(void);

#define BAR_COUNT 8
#define BAR_WIDTH 10
#define BAR_GAP 4
#define BAR_MIN_HEIGHT 8
#define BAR_MAX_HEIGHT 86
#define BAR_LEFT_MARGIN 9
#define BAR_BOTTOM_MARGIN 12

static lv_obj_t *solid_rect(lv_obj_t *parent, int32_t w, int32_t h) {
    lv_obj_t *rect = lv_obj_create(parent);

    lv_obj_remove_style_all(rect);
    lv_obj_set_style_bg_color(rect, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(rect, w, h);

    return rect;
}

static void bar_set_height(void *var, int32_t value) {
    lv_obj_set_height((lv_obj_t *)var, value);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* A rule across the top, so a static render is obvious even if the
     * animation below never ticks. */
    lv_obj_t *rule = solid_rect(screen, 110, 3);
    lv_obj_set_align(rule, LV_ALIGN_TOP_MID);
    lv_obj_set_pos(rule, 0, 14);

    for (int i = 0; i < BAR_COUNT; i++) {
        lv_obj_t *bar = solid_rect(screen, BAR_WIDTH, BAR_MIN_HEIGHT);

        /*
         * Set the alignment rather than calling lv_obj_align(), so that the
         * bottom edge stays pinned as the animation changes the height and
         * the bars grow upward instead of off the bottom of the panel.
         */
        lv_obj_set_align(bar, LV_ALIGN_BOTTOM_LEFT);
        lv_obj_set_pos(bar, BAR_LEFT_MARGIN + i * (BAR_WIDTH + BAR_GAP), -BAR_BOTTOM_MARGIN);

        /*
         * Deliberately mismatched durations, so the bars drift out of phase
         * with each other instead of moving as one block.
         */
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, bar);
        lv_anim_set_exec_cb(&anim, bar_set_height);
        lv_anim_set_values(&anim, BAR_MIN_HEIGHT, BAR_MAX_HEIGHT - (i % 4) * 16);
        lv_anim_set_duration(&anim, 420 + i * 130);
        lv_anim_set_playback_duration(&anim, 380 + i * 90);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&anim);
    }

    return screen;
}
