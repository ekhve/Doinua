/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 (eng_va.c) filter, velocity curve and amplitude ramp.
 * fm1-scales: caller supplies mix buffers and per-voice state. */
#pragma once

#include "va_filter.h"

typedef struct {
    int32_t left[2], right[2], amp, dc;
    uint8_t spread;
} cf_va_output_t;

/* Mix into out and, when provided, side. acc is the disposable oscillator mix. */
static inline int cf_va_output(cf_va_output_t *s, const int8_t p[VA_NP], const cf_va_modulation_t *mod,
                              const int32_t env[4], int32_t pitch16, int32_t cutoff, int32_t amp,
                              int32_t velocity, int unison, unsigned voice_index,
                              int32_t acc[CTL], int32_t *out, int32_t *side, uint32_t n)
{
    const int32_t *targets = mod->destination;
    int32_t dfenv = targets[CF_VA_DST_FILTER_ENV], dcut = targets[CF_VA_DST_CUTOFF];
    int32_t dres = targets[CF_VA_DST_RESONANCE], dspr = targets[CF_VA_DST_SPREAD];
    int32_t dfm = targets[CF_VA_DST_FILTER_TYPE], ddrv = targets[CF_VA_DST_DRIVE];
    int32_t A0, A1, dA, a, cut, kd, spr, up = 0;
    uint32_t fpos;
    tsvf_t c, cr;
    /* the filter: cutoff from CUT, ENV 2 by FENV, KTRK, the track's (ENV / LFO -> FLT), the matrix */
    int32_t filter_env = clamp(p[VA_FENV] + (dfenv >> 15), -64, 63);
    int32_t key_tracking = ((pitch16 - 60 * 16) * p[VA_FKTRK] * 150) >> 10;
    cut = (p[VA_CUT] << 8) + ((env[1] * filter_env) >> 6) + key_tracking + cutoff + (dcut >> 7);
    kd = 8192 - clamp(p[VA_RES] + (dres >> 14), 0, 127) * 60;
    spr = dspr ? clamp(p[VA_FSPREAD] + (dspr >> 14), 0, 127) : p[VA_FSPREAD];   /* SPREAD: the left SVF's cutoff
                                                                                  * down, the right's up, +-1 octave */
    if (spr) {
        int32_t off = (spr * 7235) >> 8;         /* (14.02 cutoff steps an octave, << 8) */
        tsvf_coef_k(&c, cut - off, kd);
        tsvf_coef_k(&cr, cut + off, kd);
    } else {
        tsvf_coef_k(&c, cut, kd);
    }
    /* FTYPE's position, 1/8192 of a type (LP BP HP NOTCH at 0 8192 16384 24576), round the cycle; the matrix's FTYPE
     * at most one cycle either way */
    fpos = ((uint32_t)p[VA_FTYPE] << 8) + (uint32_t)clamp(dfm >> 6, -32767, 32767);
    fpos &= 32767u;
    if (side && unison && p[VA_USPREAD]) {     /* USPREAD: voice i of 8 at (2i - 7) / 7 */
        int32_t kk = 2 * (int32_t)voice_index - 7;
        up = clamp(kk * p[VA_USPREAD] * 37, -32767, 32767) >> 4;   /* Q11 */
    }
    /* the amplitude: ENV 1 by velocity, the matrix's AMP, the voice's (fades, LFO -> AMP); UNISON as FM6 */
    {
        int32_t va = p[VA_VEL];
        int32_t velf = 32767 - ((va * (127 - velocity) * 2080) >> 10);
        A1 = mulq15(mulq15(env[0], velf), mod->gain);
        if (unison)
            A1 = (A1 * 13107) >> 15;   /* 2 / 5 */
        A1 = mulq15(A1, amp);
    }
    A0 = s->amp;
    dA = (A1 - A0) >> CTL_LOG2;
    a = A0;
    {
        int32_t ic1 = s->left[0], ic2 = s->left[1], drv = ddrv ? clamp(p[VA_DRIVE] + (ddrv >> 14), 0, 127) : p[VA_DRIVE];
        int32_t g = 4096 + drv * 97;             /* DRIVE: 1x .. 4x */
        uint32_t ty = fpos >> 13;
        if (p[VA_SYNC2]) {                       /* ~27 Hz high-pass */
            int32_t dc = s->dc;
            for (uint32_t i = 0; i < n; i++) {
                acc[i] -= dc >> 8;
                dc += acc[i];
            }
            s->dc = dc;
        }
        if (!(fpos & 8191u) && !spr && !up) {    /* one SVF, a discrete type, mono: the plain loop */
            s->spread = 0;
            for (uint32_t i = 0; i < n; i++) {
                int32_t x = acc[i] >> 1, v1, v2, v3, y;
                if (drv)
                    x = softclip((x * (g >> 4)) >> 8);
                v3 = x - ic2;
                v1 = (c.a1 * ic1 + c.a2 * v3) >> 13;
                v2 = ic2 + ((c.a2 * ic1 + c.a3 * v3) >> 13);
                ic1 = clamp(2 * v1 - ic1, -150000, 150000);
                ic2 = clamp(2 * v2 - ic2, -150000, 150000);
                switch (ty) {
                case VF_BP: y = (kd * v1) >> 12; break;
                case VF_HP: y = x - ((kd * v1) >> 12) - v2; break;
                case VF_NOTCH: y = x - ((kd * v1) >> 12); break;
                default: y = v2; break;
                }
                y = soft_knee(clamp(y, -200000, 200000), 16000);
                a += dA;
                out[i] += (mulq15(y, a) * (VOICE_FS / 4)) >> 11;
            }
        } else {                                 /* between types, SPREAD (two SVFs: mid + side), USPREAD (the pan) */
            int32_t jc1 = 0, jc2 = 0;
            cf_va_filter_weights_t fw = cf_va_filter_weights(ty, (int32_t)(fpos & 8191u) >> 3);
            if (spr) {
                if (!s->spread) {                   /* SPREAD came on: the right SVF starts from the left one's state */
                    s->right[0] = ic1;
                    s->right[1] = ic2;
                    s->spread = 1;
                }
                jc1 = s->right[0];
                jc2 = s->right[1];
            } else {
                s->spread = 0;
            }
            for (uint32_t i = 0; i < n; i++) {
                int32_t x = acc[i] >> 1, y, o;
                if (drv)
                    x = softclip((x * (g >> 4)) >> 8);
                y = cf_va_filter_sample(&c, x, &ic1, &ic2, kd, fw);
                a += dA;
                if (spr) {
                    int32_t yr = cf_va_filter_sample(&cr, x, &jc1, &jc2, kd, fw);
                    o = (mulq15((y + yr) >> 1, a) * (VOICE_FS / 4)) >> 11;
                    if (side)
                        side[i] += ((mulq15((yr - y) >> 1, a) * (VOICE_FS / 4)) >> 11) + ((o * up) >> 11);
                } else {
                    o = (mulq15(y, a) * (VOICE_FS / 4)) >> 11;
                    if (up)
                        side[i] += (o * up) >> 11;
                }
                out[i] += o;
            }
            if (spr) {
                s->right[0] = jc1;
                s->right[1] = jc2;
            }
        }
        s->left[0] = ic1;
        s->left[1] = ic2;
    }
    s->amp = A1;
    return side && (spr || up);
}
