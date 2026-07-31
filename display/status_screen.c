/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Custom ZMK status screen for the 128x128 OLED on the left half.
 *
 * The left half is the split peripheral, so the state the stock status
 * screen shows (active layer, output/endpoint, HID indicators) is only
 * known on the central and never reaches this display. Rather than render
 * widgets that would sit empty, this screen is self-contained: a title and
 * a bank of bars animated by LVGL, which needs no keyboard state at all.
 */

#include <lvgl.h>

/*
 * Declared rather than included: zmk/display/status_screen.h lives under the
 * ZMK app's include directory, which is not on an external module's include
 * path. This must stay in step with that header.
 */
lv_obj_t *zmk_display_status_screen(void);

#define BAR_COUNT 9
#define BAR_WIDTH 10
#define BAR_GAP 3
#define BAR_MIN_HEIGHT 6
#define BAR_BOTTOM_MARGIN 10
#define BAR_LEFT_MARGIN 4

static void bar_set_height(void *var, int32_t value) {
    lv_obj_set_height((lv_obj_t *)var, value);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "DILEMMA");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *rule = lv_obj_create(screen);
    lv_obj_set_size(rule, 96, 2);
    lv_obj_set_style_bg_color(rule, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rule, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(rule, 0, LV_PART_MAIN);
    lv_obj_align(rule, LV_ALIGN_TOP_MID, 0, 30);

    for (int i = 0; i < BAR_COUNT; i++) {
        lv_obj_t *bar = lv_obj_create(screen);

        lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);

        lv_obj_set_width(bar, BAR_WIDTH);
        lv_obj_set_height(bar, BAR_MIN_HEIGHT);

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
        lv_anim_set_values(&anim, BAR_MIN_HEIGHT, 68 - (i % 4) * 12);
        lv_anim_set_duration(&anim, 420 + i * 130);
        lv_anim_set_playback_duration(&anim, 380 + i * 90);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&anim);
    }

    return screen;
}
