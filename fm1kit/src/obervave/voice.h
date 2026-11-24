/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave a1f660c note lifecycle and track LFO. */
#pragma once
#include "control.h"

static const uint16_t obv_pan_q12[129] = {
    0, 50, 101, 151, 201, 251, 301, 351, 401, 451,
    501, 551, 601, 651, 700, 750, 799, 848, 897, 946,
    995, 1044, 1092, 1141, 1189, 1237, 1285, 1332, 1380, 1427,
    1474, 1521, 1567, 1614, 1660, 1706, 1751, 1797, 1842, 1886,
    1931, 1975, 2019, 2062, 2106, 2149, 2191, 2234, 2276, 2317,
    2359, 2399, 2440, 2480, 2520, 2559, 2598, 2637, 2675, 2713,
    2751, 2788, 2824, 2861, 2896, 2932, 2967, 3001, 3035, 3068,
    3102, 3134, 3166, 3198, 3229, 3260, 3290, 3320, 3349, 3378,
    3406, 3433, 3461, 3487, 3513, 3539, 3564, 3588, 3612, 3636,
    3659, 3681, 3703, 3724, 3745, 3765, 3784, 3803, 3822, 3839,
    3857, 3873, 3889, 3905, 3920, 3934, 3948, 3961, 3973, 3985,
    3996, 4007, 4017, 4027, 4036, 4044, 4052, 4059, 4065, 4071,
    4076, 4081, 4085, 4088, 4091, 4093, 4095, 4096, 4096,
};

static inline void obv_voice_pan(obv_voice_t *voice, int32_t pan)
{
    uint32_t pos = (uint32_t)(clampi(pan, -64, 64) + 64);
    voice->render.gl = obv_pan_q12[128u - pos];
    voice->render.gr = obv_pan_q12[pos];
}

static inline void obv_voice_start(obv_voice_t *voice, const uint8_t *params,
                                   uint8_t note, uint8_t velocity, int32_t previous_pitch,
                                   int32_t detune, int32_t pan)
{
    voice->gate = 1;
    voice->note = note;
    voice->vel = velocity;
    voice->det8 = detune;
    voice->target16 = (int32_t)note << 16;
    voice->pitch16 = params[P_GLIDE] ? previous_pitch : voice->target16;
    voice->fe.stage = E_ATT;
    voice->ae.stage = E_ATT;
    voice->lfo_blk = 0;
    voice->lfo_fade = params[P_LFO_DELAY] ? 0 : 32767;
    obv_voice_pan(voice, pan);
}

/* A new key in a legato group keeps the envelopes and modulation running. */
static inline void obv_voice_legato(obv_voice_t *voice, const uint8_t *params,
                                    uint8_t note, int32_t detune, int32_t pan)
{
    voice->gate = 1;
    voice->note = note;
    voice->det8 = detune;
    voice->target16 = (int32_t)note << 16;
    if (!params[P_GLIDE]) voice->pitch16 = voice->target16;
    obv_voice_pan(voice, pan);
}

static inline void obv_part_tick(obv_part_t *part, const uint8_t *params, uint32_t *random,
                                 int32_t bend)
{
    uint32_t old_phase = part->lfo_ph;
    part->lfo_ph += LFO_INC[params[P_LFO_RATE]];
    if (part->lfo_ph < old_phase) {
        part->lfo_rnd_a = part->lfo_rnd_b;
        part->lfo_rnd_b = xorshift(random);
    }
    part->lfo_val = obv_lfo_shape(part->lfo_ph, params[P_LFO_SHAPE],
                                  part->lfo_rnd_a, part->lfo_rnd_b);
    obv_block_globals(part, params, bend);
}
