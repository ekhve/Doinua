/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 (eng_va.c) four-oscillator mixer.
 * fm1-scales: caller supplies per-voice state and per-track modulation. */
#pragma once

#include "va_osc_block.h"
#include "va_wave.h"

typedef struct {
    uint32_t phase[4], increment[4];
    int16_t level[4], shape[4];
    int32_t noise_seed, noise[4][2];
} cf_va_oscillators_t;

/* Clear and fill the pre-filter mix. pwm_phase includes the voice's phase offset. */
static inline void cf_va_oscillators(cf_va_oscillators_t *s, const int8_t p[VA_NP],
                                     const int32_t targets[CF_VA_DST_LFO_DEPTH1],
                                     int32_t pitch16, int32_t fine, int32_t shape,
                                     uint32_t pwm_phase, int32_t wow, int32_t acc[CTL], uint32_t n)
{
    uint32_t k, i, wrap = 0, osc_on = 0, vinyl = 0;
    uint32_t sph[CTL], tinc[4];
    int32_t r3[CTL], tlvl[4], tshp[4], tsh[4], mixg[2], dwow = 0;
    /* Set pitch, level and shape targets for the next control tick. */
    {
        int32_t om = p[VA_OMIX], d3 = (p[VA_DETUNE] * 68) >> 10;   /* DTN: a third of up to 25 cents (no divide) */
        static const int8_t SPREAD[4] = {-3, 3, -1, 1};
        mixg[0] = om <= 64 ? 32767 : (127 - om) * 520;           /* MIX: 64 both pairs, 0 OSC 1+2 only, 127 3+4 */
        mixg[1] = om >= 64 ? 32767 : om * 512;
        for (k = 0; k < 4u; k++)                 /* a VINYL oscillator sounding: the voice's pitch wows (+-6 ct) */
            if (p[VA_OMODE0 + k] == VOM_NOISE && p[VA_OSC(k, VO_WAVE)] == VN_VINYL && p[VA_OSC(k, VO_LEVEL)])
                vinyl = 1;
        if (vinyl)
            dwow = (wow * 15) >> 15;
        for (k = 0; k < 4u; k++) {
            int32_t base = p[VA_OSC(k, VO_KTRK)] ? pitch16 : 60 * 16;
            int32_t ct = p[VA_OSC(k, VO_FINE)] + SPREAD[k] * d3;
            int32_t q = (base << 4) + p[VA_OSC(k, VO_COARSE)] * 256 + ((ct * 2621) >> 10) + (((targets[CF_VA_DST_PITCH] + targets[CF_VA_DST_OSC_PITCH1 + k]) * 3) >> 11) +
                        dwow;
            int32_t lv, sh, fu;
            uint32_t inc;
            q = clamp(q, 0, 2047 << 4);
            inc = pitch_inc((uint32_t)q >> 4);
            fu = (p[VA_OSC(k, VO_KTRK)] ? fine : 0) + (((q & 15) * 237) >> 8);
            if (fu)
                inc += (uint32_t)((int32_t)(inc >> 12) * fu);
            tinc[k] = inc;
            lv = clamp(p[VA_OSC(k, VO_LEVEL)] + (targets[CF_VA_DST_OSC_LEVEL1 + k] >> 14), 0, 127);
            tlvl[k] = mulq15(lv * lv * 2, mixg[k >> 1]);
            sh = clamp(p[VA_OSC(k, VO_SHAPE)] + (targets[CF_VA_DST_OSC_SHAPE1 + k] >> 14) + ((shape - (64 << 8)) >> 8), 0, 127);
            tshp[k] = sh * 258;
            tsh[k] = sh;
            if (tlvl[k] || s->level[k])
                osc_on |= 1u << k;
        }
        if ((osc_on & 2u) && p[VA_SYNC2])
            osc_on |= 1u;                        /* OSC 1 drives OSC 2's sync, OSC 3 OSC 4's ring */
        if ((osc_on & 8u) && p[VA_RING4])
            osc_on |= 4u;
    }
    for (i = 0; i < n; i++)
        acc[i] = 0;
    for (k = 0; k < 4u; k++) {
        uint32_t ph = s->phase[k], inc0 = s->increment[k], inc = inc0 ? inc0 : tinc[k];
        int32_t dinc = ((int32_t)(tinc[k] - inc) + ((int32_t)(tinc[k] - inc) >> 31 & (CTL - 1))) >> CTL_LOG2;
        int32_t l = (int32_t)s->level[k] << 16, dl = (((int32_t)tlvl[k] << 16) - l) >> CTL_LOG2;
        int32_t sh = p[VA_OMODE0 + k] != VOM_BASIC && !inc0 ? tshp[k] : (int32_t)s->shape[k];   /* (MORPH / NOISE: */
        int32_t dsh = (tshp[k] - sh) >> CTL_LOG2;                    /* a fresh voice starts at its shape) */
        uint32_t sync = k == 1u && p[VA_SYNC2], rec = k == 0u && p[VA_SYNC2];
        uint32_t keep = k == 2u && p[VA_RING4], ring = k == 3u && p[VA_RING4], ratio = 0, w = (uint32_t)p[VA_OSC(k, VO_WAVE)];
        uint32_t mode = (uint32_t)p[VA_OMODE0 + k];
        if (!((osc_on >> k) & 1u)) {             /* silent: the phase runs on (no step when it comes back) */
            ph += ((inc >> 1) + (tinc[k] >> 1)) * n;
            s->phase[k] = ph;
            s->increment[k] = tinc[k];
            s->level[k] = (int16_t)tlvl[k];
            s->shape[k] = (int16_t)tshp[k];
            continue;
        }
        if (sync)
            ratio = tinc[1] / ((tinc[0] >> 16) | 1u);   /* OSC 2 / OSC 1, Q16 */
        if (sync && ratio > (64u << 16))
            ratio = 64u << 16;
        if (mode == VOM_MORPH)
            w = VW_N;                            /* (the switch's MORPH) */
        else if (mode == VOM_NOISE)
            w = w == VN_BROWN ? VW_N + 1u : w == VN_VINYL ? VW_N + 2u : VW_NOIS;
        switch (w) {
        case VW_SQR:
            CF_VA_OSC_LOOP(osc_pulse(ph, inc, CF_VA_PULSE_PW(sh)))
            break;
        case VW_TRI:
            CF_VA_OSC_LOOP(osc_tri(ph))
            break;
        case VW_SIN:
            if (!sh && !dsh) {                   /* no fold: the plain sine */
                CF_VA_OSC_LOOP(sine_i(ph))
                break;
            }
            CF_VA_OSC_LOOP(osc_tri((uint32_t)(((sine_i(ph) * ((32768 + 3 * sh) >> 2)) >> 13) + 32768) << 15))   /* SHAPE folds */
            break;
        case VW_PWM: {
            uint32_t pw = 0x80000000u + (uint32_t)((osc_tri(pwm_phase) *
                                                    (sh >> 7)) * 0x58);
            CF_VA_OSC_LOOP(osc_pulse(ph, inc, pw))
            break;
        }
        case VW_NOIS: {
            int32_t nst = s->noise_seed;
            CF_VA_OSC_LOOP(((int32_t)(noise32(&nst) >> 16) - 32768))
            s->noise_seed = nst;
            break;
        }
        case VW_N:                               /* MORPH */
            CF_VA_OSC_LOOP(cf_va_morph(ph, inc, sh))
            break;
        case VW_N + 1u: {                        /* NOISE BROWN: SHAPE the corner, KTRK moves it with the key */
            int32_t nst = s->noise_seed, *y = &s->noise[k][0], e, j, fq, kq, gq;
            e = clamp(tsh[k] + (p[VA_OSC(k, VO_KTRK)] ? ((pitch16 - 60 * 16) * 13) >> 7 : 0), 0, 127);
            j = e >> 3;
            fq = e & 7;
            kq = cf_va_brown_k[j] + (((cf_va_brown_k[j + 1] - cf_va_brown_k[j]) * fq) >> 3);
            gq = cf_va_brown_gain[j] + (((cf_va_brown_gain[j + 1] - cf_va_brown_gain[j]) * fq) >> 3);
            CF_VA_OSC_LOOP(cf_va_brown(&nst, y, kq, gq))
            s->noise_seed = nst;
            break;
        }
        case VW_N + 2u: {                        /* NOISE VINYL: SHAPE the crackle's density */
            int32_t nst = s->noise_seed, *cz = &s->noise[k][0], *hz = &s->noise[k][1];
            uint32_t thr = 2u + (uint32_t)(tsh[k] * tsh[k] * tsh[k]) / 7000u;
            CF_VA_OSC_LOOP(cf_va_vinyl(&nst, cz, hz, thr))
            s->noise_seed = nst;
            break;
        }
        default:
            CF_VA_OSC_LOOP(osc_saw(ph, inc) + ((sh ? (osc_tri(ph) - osc_saw(ph, inc)) * (sh >> 3) : 0) >> 12))
            break;
        }
        s->phase[k] = ph;
        s->increment[k] = tinc[k];
        s->level[k] = (int16_t)tlvl[k];
        s->shape[k] = (int16_t)tshp[k];
    }
}
