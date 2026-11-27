/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 modulation matrix. Include after dsp.c, va_envelope.h and va_lfo.h. */
#pragma once

/* Source and destination numbers are fixed by the upstream patch format. */
enum {
    CF_VA_SRC_OFF, CF_VA_SRC_ENV1, CF_VA_SRC_LFO1 = CF_VA_SRC_ENV1 + 4,
    CF_VA_SRC_VEL = CF_VA_SRC_LFO1 + 4, CF_VA_SRC_KEY, CF_VA_SRC_RANDOM,
    CF_VA_SRC_WHEEL, CF_VA_SRC_COUNT
};
enum {
    CF_VA_DST_OFF, CF_VA_DST_PITCH, CF_VA_DST_OSC_PITCH1,
    CF_VA_DST_OSC_LEVEL1 = CF_VA_DST_OSC_PITCH1 + 4,
    CF_VA_DST_OSC_SHAPE1 = CF_VA_DST_OSC_LEVEL1 + 4,
    CF_VA_DST_CUTOFF = CF_VA_DST_OSC_SHAPE1 + 4, CF_VA_DST_RESONANCE,
    CF_VA_DST_AMP, CF_VA_DST_PAN, CF_VA_DST_LFO_RATE1,
    CF_VA_DST_FILTER_TYPE = CF_VA_DST_LFO_RATE1 + 4, CF_VA_DST_DRIVE,
    CF_VA_DST_SPREAD, CF_VA_DST_FILTER_ENV, CF_VA_DST_LFO_DEPTH1,
    CF_VA_DST_COUNT = CF_VA_DST_LFO_DEPTH1 + 4
};

typedef struct {
    int32_t destination[CF_VA_DST_LFO_DEPTH1];
    int32_t gain;
} cf_va_modulation_t;

static inline int32_t cf_va_fade_lfo(int32_t sample, int32_t fade)
{
    return fade >= 32767 ? sample : (sample * fade) >> 15;
}

/* Matrix routes are eight signed-byte triples (source, destination, amount).
 * note_sources are velocity, key, random per note, and mod wheel in Q15. */
static inline void cf_va_modulate(cf_va_modulation_t *mod,
                                  const int8_t routes[24], const int32_t envelopes[4],
                                  const cf_va_lfo_t lfo[4], const uint8_t depths[4],
                                  const uint8_t fades[4], uint16_t ticks,
                                  const int32_t note_sources[4], int depth_modulation)
{
    int32_t source[CF_VA_SRC_COUNT] = {0};
    int32_t fade[4];
    *mod = (cf_va_modulation_t){.gain = 32767};

    for (unsigned i = 0; i < 4; ++i) {
        fade[i] = fades[i] ? (int32_t)ticks * (int32_t)(cf_va_env_lin[fades[i] & 127] >> 9) : 32767;
        source[CF_VA_SRC_ENV1 + i] = envelopes[i];
        source[CF_VA_SRC_LFO1 + i] = cf_va_fade_lfo(lfo[i].output, fade[i]);
        source[CF_VA_SRC_VEL + i] = note_sources[i];
    }

    /* Resolve LFO depth routes using the initial sources before applying other routes. */
    if (depth_modulation) {
        int32_t depth_offset[4] = {0};
        for (unsigned i = 0; i < 24; i += 3) {
            unsigned src = (uint8_t)routes[i];
            unsigned dst = (uint8_t)routes[i + 1];
            if (src && src < CF_VA_SRC_COUNT && dst >= CF_VA_DST_LFO_DEPTH1 && dst < CF_VA_DST_COUNT)
                depth_offset[dst - CF_VA_DST_LFO_DEPTH1] += source[src] * routes[i + 2];
        }
        for (unsigned i = 0; i < 4; ++i) {
            if (depth_offset[i]) {
                int32_t depth = clamp(depths[i] + (depth_offset[i] >> 14), 0, 127);
                int32_t sample = (lfo[i].raw * depth * 258) >> 15;
                source[CF_VA_SRC_LFO1 + i] = cf_va_fade_lfo(sample, fade[i]);
            }
        }
    }

    for (unsigned i = 0; i < 24; i += 3) {
        unsigned src = (uint8_t)routes[i];
        unsigned dst = (uint8_t)routes[i + 1];
        int32_t amount = routes[i + 2];
        if (!src || src >= CF_VA_SRC_COUNT || !dst || dst >= CF_VA_DST_LFO_DEPTH1 || !amount)
            continue;
        int32_t sample = source[src];
        if (dst == CF_VA_DST_AMP) {
            if ((src >= CF_VA_SRC_LFO1 && src < CF_VA_SRC_VEL) || src >= CF_VA_SRC_KEY)
                sample = (sample + 32768) >> 1;
            sample = clamp(sample, 0, 32767);
            int32_t attenuation = amount > 0 ? (32767 - sample) * amount : sample * -amount;
            mod->gain = mulq15(mod->gain, clamp(32767 - (attenuation >> 6), 0, 32767));
        } else {
            mod->destination[dst] += sample * amount;
        }
    }
}
