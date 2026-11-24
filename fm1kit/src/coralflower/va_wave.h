/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 morph and coloured noise. Include after Felucca's dsp.c. */
#pragma once

#define CF_VA_PULSE_PW(sh) (0x80000000u - (uint32_t)(sh) * 0xE666u)   /* SHAPE: 50 % .. ~5 % */

/* MORPH: sine (SHAPE 0) -> triangle (24) -> saw (48) -> ramp (72) -> square (96) -> a narrowing pulse (127: BASIC
 * SQR's width at SHAPE 127); between two neighbours a linear crossfade of the band-limited waves (polyBLEP saw, ramp
 * and square). The waves are aligned on the saw's fundamental (SAW and SQR are BASIC's own; the sine is shifted by
 * half a cycle, the triangle by three quarters, the ramp is the saw reversed), so no crossfade cancels the
 * fundamental. sh: Q15 (SHAPE x 258) */
#define CF_VA_MORPH_POINT(k) ((k) * 24 * 258)               /* the shapes' positions, Q15 */
static inline int32_t cf_va_morph(uint32_t ph, uint32_t inc, int32_t sh)
{
    int32_t a, b, f;
    if (sh >= CF_VA_MORPH_POINT(2)) {
        if (sh >= CF_VA_MORPH_POINT(4))                      /* square -> narrow pulse */
            return osc_pulse(ph, inc, CF_VA_PULSE_PW(clamp(((sh - CF_VA_MORPH_POINT(4)) * 16785) >> 12, 0, 32766)));
        b = -osc_saw(ph + 0x80000000u, inc);     /* the ramp */
        if (sh >= CF_VA_MORPH_POINT(3)) {                    /* ramp -> square (BASIC SQR: (saw - saw half a cycle on) / 2) */
            a = b;
            b = (osc_saw(ph, inc) + b) >> 1;
            f = sh - CF_VA_MORPH_POINT(3);
        } else {                                 /* saw -> ramp */
            a = osc_saw(ph, inc);
            f = sh - CF_VA_MORPH_POINT(2);
        }
    } else if (sh >= CF_VA_MORPH_POINT(1)) {                 /* triangle -> saw */
        a = osc_tri(ph + 0xC0000000u);
        b = osc_saw(ph, inc);
        f = sh - CF_VA_MORPH_POINT(1);
    } else {                                     /* sine -> triangle */
        a = sine_i(ph + 0x80000000u);
        b = osc_tri(ph + 0xC0000000u);
        f = sh;
    }
    return a + (((b - a) * ((f * 2710) >> 12)) >> 12);   /* (Q12 within the segment) */
}

/* NOISE BROWN: white noise through a leaky integrator (a one-pole low-pass: SHAPE (COLOR) sets its corner, 30 Hz ..
 * 3 kHz, KTRK moves it with the key), made up to about half the white noise's RMS. The corner's coefficient (Q15)
 * and the make-up gain (Q8) at SHAPE 0, 8, .. 127 */
static const uint16_t cf_va_brown_k[17] = {140, 187, 249, 333, 444, 592, 789, 1050, 1396, 1852, 2451, 3234, 4249, 5552,
                                        7200, 9248, 11397};
static const uint16_t cf_va_brown_gain[17] = {2769, 2395, 2072, 1792, 1550, 1341, 1160, 1003, 868, 751, 649, 562, 486, 421,
                                        364, 316, 279};
static inline int32_t cf_va_brown(int32_t *nst, int32_t *y, int32_t k, int32_t g)
{
    int32_t w = (int32_t)(noise32(nst) >> 16) - 32768;
    *y += ((w - *y) * k) >> 15;
    return clamp((*y * g) >> 8, -32767, 32767);
}
/* NOISE VINYL: sparse crackle (clicks of random size and sign, each gone in ~0.2 ms; SHAPE (DENS) sets how many:
 * thr / 65536 a sample, ~1 .. 200 a second) over a low hiss (white noise low-passed at ~2.5 kHz, about -30 dB) */
static inline int32_t cf_va_vinyl(int32_t *nst, int32_t *c, int32_t *h, uint32_t thr)
{
    uint32_t r = noise32(nst);
    *h += ((((int32_t)(r >> 16) - 32768) - *h) * 10000) >> 15;
    *c -= (*c >> 3) + (*c > 0);                  /* (to 0 from either side) */
    if ((r & 0xFFFFu) < thr)
        *c = (int32_t)(noise32(nst) >> 16) - 32768;
    return clamp(*c + (*h >> 3), -32767, 32767);
}
