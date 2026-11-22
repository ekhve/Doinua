/* SPDX-License-Identifier: GPL-3.0-only */
/* image.c (D-028): kit_image draws a run-length coded, palette-indexed image
 *   - the right colours at the right pixels; the transparent index leaves what was there
 *   - at scale 2 every pixel is a 2x2 block
 *   - clipped at every edge, never writing outside the framebuffer
 *   - the dirty rows cover the image; a malformed image (a zero run) stops instead of looping */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

static uint16_t mem[KIT_W * KIT_H + 2 * 64];          /* the framebuffer with a guard on each side */
uint16_t *kit__fb = mem + 64;
static int d0 = 9999, d1 = -1;
void kit__dirty(int y0, int y1) { if (y0 < d0) d0 = y0; if (y1 > d1) d1 = y1; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define PX(x, y) kit__fb[(y) * KIT_W + (x)]

/* 3x2:  A . B
 *       B B A      (index 0 transparent, 1 = A, 2 = B) */
static const kit_color_t pal[] = { 0, 0xF800, 0x07E0 };
static const uint8_t rle[] = { 1, 1, 1, 0, 3, 2, 1, 1 };
static const kit_image_t img = { 3, 2, pal, rle, 0, 0 };
/* the same 3x2 LZ coded (D-089): six literals; and a 4x2 whose second row copies the first (4 back) */
static const uint8_t lz1[] = { 5, 1, 0, 2, 2, 2, 1 };
static const kit_image_t img_lz = { 3, 2, pal, lz1, 0, 1 };
static const uint8_t lz2[] = { 3, 1, 2, 1, 2, 0x81, 4, 0 };
static const kit_image_t img_lz2 = { 4, 2, pal, lz2, -1, 1 };
static const uint8_t lz3[] = { 0, 1, 0x80, 9, 0 };      /* a copy from before the start: malformed */
static const kit_image_t img_lz3 = { 4, 2, pal, lz3, -1, 1 };

static void fill(uint16_t v) { int i; for (i = 0; i < (int)(sizeof mem / 2); i++) mem[i] = v; }
static int guards(void)
{
    int i;
    for (i = 0; i < 64; i++)
        if (mem[i] != 0x1234 || mem[64 + KIT_W * KIT_H + i] != 0x1234) return 0;
    return 1;
}

int main(void)
{
    static const uint8_t bad[] = { 2, 1, 0, 0 };
    static const kit_image_t broken = { 3, 2, pal, bad, -1, 0 };
    fill(0x1234);
    kit_image(10, 20, &img, 1);
    CHECK(PX(10, 20) == 0xF800 && PX(12, 20) == 0x07E0, "row 0: A and B");
    CHECK(PX(11, 20) == 0x1234, "the transparent pixel keeps what was there");
    CHECK(PX(10, 21) == 0x07E0 && PX(11, 21) == 0x07E0 && PX(12, 21) == 0xF800, "row 1: B B A (a run across the row end)");
    CHECK(PX(13, 20) == 0x1234 && PX(10, 22) == 0x1234, "nothing outside the image");
    CHECK(d0 == 20 && d1 == 21, "the dirty rows are the image's (%d..%d)", d0, d1);

    fill(0x1234);
    kit_image(50, 50, &img, 2);
    CHECK(PX(50, 50) == 0xF800 && PX(51, 51) == 0xF800, "scale 2: A is a 2x2 block");
    CHECK(PX(52, 50) == 0x1234 && PX(53, 51) == 0x1234, "scale 2: the transparent block too");
    CHECK(PX(55, 53) == 0xF800 && PX(56, 53) == 0x1234, "scale 2: the last pixel ends at x 55");

    fill(0x1234);
    kit_image(-2, -1, &img, 3);
    kit_image(KIT_W - 2, KIT_H - 2, &img, 4);
    kit_image(-500, 100, &img, 8);
    CHECK(guards(), "clipped at every edge: nothing written outside the framebuffer");
    CHECK(PX(0, 0) == 0xF800, "the visible part of a clipped image is drawn");

    fill(0x1234);
    kit_image(0, 0, &broken, 1);
    CHECK(PX(0, 0) == 0xF800 && PX(2, 0) == 0x1234, "a zero run stops the image where it is");

    fill(0x1234);                                 /* D-089: LZ coded */
    kit_image(10, 20, &img_lz, 1);
    CHECK(PX(10, 20) == 0xF800 && PX(11, 20) == 0x1234 && PX(12, 20) == 0x07E0 && PX(10, 21) == 0x07E0 && PX(12, 21) == 0xF800, "LZ: literals draw as run-length does");
    fill(0x1234);
    kit_image(0, 0, &img_lz2, 1);
    CHECK(PX(0, 1) == 0xF800 && PX(1, 1) == 0x07E0 && PX(2, 1) == 0xF800 && PX(3, 1) == 0x07E0, "LZ: a copy repeats the row above");
    fill(0x1234);
    kit_image(0, 0, &img_lz3, 1);
    CHECK(PX(0, 0) == 0xF800 && PX(1, 0) == 0x1234 && guards(), "LZ: a copy from before the start stops there");
    printf("%s: image, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
