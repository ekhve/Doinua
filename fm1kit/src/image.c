/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit images (D-028, D-089): palette-indexed images, run-length or LZ coded, drawn into the framebuffer, clipped to the
 * screen, at an integer scale, with one palette index left transparent. tools/gen_images.py makes them from an
 * app's PNGs. Portable; factory/unit/test_image.c checks it. */
#include "kit_internal.h"

static int im_x, im_y, im_w, im_scale, im_col, im_row;
static const kit_image_t *im_cur;

static void im_put(int idx)                     /* the next pixel of the image, row after row */
{
    if (idx != im_cur->clear) {
        kit_color_t c = im_cur->pal[idx];
        int px = im_x + im_col * im_scale, py = im_y + im_row * im_scale, sx, sy;
        if (im_scale == 1) {                    /* the common case, one pixel each */
            if ((unsigned)py < KIT_H && (unsigned)px < KIT_W) kit__fb[py * KIT_W + px] = c;
        } else
            for (sy = 0; sy < im_scale; sy++) {
                int yy = py + sy;
                if ((unsigned)yy >= KIT_H)
                    continue;
                for (sx = 0; sx < im_scale; sx++) {
                    int xx = px + sx;
                    if ((unsigned)xx < KIT_W)
                        kit__fb[yy * KIT_W + xx] = c;
                }
            }
    }
    if (++im_col == im_w) { im_col = 0; im_row++; }   /* (no division a pixel: D-063) */
}

#define LZ_WIN 4096
static uint8_t lz_win[LZ_WIN];                  /* the last 4096 indices, for the copies */

void kit_image(int x, int y, const kit_image_t *im, int scale)
{
    const uint8_t *p = im->rle;
    int n = im->w * im->h, i = 0;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    im_x = x; im_y = y; im_w = im->w; im_scale = scale; im_col = 0; im_row = 0; im_cur = im;
    if (!im->lz) {
        while (i < n) {
            int run = p[0], idx = p[1], k;
            p += 2;
            if (run == 0)
                break;                          /* (a malformed image: stop rather than loop) */
            for (k = 0; k < run && i < n; k++, i++)
                im_put(idx);
        }
    } else {
        while (i < n) {                         /* D-089: literals, and copies of what came before */
            int b = *p++, k;
            if (b < 0x80) {
                for (k = 0; k <= b && i < n; k++, i++) { lz_win[i & (LZ_WIN - 1)] = *p; im_put(*p++); }
            } else {
                int len = (b & 0x7F) + 3, off = p[0] | p[1] << 8;
                p += 2;
                if (off < 1 || off > LZ_WIN || off > i)
                    break;                      /* (a malformed image) */
                for (k = 0; k < len && i < n; k++, i++) {
                    int v = lz_win[(i - off) & (LZ_WIN - 1)];
                    lz_win[i & (LZ_WIN - 1)] = (uint8_t)v;
                    im_put(v);
                }
            }
        }
    }
    kit__dirty(y, y + im->h * scale - 1);
}
