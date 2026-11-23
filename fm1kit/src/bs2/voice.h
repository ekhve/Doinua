/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown */
/* fm1-scales: ported from imijes/bass-something-2 at 4c8a82f. */
#pragma once
#include "patch.h"
#include "envelope.h"
#define BS2_RATE 44100.0f
typedef struct {
    float phase, phase2, phase_sub;      /* the oscillators, 0..1 */
    float pitch;                         /* gliding to the note: a MIDI note, in semitones */
    float o1, o2;                        /* oscillators 1 and 2 a sample late: a sync's step corrected on both sides */
    ladder_t flt;
    env_t amp, menv;                     /* the amplitude envelope, the mod envelope */
    adaa_t dist;                         /* the distortion */
    float lim_e;                         /* the VCA limiter's level followed */
    float dc_x, dc_y;                    /* the output's coupling: the filter's last sample, and its own */
    float vel_now;
    float vel;                           /* the note's level from its velocity (VELOCITY AMP), 0..1 */
    float vm;                            /* and the mod envelope's depths' (VELOCITY MOD), 0..1 */
    float drift[2], drift_x[2], drift_t[2];   /* oscillators 1 and 2's wander (semitones), its first smoothing, its aim */
    float sag_f, sag_g;                  /* the supply's sag: the output's level followed, the gain it leaves */
    float heat;                          /* the filter's warming: 0 cold .. 1 (playing loud, resonance high) */
    int32_t note;                        /* the note it sounds, kept through the release */
    uint32_t trig;                       /* the note-ons it has started */
} voice_t;

typedef struct {
    float inc, inv, inc2, inv2, incs, invs, pw1, pw2, ring, l1, l2, ls, noise, le, ofd, dg, dm, sg, lim_th;
    uint32_t wave, wave2, subw, sync, need1, need2, awake, ofm, dist, lim;
    const int32_t *ext;                      /* Ext's samples at the step's first (NULL: none) */
    const fcoef_t *fc;                       /* the filter's for the step; Osc Filter Mod's, made each sample */
    fcoef_t *fm;
    const adsr_t *aenv;
    uint32_t rng;                            /* the noise's (back in it after) */
} vstep_t;
static inline void voice_run(voice_t *voice, float *out, uint32_t m, vstep_t *k)
{
    const float inc = k->inc, inv = k->inv, inc2 = k->inc2, incs = k->incs;
    uint32_t j, rng = k->rng;
    for (j = 0; j < m; j++) {
        float s, o1, o2;
        uint32_t wrap = (voice->phase += inc) >= 1.0f;
        voice->vel_now += (voice->vel - voice->vel_now) * 0.004f;
        if (wrap)
            voice->phase -= 1.0f;
        if ((voice->phase2 += inc2) >= 1.0f)
            voice->phase2 -= 1.0f;
        if ((voice->phase_sub += incs) >= 1.0f)
            voice->phase_sub -= 1.0f;
        if (!k->awake) {
            rng = rng * 1664525u + 1013904223u;   /* (the noise's on, as if heard; sync's restarts too) */
            if (wrap && k->sync)
                voice->phase2 = voice->phase * inv * inc2;
            out[j] = 0.0f;
            continue;
        }
        o1 = k->need1 ? osc(voice->phase, inc, inv, k->wave, k->pw1) : 0.0f;
        if (wrap && k->sync)                /* hard sync: 1's wrap restarts 2 */
            o2 = k->need2 ? osc_sync(&voice->phase2, &voice->o2, voice->phase * inv, inc2, k->inv2, k->wave2, k->pw2)
                       : (voice->phase2 = voice->phase * inv * inc2, 0.0f);
        else
            o2 = k->need2 ? osc(voice->phase2, inc2, k->inv2, k->wave2, k->pw2) : 0.0f;
        rng = rng * 1664525u + 1013904223u;
        /* the mixer: 1 + 2 at their levels faded to their ring modulation (twice: as loud), the sub (its narrow
         * pulse a quarter), the noise and Ext (full scale as loud as an oscillator); 1 and 2 a sample late */
        s = 0.4f * ((1.0f - k->ring) * (k->l1 * voice->o1 + k->l2 * voice->o2) + 2.0f * k->ring * voice->o1 * voice->o2
                    + (k->ls != 0.0f ? k->ls * osc(voice->phase_sub, incs, k->invs, k->subw, 0.25f) : 0.0f)
                    + k->noise * (float)(int32_t)rng * (1.0f / 2147483648.0f)
                    + (k->ext ? k->le * (float)(k->ext[2u * j] + k->ext[2u * j + 1u]) : 0.0f));
        voice->o1 = o1;
        voice->o2 = o2;
        if (k->ofm) {                 /* Osc Filter Mod: oscillator 2 (as it is, before its level) on the cutoff,
                                              * up to 3 octaves either way (the guide gives no scale) */
            fcoef_fm(k->fm, k->fc, vm_exp2f(k->ofd * o2));
            s = ladder(&voice->flt, k->fm, s);
        } else
            s = ladder(&voice->flt, k->fc, s);
        /* the output's coupling (a one-pole high pass at 5 Hz): what DC the filter passes (the ring's, a tanh's
         * on an uneven wave) settles while the voice runs between notes, so the amplifier's gate is no step */
        voice->dc_y = s - voice->dc_x + (1.0f - 2.0f * 3.14159265359f * 5.0f / BS2_RATE) * voice->dc_y;
        voice->dc_x = s;
        s = voice->dc_y;
        /* the distortion before the VCA, the Bass Station II's: its character the same as the envelope fades */
        if (k->dist)
            s = overdrive(&voice->dist, s * k->dg) * k->dm;
        else                                 /* (off: from 0 when it comes on, no step) */
            voice->dist.x1 = voice->dist.F1 = 0.0f;
        s *= adsr_step(&voice->amp, k->aenv) * voice->vel_now;
        voice->sag_f += (vm_fabsf(s) - voice->sag_f) * (vm_fabsf(s) > voice->sag_f ? 0.01f : 0.0003f);
        voice->sag_g += (k->sg - voice->sag_g) * 0.02f;
        s *= voice->sag_g;
        if (k->lim) {                 /* the VCA limiter: over its threshold (0 to -24 dB under full), the level
                                              * held to it; the level followed at once up, over about 50 ms down */
            float a = vm_fabsf(s);
            voice->lim_e = a > voice->lim_e ? a : voice->lim_e * 0.99955f;
            if (voice->lim_e > k->lim_th)
                s *= k->lim_th / voice->lim_e;
        }
        out[j] = s;
    }
    k->rng = rng;
}
