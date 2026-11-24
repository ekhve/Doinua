/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 (eng_va.c) voice and part control.
 * fm1-scales: the host owns allocation, note gates, and the mixer. Include after dsp.c. */
#pragma once

#include "va_patch.h"
#include "va_envelope.h"
#include "va_lfo.h"
#include "va_modulation.h"
#include "va_oscillators.h"
#include "va_output.h"

typedef struct {
    cf_va_lfo_t lfo[4];
    uint32_t pwm, wow_random;
    int32_t wow, wow_target;
    uint16_t wow_ticks;
    int8_t rate_offset[4], pan_offset;
    uint8_t depth_modulation;
} cf_va_part_t;

typedef struct {
    cf_va_oscillators_t oscillators;
    cf_va_output_t output;
    cf_va_envelope_t envelopes[4];
    uint16_t ticks;
} cf_va_voice_t;

typedef struct {
    uint8_t index, key, velocity, wheel;
    int16_t random;
    uint8_t gate, latest, unison;
} cf_va_note_t;

static inline void cf_va_part_tick(cf_va_part_t *part, const int8_t p[VA_NP], int32_t bpm)
{
    for (unsigned k = 0; k < 4; ++k) {
        int32_t rate = p[VA_LFO(k, VL_RATE)] + part->rate_offset[k];
        uint32_t increment = cf_va_lfo_increment(rate, p[VA_LFO(k, VL_SYNC)], bpm);
        cf_va_lfo_tick(&part->lfo[k], k, increment, p[VA_LFO(k, VL_WAVE)], p[VA_LFO(k, VL_DEPTH)]);
    }
    part->depth_modulation = 0;
    for (unsigned k = 0; k < 8; ++k) {
        unsigned dst = (uint8_t)p[VA_MOD(k, VM_DST)];
        if (dst >= VD_DEP1 && dst < VD_N && p[VA_MOD(k, VM_SRC)] && p[VA_MOD(k, VM_AMT)])
            part->depth_modulation = 1;
    }
    part->pwm += cf_va_lfo_free_increment[60];
    if (!part->wow_ticks--) {
        part->wow_random = cf_va_lfo_random(part->wow_random + 7u);
        part->wow_target = (int32_t)(part->wow_random >> 16) - 32768;
        part->wow_ticks = (uint16_t)(400u + (part->wow_random & 511u));
    }
    part->wow += (part->wow_target - part->wow) >> 8;
}

static inline void cf_va_note_on(cf_va_part_t *part, cf_va_voice_t *voice, uint32_t age,
                                 int fresh, int first_held)
{
    if (first_held)
        for (unsigned k = 0; k < 4; ++k) part->lfo[k].phase = 0;
    if (fresh) {
        uint32_t phase = age * 0x9e3779b9u;
        int32_t seed = voice->oscillators.noise_seed;
        *voice = (cf_va_voice_t){0};
        voice->oscillators.phase[1] = phase;
        voice->oscillators.phase[2] = phase * 3u;
        voice->oscillators.phase[3] = phase * 5u;
        voice->oscillators.noise_seed = seed ? seed : (int32_t)(0x2545f491u + age);
    }
    for (unsigned k = 0; k < 4; ++k) {
        voice->envelopes[k].stage = 1;
        voice->envelopes[k].hold = 0;
    }
    voice->ticks = 0;
}

static inline int cf_va_voice_done(const cf_va_voice_t *voice)
{
    return voice->envelopes[0].stage == 0;
}

static inline int cf_va_render(cf_va_part_t *part, cf_va_voice_t *voice, const int8_t p[VA_NP],
                               const cf_va_note_t *note, const vmod_t *m,
                               int32_t *out, int32_t *side, uint32_t n)
{
    if (n > CTL || cf_va_voice_done(voice))
        return 0;
    int32_t env[4], acc[CTL];
    uint8_t fades[4], depths[4];
    for (unsigned k = 0; k < 4; ++k) {
        env[k] = cf_va_envelope_tick(&voice->envelopes[k], (const uint8_t *)&p[VA_ENV(k, 0)], note->gate);
        fades[k] = (uint8_t)p[VA_LFO(k, VL_FADE)];
        depths[k] = (uint8_t)p[VA_LFO(k, VL_DEPTH)];
    }
    if (voice->ticks < UINT16_MAX) ++voice->ticks;

    int32_t sources[4] = {note->velocity * 258, clamp(((int32_t)note->key - 60) * 512, -32767, 32767),
                          note->random, note->wheel * 258};
    cf_va_modulation_t mod;
    cf_va_modulate(&mod, &p[VA_MOD0], env, part->lfo, depths, fades, voice->ticks, sources, part->depth_modulation);
    if (note->latest) {
        part->pan_offset = (int8_t)clamp(mod.destination[CF_VA_DST_PAN] >> 15, -64, 63);
        for (unsigned k = 0; k < 4; ++k)
            part->rate_offset[k] = (int8_t)clamp(mod.destination[CF_VA_DST_LFO_RATE1 + k] >> 14, -127, 127);
    }

    uint32_t pwm_phase = part->pwm + (uint32_t)note->index * 0x20000000u;
    cf_va_oscillators(&voice->oscillators, p, mod.destination, m->pitch16, m->fine, m->shape,
                      pwm_phase, part->wow, acc, n);
    return cf_va_output(&voice->output, p, &mod, env, m->pitch16, m->cutoff, m->amp1,
                        note->velocity, note->unison, note->index, acc, out, side, n);
}
