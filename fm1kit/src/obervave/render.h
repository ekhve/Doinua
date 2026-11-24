/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave a1f660c, obv_engine.h audio block, with explicit per-voice and per-track state. */
#pragma once
#include "dsp.h"

#define OBV_CONTROL_FRAMES 16u
enum { F_LP12, F_HP12, F_BP12, F_NOTCH, F_LP24, F_SEM };

typedef struct {
    osc_t o1, o2;
    svf_state_t f1;
    lad_state_t lad;
    svf_coef_t c1;
    lad_coef_t lc;
    uint32_t inc1, inc2, pw1, pw2, noise;
    int32_t amp0, amp1, gl, gr;
} obv_render_voice_t;

typedef struct {
    int32_t lvl1, lvl2, lvln, drive_q12, morph_q;
    uint32_t ftype, xmod_q, sync_on, w1, w2;
} obv_render_controls_t;

static inline void obv_voice_render(obv_render_voice_t *v, const obv_render_controls_t *g, int32_t *out)
{
    uint32_t i, inc1 = v->inc1, inc2 = v->inc2, pw1 = v->pw1, pw2 = v->pw2, d1 = inc1 >> 15, d2 = inc2 >> 15;
    int32_t amp0 = v->amp0, da = v->amp1 - v->amp0, gl = v->gl, gr = v->gr, o2n = v->o2.prev;
    int32_t l1 = g->lvl1, l2 = g->lvl2, ln = g->lvln, drv = g->drive_q12, morph_q = g->morph_q;
    uint32_t ft = g->ftype, xq = g->xmod_q, sy = g->sync_on, ww1 = g->w1, ww2 = g->w2;
    int32_t hw1 = (ww1 & OSC_SAW ? -65536 : 0) + (ww1 & OSC_PLS ? -65536 : 0);
    int32_t hw2 = (ww2 & OSC_SAW ? -65536 : 0) + (ww2 & OSC_PLS ? -65536 : 0);
    for (i = 0; i < OBV_CONTROL_FRAMES; i++) {
        int32_t s1, s2, mix, lp, bp, hp, y, amp, rt = -1;
        uint32_t ie = inc1;
        if (xq) {                               /* OSC2 -> OSC1 linear FM, the step never below inc/16 */
            int32_t m = (int32_t)(inc1 >> 8 > 0x200000u ? 0x200000u : inc1 >> 8) * (((o2n >> 8) * (int32_t)xq) >> 6);
            m += (int32_t)inc1;
            ie = m < (int32_t)(inc1 >> 4) ? inc1 >> 4 : (uint32_t)m;
            d1 = ie >> 15;
        }
        s1 = osc_step(&v->o1, ie, d1, pw1, ww1, hw1, -1);
        if (sy && v->o1.ph < ie && d1)
            rt = (int32_t)(v->o1.ph / d1);
        s2 = osc_step(&v->o2, inc2, d2, pw2, ww2, hw2, rt);
        o2n = v->o2.prev;
        mix = s1 * l1 + s2 * l2;
        if (ln) mix += ((int32_t)(xorshift(&v->noise) >> 16) - 32768) * ln;
        mix >>= 7;
        mix = sat_in(((mix >> 3) * drv) >> 9);  /* the input stage: DRIVE's gain, then a tanh (never a hard clip) */
        if (ft == F_LP24)
            y = lad_step(&v->lc, &v->lad, mix);
        else {
            svf_step(&v->c1, &v->f1, mix, &lp, &bp, &hp);
            switch (ft) {
            case F_HP12: y = hp; break;
            case F_BP12: y = bp; break;
            case F_NOTCH: y = hp + lp; break;
            case F_SEM: y = lp + (((hp - lp) * morph_q) >> 7); break;   /* the SEM's LP .. notch .. HP pot */
            default: y = lp; break;
            }
        }
        y = sat_knee(y);
        amp = amp0 + ((da * (int32_t)i) >> 4);
        y = (y * amp) >> 16;
        out[2u * i] += (y * gl) >> 12;
        out[2u * i + 1u] += (y * gr) >> 12;
    }
}
