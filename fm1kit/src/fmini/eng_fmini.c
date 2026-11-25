/* SPDX-License-Identifier: GPL-3.0-only
 * FMINI: fmini-1's Minimoog-style engine (https://github.com/ruppert/fmini-1, firmware/src/eng_fmini.c at 649a8bb,
 * GPL-3.0-only, by its authors), ported to fm1kit's engine host (engines.c, D-086). What changed, and only that:
 *   - the patch: one per part, set from the bank (fmini/fmini_bank.h, the owner's 66 picks) when the sound is chosen;
 *     fmini-1's slots, editor, projects and tempo are not here
 *   - the three 64-bit multiplies are 32-bit ones (felucca-bsp docs/CPU.md: this CPU gets them wrong): fm_mulhi is the
 *     high word of a 32 x 32 product from 16-bit halves, exact; the knobs' glide works on d >> 4
 *   - ENV_EXP / ENV_LIN are FM_ENV_EXP / FM_ENV_LIN (kit_tables.h, made as fmini-1's tools/gen_tables.py makes them);
 *     pitch_cur is the host's (1/16 semitone << GQ); the GLO tuning is the kit's
 * The sound itself (oscillators, mixer, ladder, contour) is fmini-1's, line for line.
 *
 * fmini-1's notes on the engine:
 * FMINI (FMini-1): a monosynth in the style of the classic three-oscillator Moog instruments.
 *
 * Signal path (per voice): three oscillators (TRI, SHARK, SAW, SQR, wide and narrow PULSE, band-limited by polyBLEP,
 * ranges LO 32' 16' 8' 4' 2') and a noise source (white or pink) -> the mixer (a level each, the sum saturates softly:
 * the "overload" of the original) -> a 24 dB/oct transistor-ladder low-pass modelled on the Minimoog's (see below)
 * with its own contour (ATTACK DECAY SUSTAIN; the release takes the DECAY time) and keyboard tracking at 1 V/oct. The
 * loudness contour is the host's
 * ADSR (P_ATK .. P_REL). Modulation: the track's LFO (or noise, MOD MIX) to the pitch (MOD OSC) and the filter
 * (MOD FLT), its depth E7 MOD (the matrix's MODW source can drive it); the mod wheel (CC1) is a vibrato on the pitch, +-1 semitone.
 *
 * The sound is the track's patch (fmini_patch, FG_SIZE bytes, stored in projects and user presets in the slot of the
 * FM6 patch: the track plays one engine at a time, project.c, up_fm6.c); the eight EDIT values are the ones to play
 * with (E0 CUT, E1 EMPH, E2 CNTR the contour's amount, E3 F.ATK, E4 F.DEC, E5 F.SUS, E6 MOD, E7 SLOT 1..100 / OWN).
 * The patch holds the rest (ranges, waves, tuning, mixer, switches, keyboard tracking, TUNE). Edited with the FMINI
 * pages (ui_fmini.c). State per voice: s[0..3] the ladder, s[4] the feedback's lag pole (all Q8), s[7] the contour (stage << 24 |
 * level, Q23); the noise is the part's (fmini_nbuf: one source, made once a block). */
#define FG_SIZE 32u

enum {                           /* the patch's bytes */
    FG_R1, FG_R2, FG_R3,         /* range: 0 LO, 1 32', 2 16', 3 8', 4 4', 5 2' */
    FG_W1, FG_W2, FG_W3,         /* wave: 0 TRI, 1 SHARK, 2 SAW, 3 SQR, 4 PLS W, 5 PLS N, 6 SIN */
    FG_F2, FG_F3,                /* OSC 2 / 3 frequency, 64 = 0, +-7 semitones */
    FG_L1, FG_L2, FG_L3, FG_LN,  /* mixer levels 0..127 (a squared curve) */
    FG_SW1,                      /* FS_ bits */
    FG_SW2,                      /* FT_ bits */
    FG_TUNE,                     /* 64 = A 440, +-100 cents */
    FG_MMIX,                     /* modulation source: 0 LFO .. 127 noise */
    FG_KBD,                      /* keyboard tracking of the cutoff, 0..127 = 0..1 */
    FG_GLID,                     /* glide time kept while the GLIDE switch is off */
    FG_DC2L, FG_DC2H,            /* DET 2 in cents: a signed 16-bit (low, high byte), -700 .. +700 */
    FG_DC3L, FG_DC3H,            /* DET 3 in cents, the same */
    FG_BPML, FG_BPMH,            /* TEMPO in BPM (not used here: the song's tempo stays) */
    FG_NUM
};
enum { FS_O1 = 1, FS_O2 = 2, FS_O3 = 4, FS_NZ = 8, FS_PINK = 16, FS_MOSC = 32, FS_MFLT = 64 };   /* FG_SW1 */
enum { FT_GLIDE = 1, FT_SQR = 2, FT_HP = 4 };                                                      /* FG_SW2 */

static int32_t fmini_det(const uint8_t *g, uint32_t lo)
{
    int32_t m = g[lo] | (int32_t)(g[lo + 1] & 7) << 7;
    return g[lo + 1] & 8 ? -m : m;
}

/* the high word of a 32 x 32-bit product, from 16-bit halves (no 64-bit multiply), exact */
static inline uint32_t fm_mulhi(uint32_t a, uint32_t b)
{
    uint32_t al = a & 0xFFFFu, ah = a >> 16, bl = b & 0xFFFFu, bh = b >> 16;
    uint32_t ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    uint32_t mid = (ll >> 16) + (lh & 0xFFFFu) + (hl & 0xFFFFu);
    return hh + (lh >> 16) + (hl >> 16) + (mid >> 16);
}
/* the contour's attack step: (a * step * 824) >> 32 for a < 2^24, step < 2^23 (fmini-1: a 64-bit product); a >> 2 times
 * 824 fits 32 bits, and its two dropped bits are below a part in 2^19 of a */
static inline int32_t fm_charge(int32_t a, int32_t step)
{
    if (a <= 0 || step <= 0)
        return 0;
    return (int32_t)(fm_mulhi((uint32_t)(a >> 2) * 824u, (uint32_t)step) << 2);
}

#define FGP(r1, r2, r3, w1, w2, w3, f2, f3, l1, l2, l3, ln, s1, s2, tn, mm, kb, gl, d2l, d2h, d3l, d3h, bl, bh) \
    {r1, r2, r3, w1, w2, w3, f2, f3, l1, l2, l3, ln, s1, s2, tn, mm, kb, gl, d2l, d2h, d3l, d3h, bl, bh}
typedef struct {                 /* a patch's extras: arpeggiator, LFO {RATE WAVE FADE SYNC}, FX, LEVEL, mod {LFO>PITCH LFO>AMP CONTOUR>PITCH}, octave */
    uint8_t arp[7], lfo[4], fx[12], level;
    int8_t mod[3], oct;
} fmini_x_t;
#include "fmini/fmini_bank.h"           /* the owner's picks: FMB_PATCH, FMB_PRE, FMB_X (tools/gen_fmini.py) */
#include "fmini/eng_fmini_filter.h"     /* fmini-1's ladder laws: FMINI_CUT FMINI_K FMINI_TRIM FMINI_G FMINI_GX FMINI_CNTR */

static uint8_t fmini_patch[NPART][FG_SIZE];       /* each part's patch (set with its sound: kit__eng_select) */
static int32_t fmini_nst[NPART] = {0x2F6E2B1, 0x1A2B3C5, 0x3C4D5E7, 0x0F1E2D3};   /* a part's noise generator */
static int32_t fmini_pk[NPART][3];                /* its pink filter */
static int32_t fmini_nbuf[NPART][CTL];            /* the part's noise for this block, +-32768: one source, all its voices */
static int32_t fmini_sm[NPART][3];                /* the smoothed CUTOFF, EMPH, CNTR knobs (Q16) */
static uint8_t fmini_sm_ok;                       /* bit tr: fmini_sm[tr] follows the knobs (clear = snap to them) */
static int32_t fmini_mn[NPART];                   /* the modulation noise of this block, +-16384 */

/* once a block and part, before its voices: the knobs' glide, the noise (one source for all the part's voices, white
 * or pink: Kellet's filter) and the modulation noise */
static void fmini_block(track_t *t)
{
    uint32_t tr = (uint32_t)(t - trk), i;
    const uint8_t *g;
    if (tr >= NPART)
        return;
    for (i = 0; i < NVOICE && !t->v[i].active; i++)
        ;
    {   /* the knobs' steps glide: a one-pole per block, time constant 200 ms (238/65536 of the distance a block),
         * exact landing; silent = snap. (fmini-1: d * 238 >> 16 in 64 bits; here (d >> 4) * 238 >> 12) */
        static const uint8_t KN[3] = {P_E0, P_E1, P_E2};
        uint32_t j;
        for (j = 0; j < 3; j++) {
            int32_t want = (int32_t)clamp(t->p[KN[j]], 0, 127) << 16, d = want - fmini_sm[tr][j], st;
            if (i == NVOICE || !(fmini_sm_ok & (1u << tr))) {
                fmini_sm[tr][j] = want;
                continue;
            }
            st = d >= 0 ? ((d >> 4) * 238) >> 12 : -(((-d >> 4) * 238) >> 12);
            if (!st)
                st = d > 0 ? 1 : d < 0 ? -1 : 0;
            fmini_sm[tr][j] += st;
        }
        if (i == NVOICE)
            fmini_sm_ok &= (uint8_t)~(1u << tr);
        else
            fmini_sm_ok |= (uint8_t)(1u << tr);
    }
    if (i == NVOICE)
        return;
    g = fmini_patch[tr];
    fmini_mn[tr] = (int32_t)(noise32(&fmini_nst[tr]) >> 17) - 16384;
    if (g[FG_SW1] & FS_NZ) {
        int32_t b0 = fmini_pk[tr][0], b1 = fmini_pk[tr][1], b2 = fmini_pk[tr][2];
        for (i = 0; i < CTL; i++) {
            int32_t w = (int32_t)(noise32(&fmini_nst[tr]) >> 16) - 32768;
            if (g[FG_SW1] & FS_PINK) {
                b0 += ((w * 3245) >> 15) - ((b0 * 77) >> 15);
                b1 += ((w * 9716) >> 15) - ((b1 * 1212) >> 15);
                b2 += ((w * 34495) >> 15) - ((b2 * 14090) >> 15);
                w = (b0 + b1 + b2 + ((w * 6056) >> 15)) >> 1;
            }
            fmini_nbuf[tr][i] = w;
        }
        fmini_pk[tr][0] = b0;
        fmini_pk[tr][1] = b1;
        fmini_pk[tr][2] = b2;
    }
}

/* ------------------------------------------------------------------ voice --- */
static inline int32_t fmini_shark(uint32_t ph)            /* a triangle that rises for 3/4 of the cycle */
{
    uint32_t x = ph >> 16;
    return x < 49152u ? (int32_t)((x * 43691u) >> 15) - 32768 : 32767 - (int32_t)((x - 49152u) * 4u);
}

/* the ladder's input is the mixer's sum halved (6 dB below full scale) and its output raised by the same: the
 * CA-72's ladder runs well under its pairs' knee, so a sawtooth or triangle comes out as drawn (its 3rd harmonic
 * at -20 dB in the triangle, measured on the CA-72 oscillator through the open ladder), while the singing sits at
 * the CA-72's level against the waveform. Harmonics relative to the fundamental; a shift, not a gain change. */
#define FMINI_LADDER_IN 2         /* 12 dB below full scale: the ladder stays under its knee at every oscillator level */
/* the differential pair: tanh with its bias, the bias's own offset taken away (the CA-72's pairs sit on a bias; its 2nd
 * harmonic at full level is -37 dB, the triangle's odd harmonics unchanged). pair_off = softclip(FMINI_PAIR_BIAS), once a block */
#define FMINI_PAIR_BIAS 1500
static inline int32_t pairb(int32_t x, int32_t pair_off)
{
    return softclip(x + FMINI_PAIR_BIAS) - pair_off;
}

/* one oscillator of the mix: mix[i] += its sample at level g (Q15); a loop per wave. keep = -1 adds to what the mix
 * holds; keep = 0 writes it (the first oscillator: no zeroing pass, 0 + x = x exactly).
 * The polyBLEP residual is zero except within one step of a wrap: its bounds (lo, hi) and the step d are
 * worked out once per call, so each sample tests two constants (the same branches as dsp.c's blep, same bits). */
static inline int32_t fm_blep(uint32_t ph, uint32_t d, uint32_t lo, uint32_t hi)
{
    int32_t x;
    if (ph < lo) {
        x = (int32_t)(ph / d);
        return x + x - ((x * x) >> 15) - 32768;
    }
    if (ph > hi) {
        x = -(int32_t)((0xFFFFFFFFu - ph) / d);
        return ((x * x) >> 15) + x + x + 32768;
    }
    return 0;
}

static void fmini_wave(uint32_t w, uint32_t *php, uint32_t inc, int32_t g, int32_t *mix, uint32_t n, int32_t keep)
{
    uint32_t ph = *php, i, d = inc >> 15;
    uint32_t lo = d ? inc : 0u, hi = d ? 0xFFFFFFFFu - inc : 0xFFFFFFFFu;   /* d == 0: no residual at all (dsp.c blep) */
    switch (w) {
    case 0:
        for (i = 0; i < n; i++) {
            mix[i] = (mix[i] & keep) + mulq15(osc_tri(ph), g);
            ph += inc;
        }
        break;
    case 1:
        for (i = 0; i < n; i++) {
            mix[i] = (mix[i] & keep) + mulq15(fmini_shark(ph), g);
            ph += inc;
        }
        break;
    case 2:
        for (i = 0; i < n; i++) {
            mix[i] = (mix[i] & keep) + mulq15(((int32_t)(ph >> 16) - 32768) - fm_blep(ph, d, lo, hi), g);
            ph += inc;
        }
        break;
    case 3:                                             /* PULSE 50 %: the saw minus the saw half a turn on */
        for (i = 0; i < n; i++) {
            uint32_t q = ph + 0x80000000u;
            mix[i] = (mix[i] & keep) + mulq15((((int32_t)(ph >> 16) - 32768) - fm_blep(ph, d, lo, hi)
                            - (((int32_t)(q >> 16) - 32768) - fm_blep(q, d, lo, hi))) >> 1, g);
            ph += inc;
        }
        break;
    case 4:
        for (i = 0; i < n; i++) {
            uint32_t q = ph + 0x55555555u;
            mix[i] = (mix[i] & keep) + mulq15((((int32_t)(ph >> 16) - 32768) - fm_blep(ph, d, lo, hi)
                            - (((int32_t)(q >> 16) - 32768) - fm_blep(q, d, lo, hi))) >> 1, g);
            ph += inc;
        }
        break;
    case 6:                                             /* SIN: the sine table (dsp.c), no harmonics to fold back */
        for (i = 0; i < n; i++) {
            mix[i] = (mix[i] & keep) + mulq15(osc_sine(ph), g);
            ph += inc;
        }
        break;
    default:
        for (i = 0; i < n; i++) {
            uint32_t q = ph + 0x1C71C71Cu;
            mix[i] = (mix[i] & keep) + mulq15((((int32_t)(ph >> 16) - 32768) - fm_blep(ph, d, lo, hi)
                            - (((int32_t)(q >> 16) - 32768) - fm_blep(q, d, lo, hi))) >> 1, g);
            ph += inc;
        }
        break;
    }
    *php = ph;
}

/* range of an oscillator in 1/16 semitones (LO: very low) */
static int32_t fmini_range16(uint32_t r)
{
    static const int16_t R[6] = {-48 * 16, -24 * 16, -12 * 16, 0, 12 * 16, 24 * 16};
    return R[r % 6u];
}

static void fmini_note_on(track_t *t, voice_t *v)
{
    if (!v->env && !v->env_out) {                          /* from silence: the ladder and the contour from 0 */
        v->s[0] = v->s[1] = v->s[2] = v->s[3] = 0;
        v->s[4] = 0;
        v->s[7] = 0;
        if (t->p[P_E1] >= 96)                                         /* it will sing: a nudge to start from (the hardware has its own noise) */
            v->s[0] = (noise32(&fmini_nst[(uint32_t)(t - trk) % NPART]) & 0x80000000u) ? 700 * 256 : -700 * 256;
        v->s[5] = (int32_t)(noise32(&fmini_nst[(uint32_t)(t - trk) % NPART]) >> 20) - 2048;   /* the VCOs start a little apart: drift, cents * 256 */
        v->s[6] = (int32_t)(noise32(&fmini_nst[(uint32_t)(t - trk) % NPART]) >> 20) - 2048;
    }
    v->s[7] = (int32_t)(1u << 24) | (v->s[7] & 0xFFFFFF);  /* the contour attacks from where it is */
}

/* the contour, once a block: stage 1 attack, 2 decay to the sustain, 3 release; level Q23 */
static int32_t fmini_contour(const int16_t *p, voice_t *v)
{
    uint32_t st = (uint32_t)v->s[7] >> 24;
    int32_t e = v->s[7] & 0xFFFFFF, sus = (int32_t)p[P_E5] << 16, k = (int32_t)FM_ENV_EXP[p[P_E4] & 127];
    if (!v->gate && st && st != 3u)
        st = 3;
    switch (st) {
    case 1:
        {   /* the capacitor charges toward a level above full: a concave rise that ends in a corner, as in the Minimoog */
            int32_t step = (int32_t)(FM_ENV_LIN[p[P_E3] & 127] >> 1), d = fm_charge((5 << 21) - e, step);   /* (no 64-bit multiply: fm_charge) */
            e += d < 1 ? 1 : d;
        }
        if (e >= (1 << 23)) {
            e = 1 << 23;
            st = 2;
        }
        break;
    case 2:
        e += mulq16(sus - e, (uint32_t)k);                  /* the DECAY time is the drop from the peak to the SUSTAIN level, whatever it is */
        break;
    case 3:
        e -= mulq16(e, p[P_FREL] == FM_ENV_FOLLOW_DECAY ? (uint32_t)k : FM_ENV_EXP[p[P_FREL]]);
        break;
    default:
        break;
    }
    v->s[7] = (int32_t)(st << 24) | (e & 0xFFFFFF);
    return e >> 8;                                          /* Q15 */
}

/* a * k / 65536 rounded to the nearest (mulq16 floors: four stages of it leave a bias that a quiet, low ladder cannot sing against) */
static inline int32_t fmini_mulr(int32_t a, uint32_t k)
{
    return (int32_t)(((a >> 16) * (int32_t)k) + (int32_t)(((uint32_t)(a & 0xFFFF) * k + 32768u) >> 16));
}

/* The filter, after the Minimoog's (tools/fmini_filter.py has the laws and the reasoning, the CA-72 project's calibration the
 * measurements): four equal one-pole stages, each a differential pair (tanh in front), the last stage's output fed back to
 * the first through a lag pole (the emphasis network), solved without a delay in the loop (zero-delay feedback: each stage
 * is a trapezoidal integrator, G = g / (1 + g), and the feedback is found from the stages' states before the sample is
 * processed). The lag pole puts the loop's 180-degree point at 0.686 of the stage pole: the filter sings at 1.578 times
 * its -3 dB corner, and it sings when the feedback k reaches 2.92. The knobs and the pitch work in 1/16 semitones of the
 * singing pitch (MIDI x 16): CUTOFF is the dial -5 .. +5 (FMINI_CUT), the keyboard adds 1 V/oct from F (MIDI 41) at KBD 127,
 * the contour up to 6.3 octaves through the CA-72's AMOUNT taper (FMINI_CNTR); EMPHASIS is the dial 0 .. 10 (FMINI_K) with the hardware's passband loss (FMINI_TRIM). With
 * the mixer silent and EMPHASIS past 8 the ladder sings a sine at the singing pitch, 1 V/oct over the keyboard.
 * Q formats: signals +-32768 (tanh(1)); the states Q8 (a quiet ladder at a low cutoff moves less than a LSB a sample); G Q16; the prediction's coefficients c Q14; k Q12. */
static void fmini_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    const uint8_t *g = fmini_patch[(uint32_t)(t - trk) % NPART];
    int32_t mix[CTL];
    int32_t env15, mod, lfo, vib, lc, p16, fine, k12, G16, gx16, c0, c1, c2, c3, inv14, kinv12, trim12, pair_off;
    int32_t l1 = 0, l2 = 0, l3 = 0, ln = 0, dr1, dr2, dr3;
    uint32_t hp = (uint32_t)(g[FG_SW2] & FT_HP) ? 1u : 0u;      /* the output: LP (the last stage) or HP ((1 - LP)^4 of the same stages) */
    uint32_t tr = (uint32_t)(t - trk) % NPART, i, sw = g[FG_SW1];
    uint32_t inc1, inc2, inc3;
    int32_t s0 = v->s[0], s1 = v->s[1], s2 = v->s[2], s3 = v->s[3], sx = v->s[4];
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], ph2 = v->ph[2];
    if (n > CTL)
        n = CTL;
    env15 = fmini_contour(p, v);

    /* modulation: the LFO (a square when FT_SQR) or noise held for the block, MOD deep */
    lfo = mulq15(t->lfo_val, t->lfo_fade);
    if (g[FG_SW2] & FT_SQR)
        lfo = lfo >= 0 ? 32767 : -32767;
    mod = mulq15(lfo, 32767 - (int32_t)g[FG_MMIX] * 258) + mulq15(fmini_mn[tr] << 1, (int32_t)g[FG_MMIX] * 258);
    mod = mulq15(mod, (int32_t)p[P_E6] * 258);                          /* the patch's MOD: Q15 of its depth */
    /* the mod wheel (CC1, t->mw) is the vibrato: the LFO on the pitch, +-1 semitone at full wheel, whatever MOD OSC is */
    vib = (mulq15(lfo, (int32_t)t->mw * 258) * 16) >> 15;

    /* oscillator pitches */
    p16 = m->pitch16 + vib + (sw & FS_MOSC ? (mod * 16) >> 14 : 0);    /* full depth: +-2 semitones */
    fine = m->fine + ((int32_t)g[FG_TUNE] - 64) * 237 / 64;   /* (the kit's TUNE is in the host's pitch) */
    {   /* the VCOs wander a few cents (temperature, the integrators): a slow random walk per oscillator, about +-2 cents */
        int32_t d1 = v->s[5], d2 = v->s[6];
        d1 += (((int32_t)(noise32(&fmini_nst[tr]) >> 20) - 2048) >> 6) - (d1 >> 11);
        d2 += (((int32_t)(noise32(&fmini_nst[tr]) >> 20) - 2048) >> 6) - (d2 >> 11);
        v->s[5] = d1;
        v->s[6] = d2;
        dr1 = d1 >> 8;
        dr2 = d2 >> 8;
        dr3 = (d1 - d2) >> 9;
    }
    inc1 = cents_inc(clamp(p16 + fmini_range16(g[FG_R1]), 0, 2047), dr1, fine);
    inc2 = cents_inc(clamp(p16 + fmini_range16(g[FG_R2]), 0, 2047), fmini_det(g, FG_DC2L) + dr2, fine);
    inc3 = cents_inc(clamp(p16 + fmini_range16(g[FG_R3]), 0, 2047), fmini_det(g, FG_DC3L) + dr3, fine);

    /* mixer levels: a squared curve */
    if (sw & FS_O1) l1 = (int32_t)(g[FG_L1] * g[FG_L1] * 2) + 0;
    if (sw & FS_O2) l2 = (int32_t)(g[FG_L2] * g[FG_L2] * 2);
    if (sw & FS_O3) l3 = (int32_t)(g[FG_L3] * g[FG_L3] * 2);
    if (sw & FS_NZ) ln = (int32_t)(g[FG_LN] * g[FG_LN] * 2);
    {   /* the first oscillator that runs writes the mix (keep 0); the others add (keep -1) */
        int32_t keep = 0;
        if (l1) {
            fmini_wave(g[FG_W1], &ph0, inc1, l1, mix, n, keep);
            keep = -1;
        } else
            ph0 += inc1 * n;
        if (l2) {
            fmini_wave(g[FG_W2], &ph1, inc2, l2, mix, n, keep);
            keep = -1;
        } else
            ph1 += inc2 * n;
        if (l3) {
            fmini_wave(g[FG_W3], &ph2, inc3, l3, mix, n, keep);
            keep = -1;
        } else
            ph2 += inc3 * n;
        if (!keep)                                      /* no oscillator: the mix starts at zero */
            for (i = 0; i < n; i++)
                mix[i] = 0;
    }
    if (ln)
        for (i = 0; i < n; i++)
            mix[i] += mulq15(fmini_nbuf[tr][i], ln);

    /* the singing pitch (MIDI x 16): the CUTOFF knob's dial, the contour, the keyboard, the modulation, the host's (LFO / matrix / ENV routings) */
    {
        int32_t kn = fmini_sm[tr][0], i0 = kn >> 16, fr = (kn >> 8) & 255;
        lc = FMINI_CUT[i0];
        if (i0 < 127)
            lc += ((FMINI_CUT[i0 + 1] - lc) * fr) >> 8;
    }
    lc += (m->cutoff * 55) >> 10;                                                                           /* the host's steps (1/14 octave) in 1/16 semitones */
    {                                                                                                           /* the contour: the CA-72's taper, up to 6.3 octaves */
        uint32_t ci = (uint32_t)fmini_sm[tr][2] >> 16, cf = ((uint32_t)fmini_sm[tr][2] & 0xFFFFu) >> 1;
        int32_t ctap;
        if (ci > 126u) { ci = 126u; cf = 32768u; }
        ctap = (int32_t)FMINI_CNTR[ci] + (int32_t)((((int32_t)FMINI_CNTR[ci + 1u] - (int32_t)FMINI_CNTR[ci]) * (int32_t)cf) >> 15);
        lc += (int32_t)fm_mulhi((uint32_t)(env15 < 0 ? 0 : env15) * (uint32_t)ctap, FMINI_CNTR_FULL16 << 2);   /* (env15 ctap FULL16) >> 30, no 64-bit multiply */
    }
    lc += (((v->pitch_cur >> GQ) - FMINI_NOTE0 * 16) * (int32_t)g[FG_KBD] * 516) >> 16;                              /* the keyboard: 1 V/oct at 127, 0 V on F */
    if (sw & FS_MFLT)
        lc += (mod * 547) >> 15;                                                                            /* +-2.85 octaves at full depth */
    lc = clamp(lc, FMINI_GM0 * 16, FMINI_FOSC_MAX16);
    {   /* the stage pole G and the lag pole's from the tables (one entry a semitone, linear between), the feedback solved ahead */
        uint32_t ix = (uint32_t)((lc >> 4) - FMINI_GM0), fr = (uint32_t)lc & 15u, gq, gxq, g2, g3, g4, om;
        gq = FMINI_G[ix] + (uint32_t)(((int32_t)(FMINI_G[ix + 1] - FMINI_G[ix]) * (int32_t)fr) >> 4);
        gxq = FMINI_GX[ix] + (uint32_t)(((int32_t)(FMINI_GX[ix + 1] - FMINI_GX[ix]) * (int32_t)fr) >> 4);
        G16 = (int32_t)((gq + 128u) >> 8);
        gx16 = (int32_t)((gxq + 128u) >> 8);
        om = 65536u - (uint32_t)G16;
        g2 = ((uint32_t)G16 * (uint32_t)G16) >> 16;
        g3 = (g2 * (uint32_t)G16) >> 16;
        g4 = (g3 * (uint32_t)G16) >> 16;
        c3 = (int32_t)(om >> 2);                                   /* y4 = G^4 u + c0 s0 + c1 s1 + c2 s2 + c3 s3 */
        c2 = (int32_t)(((uint32_t)G16 * om) >> 18);
        c1 = (int32_t)((g2 * om) >> 18);
        c0 = (int32_t)((g3 * om) >> 18);
        {   /* EMPHASIS: the feedback gain k and the passband's loss, from the knob (Q16) */
            int32_t kn = fmini_sm[tr][1], i0 = kn >> 16, fk = (kn >> 8) & 255, den;
            k12 = FMINI_K[i0];
            trim12 = FMINI_TRIM[i0];
            if (i0 < 127) {
                k12 += ((FMINI_K[i0 + 1] - k12) * fk) >> 8;
                trim12 += ((FMINI_TRIM[i0 + 1] - trim12) * fk) >> 8;
            }
            den = 65536 + ((k12 * (int32_t)((((uint32_t)gxq >> 8) * g4) >> 16)) >> 12);        /* 1 + k Gx G^4: the loop around the whole sample */
            inv14 = (int32_t)((1u << 30) / (uint32_t)den);
            kinv12 = (k12 * inv14) >> 14;
        }
    }

    const int32_t amp0 = m->amp0;                      /* the voice's ramp (voice_amp), as a running sum below */
    uint32_t xa = 0, da = (uint32_t)(m->amp1 - m->amp0);
    pair_off = softclip(FMINI_PAIR_BIAS);
    for (i = 0; i < n; i++) {
        int32_t x = (mix[i] > (32767 << FMINI_LADDER_IN) ? (32767 << FMINI_LADDER_IN) : mix[i] < -(32767 << FMINI_LADDER_IN) ? -(32767 << FMINI_LADDER_IN) : mix[i]) >> FMINI_LADDER_IN, S, fbc, u, xin, v1, v2, v3, v4, y1, y2, y3, y4, w, o;   /* the mixer is linear (the CA-72's is) up to 12 dB over full scale, then clips hard: the shift is its headroom */
        S = (c0 * ((s0 + 128) >> 8) + c1 * ((s1 + 128) >> 8) + c2 * ((s2 + 128) >> 8) + c3 * ((s3 + 128) >> 8)) >> 14;   /* what the stages already hold */
        fbc = sx + fmini_mulr((S << 8) - sx, (uint32_t)gx16);              /* through the lag pole (Q8) */
        u = ((x * inv14) >> 14) - ((kinv12 * ((fbc + 128) >> 8)) >> 12);  /* the input less the feedback, the loop solved */
        xin = (pairb(u, pair_off)) << 8;                                              /* the first stage's input (Q8) */
        v1 = fmini_mulr(xin - s0, (uint32_t)G16);                          /* each stage is a differential pair: tanh in front of it */
        y1 = v1 + s0;
        s0 = y1 + v1;
        v2 = fmini_mulr((pairb((y1 + 128) >> 8, pair_off) << 8) - s1, (uint32_t)G16);
        y2 = v2 + s1;
        s1 = y2 + v2;
        v3 = fmini_mulr((pairb((y2 + 128) >> 8, pair_off) << 8) - s2, (uint32_t)G16);
        y3 = v3 + s2;
        s2 = y3 + v3;
        v4 = fmini_mulr((pairb((y3 + 128) >> 8, pair_off) << 8) - s3, (uint32_t)G16);
        y4 = v4 + s3;
        s3 = y4 + v4;
        w = fmini_mulr(y4 - sx, (uint32_t)gx16);                           /* the lag pole follows the output */
        sx += w + w;
        /* HP: the stages are one-pole lows H; (1 - H)^4 = 1 - 4H + 6H^2 - 4H^3 + H^4 on the stages' outputs (linear, the states as they are) */
        o = hp ? xin - 4 * y1 + 6 * y2 - 4 * y3 + y4 : y4;
        {   /* voice_amp with amp_at's ramp by its running sum (x = (amp1 - amp0) * i, one add a sample, mod 2^32 as the product) */
            int32_t xr = (int32_t)xa, amp = amp0 + ((xr + ((xr >> 31) & (CTL - 1))) >> CTL_LOG2);
            xa += da;
            out[i] += mulq15(mulq15((((o + 128) >> 8) * trim12) >> 12, amp), VOICE_FS) << (1 + FMINI_LADDER_IN);
        }
    }
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->ph[2] = ph2;
    v->s[0] = s0;
    v->s[1] = s1;
    v->s[2] = s2;
    v->s[3] = s3;
    v->s[4] = sx;
}

/* --------------------------------------------------------------- the engine --- */

static engine_t ENG_FMINI = {
    .name = "FMINI",
    .presets = FMB_PRE,
    .npresets = FMB_N,
    .note_on = fmini_note_on,
    .render = fmini_render,
    .poly = 4,                                  /* 4 voices a part: Felucca's FMINI_POLY beside other engines (fmini-1 alone: 12); about 9 % of
                                                 * the FM-1's CPU a voice (factory/cpu.py) */
    .keep = 0x13,
    .block = fmini_block,
};
