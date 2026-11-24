/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 per-block oscillator ramps, ring modulation and hard sync. */
#pragma once
#include <stdint.h>

/* Phase times a Q16 ratio, reduced modulo 2^32 with native multiplies. */
static inline uint32_t cf_va_phase_q16(uint32_t phase, uint32_t ratio)
{
    uint32_t low = phase & 65535u;
    return (phase >> 16) * ratio + low * (ratio >> 16) +
           ((low * (ratio & 65535u)) >> 16);
}

/* the oscillator loop for one wave: x(ph, inc, sh) the sample; adds level-ramped into acc */
#define CF_VA_OSC_LOOP_FULL(X)                                                            \
    for (i = 0; i < n; i++) {                                                             \
        int32_t x_ = (X);                                                                 \
        uint32_t o_ = ph;                                                                 \
        if (keep)                                                                         \
            r3[i] = x_;                                                                   \
        if (ring)                                                                         \
            x_ = (x_ * r3[i]) >> 15;                                                      \
        acc[i] += (x_ * (l >> 16)) >> 15;                                                 \
        l += dl;                                                                          \
        sh += dsh;                                                                        \
        ph += inc;                                                                        \
        inc += dinc;                                                                      \
        if (rec && ph < o_) {                  /* OSC 1 wrapped: OSC 2 restarts there */  \
            wrap |= 1u << i;                                                              \
            sph[i] = ph;                                                                  \
        }                                                                                 \
        if (sync && ((wrap >> i) & 1u))                                                   \
            ph = cf_va_phase_q16(sph[i], ratio);                                          \
    }
#define CF_VA_OSC_LOOP(X)                                                                 \
    if (keep | ring | rec | sync) {                                                       \
        CF_VA_OSC_LOOP_FULL(X)                                                            \
    } else {                                   /* (no sync, no ring: the plain loop) */   \
        for (i = 0; i < n; i++) {                                                         \
            int32_t x_ = (X);                                                             \
            acc[i] += (x_ * (l >> 16)) >> 15;                                             \
            l += dl;                                                                      \
            sh += dsh;                                                                    \
            ph += inc;                                                                    \
            inc += dinc;                                                                  \
        }                                                                                 \
    }
