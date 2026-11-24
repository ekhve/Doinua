/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * Patch layout and factory preset decoding from CoralFlower 46790b9 (eng_va.c).
 * fm1-scales: use the selected preset bank and keep this internal to the VA renderer. */
#pragma once

#include <stdint.h>
#include "felucca_engine.h"

enum { VO_WAVE, VO_LEVEL, VO_COARSE, VO_FINE, VO_SHAPE, VO_KTRK, VO_N };           /* per oscillator */
enum { VE_ATK, VE_HOLD, VE_DEC, VE_SUS, VE_REL, VE_N };                            /* per envelope */
enum { VL_RATE, VL_WAVE, VL_DEPTH, VL_FADE, VL_SYNC, VL_N };                       /* per LFO */
enum { VM_SRC, VM_DST, VM_AMT, VM_N };                                             /* per matrix slot */
#define VA_OSC(k, f) ((k) * VO_N + (f))
enum {
    VA_SYNC2 = 4 * VO_N, VA_RING4,
    VA_FTYPE, VA_CUT, VA_RES, VA_DRIVE, VA_FKTRK, VA_FENV,
    VA_ENV0, VA_VEL = VA_ENV0 + 4 * VE_N,
    VA_LFO0, VA_MOD0 = VA_LFO0 + 4 * VL_N,
    VA_OMIX = VA_MOD0 + 8 * VM_N, VA_DETUNE,
    VA_OMODE0,
    VA_FRSV = VA_OMODE0 + 4, VA_FSPREAD, VA_USPREAD,     /* MODE of OSC 1..4, (version 2's FILTER MORPH: version 3
                                                          * keeps it 0, FTYPE holds both), SPREAD, USPREAD */
    VA_NP
};
#define VA_ENV(k, f) (VA_ENV0 + (k) * VE_N + (f))
#define VA_LFO(k, f) (VA_LFO0 + (k) * VL_N + (f))
#define VA_MOD(k, f) (VA_MOD0 + (k) * VM_N + (f))
_Static_assert(VA_NP == 106 && VA_OMODE0 == 99, "CoralFlower VA patch layout");

enum { VW_SAW, VW_SQR, VW_TRI, VW_SIN, VW_PWM, VW_NOIS, VW_N };
enum { VF_LP, VF_BP, VF_HP, VF_NOTCH };
enum { VOM_BASIC, VOM_MORPH, VOM_NOISE, VOM_N };                                  /* oscillator MODE */
enum { VN_WHITE, VN_BROWN, VN_VINYL, VN_N };                                      /* NOISE: WAVE is the type */
enum { VS_OFF, VS_ENV1, VS_LFO1 = VS_ENV1 + 4, VS_VEL = VS_LFO1 + 4, VS_KEY, VS_RAND, VS_MODW, VS_N };
enum { VD_OFF, VD_PITCH, VD_PIT1, VD_LVL1 = VD_PIT1 + 4, VD_SHP1 = VD_LVL1 + 4, VD_CUT = VD_SHP1 + 4, VD_RES, VD_AMP,
       VD_PAN, VD_RATE1, VD_FTYPE = VD_RATE1 + 4, VD_DRIVE, VD_SPREAD, VD_FENV, VD_DEP1,
       VD_N = VD_DEP1 + 4 };     /* (append only: the blobs store them) */

/* the range and the init value of patch value i */
typedef struct { int8_t min, max, def; } va_rng_t;
static inline va_rng_t cf_va_range(uint32_t i)
{
    va_rng_t r = {0, 127, 0};
    if (i < VA_SYNC2) {
        uint32_t k = i / VO_N;
        switch (i % VO_N) {
        case VO_WAVE: r.max = VW_N - 1; break;
        case VO_LEVEL: r.def = k ? 0 : 100; break;
        case VO_COARSE: r.min = -24; r.max = 24; break;
        case VO_FINE: r.min = -64; r.max = 63; break;
        case VO_KTRK: r.max = 1; r.def = 1; break;
        default: break;
        }
    } else if (i == VA_SYNC2 || i == VA_RING4) {
        r.max = 1;
    } else if (i < VA_ENV0) {
        static const va_rng_t F[6] = {{0, 127, 0}, {0, 127, 100}, {0, 127, 0}, {0, 127, 0}, {0, 127, 64}, {-64, 63, 0}};
        r = F[i - VA_FTYPE];
    } else if (i < VA_VEL) {
        static const int8_t D[VE_N] = {0, 0, 64, 127, 40};
        r.def = D[(i - VA_ENV0) % VE_N];
        if ((i - VA_ENV0) % VE_N == VE_SUS && i >= VA_ENV(1, 0))
            r.def = 0;                                  /* ENV 2..4: a decay to 0 */
    } else if (i == VA_VEL) {
        r.def = 64;
    } else if (i < VA_MOD0) {
        static const va_rng_t L[VL_N] = {{0, 127, 70}, {0, 4, 0}, {0, 127, 127}, {0, 127, 0}, {0, 1, 0}};
        r = L[(i - VA_LFO0) % VL_N];
    } else if (i < VA_OMIX) {
        static const va_rng_t M[VM_N] = {{0, VS_N - 1, 0}, {0, VD_N - 1, 0}, {-64, 63, 0}};
        r = M[(i - VA_MOD0) % VM_N];
    } else if (i == VA_OMIX) {
        r.def = 64;
    } else if (i >= VA_OMODE0 && i < VA_FRSV) {
        r.max = VOM_N - 1;
    } else if (i == VA_FRSV) {
        r.max = 0;                                      /* (reserved) */
    }
    return r;
}

static inline void cf_va_init_patch(int8_t *p)
{
    uint32_t i;
    for (i = 0; i < VA_NP; i++)
        p[i] = cf_va_range(i).def;
}

static inline int8_t cf_va_clamp_value(uint32_t i, int32_t v)
{
    va_rng_t r = cf_va_range(i);
    return (int8_t)(v < r.min ? r.min : v > r.max ? r.max : v);
}

#define O_(k, f) VA_OSC(k, VO_##f)
#define E_(k, f) VA_ENV(k, VE_##f)
#define L_(k, f) VA_LFO(k, VL_##f)
#define M_(k, s, d, a) VA_MOD(k, VM_SRC), (s), VA_MOD(k, VM_DST), (d), VA_MOD(k, VM_AMT), (uint8_t)(a)
#define S8(v) (uint8_t)(int8_t)(v)
#define ENV1(a, d, s, r) E_(0, ATK), a, E_(0, DEC), d, E_(0, SUS), s, E_(0, REL), r
#define ENV2(a, d, s, r) E_(1, ATK), a, E_(1, DEC), d, E_(1, SUS), s, E_(1, REL), r
#define ENV3(a, d, s, r) E_(2, ATK), a, E_(2, DEC), d, E_(2, SUS), s, E_(2, REL), r
#define OSC(k, w, l, c, f) O_(k, WAVE), VW_##w, O_(k, LEVEL), l, O_(k, COARSE), S8(c), O_(k, FINE), S8(f)
#define FLT(ty, c, r, fe) VA_FTYPE, 32 * VF_##ty, VA_CUT, c, VA_RES, r, VA_FENV, S8(fe)
#define LFO(k, r, w, d, f) L_(k, RATE), r, L_(k, WAVE), w, L_(k, DEPTH), d, L_(k, FADE), f
#define MODE(k, m) VA_OMODE0 + (k), VOM_##m
#include "va_bank.h"
#undef O_
#undef E_
#undef L_
#undef M_
#undef S8
#undef ENV1
#undef ENV2
#undef ENV3
#undef OSC
#undef FLT
#undef LFO
#undef MODE

#define CF_VA_PRESET_COUNT (sizeof VA_PRESETS / sizeof VA_PRESETS[0])
_Static_assert(CF_VA_PRESET_COUNT == sizeof VA_PRESET_EDITS / sizeof VA_PRESET_EDITS[0],
               "Each selected CoralFlower preset has a patch");

static inline void cf_va_preset_patch(uint32_t k, int8_t *p)
{
    const uint8_t *e;
    cf_va_init_patch(p);
    if (k >= CF_VA_PRESET_COUNT)
        return;
    for (e = VA_PRESET_EDITS[k]; *e != 0xFFu; e += 2)
        p[e[0]] = cf_va_clamp_value(e[0], (int8_t)e[1]);
}
