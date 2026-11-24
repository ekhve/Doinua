/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown */
/* fm1-scales: ported from imijes/bass-something-2 at 4c8a82f. */
#pragma once
#include "voice_math.h"
static inline float bs2_exp(float x) { return vm_exp2f(x * 1.44269504089f); }
static inline float bs2_max(float a, float b) { return a > b ? a : b; }
enum { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };
typedef struct {
    float att;                           /* attack: a one-pole coefficient toward ATT_AIM (a capacitor's charge) */
    float dec, rel;                      /* decay and release: one-pole coefficients (exponential) */
    float sus;                           /* sustain level, 0..1 */
    uint8_t loop, count, fixed;          /* the Bass Station II's retriggering (again at its decay's end, count times:
                                          * 0 for ever), its fixed-duration sustain (no decay: the sustain held this
                                          * many steps, then the release, the key held or not) */
    float hold;
} adsr_t;
typedef struct {
    float lvl;                           /* the level, 0..1 */
    uint8_t stage;                       /* ENV_* */
    uint8_t loops;                       /* retriggered since the note began */
    float held;                          /* a fixed-duration sustain: steps left */
} env_t;
static inline float env_coef(float sec, float rate) /* one pole: about 99 % of the way in sec */
{
    return 1.0f - bs2_exp(-4.6f / (sec * rate));
}
/* the attack charges like a capacitor toward ATT_AIM, over the top, and stops at 1 (as an analog envelope's comparator
 * does): fastest at its start, so a short one snaps; it takes as long as a linear one did (ln(AIM / (AIM - 1))) */
#define ATT_AIM 1.3f
#define ATT_K 1.4663f
/* an envelope's settings: times in seconds, steps a second; its retriggering (loop, count) and fixed sustain (its
 * decay time then the sustain's) */
static inline void adsr_set(adsr_t *e, float a, float d, float s, float r, float rate, uint32_t loop, uint32_t count,
                     uint32_t fixed)
{
    e->fixed = fixed != 0u;
    e->loop = loop != 0u;
    e->count = (uint8_t)count;
    e->att = 1.0f - bs2_exp(-ATT_K / (bs2_max(a, 0.0005f) * rate));
    e->dec = env_coef(e->fixed ? 0.001f : bs2_max(d, 0.001f), rate);
    e->sus = vm_clampf(s, 0.0f, 1.0f);
    e->rel = env_coef(bs2_max(r, 0.001f), rate);
    e->hold = d * rate;
}
/* the faders' times, the Bass Station II's (its user guide: the attack at once at the bottom, about 250 ms at the
 * middle, over 5 s at the top; the decay about 150 ms at the middle; the release about 360 ms at the middle, over 10 s
 * at the top), each an exponential through them: lo + a (2^(b v) - 1). (The decay's top the guide does not give: its
 * curve the release's, 4.3 s) */
static inline float env_curve(uint32_t v, float lo, float a, float b) { return lo + a * (vm_exp2f((float)v * b) - 1.0f); }
static inline float att_time(uint32_t v) { return env_curve(v, 0.0005f, 0.011804f, 0.069819f); }   /* 0.5 ms .. 5.5 s */
static inline float dec_time(uint32_t v) { return env_curve(v, 0.002f, 0.005117f, 0.076614f); }    /* 2 ms .. 4.3 s */
static inline float rel_time(uint32_t v) { return env_curve(v, 0.002f, 0.012377f, 0.076614f); }    /* 2 ms .. 10.5 s */

static inline float adsr_step(env_t *v, const adsr_t *e)
{
    switch (v->stage) {
    case ENV_ATTACK:
        if ((v->lvl += (ATT_AIM - v->lvl) * e->att) >= 1.0f) {
            v->lvl = 1.0f;
            v->stage = ENV_DECAY;
            v->held = e->hold;
        }
        break;
    case ENV_DECAY:                          /* (fixed-duration: no decay, a millisecond's to the sustain) */
        v->lvl += (e->sus - v->lvl) * e->dec;
        if (v->lvl - e->sus < 1e-4f) {
            if (!e->fixed && e->loop && (!e->count || v->loops < e->count)) {   /* retriggering: again */
                v->loops++;
                v->stage = ENV_ATTACK;
            } else
                v->stage = ENV_SUSTAIN;
        }
        break;
    case ENV_SUSTAIN:
        v->lvl = e->sus;
        if (e->fixed && (v->held -= 1.0f) <= 0.0f)   /* fixed-duration: its time up, the release */
            v->stage = ENV_RELEASE;
        break;
    case ENV_RELEASE:
        v->lvl -= v->lvl * e->rel;
        if (v->lvl < 1e-5f) {
            v->lvl = 0.0f;
            v->stage = ENV_IDLE;
        }
        break;
    default:
        v->lvl = 0.0f;
    }
    return v->lvl;
}
static inline void env_release(env_t *v)
{
    if (v->stage != ENV_IDLE && v->stage != ENV_RELEASE)
        v->stage = ENV_RELEASE;
}
