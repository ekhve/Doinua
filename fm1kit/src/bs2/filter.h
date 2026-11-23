/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown */
/* fm1-scales: ported from imijes/bass-something-2 at 4c8a82f. */
#pragma once
#include "voice_math.h"
#include "oscillator.h"
/* 0..0.45 pi: same sine approximation as the oscillator. */
static inline float bs2_tan(float radians)
{
    float turns = radians * 0.15915494309f;
    return vm_sin_turns(turns) / vm_sin_turns(turns + 0.25f);
}
typedef struct {
    float x1, F1;                        /* a tanh's last input, and its antiderivative there (ADAA) */
} adaa_t;
typedef struct {
    float s[4];                          /* the ladder's four stages (trapezoidal integrators) */
    float hs;                            /* Acid: the high pass in the feedback */
    adaa_t drv;                          /* the drive */
} ladder_t;
enum { FLT_LP24, FLT_LP12, FLT_BP24, FLT_BP12, FLT_HP24, FLT_HP12, FLT_ACID, NFLT };
/* ----------------------------------------------------------------- filter --- */
/* four one-pole stages (trapezoidal, zero-delay feedback) with the fourth fed back through a tanh at the input,
 * the drive before it. Classic: equal stages, its modes mixed from the stages' outputs. Acid, a diode ladder
 * approximated: the stages spread (a diode ladder's poles do not coincide), a high pass in the feedback (the bass
 * stays under the resonance, as in a 303), the tanh a little asymmetric, and no make-up gain (it thins).
 * The resonance is a share of the feedback at which the loop starts to sing (the gain at its -180 degrees): Classic's
 * 4 at every cutoff, Acid's from ACID_K; at the top a little past it, so both self-oscillate everywhere, settling
 * where the tanh gives up that little (a clean sine). The drive is its own tanh before the loop, its
 * antiderivative's difference (ADAA): what it adds above Nyquist folds back 10 dB or more less */
typedef struct {
    float g[4];                              /* each stage's g (tan's, pre-warped): Osc Filter Mod's to scale */
    float G[4], om[4];                       /* each stage's g / (1 + g), and 1 - that */
    float P, c1, c2;                         /* G4 G3 G2 G1, G4 G3 G2, G4 G3 */
    float Gh, a;                             /* the feedback high pass: its G, and 1 - G (Classic: 0 and 1) */
    float k, pre, gin, post, bias, bias0;    /* resonance, drive, make-up in and out, tanh(bias) */
    float ka, kpa, kpa_r;                    /* the loop's k a, k P a and 1 / (1 + k P a): the step's (fcoef_loop) */
    uint32_t mode;                           /* FLT_* */
} fcoef_t;
static inline void fcoef_loop(fcoef_t *c)    /* the loop's gains from k, a and P (not every sample: a division) */
{
    c->ka = c->k * c->a;
    c->kpa = c->ka * c->P;
    c->kpa_r = 1.0f / (1.0f + c->kpa);
}

static inline void filter_coefs(fcoef_t *c, uint8_t mode, uint8_t resonance, uint8_t drive, float cutoff, float trim, float rate)   /* each modulation step; cutoff 0..127 */
{
    static const float ACID_SPREAD[4] = {1.6f, 1.0f, 1.0f, 0.7f};
    static const float ACID_K[17] = {5.72f, 5.63f, 5.63f, 5.63f, 5.62f, 5.62f, 5.62f, 5.62f, 5.19f,   /* the loop's */
                                     4.91f, 4.72f, 4.61f, 4.56f, 4.57f, 4.76f, 6.07f, 5.57f};    /* every 8 */
    /* 40 Hz .. 17.8 kHz: the bottom of the dial still under a bass note's fundamentals, not shut (from 20 Hz a
     * patch made low on a Bass Station II, 47 Sub Bass at 11, was 25 dB under its others) */
    float fc = 40.0f * vm_exp2f(cutoff * (8.8f / 127.0f));
    fc *= trim;
    float r = (float)resonance / 127.0f, kc = 4.0f;
    uint32_t i, acid = mode == FLT_ACID;
    for (i = 0; i < 4u; i++) {
        float g = bs2_tan(3.14159265359f * vm_minf(fc * (acid ? ACID_SPREAD[i] : 1.0f), 0.45f * rate) / rate);
        c->g[i] = g;
        c->G[i] = g / (1.0f + g);
        c->om[i] = 1.0f - c->G[i];
    }
    c->c2 = c->G[3] * c->G[2];
    c->c1 = c->c2 * c->G[1];
    c->P = c->c1 * c->G[0];
    c->Gh = 0.0f;
    if (acid) {                              /* (under a low cutoff it follows it down: the loop still sings there) */
        float g = bs2_tan(3.14159265359f * vm_minf(150.0f, 0.25f * fc) / rate), x = cutoff * 0.125f;
        c->Gh = g / (1.0f + g);
        i = x < 15.0f ? (uint32_t)x : 15u;
        kc = ACID_K[i] + (ACID_K[i + 1u] - ACID_K[i]) * (x - (float)i);
    }
    c->a = 1.0f - c->Gh;
    c->k = r * kc * 1.12f;
    c->pre = 1.0f + (float)drive * (7.0f / 127.0f);
    /* the loop fed at 0.3 and made up after it: a note playing does not choke its tanh, so it still sings over one
     * (fed whole, the loop's gain under the note fell short of the little past singing) */
    c->post = 1.0f / __builtin_sqrtf(1.0f + 0.4f * (c->pre - 1.0f)) * (1.0f / 0.3f);   /* full drive about 1 dB louder */
    c->gin = 0.3f * (acid ? 1.0f : 1.0f + 0.5f * c->k);   /* Classic: make-up for the loss in the pass band */
    c->bias = acid ? 0.1f : 0.0f;
    c->bias0 = vm_tanhf(c->bias);
    c->mode = mode;
    fcoef_loop(c);
}

/* Osc Filter Mod, the Bass Station II's: the cutoff moved at audio rate by oscillator 2, each stage's g scaled by m (2
 * to the octaves it moves) and the loop's products made again; the rest as the step's (c) */
static inline void fcoef_fm(fcoef_t *d, const fcoef_t *c, float m)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        float g = vm_minf(c->g[i] * m, 6.31f);   /* (held under 0.45 of the rate: tan's there) */
        d->G[i] = g / (1.0f + g);
        d->om[i] = 1.0f - d->G[i];
    }
    d->c2 = d->G[3] * d->G[2];
    d->c1 = d->c2 * d->G[1];
    d->P = d->c1 * d->G[0];
    fcoef_loop(d);
}

static inline float lncosh(float x)          /* ln(cosh x): tanh's antiderivative, without overflow: |x| - ln 2 +
                                              * ln(1 + y), y = e^-2|x| (0..1), that a polynomial (within 3e-7; its
                                              * constant left off: ADAA takes differences), no log, no divide */
{
    const float a = vm_fabsf(x), y = vm_exp2f(-2.885390082f * a);
    return a - 0.69314718056f + y * (9.999702573e-01f + y * (-4.993340671e-01f + y * (3.275122344e-01f + y * (-2.239679843e-01f
                         + y * (1.319908351e-01f + y * (-5.326809362e-02f + y * 1.024394855e-02f))))));
}
static inline float overdrive(adaa_t *f, float x)   /* tanh averaged over the sample (first-order ADAA) */
{
    float d = x - f->x1, F = lncosh(x);
    float y = vm_fabsf(d) > 0.01f ? (F - f->F1) / d : vm_tanhf(0.5f * (x + f->x1));   /* (close: the midpoint's) */
    f->x1 = x;
    f->F1 = F;
    return y;
}

/* the mixer's bus into the filter: the sum of the oscillators, the sub and the noise goes through the drive's tanh (at
 * DRIVE 0 too: its loud peaks round off, waves together intermodulate), a little off its centre as an analog summing
 * stage is, so it warms with even harmonics as well (its DC: the output's coupling's) */
#define MIX_BIAS 0.15f
#define MIX_BIAS0 0.148885f                  /* tanh(MIX_BIAS) */
static inline float ladder(ladder_t *f, const fcoef_t *c, float in)
{
    float u = (overdrive(&f->drv, in * c->pre + MIX_BIAS) - MIX_BIAS0) * c->gin, x, y1, y2, y3, y4, v;
    float S = c->c1 * f->s[0] * c->om[0] + c->c2 * f->s[1] * c->om[1] + c->G[3] * f->s[2] * c->om[2]
              + f->s[3] * c->om[3];
    y4 = (c->P * u + c->kpa * f->hs + S) * c->kpa_r;   /* the loop solved linearly, then the tanh on it */
    x = vm_tanhf(u - c->ka * (y4 - f->hs) + c->bias) - c->bias0;
    v = (x - f->s[0]) * c->G[0];
    y1 = v + f->s[0];
    f->s[0] = y1 + v;
    v = (y1 - f->s[1]) * c->G[1];
    y2 = v + f->s[1];
    f->s[1] = y2 + v;
    v = (y2 - f->s[2]) * c->G[2];
    y3 = v + f->s[2];
    f->s[2] = y3 + v;
    v = (y3 - f->s[3]) * c->G[3];
    y4 = v + f->s[3];
    f->s[3] = y4 + v;
    v = (y4 - f->hs) * c->Gh;
    f->hs += 2.0f * v;
    switch (c->mode) {
    case FLT_LP12:
        y4 = y2;
        break;
    case FLT_BP24:
        y4 = 4.0f * (y2 - 2.0f * y3 + y4);
        break;
    case FLT_BP12:
        y4 = 2.0f * (y1 - y2);
        break;
    case FLT_HP24:
        y4 = x - 4.0f * y1 + 6.0f * y2 - 4.0f * y3 + y4;
        break;
    case FLT_HP12:
        y4 = x - 2.0f * y1 + y2;
        break;
    default:                                 /* LP24, ACID */
        break;
    }
    return y4 * c->post;
}
