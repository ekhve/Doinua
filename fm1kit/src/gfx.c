/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit graphics: a 240x240 RGB565 framebuffer (the backend owns the memory and the flush),
 * shapes and anti-aliased text from fonts rasterised at build time (tools/gen_font.py). Portable. */
#include "kit_internal.h"
#include "kit_font_data.h"      /* generated: KIT_FONT_DATA[] */

int kit__dirty_y0 = KIT_H, kit__dirty_y1 = -1;

void kit__dirty(int y0, int y1)
{
    if (y0 < 0) y0 = 0;
    if (y1 >= KIT_H) y1 = KIT_H - 1;
    if (y0 > y1) return;
    if (y0 < kit__dirty_y0) kit__dirty_y0 = y0;
    if (y1 > kit__dirty_y1) kit__dirty_y1 = y1;
}

void kit_rect(int x, int y, int w, int h, kit_color_t c)
{
    int i, j;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > KIT_W) w = KIT_W - x;
    if (y + h > KIT_H) h = KIT_H - y;
    if (w <= 0 || h <= 0) return;
    for (j = y; j < y + h; j++) {
        uint16_t *p = kit__fb + j * KIT_W + x;
        for (i = 0; i < w; i++)
            p[i] = c;
    }
    kit__dirty(y, y + h - 1);
}

void kit_clear(kit_color_t c) { kit_rect(0, 0, KIT_W, KIT_H, c); }

void kit_pixel(int x, int y, kit_color_t c)
{
    if ((unsigned)x >= KIT_W || (unsigned)y >= KIT_H) return;
    kit__fb[y * KIT_W + x] = c;
    kit__dirty(y, y);
}

void kit_rect_outline(int x, int y, int w, int h, int t, kit_color_t c)
{
    kit_rect(x, y, w, t, c);
    kit_rect(x, y + h - t, w, t, c);
    kit_rect(x, y, t, h, c);
    kit_rect(x + w - t, y, t, h, c);
}

void kit_circle(int cx, int cy, int r, kit_color_t c)
{
    int dy, dx = r;                             /* each row's half-width: the largest dx with dx*dx + dy*dy <= r*r, walked
                                                 * down from the middle row (the same pixels as searching each row from 0,
                                                 * at a fraction of the work) */
    if (r < 0) return;
    for (dy = 0; dy <= r; dy++) {
        while (dx > 0 && dx * dx + dy * dy > r * r) dx--;
        kit_rect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
        if (dy) kit_rect(cx - dx, cy - dy, 2 * dx + 1, 1, c);
    }
}

void kit_round_rect(int x, int y, int w, int h, int r, kit_color_t c)
{
    int dy;
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    kit_rect(x, y + r, w, h - 2 * r, c);
    for (dy = 0; dy < r; dy++) {
        int yy = r - dy, dx = 0;      /* distance from the corner centre's row */
        while ((dx + 1) * (dx + 1) + yy * yy <= r * r) dx++;
        kit_rect(x + r - dx, y + dy, w - 2 * r + 2 * dx, 1, c);
        kit_rect(x + r - dx, y + h - 1 - dy, w - 2 * r + 2 * dx, 1, c);
    }
}

void kit_line(int x0, int y0, int x1, int y1, kit_color_t c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {                                  /* Bresenham: both tests on the error as it was (taking it again after
                                                 * the x step ran most lines past their end, for ever) */
        kit_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* ---- text ---- */
static const kit_glyph_t *glyph(const kit_font_t *f, int ch)
{
    int lo = 0, hi = f->n - 1;
    while (lo <= hi) {          /* glyphs are sorted by code point */
        int mid = (lo + hi) / 2;
        if (f->g[mid].cp == ch) return &f->g[mid];
        if (f->g[mid].cp < ch) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

static kit_color_t blend(kit_color_t bg, kit_color_t fg, unsigned a)   /* a 0..15 */
{
    unsigned br = bg >> 11, bgg = (bg >> 5) & 63u, bb = bg & 31u;
    unsigned fr = fg >> 11, fgg = (fg >> 5) & 63u, fb = fg & 31u;
    unsigned r = br + ((int)(fr - br) * (int)a) / 15;
    unsigned g = bgg + ((int)(fgg - bgg) * (int)a) / 15;
    unsigned b = bb + ((int)(fb - bb) * (int)a) / 15;
    return (kit_color_t)((r << 11) | (g << 5) | b);
}

int kit_font_height(int font)
{
    if (font < 0 || font >= KIT_NFONTS) font = 0;
    return KIT_FONT_DATA[font].height;
}

int kit_text_width(const char *s, int font)
{
    const kit_font_t *f;
    int w = 0;
    if (font < 0 || font >= KIT_NFONTS) font = 0;
    f = &KIT_FONT_DATA[font];
    for (; *s; s++) {
        const kit_glyph_t *g = glyph(f, (unsigned char)*s);
        w += g ? g->adv : f->height / 3;
    }
    return w;
}

int kit_text(int x, int y, const char *s, int font, int align, kit_color_t fg)
{
    const kit_font_t *f;
    int w = kit_text_width(s, font);
    if (font < 0 || font >= KIT_NFONTS) font = 0;
    f = &KIT_FONT_DATA[font];
    if (align == KIT_ALIGN_CENTER) x -= w / 2;
    else if (align == KIT_ALIGN_RIGHT) x -= w;
    for (; *s; s++) {
        const kit_glyph_t *g = glyph(f, (unsigned char)*s);
        int gx, gy, stride;
        const uint8_t *d;
        if (!g) { x += f->height / 3; continue; }
        stride = (g->w + 1) / 2;
        d = f->data + g->off;
        if (f->rle) {                           /* runs: alpha << 4 | length - 1, row after row (D-092) */
            unsigned a = 0, run = 0;
            for (gy = 0; gy < g->h; gy++) {
                int py = y + g->yo + gy;
                for (gx = 0; gx < g->w; gx++) {
                    int px = x + g->xo + gx;
                    if (!run) { a = *d >> 4; run = (*d & 15u) + 1u; d++; }
                    run--;
                    if (!a || (unsigned)py >= KIT_H || (unsigned)px >= KIT_W) continue;
                    kit__fb[py * KIT_W + px] = a == 15u ? fg : blend(kit__fb[py * KIT_W + px], fg, a);
                }
            }
        } else
        for (gy = 0; gy < g->h; gy++) {
            int py = y + g->yo + gy;
            if ((unsigned)py >= KIT_H) continue;
            for (gx = 0; gx < g->w; gx++) {
                int px = x + g->xo + gx;
                unsigned a = d[gy * stride + gx / 2];
                a = gx & 1 ? a & 15u : a >> 4;
                if (!a || (unsigned)px >= KIT_W) continue;
                kit__fb[py * KIT_W + px] = a == 15u ? fg : blend(kit__fb[py * KIT_W + px], fg, a);
            }
        }
        kit__dirty(y + g->yo, y + g->yo + g->h - 1);
        x += g->adv;
    }
    return w;
}
