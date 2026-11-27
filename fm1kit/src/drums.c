/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit's drum kits (API 23, D-053), included by synth.c. Four kits of 27 sounds, one a key from F3, named as their
 * sound files will be (kick1, snare2, conga1); the recipes follow the drum lab's (scratchpad sim/drum-lab.html,
 * approved by the owner as ST-30). A drum voice has up to three layers, each with its own level and decay:
 *   tone   a sine, triangle or square falling from f0 to f1 (a kick, a tom), with an optional second oscillator at a
 *          ratio (a cowbell's two squares, a bell's partials);
 *   noise  white noise through a one-pole high-pass and low-pass (a band between them), retriggered in bursts for a
 *          clap or a shaker;
 *   metal  six square waves at the 808's ratios, high-passed (an 808 hat, a cymbal's shimmer).
 * A track's pads can each play any kit's sound, tuned in semitones and with a longer or shorter decay (My kit).
 * Integer only, no 64-bit multiply (rule 8); render is called per voice from kit__synth_render. */

enum { SL_KICK = 0, SL_RIM, SL_SNARE, SL_CLAP, SL_GHOST, SL_TOML, SL_HATC, SL_TOMM, SL_SHAKER, SL_TOMH, SL_HATO, SL_PERC,
       SL_CRASH, SL_VINYL, SL_N };
enum { W_NONE = 0, W_SINE, W_TRI, W_SQR };

typedef struct {
    uint8_t wave;               /* the tone layer: W_* */
    uint16_t f0, f1;            /* Hz, start and end */
    uint8_t fall;               /* ms to fall */
    uint16_t tlen;              /* ms to silence */
    uint8_t tlvl;               /* 0..255 */
    uint8_t ratio2;             /* a second oscillator at ratio2 / 64 of the first (0: none) */
    uint16_t hp, lp;            /* the noise layer's filters, Hz (0: none) */
    uint16_t nlen;
    uint8_t nlvl;
    uint8_t bursts;             /* the noise retriggered this many more times, 11 ms apart */
    uint8_t mbase;              /* the metal layer: base Hz (0: none) */
    uint16_t mlen;
    uint8_t mlvl;
} recipe_t;

/* [kit][slot]: Dusty, 808, Live, Latin */
static const recipe_t RECIPES[4][SL_N] = {
    {   /* Dusty: soft and dark, a dusty sampler */
        { W_SINE, 92, 50, 70, 320, 240, 0, 0, 1800, 20, 30, 0, 0, 0, 0 },          /* kick */
        { W_TRI, 1700, 1600, 10, 25, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             /* rim */
        { W_TRI, 190, 160, 40, 80, 90, 0, 1100, 4200, 170, 140, 0, 0, 0, 0 },      /* snare */
        { W_NONE, 0, 0, 0, 0, 0, 0, 800, 1600, 140, 120, 2, 0, 0, 0 },             /* clap */
        { W_TRI, 190, 160, 40, 50, 30, 0, 1100, 2500, 100, 50, 0, 0, 0, 0 },       /* ghost */
        { W_SINE, 133, 95, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },              /* tom low */
        { W_NONE, 0, 0, 0, 0, 0, 0, 5000, 9000, 40, 90, 0, 0, 0, 0 },              /* hat */
        { W_SINE, 196, 140, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             /* tom mid */
        { W_NONE, 0, 0, 0, 0, 0, 0, 4500, 8000, 60, 72, 0, 0, 0, 0 },              /* shaker */
        { W_SINE, 266, 190, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             /* tom high */
        { W_NONE, 0, 0, 0, 0, 0, 0, 5500, 0, 420, 66, 0, 0, 0, 0 },                /* open hat */
        { W_NONE, 0, 0, 0, 0, 0, 0, 7000, 0, 120, 64, 0, 70, 100, 15 },            /* tambourine */
        { W_NONE, 0, 0, 0, 0, 0, 0, 4800, 0, 1600, 52, 0, 90, 1200, 13 },          /* crash */
        { W_NONE, 0, 0, 0, 0, 0, 0, 2500, 6500, 8, 70, 5, 0, 0, 0 },               /* vinyl */
    },
    {   /* 808: the drum machine */
        { W_SINE, 115, 46, 90, 1100, 240, 0, 0, 3000, 15, 26, 0, 0, 0, 0 },
        { W_TRI, 1700, 1600, 10, 25, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { W_SINE, 238, 230, 50, 100, 100, 128, 1800, 0, 160, 100, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 800, 1600, 220, 120, 2, 0, 0, 0 },
        { W_SINE, 238, 230, 50, 60, 35, 128, 1800, 6000, 100, 40, 0, 0, 0, 0 },
        { W_SINE, 133, 95, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 40, 45, 190 },
        { W_SINE, 196, 140, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 4500, 8000, 60, 72, 0, 0, 0, 0 },              /* maracas */
        { W_SINE, 266, 190, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 40, 450, 160 },
        { W_SQR, 540, 540, 1, 300, 50, 95, 0, 0, 0, 0, 0, 0, 0, 0 },               /* cowbell: 540 and 800 Hz */
        { W_NONE, 0, 0, 0, 0, 0, 0, 4800, 0, 1600, 52, 0, 90, 1200, 13 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 2500, 6500, 8, 70, 5, 0, 0, 0 },
    },
    {   /* Live: a drummer's kit */
        { W_SINE, 105, 54, 50, 280, 230, 0, 2400, 4200, 12, 64, 0, 0, 0, 0 },
        { W_TRI, 1700, 1600, 10, 25, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             /* cross stick */
        { W_TRI, 210, 180, 30, 70, 108, 0, 900, 9000, 240, 140, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 800, 1600, 140, 120, 2, 0, 0, 0 },
        { W_TRI, 210, 180, 30, 45, 38, 0, 900, 5400, 145, 50, 0, 0, 0, 0 },
        { W_SINE, 133, 95, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },              /* floor tom */
        { W_NONE, 0, 0, 0, 0, 0, 0, 7500, 0, 45, 77, 0, 0, 0, 0 },
        { W_SINE, 196, 140, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             /* rack tom */
        { W_NONE, 0, 0, 0, 0, 0, 0, 4500, 8000, 60, 72, 0, 0, 0, 0 },
        { W_SINE, 266, 190, 40, 300, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 6800, 0, 420, 66, 0, 0, 0, 0 },
        { W_SINE, 2400, 2400, 1, 600, 52, 96, 0, 0, 0, 0, 0, 0, 0, 0 },            /* ride bell: 2400 and 3600 Hz */
        { W_NONE, 0, 0, 0, 0, 0, 0, 4800, 0, 1600, 52, 0, 90, 1200, 13 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 2500, 6500, 8, 70, 5, 0, 0, 0 },
    },
    {   /* Latin: congas, bongos, a clave, a timbal */
        { W_SINE, 150, 52, 60, 380, 230, 0, 1000, 2500, 12, 38, 0, 0, 0, 0 },
        { W_SINE, 2500, 2500, 1, 30, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             /* clave */
        { W_TRI, 260, 240, 20, 60, 90, 0, 2100, 4300, 120, 128, 0, 0, 0, 0 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 800, 1600, 140, 120, 2, 0, 0, 0 },
        { W_TRI, 260, 240, 20, 40, 32, 0, 2100, 3000, 80, 45, 0, 0, 0, 0 },
        { W_SINE, 279, 200, 40, 180, 180, 0, 1800, 3500, 15, 64, 0, 0, 0, 0 },     /* conga low */
        { W_NONE, 0, 0, 0, 0, 0, 0, 7500, 0, 45, 77, 0, 0, 0, 0 },
        { W_SINE, 412, 294, 40, 180, 180, 0, 1800, 3500, 15, 64, 0, 0, 0, 0 },     /* conga */
        { W_NONE, 0, 0, 0, 0, 0, 0, 3000, 5800, 12, 64, 5, 0, 0, 0 },              /* guiro */
        { W_SINE, 559, 399, 40, 180, 180, 0, 1800, 3500, 15, 64, 0, 0, 0, 0 },     /* bongo */
        { W_NONE, 0, 0, 0, 0, 0, 0, 6800, 0, 420, 66, 0, 0, 0, 0 },
        { W_SINE, 420, 400, 30, 150, 128, 0, 3000, 0, 30, 51, 0, 0, 0, 0 },        /* timbal */
        { W_NONE, 0, 0, 0, 0, 0, 0, 4800, 0, 1600, 52, 0, 90, 1200, 13 },
        { W_NONE, 0, 0, 0, 0, 0, 0, 2500, 6500, 8, 70, 5, 0, 0, 0 },
    },
};
static const char *const KIT_NAMES[KIT_DRUM_KITS + 1] = { "Dusty", "808", "Live", "Latin", "USER" };
static const kit_sample_t *user_pad[KIT_DRUM_PADS];   /* the user kit's sounds, by pad (D-056) */
static int user_n;
void kit_drum_user_kit(const kit_sample_t *s, int n)
{
    int i;
    for (i = 0; i < KIT_DRUM_PADS; i++) user_pad[i] = 0;
    user_n = 0;
    for (i = 0; i < n; i++)
        if (s[i].pad < KIT_DRUM_PADS && s[i].pcm && s[i].len) { user_pad[s[i].pad] = &s[i]; user_n++; }
}
static int nkits(void) { return KIT_DRUM_KITS + (user_n > 0); }

/* a pad: its name, the slot and the kit whose recipe it plays (-1: its own kit's), tune (x/64) and decay (x/64) */
typedef struct { const char *name; uint8_t slot; int8_t rk; uint8_t tf, df; } pad_t;
static const pad_t PADS_COMMON[22] = {
    { "kick1", SL_KICK, -1, 64, 64 }, { "kick2", SL_KICK, -1, 56, 90 }, { "kick3", SL_KICK, -1, 76, 45 }, { "kick4", SL_KICK, 1, 58, 102 },
    { "snare1", SL_SNARE, -1, 64, 64 }, { "snare2", SL_SNARE, -1, 72, 51 }, { "snare3", SL_SNARE, 2, 61, 77 },
    { "rim1", SL_RIM, -1, 64, 64 }, { "rim2", SL_RIM, -1, 80, 51 }, { "clap1", SL_CLAP, -1, 64, 64 }, { "clap2", SL_CLAP, 1, 70, 83 },
    { "ghost1", SL_GHOST, -1, 64, 64 }, { "hat1", SL_HATC, -1, 64, 64 }, { "hat2", SL_HATC, -1, 74, 45 }, { "pedal1", SL_HATC, -1, 54, 32 },
    { "open1", SL_HATO, -1, 64, 64 }, { "open2", SL_HATO, -1, 70, 38 }, { "shaker1", SL_SHAKER, -1, 64, 64 }, { "shaker2", SL_SHAKER, -1, 51, 90 },
    { "tom1", SL_TOML, -1, 64, 64 }, { "tom2", SL_TOMM, -1, 64, 64 }, { "tom3", SL_TOMH, -1, 64, 64 },
};
static const pad_t PADS_EXTRA[4][4] = {
    { { "tamb1", SL_PERC, 0, 64, 64 }, { "tamb2", SL_PERC, 0, 77, 38 }, { "perc1", SL_PERC, 2, 51, 45 }, { "vinyl1", SL_VINYL, 0, 64, 64 } },
    { { "cowbell1", SL_PERC, 1, 64, 64 }, { "clave1", SL_RIM, 3, 64, 64 }, { "maracas1", SL_SHAKER, 3, 64, 64 }, { "conga1", SL_TOMM, 3, 64, 64 } },
    { { "ride1", SL_PERC, 2, 64, 64 }, { "bell1", SL_PERC, 2, 90, 38 }, { "cowbell1", SL_PERC, 1, 64, 64 }, { "tamb1", SL_PERC, 0, 64, 64 } },
    { { "conga1", SL_TOML, 3, 64, 64 }, { "conga2", SL_TOMM, 3, 64, 64 }, { "bongo1", SL_TOMH, 3, 64, 64 }, { "timbal1", SL_PERC, 3, 64, 64 } },
};
static const pad_t PAD_CRASH = { "crash1", SL_CRASH, -1, 64, 64 };

/* The drum map v2 (D-121, D-122): the key each sound is on. The 12 sounds AUTO and IDEAS play on F3..E4, General MIDI's
 * way (F3 kick, F#3 hat, G3 snare, G#3 open hat, A3 clap, A#3 shaker, B3 rim, C4 tom 1, C#4 ghost, D4 tom 2, D#4 perc,
 * E4 tom 3); F4..G5 the others. A "sound" below is the index in PADS_COMMON / PADS_EXTRA / PAD_CRASH order. */
static const uint8_t KEY_SOUND[KIT_DRUM_PADS] = { 0, 12, 4, 15, 9, 17, 7, 19, 11, 20, 22, 21,
                                                  1, 13, 5, 16, 10, 14, 8, 2, 18, 6, 23, 3, 24, 25, 26 };
int kit_drum_key_of(int sound)
{
    int k;
    for (k = 0; k < KIT_DRUM_PADS; k++) if (KEY_SOUND[k] == sound) return k;
    return 0;
}
static pad_t pad_of(int kit, int key)
{
    pad_t p;
    int pad;
    if (kit == KIT_DRUM_USER) {                 /* the user kit: its sound's name (by key); a key without one is Dusty's */
        p = pad_of(0, key);
        if (user_pad[key]) p.name = user_pad[key]->name;
        return p;
    }
    pad = KEY_SOUND[key];
    if (pad < 22) p = PADS_COMMON[pad];
    else if (pad < 26) p = PADS_EXTRA[kit][pad - 22];
    else p = PAD_CRASH;
    if (kit == 2 && p.slot == SL_RIM && pad < 22) p.name = pad == 7 ? "stick1" : "stick2";
    if (kit == 3 && pad < 22) {
        if (pad == 7) p.name = "clave2";
        if (pad == 17) p.name = "guiro1";
        if (pad == 18) p.name = "guiro2";
    }
    if (p.rk < 0) p.rk = (int8_t)kit;
    return p;
}

const char *kit_drum_kit_name(int kit) { return kit >= 0 && kit < nkits() ? KIT_NAMES[kit] : "?"; }
const char *kit_drum_name(int kit, int pad)
{
    if (kit < 0 || kit >= nkits() || pad < 0 || pad >= KIT_DRUM_PADS) return "?";
    return pad_of(kit, pad).name;
}
int kit_drum_group(int kit, int pad)            /* 0 kick, 1 snare and claps, 2 hats and shakers, 3 toms and the rest */
{
    int s;
    if (kit < 0 || kit >= nkits() || pad < 0 || pad >= KIT_DRUM_PADS) return 3;
    s = pad_of(kit, pad).slot;
    return s == SL_KICK ? 0 : s == SL_SNARE || s == SL_CLAP || s == SL_GHOST || s == SL_RIM ? 1
         : s == SL_HATC || s == SL_HATO || s == SL_SHAKER || s == SL_CRASH ? 2 : 3;
}

/* 2^(s/12) for s = -12..12, Q16 */
static const uint32_t SEMI[25] = { 32768, 34716, 36781, 38968, 41285, 43740, 46341, 49097, 52016, 55109, 58386, 61858,
                                   65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032, 110218, 116772, 123715, 131072 };

static uint32_t hz_q16(uint32_t hz, uint32_t f)  /* hz * f (Q16), as Hz */
{
    return (hz * (f >> 4)) >> 12;
}

int kit_drum_note(int kit, int pad, int tune)   /* the MIDI note a pitched sound (kick, tom, bell) sounds, or -1 */
{
    pad_t p;
    const recipe_t *r;
    uint32_t chz, inc;
    int n, best = -1;
    if (kit < 0 || kit >= nkits() || pad < 0 || pad >= KIT_DRUM_PADS) return -1;
    if (kit == KIT_DRUM_USER && user_pad[pad]) return -1;   /* a sample: its note is not known */
    p = pad_of(kit, pad);
    r = &RECIPES[p.rk][p.slot];
    if (r->wave == W_NONE || r->tlen < 100) return -1;   /* noise, a click: no note */
    if (tune < -12) tune = -12;
    if (tune > 12) tune = 12;
    chz = hz_q16(hz_q16((uint32_t)r->f1 * 100u, (uint32_t)p.tf << 10), SEMI[tune + 12]);   /* centi-Hz */
    inc = chz * 974u;                           /* the oscillator's increment (2^32 / 44100 / 100 = 973.9) */
    for (n = 0; n < 127; n++)                   /* the nearest note: between two, the one it is closer to */
        if (KIT_NOTE_INC[n] <= inc && inc < KIT_NOTE_INC[n + 1]) {
            best = inc - KIT_NOTE_INC[n] < KIT_NOTE_INC[n + 1] - inc ? n : n + 1;
            break;
        }
    return best;
}

/* ---- the voice ---- */
typedef struct {
    uint32_t tinc, tinc1, tph, tph2;            /* tone: increment now and at the end, phases */
    uint16_t tfall, tdec, ndec, mdec, bdec;     /* per block, Q16 */
    int32_t tenv, nenv, menv;                   /* Q15 x level */
    uint16_t tlvl, nlvl, mlvl;
    uint8_t wave, ratio2, bursts, bgap, slot, choke;
    int32_t lpa, hpa, mhpa;                     /* one-pole coefficients, Q15 (0: off) */
    int32_t lps, hps, mlps;                     /* their states */
    uint32_t mph[6], minc;
    const int16_t *pcm;                         /* a user sound: its samples (22 050 Hz), where (Q16), how fast */
    uint32_t plen, ppos, pinc;
    int32_t penv;
    uint16_t pdec;
} dvoice_t;
static dvoice_t dv[KIT_NVOICES];
static const uint16_t METAL_R[6] = { 128, 192, 266, 348, 435, 525 };   /* 2, 3, 4.16, 5.43, 6.79, 8.21 x 64 */

static uint16_t decay_mul(uint32_t ms)          /* to -80 dB in ms: per block, Q16 (tau = ms / 9.2) */
{
    uint32_t tb = ms * (KIT_FS / KIT_BLOCK) / 9200u;   /* tau in blocks */
    uint32_t m;
    if (tb < 1) tb = 1;
    m = 65536u - 65536u / tb + 32768u / (tb * tb);
    return (uint16_t)(m > 65535u ? 65535u : m);
}
static int32_t onepole(uint32_t hz)             /* 1 - e^(-2 pi f / fs), Q15, by a rational fit */
{
    uint32_t x = hz * 4669u / 1000u;            /* 2 pi f / fs, Q15 */
    uint32_t den;
    if (!hz) return 0;
    den = 32768u + x / 2u + (x * (x >> 8)) / 3072u;
    return (int32_t)((x * 32768u / den) > 32767u ? 32767u : x * 32768u / den);
}
static uint32_t inc_hz(uint32_t hz) { return hz * 97391u; }   /* 2^32 / 44100 = 97391.5 */

static void drum_start(int i, int kit, int pad, int tune, int decay)   /* decay: x/64 of the sound's */
{
    dvoice_t *d = &dv[i];
    pad_t p = pad_of(kit, pad);
    const recipe_t *r = &RECIPES[p.rk][p.slot];
    uint32_t tf = (uint32_t)p.tf << 10, df = (uint32_t)p.df * (uint32_t)decay;   /* tf Q16; df x/4096 */
    uint32_t take = (vs[i].age & 1u) ? 65536u : 63570u;   /* two takes, a little different (round robin) */
    if (tune < -12) tune = -12;
    if (tune > 12) tune = 12;
    d->pcm = 0;
    if (kit == KIT_DRUM_USER && user_pad[pad]) {   /* a user sound: played as it is, tuned, faded by DECAY below 100 % */
        const kit_sample_t *u = user_pad[pad];
        d->pcm = u->pcm;
        d->plen = u->len;
        d->ppos = 0;
        d->pinc = SEMI[tune + 12] >> 1;         /* 22 050 Hz at 44 100: half a sample a sample */
        d->penv = 32767;
        d->pdec = decay < 64 ? decay_mul(u->len * 1000u / 22050u * (uint32_t)decay / 64u) : 65535;
        d->tenv = d->nenv = d->menv = 0;
        d->bursts = 0;
        d->slot = p.slot;
        d->choke = 0;
        return;
    }
    tf = hz_q16(tf, SEMI[tune + 12]);           /* (a Q16 x Q16 product, as Q16) */
    d->wave = r->wave;
    d->ratio2 = r->ratio2;
    d->tinc = inc_hz(hz_q16(hz_q16(r->f0, tf), take));
    d->tinc1 = inc_hz(hz_q16(r->f1, tf));
    d->tph = d->tph2 = 0;
    d->tfall = decay_mul((uint32_t)r->fall * 3u);
    d->tdec = decay_mul(r->tlen * df / 4096u);
    d->tenv = r->wave ? 32767 : 0;
    d->tlvl = r->tlvl;
    d->hpa = onepole(hz_q16(r->hp, tf));
    d->lpa = onepole(hz_q16(r->lp, tf));
    d->lps = d->hps = 0;
    d->ndec = decay_mul(r->nlen * df / 4096u);
    d->bdec = decay_mul(4);
    d->nenv = r->nlvl ? 32767 : 0;
    d->nlvl = r->nlvl;
    d->bursts = r->bursts;
    d->bgap = 15;                               /* 11 ms in blocks */
    d->menv = r->mbase ? 32767 : 0;
    d->mlvl = r->mlvl;
    d->mdec = decay_mul(r->mlen * df / 4096u);
    d->minc = inc_hz(hz_q16(r->mbase, tf));
    d->mhpa = onepole(3000);
    d->mlps = 0;
    {
        int k;
        for (k = 0; k < 6; k++) d->mph[k] = 0;
    }
    d->slot = p.slot;
    d->choke = p.slot == SL_HATO;
}

/* one block of voice i into mix; returns 0 when it has gone silent */
static int drum_render(int i, int32_t *mix, int32_t amp)
{
    dvoice_t *d = &dv[i];
    voice_t *v = &vs[i];
    uint32_t r = v->noise;
    int j, k;
    if (d->pcm) {                               /* a user sound */
        int32_t e0 = d->penv, e1 = d->pdec == 65535 ? e0 : (int32_t)(((uint32_t)e0 * d->pdec) >> 16), de = (e1 - e0) / KIT_BLOCK;
        for (j = 0; j < KIT_BLOCK; j++) {
            uint32_t at = d->ppos >> 16, f = d->ppos & 0xFFFFu;
            int32_t a, b, s;
            if (at + 1 >= d->plen) { d->pcm = 0; break; }
            a = d->pcm[at]; b = d->pcm[at + 1];
            s = a + (((b - a) * (int32_t)(f >> 1)) >> 15);
            s = (s * (e0 >> 3)) >> 12;
            mix[j] += (s * amp) >> 15;
            d->ppos += d->pinc;
            e0 += de;
        }
        d->penv = e1;
        if (e1 < 8) d->pcm = 0;
        return d->pcm != 0;
    }
    int32_t t0 = d->tenv, n0 = d->nenv, m0 = d->menv, t1, n1, m1, dt, dn, dm;
    /* the envelopes for the block's end; bursts: the noise starts again every 11 ms */
    t1 = (int32_t)(((uint32_t)t0 * d->tdec) >> 16);
    if (d->bursts) {
        n1 = (int32_t)(((uint32_t)n0 * d->bdec) >> 16);
        if (--d->bgap == 0) { d->bursts--; d->bgap = 15; n1 = 32767; }
    } else {
        n1 = (int32_t)(((uint32_t)n0 * d->ndec) >> 16);
    }
    m1 = (int32_t)(((uint32_t)m0 * d->mdec) >> 16);
    if (t1 < 8) t1 = 0;
    if (n1 < 8 && !d->bursts) n1 = 0;
    if (m1 < 8) m1 = 0;
    dt = (t1 - t0) / KIT_BLOCK;
    dn = (n1 - n0) / KIT_BLOCK;
    dm = (m1 - m0) / KIT_BLOCK;
    for (j = 0; j < KIT_BLOCK; j++) {
        int32_t s = 0;
        if (t0) {
            int32_t o;
            uint32_t ph = d->tph;
            o = d->wave == W_SINE ? sine(ph) : d->wave == W_SQR ? (ph < 0x80000000u ? 16000 : -16000)
              : (int32_t)((ph >> 16) < 32768u ? (ph >> 15) : 131071u - (ph >> 15)) - 32768;   /* triangle */
            if (d->ratio2) {
                uint32_t p2 = d->tph2;
                o = (o >> 1) + ((d->wave == W_SQR ? (p2 < 0x80000000u ? 16000 : -16000) : sine(p2)) >> 1);
                d->tph2 = p2 + (d->tinc >> 6) * d->ratio2;
            }
            d->tph = ph + d->tinc;
            s += ((o * (t0 >> 3)) >> 12) * d->tlvl >> 8;
        }
        if (n0) {
            int32_t nz, x;
            r ^= r << 13; r ^= r >> 17; r ^= r << 5;   /* xorshift noise */
            nz = ((int32_t)(r >> 16) - 32768) >> 1;   /* (half: the filters stay within 32 bits) */
            x = nz;
            if (d->hpa) { d->hps += ((x - d->hps) * d->hpa) >> 15; x = x - d->hps; }
            if (d->lpa) { d->lps += ((x - d->lps) * d->lpa) >> 15; x = d->lps; }
            s += ((x * (n0 >> 3)) >> 9) * d->nlvl >> 8;    /* filtered noise is quieter than a tone: four times */
        }
        if (m0) {
            int32_t x = 0;
            for (k = 0; k < 6; k++) {
                x += d->mph[k] < 0x80000000u ? 2700 : -2700;
                d->mph[k] += (d->minc >> 6) * METAL_R[k];
            }
            d->mlps += ((x - d->mlps) * d->mhpa) >> 15;   /* high-passed: the metal's top */
            x -= d->mlps;
            s += ((x * (m0 >> 3)) >> 9) * d->mlvl >> 8;     /* the metal's top is small: eight times */
        }
        if (s > 65535) s = 65535;                   /* three layers: kept within 32 bits in the product */
        if (s < -65535) s = -65535;
        mix[j] += (((s >> 1) * amp) >> 14) * 3 >> 2;   /* three quarters: the loudest hits stay below the clip */
        t0 += dt;
        n0 += dn;
        m0 += dm;
    }
    d->tenv = t1;
    d->nenv = n1;
    d->menv = m1;
    if (d->tinc > d->tinc1)                     /* the tone falls towards its end */
        d->tinc = d->tinc1 + ((d->tinc - d->tinc1) >> 12) * (uint32_t)(d->tfall >> 4);   /* (within 32 bits) */
    v->noise = r;
    return t1 || n1 || m1 || d->bursts;
}
