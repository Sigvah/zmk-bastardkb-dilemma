/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Conway's Game of Life on the left half's 128x128 OLED.
 *
 * The left half is the split peripheral, so no layer or output state ever
 * reaches this display; the screen is fully self-contained. A 32x32
 * toroidal world is stepped by an LVGL timer and drawn as 4x4 pixel cells
 * into an I1 canvas whose buffer lives in static memory, so rendering
 * never touches the LVGL heap and involves no fonts or theme styling.
 */

#include <zephyr/kernel.h>

#include <lvgl.h>

/*
 * Declared rather than included: zmk/display/status_screen.h lives under the
 * ZMK app's include directory, which is not on an external module's include
 * path. This must stay in step with that header.
 */
lv_obj_t *zmk_display_status_screen(void);

#define GRID 32
#define CELL 4 /* pixels per cell edge; GRID * CELL == panel resolution */
#define CANVAS_PX (GRID * CELL)
#define STRIDE (CANVAS_PX / 8) /* bytes per canvas row at 1bpp */
#define PALETTE_BYTES 8        /* I1: two lv_color32_t palette entries */
#define STEP_MS 150
#define STAGNANT_GENS 24 /* period <= 2 for this many steps -> reseed */

static uint8_t canvas_buf[LV_CANVAS_BUF_SIZE(CANVAS_PX, CANVAS_PX, 1, 1)];

/* World plus one generation of history for period-1/2 stagnation checks. */
static uint8_t world[GRID][GRID];
static uint8_t scratch[GRID][GRID];
static uint8_t prev2[GRID][GRID];
static uint16_t stagnant_count;

static lv_obj_t *canvas;

static uint32_t rng_state;

static uint32_t rng_next(void) {
    /* xorshift32: tiny and plenty for soup seeding */
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void seed_world(void) {
    /*
     * Random soup at ~1/3 density across the middle of the grid, leaving a
     * quiet border so early activity does not immediately wrap around the
     * torus and collide with itself.
     */
    memset(world, 0, sizeof(world));
    for (int y = 3; y < GRID - 3; y++) {
        for (int x = 3; x < GRID - 3; x++) {
            world[y][x] = (rng_next() % 3) == 0;
        }
    }
    stagnant_count = 0;
}

static void step_world(void) {
    for (int y = 0; y < GRID; y++) {
        int up = (y + GRID - 1) % GRID, down = (y + 1) % GRID;
        for (int x = 0; x < GRID; x++) {
            int left = (x + GRID - 1) % GRID, right = (x + 1) % GRID;
            int n = world[up][left] + world[up][x] + world[up][right] + world[y][left] +
                    world[y][right] + world[down][left] + world[down][x] + world[down][right];
            scratch[y][x] = n == 3 || (n == 2 && world[y][x]);
        }
    }
}

static void draw_world(void) {
    /*
     * Write the I1 canvas buffer directly: an 8 byte palette header, then
     * row-major rows of STRIDE bytes, most significant bit leftmost. Each
     * cell row expands to a nibble pattern repeated for CELL panel rows.
     */
    uint8_t *rows = canvas_buf + PALETTE_BYTES;

    for (int cy = 0; cy < GRID; cy++) {
        uint8_t line[STRIDE];
        for (int b = 0; b < STRIDE; b++) {
            /* two cells per byte at 4px per cell */
            uint8_t hi = world[cy][b * 2] ? 0xF0 : 0x00;
            uint8_t lo = world[cy][b * 2 + 1] ? 0x0F : 0x00;
            line[b] = hi | lo;
        }
        for (int r = 0; r < CELL; r++) {
            memcpy(rows + (cy * CELL + r) * STRIDE, line, STRIDE);
        }
    }

    lv_obj_invalidate(canvas);
}

static void life_tick(lv_timer_t *timer) {
    step_world();

    bool period1 = memcmp(scratch, world, sizeof(world)) == 0;
    bool period2 = memcmp(scratch, prev2, sizeof(world)) == 0;
    if (period1 || period2) {
        if (++stagnant_count >= STAGNANT_GENS) {
            seed_world();
            draw_world();
            return;
        }
    } else {
        stagnant_count = 0;
    }

    memcpy(prev2, world, sizeof(world));
    memcpy(world, scratch, sizeof(world));
    draw_world();
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_PX, CANVAS_PX, LV_COLOR_FORMAT_I1);
    lv_canvas_set_palette(canvas, 0, (lv_color32_t){.red = 0, .green = 0, .blue = 0, .alpha = 0xFF});
    lv_canvas_set_palette(canvas, 1,
                          (lv_color32_t){.red = 0xFF, .green = 0xFF, .blue = 0xFF, .alpha = 0xFF});
    lv_obj_center(canvas);

    /*
     * Boot-time cycle counter as the seed: varies run to run without
     * needing an RNG driver, and zero is unreachable so xorshift cannot
     * lock up.
     */
    rng_state = k_cycle_get_32() | 1;

    seed_world();
    memcpy(prev2, world, sizeof(world));
    draw_world();

    lv_timer_create(life_tick, STEP_MS, NULL);

    return screen;
}
