/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown */
/* fm1-scales: ported from imijes/bass-something-2 at 4c8a82f. */
#pragma once
#include "filter.h"
static inline int bs2_clamp(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}
enum { SUB_SINE, SUB_PULSE, SUB_SQUARE, NSUBW };   /* the sub's waves, in the Bass Station II's order */
enum { RANGE_16, RANGE_8, RANGE_4, RANGE_2, NRANGE };   /* an oscillator's footage: RANGE_8 the note itself */
/* the envelopes' Triggering, the Bass Station II's (its NRPN and dump values): every note starts them; only a note not
 * played legato does (no key still held); Single's with the glide on legato notes alone */
enum { ENVT_MULTI, ENVT_SINGLE, ENVT_AUTOGLIDE, NENVT };
/* the modulation's destinations (app.c mod[]): the cutoff, oscillator 1's pitch, oscillator 2's. (Version 8's LFOs
 * each had one of these, an LFO's DEST, PITCH then both oscillators: bank_load converts them) */
enum { DEST_CUTOFF, DEST_PITCH, DEST_OSC2, NDEST };
/* an LFO's wave, the Bass Station II's (its dump's values): triangle, (falling) saw, square, sample and hold */
enum { LFO_TRI, LFO_SAW, LFO_SQUARE, LFO_SH, NLFOW };
#define NSYNCV 35u                       /* an LFO's sync values: its cycle in clock ticks, 64 beats .. a 32nd triplet */
#define NLFO 2u
typedef struct {
    uint8_t wave;                        /* LFO_* */
    uint8_t rate;                        /* its speed, the Bass Station II's 8 bits: 0..255, 0.21 .. 190 Hz (BANK_VERSION
                                          * 11's 0..127) */
    int8_t depth[2];                     /* where it goes, the Bass Station II's: LFO 1 oscillator 1's pitch and 2's,
                                          * LFO 2 the cutoff (and nothing); -63..+63, full 12 semitones or the
                                          * cutoff's whole range (BANK_VERSION 9's: a DEST_* and an amount) */
} lfo_patch_t;
typedef struct {
    float ph;                            /* 0..1, free-running (keysync: from 0 at a note) */
    float hold;                          /* S&H: the value held since the last wrap */
    float out;                           /* what it gives, slewed */
    float fade;                          /* its delay's fade-in since the note: 0..1 */
} lfo_t;
/* the arpeggiator and its sequencer, the Bass Station II's: one clock (the tempo, or MIDI clock when it comes in) and
 * one rhythm (two bars of 16ths); the arp plays the notes held (or latched) in its order over its octaves, the
 * sequencer (the arp's mode PLAY) a recorded sequence's steps, one at each of the rhythm's notes. The modes the
 * Bass Station II's, by its MIDI values (CC 118, a dump's; BANK_VERSION 5's had no UPDOWN2, 7's no PLAY, REC) */
enum { ARP_UP, ARP_DOWN, ARP_UPDOWN, ARP_UPDOWN2, ARP_PLAYED, ARP_RANDOM, ARP_PLAY, ARP_REC, NARPM };
#define NRHY 32u                         /* the arp's rhythms: 1..NRHY, each two bars of 16ths (NRHY: all notes) */
#define NSTEPS 32u
#define NSEQ 4u                          /* the sequences kept (the settings' mem): the arp's OCTAVES picks one */
#define SEQ_REST 128u                    /* a step: silent */
typedef struct {                         /* a sequence: its notes and rests, each step's legato a bit */
    uint8_t len;                         /* the steps recorded: 0 (none) .. NSTEPS */
    uint8_t step[NSTEPS];                /* each a MIDI note or SEQ_REST */
    uint8_t leg[NSTEPS / 8u];            /* step i's note held into the next one's (not played again: a tie when the
                                          * same note) */
} seqmem_t;
typedef struct {                         /* the sound's settings: the main loop writes them, the audio reads them;
                                          * stored as they are (app.c bank_t): a change here is a new BANK_VERSION */
    uint8_t wave1, wave2;                /* the oscillators' WAVE_* */
    int8_t coarse2;                      /* oscillator 2: semitones, -12..+12 */
    int8_t fine2;                        /* and cents, -100..+100 */
    uint8_t sub_oct;                     /* the sub oscillator: 1 or 2 octaves under oscillator 1 */
    uint8_t filter;                      /* FLT_* */
    uint8_t cutoff, reso, drive;         /* 0..127 */
    uint8_t menv_a, menv_d, menv_s, menv_r;   /* the mod envelope, 0..127: times 2 ms .. 10 s, sustain 0..1 */
    int8_t menv_cut, menv_o1;            /* its depths, -63..+63 as an LFO's: the cutoff, oscillator 1's pitch (and
                                          * oscillator 2's below; BANK_VERSION 3's DEST_* and amount here) */
    uint8_t sync;                        /* oscillator 2 hard-synced to 1: 0 or 1 (BANK_VERSION 1's spare byte) */
    lfo_patch_t lfo[NLFO];
    uint8_t ring;                        /* 0..127: the mix from oscillators 1 + 2 to their ring modulation 1 x 2 */
    uint8_t arp_on;                      /* the arpeggiator: 0 or 1 (from here on BANK_VERSION 3's) */
    uint8_t arp_mode;                    /* ARP_* */
    uint8_t arp_oct;                     /* its range: 1..4 octaves; in REC and PLAY the sequence, 1..NSEQ */
    uint8_t latch;                       /* the notes kept after the keys let go, until the next chord: 0 or 1 */
    uint8_t rate_v7;                     /* (version 7's sequencer step, a division: unused, the rhythm's 16ths now) */
    uint8_t gate;                        /* a step's note sounds for this share of it: 5..100 % */
    uint8_t tempo;                       /* 40..240 BPM, while no MIDI clock comes in */
    seqmem_t sq;                         /* the sequence the arp's PLAY plays (version 7's: 16 steps, a tie a step) */
    int8_t menv_o2;                      /* the mod envelope on oscillator 2's pitch (from here on BANK_VERSION 4's) */
    uint8_t pw1, pw2;                    /* the pulses' widths: 5..95 % */
    int8_t pwm1, pwm2;                   /* LFO 2 on them: -63..+63, full 45 % either way */
    uint8_t noise;                       /* white noise in the mix: 0..127 */
    uint8_t glide;                       /* the pitch from note to note: 0 at once, 1..127 3 ms .. 2.5 s */
    uint8_t rhythm;                      /* the arp's: 1..NRHY */
    uint8_t swing;                       /* the arp's (and so the sequencer's): 3..97 %, a pair's first 16th's share */
    uint8_t lvl1, lvl2, lvl_sub;         /* the mixer's levels: 0..127 (from here on BANK_VERSION 5's) */
    uint8_t sub_wave;                    /* SUB_* */
    uint8_t range1, range2;              /* the oscillators' RANGE_*: 16' .. 2' */
    uint8_t amp_a, amp_d, amp_s, amp_r;  /* the amplitude envelope, 0..127 as the mod envelope's */
    uint8_t dist;                        /* the distortion after the amplifier: 0 (none) .. 127 */
    uint8_t arp_retrig;                  /* SEQ RETRIG: the arp's notes and the sequence from their first at each
                                          * rhythm's start (else they run on across it): 0 or 1 (from
                                          * here on BANK_VERSION 6's) */
    int8_t coarse1;                      /* oscillator 1 (and the sub): semitones, -12..+12 (from here on BANK_VERSION
                                          * 7's: the Bass Station II's fields a dump had and the FM-1 had not) */
    int8_t fine1;                        /* and cents, -100..+100 */
    int8_t menv_pw1, menv_pw2;           /* the mod envelope on the pulses' widths: -63..+63, full 45 % either way */
    int8_t vel_amp, vel_menv;            /* velocity on the amp envelope (its level) and on the mod envelope (its
                                          * depths): -63..+63, 0 none, +63 a soft note quiet (as before), - inverted */
    uint8_t key_trk;                     /* the cutoff following the keys: 0 none .. 7 full (an octave an octave, from
                                          * C3; the Bass Station II's 7 .. 0) */
    uint8_t lvl_ext;                     /* the mixer's Ext: the computer's USB audio in, 0..127 (BANK_VERSION 9's) */
    uint8_t env_trig[2];                 /* the amp and the mod envelope's Triggering, ENVT_* (from here on
                                          * BANK_VERSION 11's) */
    uint8_t env_loop[2];                 /* their retriggering: 0 or 1 */
    uint8_t env_count[2];                /* its count: 0 for ever, 1..16 */
    uint8_t env_fixed[2];                /* fixed-duration sustain: 0 or 1 */
    uint8_t lfo_delay[2];                /* the LFOs' fade-in at a note: 0 (none) .. 127 (from here on BANK_VERSION
                                          * 12's) */
    uint8_t lfo_slew[2];                 /* their edges softened: 0 .. 127 */
    uint8_t lfo_keysync[2];              /* from the wave's start at a note: 0 or 1 */
    uint8_t lfo_sync[2];                 /* their speed the clock's (Speed/Sync at Sync): 0 or 1 */
    uint8_t lfo_syncv[2];                /* then their cycle: 0..NSYNCV-1 (64 beats .. a 32nd triplet) */
    uint8_t ofm;                         /* Osc Filter Mod: oscillator 2 on the cutoff at audio rate, 0..127 (from here
                                          * on BANK_VERSION 13's) */
    uint8_t lim;                         /* the VCA limiter: 0 (off) .. 127 */
} patch_t;
#define BSX_LEN 154u                     /* a dump with its name: F0 .. F7 */
#define BSX_MIN 122u                     /* one without (before firmware 2.5) */
#define BSX_NAME 137u                    /* the name's first byte */
#define BSX_NAME_N 16u

enum {                                   /* the fields the FM-1 has: (the first bit, counted from F0's, the width) */
    BX_SYNC, BX_W1, BX_RANGE1, BX_COARSE1, BX_FINE1, BX_W2, BX_RANGE2, BX_COARSE2, BX_FINE2, BX_SUBOCT,
    BX_LVL1, BX_LVL2, BX_RING, BX_FREQ, BX_RESO, BX_DRIVE, BX_SLOPE, BX_TYPE, BX_SHAPE,
    BX_MA, BX_MD, BX_MS, BX_MR, BX_L1WAVE, BX_L1SPEED, BX_L2WAVE, BX_L2SPEED,
    BX_LATCH, BX_ARP, BX_ARPOCT, BX_ARPMODE, BX_L1OSC1, BX_L1OSC2, BX_L2FILT, BX_MEOSC1, BX_MEOSC2, BX_MEFILT,
    BX_GLIDE, BX_PW1, BX_PW2, BX_NOISE, BX_RHYTHM, BX_SWING, BX_PWM1, BX_PWM2,
    BX_SUBWAVE, BX_LVLSUB, BX_AA, BX_AD, BX_AS, BX_AR, BX_DIST, BX_MEPW1, BX_MEPW2, BX_VAMP, BX_VMOD, BX_TRACK, BX_ATRIG, BX_MTRIG, BX_ALOOP, BX_MLOOP,
    BX_L1DELAY, BX_L1SLEW, BX_L1KSYNC, BX_L1SSYNC, BX_L1SYNCV, BX_L2DELAY, BX_L2SLEW, BX_L2KSYNC, BX_L2SSYNC, BX_L2SYNCV,
    BX_OFM, NBX
};
static const struct { uint16_t pos; uint8_t w; } BX[NBX] = {
    [BX_SYNC] = {126, 1},   [BX_W1] = {133, 2},     [BX_RANGE1] = {144, 7},  [BX_COARSE1] = {151, 8},
    [BX_FINE1] = {159, 8},  [BX_W2] = {173, 2},     [BX_RANGE2] = {184, 7},  [BX_COARSE2] = {191, 8},
    [BX_FINE2] = {199, 8},  [BX_SUBOCT] = {262, 1}, [BX_LVL1] = {263, 8},    [BX_LVL2] = {271, 8},
    [BX_RING] = {295, 8},   [BX_FREQ] = {311, 8},   [BX_RESO] = {320, 7},    [BX_DRIVE] = {328, 7},
    [BX_SLOPE] = {339, 1},  [BX_TYPE] = {340, 1},   [BX_SHAPE] = {341, 2},   [BX_MA] = {400, 7},
    [BX_MD] = {408, 7},     [BX_MS] = {416, 7},     [BX_MR] = {424, 7},      [BX_L1WAVE] = {445, 2},
    [BX_L1SPEED] = {463, 8}, [BX_L2WAVE] = {493, 2}, [BX_L2SPEED] = {511, 8}, [BX_LATCH] = {541, 1},
    [BX_ARP] = {542, 1},    [BX_ARPOCT] = {548, 3}, [BX_ARPMODE] = {556, 3}, [BX_L1OSC1] = {631, 8},
    [BX_L1OSC2] = {639, 8}, [BX_L2FILT] = {679, 8}, [BX_MEOSC1] = {688, 7},  [BX_MEOSC2] = {696, 7},
    [BX_MEFILT] = {736, 7}, [BX_GLIDE] = {96, 7},   [BX_PW1] = {136, 7},     [BX_PW2] = {176, 7},
    [BX_NOISE] = {287, 8},  [BX_RHYTHM] = {562, 5}, [BX_SWING] = {568, 7},   [BX_PWM1] = {656, 7},
    [BX_PWM2] = {664, 7},   [BX_SUBWAVE] = {253, 2}, [BX_LVLSUB] = {279, 8}, [BX_AA] = {352, 7},
    [BX_AD] = {360, 7},     [BX_AS] = {368, 7},     [BX_AR] = {376, 7},      [BX_DIST] = {752, 7},
    [BX_MEPW1] = {712, 7},  [BX_MEPW2] = {720, 7},  [BX_VAMP] = {344, 7},    [BX_VMOD] = {392, 7},
    [BX_TRACK] = {788, 3},
    /* the envelopes' Triggering (0 Multi, 1 Single, 2 Autoglide) and retriggering (firmware 2.5's): BS2-Web's
     * nrpn.js, its byte and mask as a bit here (byte x 7 + 6 - the mask's top bit) */
    [BX_ATRIG] = {389, 2},  [BX_MTRIG] = {437, 2},  [BX_ALOOP] = {798, 1},  [BX_MLOOP] = {806, 1},
    /* the LFOs' delay and slew, keysync, Speed/Sync and sync value: BS2-Web's (cc.js, nrpn.js), as the above */
    [BX_L1DELAY] = {448, 7}, [BX_L1SLEW] = {456, 7}, [BX_L1KSYNC] = {485, 1}, [BX_L1SSYNC] = {486, 1},
    [BX_L1SYNCV] = {473, 6}, [BX_L2DELAY] = {496, 7}, [BX_L2SLEW] = {504, 7}, [BX_L2KSYNC] = {533, 1},
    [BX_L2SSYNC] = {534, 1}, [BX_L2SYNCV] = {521, 6},
    [BX_OFM] = {744, 7},                 /* Osc Filter Mod (cc.js's fx_osc_filter_mod) */
};

static uint32_t bsx_get(const uint8_t *d, uint32_t f)   /* d: the dump from its F0 */
{
    uint32_t v = 0, i, p;
    for (i = 0; i < BX[f].w; i++) {
        p = BX[f].pos + i;
        v = v << 1 | (d[p / 7u] >> (6u - p % 7u) & 1u);
    }
    return v;
}
static int32_t bsx_c7(uint32_t v) { return v < 64u ? (int32_t)v - 63 : (int32_t)v - 64; }
static int32_t bsx_c8(uint32_t v) { return v < 128u ? (int32_t)v - 127 : (int32_t)v - 128; }
static int32_t bsx_semis(uint32_t v)
{
    int32_t x = (2 * (int32_t)v - 255) * 12;
    return (x < 0 ? x - 127 : x + 127) / 255;   /* rounded */
}
static uint32_t bsx_pw(uint32_t v) { return (v * 90u + 76u) / 127u + 5u; }   /* a manual PW's 0..127 to 5..95 % */
static const uint8_t BSX_ARP_IN[8] = {ARP_UP, ARP_DOWN, ARP_UPDOWN, ARP_UPDOWN2, ARP_PLAYED, ARP_RANDOM, ARP_PLAY,
                                      ARP_REC};   /* (the sequencer's play and record: BS2-Web's 6 and 7) */
static const uint8_t BSX_WAVE_IN[4] = {WAVE_SINE, WAVE_TRI, WAVE_SAW, WAVE_PULSE};
/* a frame from USB (p: the bytes between F0 and F7) into *o: 1 when it is a Bass Station II patch dump. Only the
 * fields a dump has are written (the tempo, the gate, the steps are left); then the caller's patch_sane */
static inline int bsx_import(const uint8_t *p, uint32_t n, patch_t *o, char *name)
{
    static const uint8_t HDR[5] = {0x00, 0x20, 0x29, 0x00, 0x33};
    uint8_t d[BSX_LEN];
    uint32_t i, r, l1, l2, m, t;
    if (n != BSX_MIN - 2u && n != BSX_LEN - 2u)
        return 0;
    for (i = 0; i < 5u; i++)
        if (p[i] != HDR[i])
            return 0;
    d[0] = 0xF0;
    for (i = 0; i < n; i++)
        d[i + 1u] = p[i] & 0x7Fu;
    o->wave1 = BSX_WAVE_IN[bsx_get(d, BX_W1)];
    o->wave2 = BSX_WAVE_IN[bsx_get(d, BX_W2)];
    o->pw1 = (uint8_t)bsx_pw(bsx_get(d, BX_PW1));
    o->pw2 = (uint8_t)bsx_pw(bsx_get(d, BX_PW2));
    o->pwm1 = (int8_t)bsx_c7(bsx_get(d, BX_PWM1));   /* (LFO 2 on the widths) */
    o->pwm2 = (int8_t)bsx_c7(bsx_get(d, BX_PWM2));
    o->noise = (uint8_t)(bsx_get(d, BX_NOISE) >> 1);
    o->glide = (uint8_t)bsx_get(d, BX_GLIDE);
    o->range1 = (uint8_t)bs2_clamp((int32_t)bsx_get(d, BX_RANGE1) - 63, RANGE_16, RANGE_2);   /* (63..66: 16' .. 2') */
    o->range2 = (uint8_t)bs2_clamp((int32_t)bsx_get(d, BX_RANGE2) - 63, RANGE_16, RANGE_2);
    o->coarse1 = (int8_t)bs2_clamp(bsx_semis(bsx_get(d, BX_COARSE1)), -12, 12);
    o->fine1 = (int8_t)bs2_clamp(bsx_c8(bsx_get(d, BX_FINE1)), -100, 100);
    o->coarse2 = (int8_t)bs2_clamp(bsx_semis(bsx_get(d, BX_COARSE2)), -12, 12);
    o->fine2 = (int8_t)bs2_clamp(bsx_c8(bsx_get(d, BX_FINE2)), -100, 100);
    o->menv_pw1 = (int8_t)bsx_c7(bsx_get(d, BX_MEPW1));
    o->menv_pw2 = (int8_t)bsx_c7(bsx_get(d, BX_MEPW2));
    o->vel_amp = (int8_t)bsx_c7(bsx_get(d, BX_VAMP));
    o->vel_menv = (int8_t)bsx_c7(bsx_get(d, BX_VMOD));
    o->env_trig[0] = (uint8_t)bsx_get(d, BX_ATRIG);   /* the envelopes' Triggering and retriggering; the retriggering's
                                                       * count and the fixed sustain (firmware 4's: their places not
                                                       * known) as the INIT PATCH has them */
    o->env_trig[1] = (uint8_t)bsx_get(d, BX_MTRIG);
    o->env_loop[0] = (uint8_t)bsx_get(d, BX_ALOOP);
    o->env_loop[1] = (uint8_t)bsx_get(d, BX_MLOOP);
    o->env_count[0] = o->env_count[1] = 0;
    o->env_fixed[0] = o->env_fixed[1] = 0;
    o->key_trk = (uint8_t)(7u - bsx_get(d, BX_TRACK));
    o->sub_oct = bsx_get(d, BX_SUBOCT) ? 1u : 2u;
    o->sub_wave = (uint8_t)bsx_get(d, BX_SUBWAVE);   /* (held to its range by the caller) */
    o->sync = (uint8_t)bsx_get(d, BX_SYNC);
    o->filter = bsx_get(d, BX_TYPE) ? FLT_ACID
              : (uint8_t)((uint32_t)bs2_clamp((int32_t)bsx_get(d, BX_SHAPE), 0, 2) * 2u + (bsx_get(d, BX_SLOPE) ? 0u : 1u));
    o->cutoff = (uint8_t)(bsx_get(d, BX_FREQ) >> 1);
    o->reso = (uint8_t)bsx_get(d, BX_RESO);
    o->drive = (uint8_t)bsx_get(d, BX_DRIVE);
    o->menv_a = (uint8_t)bsx_get(d, BX_MA);
    o->menv_d = (uint8_t)bsx_get(d, BX_MD);
    o->menv_s = (uint8_t)bsx_get(d, BX_MS);
    o->menv_r = (uint8_t)bsx_get(d, BX_MR);
    o->amp_a = (uint8_t)bsx_get(d, BX_AA);
    o->amp_d = (uint8_t)bsx_get(d, BX_AD);
    o->amp_s = (uint8_t)bsx_get(d, BX_AS);
    o->amp_r = (uint8_t)bsx_get(d, BX_AR);
    o->dist = (uint8_t)bsx_get(d, BX_DIST);
    o->ofm = (uint8_t)bsx_get(d, BX_OFM);
    o->lim = 0;                          /* (the VCA limiter: its place in a dump not known) */
    o->menv_cut = (int8_t)bsx_c7(bsx_get(d, BX_MEFILT));   /* the mod envelope's three depths */
    o->menv_o1 = (int8_t)bsx_c7(bsx_get(d, BX_MEOSC1));
    o->menv_o2 = (int8_t)bsx_c7(bsx_get(d, BX_MEOSC2));
    for (i = 0; i < NLFO; i++) {             /* LFO 1 on the oscillators' pitches, LFO 2 the cutoff; each its wave, its
                                              * speed's 8 bits, delay, slew, keysync and sync */
        o->lfo[i].wave = (uint8_t)bsx_get(d, i ? BX_L2WAVE : BX_L1WAVE);
        o->lfo[i].rate = (uint8_t)bsx_get(d, i ? BX_L2SPEED : BX_L1SPEED);
        o->lfo_delay[i] = (uint8_t)bsx_get(d, i ? BX_L2DELAY : BX_L1DELAY);
        o->lfo_slew[i] = (uint8_t)bsx_get(d, i ? BX_L2SLEW : BX_L1SLEW);
        o->lfo_keysync[i] = (uint8_t)bsx_get(d, i ? BX_L2KSYNC : BX_L1KSYNC);
        o->lfo_sync[i] = (uint8_t)bsx_get(d, i ? BX_L2SSYNC : BX_L1SSYNC);
        o->lfo_syncv[i] = (uint8_t)bsx_get(d, i ? BX_L2SYNCV : BX_L1SYNCV);
    }
    o->lfo[0].depth[0] = (int8_t)bs2_clamp(bsx_c8(bsx_get(d, BX_L1OSC1)) / 2, -63, 63);
    o->lfo[0].depth[1] = (int8_t)bs2_clamp(bsx_c8(bsx_get(d, BX_L1OSC2)) / 2, -63, 63);
    o->lfo[1].depth[0] = (int8_t)bs2_clamp(bsx_c8(bsx_get(d, BX_L2FILT)) / 2, -63, 63);
    o->lfo[1].depth[1] = 0;
    /* the mixer: the ring's share of it and the louder oscillator's (the FM-1's ring a fade between them), that
     * oscillator at 127 and the rest in proportion; without either, the sub as it is */
    r = bsx_get(d, BX_RING);
    l1 = bsx_get(d, BX_LVL1);
    l2 = bsx_get(d, BX_LVL2);
    m = l1 > l2 ? l1 : l2;
    t = r + m;
    o->ring = (uint8_t)(t ? (r * 127u + t / 2u) / t : 0u);
    o->lvl1 = (uint8_t)(m ? (l1 * 127u + m / 2u) / m : 0u);
    o->lvl2 = (uint8_t)(m ? (l2 * 127u + m / 2u) / m : 0u);
    o->lvl_sub = (uint8_t)(t ? bs2_clamp((int32_t)((bsx_get(d, BX_LVLSUB) * 127u + t / 2u) / t), 0, 127)
                             : (int32_t)(bsx_get(d, BX_LVLSUB) >> 1));
    o->arp_on = (uint8_t)bsx_get(d, BX_ARP);
    o->latch = (uint8_t)bsx_get(d, BX_LATCH);
    o->arp_oct = (uint8_t)bs2_clamp((int32_t)bsx_get(d, BX_ARPOCT), 1, 4);
    o->arp_mode = BSX_ARP_IN[bsx_get(d, BX_ARPMODE)];
    o->rhythm = (uint8_t)(bsx_get(d, BX_RHYTHM) + 1u);   /* (its 0..31 as 1..32) */
    o->swing = (uint8_t)bsx_get(d, BX_SWING);
    name[0] = 0;
    if (n + 2u == BSX_LEN) {                 /* the name, its spaces at the end left off */
        for (i = 0; i < BSX_NAME_N; i++)
            name[i] = (char)(d[BSX_NAME + i] >= 0x20u ? d[BSX_NAME + i] : ' ');
        while (i && name[i - 1u] == ' ')
            i--;
        name[i] = 0;
    }
    return 1;
}

