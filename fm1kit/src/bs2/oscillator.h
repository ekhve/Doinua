/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown
 * fm1-scales: oscillator core from imijes/bass-something-2 app.c at 4c8a82f.
 * Phase is in turns in [0,1). Kept private to the BS2 engine.
 */
#pragma once
#include <stdint.h>

enum { WAVE_SAW, WAVE_SQUARE, WAVE_SINE, WAVE_TRI, WAVE_PULSE, NWAVES };

static inline float vm_sin_turns(float ph)  /* fm_sin_turns's */
{
    const float x = (ph - (int)(ph + 0.5f)) * 6.2831853071795864769f, x2 = x * x;
    float p = 1.345176048e-10f;
    p = p * x2 - 2.467706523e-08f;
    p = p * x2 + 2.752946668e-06f;
    p = p * x2 - 1.984015614e-04f;
    p = p * x2 + 8.333310417e-03f;
    p = p * x2 - 1.666666459e-01f;
    p = p * x2 + 9.999999946e-01f;
    return p * x;
}
static inline float blep(float t, float dt, float inv_dt)
{
    if (t < dt) {
        t *= inv_dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) * inv_dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}
/* the corner's residual (PolyBLAMP, the step's integrated): a slope's change, rounded over a sample either side */
static inline float blamp(float t, float dt, float inv_dt)
{
    if (t < dt) {
        t = 1.0f - t * inv_dt;
        return t * t * t * (1.0f / 3.0f);
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) * inv_dt + 1.0f;
        return t * t * t * (1.0f / 3.0f);
    }
    return 0.0f;
}
static inline float wrap1(float h) { return h >= 1.0f ? h - 1.0f : h; }
/* -1..1, band-limited (the sine has nothing to band-limit); pw the pulse's width 0..1 (the square's 0.5), its DC
 * taken off */
static inline float osc(float ph, float dt, float inv_dt, uint32_t wave, float pw)
{
    switch (wave) {
    case WAVE_SAW:
        return 2.0f * ph - 1.0f - blep(ph, dt, inv_dt);
    case WAVE_SINE:
        return vm_sin_turns(ph);
    case WAVE_TRI:                           /* its corners at 0 (the slope -4 to +4 a turn) and a half (back) */
        return 1.0f - 4.0f * __builtin_fabsf(ph - 0.5f) + 4.0f * dt * (blamp(ph, dt, inv_dt) - blamp(wrap1(ph + 0.5f), dt, inv_dt));
    default:                                 /* SQUARE, PULSE: up at 0, down at pw */
        return (ph < pw ? 1.0f : -1.0f) + 1.0f - 2.0f * pw + blep(ph, dt, inv_dt)
               - blep(wrap1(ph + 1.0f - pw), dt, inv_dt);
    }
}
static inline float osc_naive(float ph, uint32_t wave, float pw)   /* the same, not band-limited (sync's steps) */
{
    switch (wave) {
    case WAVE_SAW:
        return 2.0f * ph - 1.0f;
    case WAVE_SINE:
        return vm_sin_turns(ph);
    case WAVE_TRI:
        return 1.0f - 4.0f * __builtin_fabsf(ph - 0.5f);
    default:
        return ph < pw ? 1.0f : -1.0f;
    }
}

/* oscillator 2 restarted where oscillator 1 wrapped, t samples ago (0..1): its sample, with the restart's step
 * band-limited on both sides (PolyBLEP: the side before it on *prev, the sample held back for that). osc() took
 * the restart for a wrap of its own (a full step: saw -2, the pulses +2, none for the others); the step is from
 * where it was instead */
static inline float osc_sync(float *ph, float *prev, float t, float dt, float inv_dt, uint32_t wave, float pw)
{
    float pr = *ph - t * dt, d, o, full; /* where it was at the restart */
    if (pr < 0.0f)
        pr += 1.0f;
    *ph = t * dt;
    o = osc(*ph, dt, inv_dt, wave, pw);
    d = osc_naive(0.0f, wave, pw) - osc_naive(pr, wave, pw);
    full = wave == WAVE_SAW ? -2.0f : wave == WAVE_SQUARE || wave == WAVE_PULSE ? 2.0f : 0.0f;
    o -= 0.5f * (d - full) * (1.0f - t) * (1.0f - t);
    *prev += 0.5f * d * t * t;
    return o;
}
