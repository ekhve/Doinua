/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 filter morph. Include after Felucca's dsp.c. */
#pragma once

/* the filter's output as weights (Q10) of the input, the band-pass (kd v1) and the low-pass (v2): LP (0, 0, 1),
 * BP (0, 1, 0), HP (1, -1, -1), NOTCH (1, -1, 0) (exactly va_render's switch at 1024); FTYPE crossfades the weights
 * towards the next type of the cycle LP -> BP -> HP -> NOTCH -> LP (ff, Q10) */
typedef struct { int32_t x, b, l; } cf_va_filter_weights_t;
static const int8_t cf_va_filter_modes[4][3] = {{0, 0, 1}, {0, 1, 0}, {1, -1, -1}, {1, -1, 0}};
static cf_va_filter_weights_t cf_va_filter_weights(uint32_t ty, int32_t ff)
{
    const int8_t *a = cf_va_filter_modes[ty & 3u], *b = cf_va_filter_modes[(ty + 1u) & 3u];
    cf_va_filter_weights_t w;
    w.x = a[0] * 1024 + (b[0] - a[0]) * ff;              /* (* 1024, not << 10: a weight may be -1) */
    w.b = a[1] * 1024 + (b[1] - a[1]) * ff;
    w.l = a[2] * 1024 + (b[2] - a[2]) * ff;
    return w;
}
/* one sample of the SVF c (state ic1, ic2) with the output weights w; the soft knee after it */
static inline int32_t cf_va_filter_sample(const tsvf_t *c, int32_t x, int32_t *ic1, int32_t *ic2, int32_t kd, cf_va_filter_weights_t w)
{
    int32_t v3 = x - *ic2, v1 = (c->a1 * *ic1 + c->a2 * v3) >> 13, v2 = *ic2 + ((c->a2 * *ic1 + c->a3 * v3) >> 13), y;
    *ic1 = clamp(2 * v1 - *ic1, -150000, 150000);
    *ic2 = clamp(2 * v2 - *ic2, -150000, 150000);
    y = (w.x * x + w.b * ((kd * v1) >> 12) + w.l * v2) >> 10;
    return soft_knee(clamp(y, -200000, 200000), 16000);
}
