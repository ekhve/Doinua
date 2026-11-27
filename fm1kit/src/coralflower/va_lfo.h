/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca)
 * CoralFlower 46790b9 LFO clock and waves. Include after Felucca's dsp.c. */
#pragma once

static const uint32_t cf_va_lfo_free_increment[128] = {
    155826, 164248, 173124, 182481, 192343, 202738, 213695, 225244,
    237417, 250248, 263773, 278028, 293054, 308892, 325586, 343183,
    361730, 381279, 401885, 423605, 446499, 470630, 496065, 522874,
    551133, 580919, 612314, 645406, 680287, 717053, 755806, 796653,
    839708, 885090, 932924, 983343, 1036488, 1092504, 1151548, 1213783,
    1279382, 1348525, 1421406, 1498225, 1579196, 1664543, 1754503, 1849324,
    1949270, 2054618, 2165658, 2282701, 2406068, 2536103, 2673166, 2817636,
    2969914, 3130422, 3299604, 3477930, 3665893, 3864015, 4072844, 4292959,
    4524970, 4769520, 5027287, 5298984, 5585366, 5887224, 6205397, 6540765,
    6894258, 7266855, 7659589, 8073548, 8509880, 8969793, 9454561, 9965529,
    10504112, 11071802, 11670173, 12300882, 12965678, 13666403, 14404998, 15183510,
    16004096, 16869031, 17780710, 18741661, 19754546, 20822172, 21947498, 23133641,
    24383889, 25701706, 27090743, 28554851, 30098086, 31724724, 33439273, 35246484,
    37151366, 39159195, 41275537, 43506256, 45857533, 48335884, 50948176, 53701648,
    56603931, 59663066, 62887531, 66286261, 69868674, 73644696, 77624792, 81819991,
    86241918, 90902825, 95815630, 100993944, 106452119, 112205278, 118269365, 124661182,
};

typedef struct {
    uint32_t phase;
    uint32_t random;
    int16_t raw;
    int16_t output;
} cf_va_lfo_t;

static inline uint32_t cf_va_lfo_increment(int32_t rate, int synced, int32_t bpm)
{
    /* Phase increment per control block, per BPM, in Q4. */
    static const uint32_t division[] = {25971, 51942, 103884, 207769, 415537, 554050, 831075,
                                       1246612, 1108099, 1662149, 2493224, 3324298, 4986447, 6648596};
    rate = clamp(rate, 0, 127);
    if (!synced) return cf_va_lfo_free_increment[rate];
    return (division[(unsigned)rate * 14u >> 7] * (uint32_t)clamp(bpm, 20, 300)) >> 4;
}

static inline uint32_t cf_va_lfo_random(uint32_t seed)
{
    uint32_t x = seed ? seed : 0x6c8e9cf5u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static inline void cf_va_lfo_tick(cf_va_lfo_t *lfo, unsigned slot, uint32_t increment,
                                  unsigned wave, unsigned depth)
{
    uint32_t old = lfo->phase;
    uint32_t phase = lfo->phase += increment;
    if (phase < old) lfo->random = cf_va_lfo_random(lfo->random + slot);

    int32_t sample;
    switch (wave) {
    case 1: sample = osc_tri(phase); break;
    case 2: sample = (int32_t)(phase >> 16) - 32768; break;
    case 3: sample = phase < 0x80000000u ? 32767 : -32767; break;
    case 4: sample = (int32_t)(lfo->random >> 16) - 32768; break;
    default: sample = osc_sine(phase); break;
    }
    lfo->raw = (int16_t)sample;
    lfo->output = (int16_t)((sample * (int32_t)depth * 258) >> 15);
}
