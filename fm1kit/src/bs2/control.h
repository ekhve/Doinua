/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown */
/* fm1-scales: track-local controls from imijes/bass-something-2 at 4c8a82f. */
#pragma once
#include "voice.h"
#define CTRL_N 32u
#define CTRL_RATE (BS2_RATE / CTRL_N)
static inline float lfo_hz(uint32_t v)
{
    float x = (float)v;
    return vm_exp2f(-2.250353f + 0.0827845f * x - 0.000173623f * x * x);
}
/* a sync value's cycle in MIDI clock ticks (24 a quarter note): the Bass Station II's table, 64 beats .. a 32nd
 * triplet (its 42 beats 1008: the guide's 1002 a misprint, 2 cycles in 21 bars) */
static const uint16_t SYNC_TICKS[NSYNCV] = {1536, 1152, 1008, 864, 768, 720, 672, 576, 512, 480, 448, 432, 384, 320,
                                            288,  256,  192,  144, 128, 96,  72,  64,  48,  36,  32,  24,  18,  16,
                                            12,   9,    8,    6,   4,   3,   2};
/* an LFO's step of n samples (dt: the clock's ticks in them): -1..1. Free-running (keysync: from 0 at a note,
 * lfo_note); its speed the knob's, or the clock's (sync); S&H a new value at each wrap; the slew a one-pole on what
 * it gives (its edges rounded); the delay a fade-in from the note */
static inline float lfo_step(lfo_t *l, const lfo_patch_t *p, uint32_t *rng, uint32_t n, float dt, uint32_t slew, uint32_t delay,
                      uint32_t sync, uint32_t syncv)
{
    float ph, w;
    l->ph += (sync ? dt / (float)SYNC_TICKS[syncv < NSYNCV ? syncv : NSYNCV - 1u]
                                   : lfo_hz(p->rate) * (float)n * (1.0f / BS2_RATE));
    if (l->ph >= 1.0f) {
        l->ph -= vm_floorf(l->ph);
        *rng = *rng * 1664525u + 1013904223u;
        l->hold = (float)(int32_t)*rng * (1.0f / 2147483648.0f);
    }
    ph = l->ph;
    switch (p->wave) {
    case LFO_SAW:                            /* (falling, the Bass Station II's) */
        w = 1.0f - 2.0f * ph;
        break;
    case LFO_SQUARE:
        w = ph < 0.5f ? 1.0f : -1.0f;
        break;
    case LFO_SH:
        w = l->hold;
        break;
    default:                                 /* TRI */
        w = 1.0f - 4.0f * vm_fabsf(ph - 0.5f);
    }
    /* the slew: 0 none, else a time constant 0.5 ms .. 2 s (its scale the guide does not give) */
    l->out = slew ? l->out + (w - l->out) * (1.0f - bs2_exp(-(float)n / (0.0005f * vm_exp2f((float)slew * (12.0f / 127.0f))
                                                                         * BS2_RATE)))
                  : w;
    /* the delay: a linear fade-in over the attack's time at its setting (the guide gives no scale) */
    l->fade = delay ? vm_minf(1.0f, l->fade + (float)n / (att_time(delay) * BS2_RATE)) : 1.0f;
    return l->out * l->fade;
}
static inline void lfo_note(lfo_t *l, uint32_t keysync, uint32_t delay)   /* a note (not legato) starts: keysync, delay */
{
    if (keysync)
        l->ph = 0.0f;
    if (delay)
        l->fade = 0.0f;
}

static inline float pulse_width(uint32_t wave, uint32_t pw, int32_t pwm, float lfo2, float menv)
{
    if (wave != WAVE_PULSE)
        return 0.5f;
    return vm_clampf(((float)pw + ((float)pwm * lfo2 + menv) * (45.0f / 63.0f)) * 0.01f, 0.05f, 0.95f);
}
/* velocity on an envelope (the Bass Station II's VELOCITY AMP ENV and MOD ENV): amt -63..63 of v's 0..1; at +63 the
 * level is the velocity, at 0 always full, at -63 a soft note full and a hard one none */
static inline float vel_gain(float v, int32_t amt)
{
    float k = (float)amt * (1.0f / 63.0f);
    return k >= 0.0f ? 1.0f - k * (1.0f - v) : 1.0f + k * v;
}
#define TRK_C3 48.0f                         /* the key tracking's pivot: the cutoff as set at C3 */
#define TRK_SEMI (127.0f / (8.8f * 12.0f))   /* a semitone on the cutoff's 0..127 (8.8 octaves) */
static const uint8_t SUB_WAVE[NSUBW] = {WAVE_SINE, WAVE_PULSE, WAVE_SQUARE};
static inline float glide_coef(uint32_t v)          /* the pitch's one pole a modulation step: 1 at once, 3 ms .. 2.5 s */
{
    return v ? env_coef(0.003f * vm_exp2f((float)v * (9.7f / 127.0f)), CTRL_RATE) : 1.0f;
}

typedef struct {
    patch_t patch;
    voice_t voice;
    lfo_t lfo[NLFO];
    adsr_t amp, mod;
    vstep_t step;
    fcoef_t filter, mod_filter;
    uint32_t lfo_rng, remaining;
    int held;
} bs2_track_t;

static inline void bs2_prepare(bs2_track_t *t, const patch_t *patch)
{
    *t = (bs2_track_t){.patch = *patch, .voice = {.note = -1, .sag_g = 1},
                       .held = -1, .lfo_rng = 1};
    const patch_t *p = &t->patch;
    adsr_set(&t->amp, att_time(p->amp_a), dec_time(p->amp_d), p->amp_s / 127.f,
             rel_time(p->amp_r), BS2_RATE, p->env_loop[0], p->env_count[0], p->env_fixed[0]);
    adsr_set(&t->mod, att_time(p->menv_a), dec_time(p->menv_d), p->menv_s / 127.f,
             rel_time(p->menv_r), CTRL_RATE, p->env_loop[1], p->env_count[1], p->env_fixed[1]);
    t->step = (vstep_t){.ring = p->ring / 127.f, .l1 = p->lvl1 / 127.f, .l2 = p->lvl2 / 127.f,
        .ls = p->lvl_sub / 127.f, .noise = p->noise * (0.7f / 127.f), .ofd = p->ofm * (3.f / 127.f),
        .dg = 1 + p->dist * (15.f / 127.f), .wave = p->wave1, .wave2 = p->wave2,
        .subw = SUB_WAVE[p->sub_wave < NSUBW ? p->sub_wave : SUB_SQUARE], .sync = p->sync,
        .need1 = p->lvl1 || p->ring, .need2 = p->lvl2 || p->ring || p->ofm,
        .awake = 1, .ofm = p->ofm, .dist = p->dist, .aenv = &t->amp,
        .fc = &t->filter, .fm = &t->mod_filter, .rng = 1};
    t->step.dm = 1.f / __builtin_sqrtf(1.f + 0.25f * (t->step.dg - 1.f));
}

static inline void bs2_key_down(bs2_track_t *t, int note, int velocity)
{
    voice_t *v = &t->voice;
    int legato = t->held >= 0;
    if (v->note < 0 || (!legato && (t->patch.env_trig[0] == ENVT_AUTOGLIDE ||
                                     t->patch.env_trig[1] == ENVT_AUTOGLIDE)))
        v->pitch = (float)note;
    v->note = t->held = note;
    v->vel = vel_gain(velocity / 127.f, t->patch.vel_amp);
    v->vm = vel_gain(velocity / 127.f, t->patch.vel_menv);
    if (v->amp.stage == ENV_IDLE) v->vel_now = v->vel;
    if (!legato || t->patch.env_trig[0] == ENVT_MULTI) {
        v->amp.stage = ENV_ATTACK;
        v->amp.loops = 0;
    }
    if (!legato || t->patch.env_trig[1] == ENVT_MULTI) {
        v->menv.stage = ENV_ATTACK;
        v->menv.loops = 0;
    }
    if (!legato)
        for (unsigned i = 0; i < NLFO; ++i)
            lfo_note(&t->lfo[i], t->patch.lfo_keysync[i], t->patch.lfo_delay[i]);
    t->remaining = 0;
}

static inline void bs2_key_up(bs2_track_t *t, int note)
{
    if (note != t->held) return;
    t->held = -1;
    env_release(&t->voice.amp);
    env_release(&t->voice.menv);
}

static inline void bs2_control_step(bs2_track_t *t, float bpm)
{
    const patch_t *p = &t->patch;
    voice_t *v = &t->voice;
    vstep_t *k = &t->step;
    float lv[NLFO];
    for (unsigned i = 0; i < NLFO; ++i)
        lv[i] = lfo_step(&t->lfo[i], &p->lfo[i], &t->lfo_rng, CTRL_N,
                         bpm * 24.f / (60.f * CTRL_RATE), p->lfo_slew[i], p->lfo_delay[i],
                         p->lfo_sync[i], p->lfo_syncv[i]);
    float e = adsr_step(&v->menv, &t->mod) * v->vm / 63.f;
    float kt = TRK_SEMI * (v->pitch - TRK_C3) * p->key_trk / 7.f;
    float cutoff = p->cutoff + kt + 127.f * (e * p->menv_cut + lv[1] * p->lfo[1].depth[0] / 63.f);
    filter_coefs(&t->filter, p->filter, p->reso, p->drive, vm_clampf(cutoff, 0, 127), 1, BS2_RATE);
    if (p->ofm) t->mod_filter = t->filter;
    float base = v->pitch - 69.f;
    float semis1 = base + 12.f * (p->range1 - RANGE_8 + lv[0] * p->lfo[0].depth[0] / 63.f + e * p->menv_o1)
                   + p->coarse1 + 0.01f * p->fine1;
    float semis2 = base + 12.f * (p->range2 - RANGE_8 + lv[0] * p->lfo[0].depth[1] / 63.f + e * p->menv_o2)
                   + p->coarse2 + 0.01f * p->fine2;
    v->pitch += (v->note - v->pitch) * glide_coef(p->glide);
    k->inc = vm_minf(0.4f, (440.f / BS2_RATE) * vm_exp2f(semis1 / 12.f));
    k->inc2 = vm_minf(0.4f, (440.f / BS2_RATE) * vm_exp2f(semis2 / 12.f));
    k->incs = k->inc * (p->sub_oct == 2 ? 0.25f : 0.5f);
    k->inv = 1.f / k->inc; k->inv2 = 1.f / k->inc2; k->invs = 1.f / k->incs;
    k->pw1 = pulse_width(k->wave, p->pw1, p->pwm1, lv[1], 63.f * e * p->menv_pw1);
    k->pw2 = pulse_width(k->wave2, p->pw2, p->pwm2, lv[1], 63.f * e * p->menv_pw2);
    k->sg = 1.f / (1.f + 1.2f * bs2_max(0, v->sag_f - 0.15f));
}

static inline void bs2_samples(bs2_track_t *t, float *out, unsigned count, float bpm)
{
    while (count) {
        if (t->voice.amp.stage == ENV_IDLE) {
            while (count--) *out++ = 0;
            return;
        }
        if (!t->remaining) {
            bs2_control_step(t, bpm);
            t->remaining = CTRL_N;
        }
        unsigned n = count < t->remaining ? count : t->remaining;
        voice_run(&t->voice, out, n, &t->step);
        t->remaining -= n;
        count -= n;
        out += n;
    }
}
