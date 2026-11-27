/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave a1f660c, per-track and per-voice control for each 16-frame block. */
#pragma once
#include "render.h"
#include "glide.h"
#include "lfo.h"

enum {
    P_O1_WAVE, P_O1_PW, P_O1_LVL, P_NOISE,
    P_O2_WAVE, P_O2_PW, P_O2_LVL, P_O2_SEMI,
    P_SYNC, P_XMOD, P_ENV_O2, P_O2_FINE,
    P_CUTOFF, P_RESO, P_FENV_AMT, P_KEYTRK, P_FTYPE,
    P_DRIVE, P_VEL_FLT, P_VEL_AMP,
    P_FA, P_FD, P_FS, P_FR,
    P_AA, P_AD, P_AS, P_AR,
    P_LFO_SHAPE, P_LFO_RATE, P_LFO_DELAY, P_LFO_PHASE,
    P_LFO_PITCH, P_LFO_PWM, P_LFO_FLT, P_LFO_AMP, P_LFO_PITCH_TO, P_LFO_PWM_TO,
    P_MODE, P_UNI_N, P_UNI_DET, P_GLIDE,
    P_VINTAGE, P_SPREAD, P_BEND, P_VOLUME,
    P_MOD_DEPTH,
    P_ARP_MODE, P_ARP_DIV, P_ARP_OCT, P_ARP_GATE,
    P_FMORPH,                                   /* (appended: the MIDI CC numbers of the above stay) */
    P_CHO_RATE, P_CHO_DEPTH, P_CHO_MIX,
    P_DLY_TIME, P_DLY_FB, P_DLY_TONE, P_DLY_MIX,
    P_REV_SIZE, P_REV_DAMP, P_REV_PRE, P_REV_MIX,
    NPARAM
};

enum { LP_SYNC, LP_FREE, LP_SPREAD };
enum { TO_O1, TO_O2, TO_BOTH };

typedef struct {
    obv_render_controls_t render;
    uint32_t lfo_ph, lfo_rnd_a, lfo_rnd_b, glide_k;
    int32_t lfo_val, fenv_amt, keytrk, velflt, lfo_pd, lfo_fd, lfo_pwd, lfo_ad;
    int32_t wheel_q, bend8, env_o2, det_fine8, cut_sm, reso_sm, reso_k, lad_k;
    int32_t vint, sus_f, sus_a, gain_q12;
    uint32_t pw1_base, pw2_base;
} obv_part_t;

typedef struct {
    obv_render_voice_t render;
    env_t fe, ae;
    int32_t pitch16, target16, det8, drift, lfo_fade;
    uint32_t lfo_blk;
    uint8_t note, vel, gate;
} obv_voice_t;

static const int16_t VINT_TUNE8[8] = {-20, 12, 7, -15, 18, -9, 3, -17};     /* +-8 cents */
static const int16_t VINT_CUT8[8] = {-300, 250, 120, -380, 60, 330, -180, 220};   /* +-1.5 semitones */
static const int8_t VINT_ENV[8] = {10, -6, 14, -12, 3, -16, 8, -4};         /* /128: +-12 % on the times */

static inline void obv_block_globals(obv_part_t *g, const uint8_t *pv, int32_t bend)            /* a sound's derived numbers, once per block */
{
    int32_t d;
    g->fenv_amt = (int32_t)pv[P_FENV_AMT] - 64;
    g->keytrk = pv[P_KEYTRK];
    g->velflt = pv[P_VEL_FLT];
    d = pv[P_LFO_PITCH]; g->lfo_pd = (d * d) >> 7;
    d = pv[P_LFO_FLT]; g->lfo_fd = (d * d) >> 7;
    d = pv[P_LFO_PWM]; g->lfo_pwd = (d * d) >> 7;
    g->lfo_ad = pv[P_LFO_AMP];
    g->wheel_q = pv[P_MOD_DEPTH];               /* the knob and the mod wheel (CC 1) are the same value */
    g->bend8 = (bend * pv[P_BEND] * 256) >> 13;
    g->env_o2 = (int32_t)pv[P_ENV_O2] - 64;
    g->det_fine8 = ((int32_t)pv[P_O2_FINE] - 64) * 2;
    d = pv[P_O1_LVL]; g->render.lvl1 = (d * d) >> 7;
    d = pv[P_O2_LVL]; g->render.lvl2 = (d * d) >> 7;
    d = pv[P_NOISE]; g->render.lvln = (d * d) >> 7;
    d = pv[P_DRIVE]; g->render.drive_q12 = 4096 + (d * d * 15 * 4096 / 16129);   /* 1 .. 16 into the filter's input stage */
    g->cut_sm += ((pv[P_CUTOFF] << 8) - g->cut_sm) >> 6;   /* tau = 64 blocks = 23 ms */
    g->reso_sm += ((pv[P_RESO] << 8) - g->reso_sm) >> 6;
    g->reso_k = 8192 - g->reso_sm * 8400 / (127 * 256);
    g->lad_k = (8192 - g->reso_k) >> 1;         /* the ladder's feedback, Q10: 0 .. 4.1 */
    g->vint = pv[P_VINTAGE];
    g->sus_f = pv[P_FS] * (ENV_ONE / 127);
    g->sus_a = pv[P_AS] * (ENV_ONE / 127);
    d = pv[P_VOLUME];
    g->gain_q12 = (d * d) >> 2;                 /* (x CC 7 at the output: as VOLUME x CC 7 squared was) */
    g->pw1_base = ((uint32_t)pv[P_O1_PW] << 25) + (1u << 24);
    g->pw2_base = ((uint32_t)pv[P_O2_PW] << 25) + (1u << 24);
    g->render.w1 = pv[P_O1_WAVE];
    g->render.w2 = pv[P_O2_WAVE];
    g->render.ftype = pv[P_FTYPE];
    g->render.morph_q = pv[P_FMORPH];                  /* SEM: 0 LP, 64 notch, 127 HP */
    g->render.sync_on = pv[P_SYNC];
    d = pv[P_XMOD]; g->render.xmod_q = (uint32_t)((d * d) >> 7);
    g->glide_k = obv_glide_coefficient[pv[P_GLIDE]];
}

static inline uint32_t obv_pw_mod(uint32_t base, int32_t mod)
{
    const uint32_t lo = 1u << 26, hi = UINT32_MAX - lo;
    uint32_t offset = (uint32_t)(mod < 0 ? -mod : mod) << 8;
    if (mod < 0) {
        if (base <= lo || offset >= base - lo) return lo;
        base -= offset;
    } else {
        if (base >= hi || offset >= hi - base) return hi;
        base += offset;
    }
    if (base < lo) return lo;
    if (base > hi) return hi;
    return base;
}

static inline void obv_voice_block(obv_voice_t *v, const obv_part_t *g, const uint8_t *pv, uint32_t card, int32_t tune8)
{
    int32_t lfo, p8, p1, p2, cut, fenv, tmp, velg, trem, vint = g->vint, vt = (vint * VINT_ENV[card % 8u]) >> 7, lfo_to, a;
    uint32_t ph;
    v->pitch16 = obv_glide_tick(v->pitch16, v->target16, g->glide_k);
    /* envelopes */
    env_tick(&v->fe, pv[P_FA], pv[P_FD], g->sus_f, pv[P_FR], v->gate, vt);
    env_tick(&v->ae, pv[P_AA], pv[P_AD], g->sus_a, pv[P_AR], v->gate, vt);
    fenv = v->fe.level >> 9;                    /* Q15 */
    /* the LFO as this voice sees it */
    ph = g->lfo_ph;
    if (pv[P_LFO_PHASE] == LP_SPREAD) ph += card << 29;
    lfo = pv[P_LFO_PHASE] == LP_SPREAD ? obv_lfo_shape(ph, pv[P_LFO_SHAPE], g->lfo_rnd_a, g->lfo_rnd_b) : g->lfo_val;
    if (v->lfo_fade < 32767) {
        uint32_t dl = LFO_DELAY_BLK[pv[P_LFO_DELAY]];
        if (v->lfo_blk >= dl) { v->lfo_fade += 32767 / (int32_t)(dl / 2u + 1u) + 1; if (v->lfo_fade > 32767) v->lfo_fade = 32767; }
        v->lfo_blk++;
    }
    lfo = (lfo * v->lfo_fade) >> 15;
    /* vintage: a slow random walk on the pitch */
    v->drift += (int32_t)(xorshift(&v->render.noise) & 31u) - 16;
    v->drift -= v->drift >> 11;
    /* pitch, Q8 semitones */
    p8 = (v->pitch16 >> 8) + g->bend8 + tune8 + v->det8 + ((VINT_TUNE8[card % 8u] * vint) >> 7) + ((v->drift * vint) >> 10);
    p8 += (lfo * g->wheel_q * 4) >> 15;         /* the wheel's vibrato */
    lfo_to = (lfo * g->lfo_pd * 24) >> 15;
    p1 = p8 + (pv[P_LFO_PITCH_TO] != TO_O2 ? lfo_to : 0);
    p2 = p8 + (pv[P_LFO_PITCH_TO] != TO_O1 ? lfo_to : 0) + ((int32_t)pv[P_O2_SEMI] - 24) * 256 + g->det_fine8
       + ((fenv * g->env_o2 * 97) >> 15);
    v->render.inc1 = obv_pitch_inc(p1);
    v->render.inc2 = obv_pitch_inc(p2);
    tmp = (lfo * g->lfo_pwd);
    v->render.pw1 = obv_pw_mod(g->pw1_base, pv[P_LFO_PWM_TO] != TO_O2 ? tmp : 0);
    v->render.pw2 = obv_pw_mod(g->pw2_base, pv[P_LFO_PWM_TO] != TO_O1 ? tmp : 0);
    /* cutoff, Q8 semitones */
    cut = g->cut_sm + 8 * 256 + ((fenv * g->fenv_amt * 390) >> 15)
        + ((int32_t)v->note - 60) * g->keytrk * 2 + (((int32_t)v->vel - 64) * g->velflt >> 1)
        + ((lfo * g->lfo_fd * 97) >> 15) + ((lfo * g->wheel_q * 48) >> 15) + ((VINT_CUT8[card % 8u] * vint) >> 7)
        + ((v->drift * vint) >> 7);             /* the vintage walk on the cutoff too: ~1/4 semitone at the default */
    if (g->render.ftype == F_LP24) lad_coef(&v->render.lc, cut, g->lad_k);
    else svf_coef(&v->render.c1, cut, g->reso_k);
    /* amp */
    velg = 32767 - ((((127 - (int32_t)v->vel) * (int32_t)pv[P_VEL_AMP]) >> 7) * 258);
    trem = 32767 - (((32767 - lfo) * g->lfo_ad) >> 8);
    v->render.amp0 = v->render.amp1;
    a = ((((v->ae.level >> 9) * velg) >> 15) * trem) >> 15;
    v->render.amp1 = (a * g->gain_q12) >> 12;          /* the sound's VOLUME */
}
