/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit sound: four tracks (D-029), each with its own sound: the kit's Basic tones (up to four sine partials a
 * voice), a Felucca-Engines part (engines.c) or the kit's drum kit. One allocator keeps all tracks within
 * KIT_NVOICES voices: a new note takes a chord voice first (a releasing one, then the oldest), else one of its own
 * track; a chord never takes the bass's, the melody's or the drums'. Master volume with a hard ceiling. Integer
 * only; portable. The backend calls kit__synth_render from its audio interrupt; the note functions run in the main
 * loop and take the backend's lock around voice changes. factory/unit/test_synth_tracks.c checks the allocator. */
#include "kit_internal.h"
#include "kit_tables.h"         /* generated: KIT_SINE, KIT_NOTE_INC */

#define ONE (1 << 30)           /* envelope full scale */
#define BLOCKS_PER_S (KIT_FS / KIT_BLOCK)

typedef struct {
    const char *name;
    int16_t partial[4];         /* amplitude of harmonics 1, 2, 3, 4 (Q15, sum <= 32767) */
    uint16_t attack_ms;
    uint16_t decay_mul;         /* per block, Q16: towards sustain */
    int32_t sustain;            /* Q30 */
    uint16_t release_mul;       /* per block, Q16 */
} inst_t;

/* decay_mul = 65536 * exp(-1 / (T * 1378)) for a time constant T seconds */
static const inst_t INSTS[KIT_NINSTS] = {
    { "Sine",      { 30000, 0, 0, 0 },          5, 65535, ONE,     64944 },   /* release T 80 ms */
    { "Piano",     { 20000, 7000, 3000, 1500 }, 2, 65496, 0,       64357 },   /* decay 1.2 s, release 40 ms */
    { "Organ",     { 16000, 9000, 0, 5000 },    8, 65535, ONE,     64357 },
    { "Music box", { 22000, 0, 0, 8000 },       1, 65400, 0,       64944 },   /* decay 0.35 s */
};

typedef struct {
    uint8_t note, active, gate, stage;    /* stage 1 attack, 2 decay/sustain, 3 release */
    uint8_t vel, track, inst, drum;       /* drum: 0 a tone, 1 kick, 2 snare, 3 hat */
    uint32_t inc, ph;
    int32_t env;
    uint32_t age, noise;
    int32_t prev;
    int32_t curq, gstep;                  /* API 27: the pitch now (1/4096 semitones) while it glides to note << 12, a step a block */
} voice_t;

static voice_t vs[KIT_NVOICES];         /* the Basic and drum voices of every track */
static volatile int volume = 80, vol_limit = 100;
static uint32_t ages;
static int32_t attack_step[KIT_NINSTS];
static volatile int tr_engine[KIT_NTRACKS] = { KIT_ENGINE_BASIC, KIT_ENGINE_BASIC, KIT_ENGINE_BASIC, KIT_ENGINE_DRUMS };
static volatile int tr_preset[KIT_NTRACKS] = { KIT_INST_PIANO, KIT_INST_PIANO, KIT_INST_PIANO, 0 };

/* the drum kits: drums.c (API 23, D-053); each drums track's pads: which kit's sound, tuned, its decay (x/64) */
static struct { int8_t kit, src, tune; uint8_t decay; } tr_pad[KIT_NTRACKS][KIT_DRUM_PADS];

/* a track's FX (API 23, D-055): its Basic and drum voices go through it (the drum lab's Punch, MPC, Lofi, Tape) */
static volatile uint8_t tr_fx[KIT_NTRACKS];
static int32_t fxbuf[KIT_NTRACKS][KIT_BLOCK];
static struct { int32_t env, gain, lp, hp; uint32_t crackle; uint16_t tail; } fxs[KIT_NTRACKS];
static int16_t SOFT[257];                       /* tanh-like soft clip over -2..2 (Q15 in, Q15 out), made at init */
void kit_track_fx(int t, int fx) { if (t >= 0 && t < KIT_NTRACKS) tr_fx[t] = (uint8_t)(fx < 0 || fx >= KIT_NFX ? 0 : fx); }
int kit_track_fx_of(int t) { return t >= 0 && t < KIT_NTRACKS ? tr_fx[t] : 0; }
static volatile uint8_t tr_level[KIT_NTRACKS] = { 10, 10, 10, 10 };   /* API 26: each track's fader, 0..10 */
void kit_track_level(int t, int level)
{
    if (t < 0 || t >= KIT_NTRACKS) return;
    tr_level[t] = (uint8_t)(level < 0 ? 0 : level > 10 ? 10 : level);
    if (!tr_level[t]) kit_track_notes_off(t);
}
int kit_track_level_of(int t) { return t >= 0 && t < KIT_NTRACKS ? tr_level[t] : 0; }
static int tr_pads_init;

void kit__synth_init(void)
{
    int i;
    for (i = 0; i <= 256; i++) {                /* x in -2..2 (Q10 here): x (27 + x^2) / (27 + 9 x^2), tanh near enough */
        int32_t x = (i - 128) * 16, x2 = x * x >> 10;
        int32_t y = x * (27648 + x2) / (27648 + 9 * x2);   /* (within 32 bits: |x| <= 2048) */
        SOFT[i] = (int16_t)(y * 32 > 32767 ? 32767 : y * 32 < -32767 ? -32767 : y * 32);
    }
    for (i = 0; i < KIT_NINSTS; i++) {
        int32_t blocks = (int32_t)INSTS[i].attack_ms * BLOCKS_PER_S / 1000;
        attack_step[i] = ONE / (blocks > 0 ? blocks : 1);
    }
}

static int okt(int t) { return t >= 0 && t < KIT_NTRACKS; }
static inline int32_t sine(uint32_t ph);
#include "drums.c"

static void pads_init(void)
{
    int t, p;
    for (t = 0; t < KIT_NTRACKS; t++)
        for (p = 0; p < KIT_DRUM_PADS; p++) { tr_pad[t][p].kit = -1; tr_pad[t][p].src = (int8_t)p; tr_pad[t][p].tune = 0; tr_pad[t][p].decay = 64; }
    tr_pads_init = 1;
}
void kit_drum_pad_set(int t, int pad, int kit, int src, int tune, int decay)
{
    if (!okt(t) || pad < 0 || pad >= KIT_DRUM_PADS) return;
    if (!tr_pads_init) pads_init();
    tr_pad[t][pad].kit = (int8_t)(kit < -1 || kit >= nkits() ? -1 : kit);
    tr_pad[t][pad].src = (int8_t)(src < 0 || src >= KIT_DRUM_PADS ? pad : src);
    tr_pad[t][pad].tune = (int8_t)(tune < -12 ? -12 : tune > 12 ? 12 : tune);
    tr_pad[t][pad].decay = (uint8_t)(decay < 16 ? 16 : decay > 255 ? 255 : decay);
}
int kit_drum_pad_of(int note)                   /* F3..G5 the 27 pads; General MIDI's drums; else by pitch class */
{
    static const int8_t GM[18] = { 0, 0, 7, 4, 9, 5, 19, 12, 19, 14, 20, 15, 20, 21, 26, 21, 22, 26 };   /* 35..52 */
    static const int8_t PC[12] = { 0, 7, 4, 9, 5, 19, 12, 20, 14, 21, 15, 26 };
    if (note >= KIT_DRUM_LO && note < KIT_DRUM_LO + KIT_DRUM_PADS) return note - KIT_DRUM_LO;
    if (note >= 35 && note <= 52) return GM[note - 35];
    return note < 0 ? 0 : PC[note % 12];
}
static uint8_t tr_vmode[KIT_NTRACKS], tr_slide[KIT_NTRACKS];   /* API 27: each track's voice mode and slide (kit_track_voice) */
static int is_part(int t) { return tr_engine[t] != KIT_ENGINE_BASIC && tr_engine[t] != KIT_ENGINE_DRUMS; }

int kit_engine_count(void) { return 2 + kit__eng_count(); }
/* the kit's engine e as the host's (engines.c): 1..6 its first six, 7 the drums (not the host's), 8.. its others
 * (API 32: FMini is 8), so the numbers songs and apps keep never move */
static int eng_of(int e) { return e >= 1 && e < KIT_ENGINE_DRUMS ? e - 1 : e > KIT_ENGINE_DRUMS ? e - 2 : -1; }
const char *kit_engine_name(int e)
{
    return e == KIT_ENGINE_BASIC ? "Basic" : e == KIT_ENGINE_DRUMS ? "Drums" : kit__eng_name(eng_of(e));
}
int kit_preset_count(int e)
{
    return e == KIT_ENGINE_BASIC ? KIT_NINSTS : e == KIT_ENGINE_DRUMS ? nkits() : kit__eng_npresets(eng_of(e));
}
const char *kit_preset_name(int e, int p)
{
    return e == KIT_ENGINE_BASIC ? kit_instrument_name(p) : e == KIT_ENGINE_DRUMS ? kit_drum_kit_name(p)
         : kit__eng_preset_name(eng_of(e), p);
}

static int basic_n(int t)               /* Basic or drum voices sounding on track t (-1: every track) */
{
    int i, n = 0;
    for (i = 0; i < KIT_NVOICES; i++)
        n += vs[i].active && (t < 0 || vs[i].track == t);
    return n;
}
static int voices_on(int t) { return is_part(t) ? kit__eng_nactive(t) : basic_n(t); }

int kit__synth_active(void) { return basic_n(-1) || kit__eng_active(); }
int kit__track_voices(int t) { return okt(t) ? voices_on(t) : 0; }
int kit__track_sounding(int t, int note)        /* a Basic or drum voice of track t plays note (or releases it) */
{
    int i;
    for (i = 0; i < KIT_NVOICES; i++)
        if (vs[i].active && vs[i].track == t && vs[i].note == note)
            return 1;
    return 0;
}

int kit_track_engine(int t) { return okt(t) ? tr_engine[t] : -1; }
int kit_track_preset(int t) { return okt(t) ? tr_preset[t] : -1; }
int kit_sound_engine(void) { return tr_engine[0]; }
int kit_sound_preset(void) { return tr_preset[0]; }

/* the sound's effects (API 36, D-093): its preset's sends; an FMini patch sets the bus too (its twelve FX settings,
 * fmini-1's FA_XG: reverb TYPE DECAY TONE, delay TYPE TIME FEEDBACK, chorus TYPE DEPTH RATE, drive TYPE TONE OUTPUT), as
 * fmini-1 does when a patch is loaded. fmini-1's delay TYPEs P-PONG and MULTI play as DIGITAL here (mono), REVERSE and
 * A-SYNC as ANALOG; the reverb is the kit's room, its DECAY the TYPE's size */
static void sound_fx(int t, int shared_fx)
{
    static const uint8_t DLY_OF[8] = { 0, 1, 2, 1, 1, 3, 0, 0 };
    uint8_t sends[4], x[12];
    if (kit__eng_fx(t, sends, x) && shared_fx) {
        kit_fx_t f;
        kit_fx_bus_of(&f);
        f.cho_type = (uint8_t)(x[6] % 3u);
        f.cho_depth = x[7] > 127 ? 127 : x[7];
        f.cho_rate = x[8] > 127 ? 127 : x[8];
        f.dly_type = DLY_OF[x[3] & 7u];
        f.dly_time = x[4] > 127 ? 127 : x[4];
        f.dly_fb = x[5] > 120 ? 120 : x[5];
        kit_fx_bus(&f);
        kit__rv_room(x[1]);
    }
    kit_track_sends(t, sends[0], sends[1], sends[2], sends[3]);
}

static void track_sound(int t, int e, int p, int shared_fx)
{
    uint32_t k;
    int i;
    if (!okt(t) || e < 0 || e >= kit_engine_count() || p < 0 || p >= kit_preset_count(e))
        return;
    k = kit__lock();
    for (i = 0; i < KIT_NVOICES; i++)               /* what the track sounds stops */
        if (vs[i].track == t)
            vs[i].active = 0;
    kit__eng_select(t, eng_of(e), p);
    if (e == KIT_ENGINE_FMINI)                      /* API 32: an FMini sound brings its own way of playing (MONO legato or POLY) */
        tr_vmode[t] = (uint8_t)(kit__eng_preset_mono(eng_of(e), p) ? KIT_VMODE_LEGATO : KIT_VMODE_POLY);
    if (e == KIT_ENGINE_CF_VA)
        tr_vmode[t] = (uint8_t)(kit__eng_preset_mono(eng_of(e), p) ? KIT_VMODE_MONO : KIT_VMODE_POLY);
    tr_engine[t] = e;
    tr_preset[t] = p;
    kit__unlock(k);
    sound_fx(t, shared_fx);
}
void kit_track_sound(int t, int e, int p) { track_sound(t, e, p, 1); }
void kit_track_sound_local(int t, int e, int p) { track_sound(t, e, p, 0); }
void kit_sound(int e, int p) { kit_track_sound(0, e, p); }
void kit_instrument(int inst) { kit_sound(KIT_ENGINE_BASIC, inst); }
const char *kit_instrument_name(int inst)
{
    return inst >= 0 && inst < KIT_NINSTS ? INSTS[inst].name : "?";
}
void kit_volume(int pct) { volume = pct < 0 ? 0 : pct > 100 ? 100 : pct; }
void kit_volume_limit(int pct) { vol_limit = pct < 0 ? 0 : pct > 100 ? 100 : pct; }

static int basic_steal(int t)           /* silence one of track t's Basic or drum voices: releasing first, then oldest */
{
    voice_t *best = 0;
    int i;
    for (i = 0; i < KIT_NVOICES; i++) {
        voice_t *v = &vs[i];
        if (!v->active || v->track != t)
            continue;
        if (!best || (v->gate == 0 && best->gate != 0) || (v->gate == best->gate && v->age < best->age))
            best = v;
    }
    if (!best)
        return 0;
    best->active = 0;
    return 1;
}
static int steal(int t) { return is_part(t) ? kit__eng_steal(t) : basic_steal(t); }

static int has_note(int t, int note)
{
    int i;
    if (is_part(t))
        return kit__eng_has(t, note);
    for (i = 0; i < KIT_NVOICES; i++)
        if (vs[i].active && vs[i].gate && vs[i].track == t && vs[i].note == note)
            return 1;
    return 0;
}

/* ---- the voices' budget (API 20, D-047) ---- */
/* each engine's CPU per voice, tenths of %: measured on the cycle-modelled emulator (factory/cpu.py), the heaviest
 * preset of each: Basic, Analog, Phase, Lo-fi, Trio, Organ wheel, Noise, Drums, FMini, Juno-106 */
static const uint8_t VOICE_COST[] = { 23, 46, 43, 21, 53, 53, 44, 20,   /* Drums: the layered kits (D-053), two thirds of a Basic voice at their heaviest, rounded up */
                                        92, 98, 220, 200, 120 };                        /* FMini (SOLINA), the Juno (Synth Pad): D-091 */
#define BASE_COST 100                           /* the FM-1 idle, tenths of % */
#define PART_COST 30                            /* a track with voices sounding: its per-block work */
#define JUNO_PART_COST 150                      /* .. the Juno's: its LFO, HPF, VCA and chorus run on every sample (D-091) */
int kit__bg;
static uint8_t tr_live[KIT_NTRACKS];            /* the track's last note was played live (not by the song) */
static uint32_t stolen;
static int vcost(int t) { int e = tr_engine[t]; return VOICE_COST[e >= 0 && e < (int)(sizeof VOICE_COST / sizeof VOICE_COST[0]) ? e : 0]; }
static int pcost(int t)
{
    /* BS2 has one voice per track. Its measured 22-point increment includes the track. */
    if (tr_engine[t] == KIT_ENGINE_BS2) return 0;
    return tr_engine[t] == KIT_ENGINE_JUNO ? JUNO_PART_COST : PART_COST;
}
static int basic_rel(int t)
{
    int i, n = 0;
    for (i = 0; i < KIT_NVOICES; i++) n += vs[i].active && !vs[i].gate && vs[i].track == t;
    return n;
}
static int releasing(int t) { return is_part(t) ? kit__eng_nreleasing(t) : basic_rel(t); }
static int load_now(void)                       /* tenths of %, as the voices sound now */
{
    int u, l = BASE_COST;
    for (u = 0; u < KIT_NTRACKS; u++) { int n = voices_on(u); if (n) l += pcost(u) + n * vcost(u); }
    return l;
}
static int total_now(void) { int u, n = 0; for (u = 0; u < KIT_NTRACKS; u++) n += voices_on(u); return n; }
static int fits(int t)
{
    int extra = vcost(t) + (voices_on(t) ? 0 : pcost(t));
    return total_now() < KIT_NVOICES && load_now() + extra <= KIT_CPU_LIMIT * 10;
}
static int steal_one(int t)                     /* take one voice for a note on track t; 0: none may be taken */
{
    int u, best = -1;
    for (u = 0; u < KIT_NTRACKS; u++)           /* (1) a releasing voice: the track with most of them */
        if (releasing(u) && (best < 0 || releasing(u) > releasing(best))) best = u;
    if (best < 0)
        for (u = 0; u < KIT_NTRACKS; u++) {     /* (2) another track with two or more: the song's before the live ones */
            int nu = voices_on(u);
            if (u == t || nu < 2) continue;
            if (best < 0 || tr_live[u] < tr_live[best] || (tr_live[u] == tr_live[best] && nu > voices_on(best))) best = u;
        }
    if (best < 0 && voices_on(t)) best = t;     /* (3) its own oldest */
    if (best < 0) return 0;                     /* a track never loses its last note */
    stolen++;
    return steal(best);
}
/* room for one more voice on track t: 1, or 0 when the note must be dropped (the caller holds the lock) */
static int make_room(int t)
{
    int guard = KIT_NVOICES;
    while (!fits(t) && guard--)
        if (!steal_one(t)) { stolen++; return 0; }
    return fits(t);
}
int kit_voices_used(void) { uint32_t k = kit__lock(); int n = total_now(); kit__unlock(k); return n; }
int kit_voices_load(void) { uint32_t k = kit__lock(); int l = load_now(); kit__unlock(k); return (l + 5) / 10; }
uint32_t kit_voices_stolen(void) { return stolen; }

/* ---- a MIDI keyboard's controls (API 17, D-044): bend, vibrato, sustain ---- */
static int16_t bend16[KIT_NTRACKS], bend_raw[KIT_NTRACKS];
static uint8_t bend_range[KIT_NTRACKS];         /* zero until set: the original two semitones */
static uint8_t mod_depth[KIT_NTRACKS];
static uint16_t lfo_ph[KIT_NTRACKS];
static uint8_t sus_on[KIT_NTRACKS];
static uint32_t sus_held[KIT_NTRACKS][4];       /* notes let go while the pedal is down, a bit each */
static int tune16;                              /* TUNE, 1/16 semitones (-8..8) */
static int32_t click_amp;                       /* the click: its level now (Q15), its phase and step */
static uint32_t click_ph, click_inc;
#define CLICK_DECAY 59840u                      /* a block's decay (Q16): a time constant of 8 ms */
#define CLICK_INC_1K 97391263u                  /* 1 kHz: 2^32 * 1000 / 44100 */

void kit_tune(int cents)
{
    if (cents < -50) cents = -50;
    if (cents > 50) cents = 50;
    tune16 = cents >= 0 ? (cents * 16 + 50) / 100 : -((-cents * 16 + 50) / 100);
}

/* ---- the reverb (API 24, D-060): a small Freeverb (four damped combs, two allpasses; Jezar's public-domain design)
 * on the whole mix, worked at half the rate (22 050 Hz: half the work and memory, and the tail darker), its output
 * eased back up between the pairs. 3.3 KB of delay lines. Off (level 0) it costs nothing. ---- */
#define RV_NC 4
#define RV_NA 2
static const uint16_t RV_CLEN[RV_NC] = { 558, 594, 638, 678 };   /* Freeverb's 1116 1188 1277 1356, halved */
static const uint16_t RV_ALEN[RV_NA] = { 278, 220 };             /* its 556 441, halved */
static int16_t rv_c0[558], rv_c1[594], rv_c2[638], rv_c3[678], rv_a0[278], rv_a1[220];
static int16_t *const RV_C[RV_NC] = { rv_c0, rv_c1, rv_c2, rv_c3 };
static int16_t *const RV_A[RV_NA] = { rv_a0, rv_a1 };
static uint16_t rv_ci[RV_NC], rv_ai[RV_NA];
static int32_t rv_lp[RV_NC], rv_prev;
static volatile uint8_t rv_level, rv_room = 85;  /* the wet share 0..100; the combs' feedback, % */
static uint8_t rv_sent;                         /* the sends fed it last block (D-093) */
static uint32_t rv_tail;                        /* samples of its tail still to run after the sends stop */
static uint16_t rv_tail_ms = 500;
static kit_reverb_shape_t rv_shape = {77, 70, 0};
#define RV_PRE_LEN 4096u
static int16_t rv_pre[RV_PRE_LEN] KIT_BIG;
static uint16_t rv_pre_write, rv_pre_count;
static void rv_clear(void);

static int16_t sat16(int32_t x) { return (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x); }
/* x / 32768 towards zero: a floor (>> 15) rounds every small negative down, so a comb's last -1s go round for ever
 * (a steady hum after the tail, D-094); towards zero they die out */
static int32_t q15z(int32_t x) { return x >= 0 ? x >> 15 : -((-x) >> 15); }

void kit_reverb(int level)
{
    uint32_t k;
    level = level < 0 ? 0 : level > 100 ? 100 : level;
    k = kit__lock();
    if (!rv_level && level && !rv_sent) rv_clear();   /* switched on: from silence, not an old tail */
    rv_level = (uint8_t)level;
    kit__unlock(k);
}
int kit_reverb_of(void) { return rv_level; }

void kit__rv_room(int size)                     /* the bus's DECAY (D-093): the combs' feedback 70 .. 95 % */
{
    rv_shape.size = (uint8_t)(size < 0 ? 0 : size > 127 ? 127 : size);
    rv_room = (uint8_t)(70 + rv_shape.size * 25 / 127);
    rv_tail_ms = 500;
}

void kit_reverb_shape(const kit_reverb_shape_t *shape)
{
    uint32_t k;
    int size, pre;
    if (!shape) return;
    k = kit__lock();
    size = shape->size > 127 ? 127 : shape->size;
    if (size != rv_shape.size) {
        kit__rv_room(size);
        rv_tail_ms = (uint16_t)(400 + size * 4000 / 127);
    }
    rv_shape.damp = shape->damp > 100 ? 100 : shape->damp;
    pre = shape->pre_ms > 120 ? 120 : shape->pre_ms;
    if (!pre || !rv_shape.pre_ms) rv_pre_count = 0;
    rv_shape.pre_ms = (uint8_t)pre;
    kit__unlock(k);
}

void kit_reverb_shape_of(kit_reverb_shape_t *shape) { if (shape) *shape = rv_shape; }
int kit_reverb_tail_ms_of(void) { return rv_tail_ms; }

static void rv_clear(void)
{
    int i, j;
    for (i = 0; i < RV_NC; i++) { for (j = 0; j < RV_CLEN[i]; j++) RV_C[i][j] = 0; rv_ci[i] = 0; rv_lp[i] = 0; }
    for (i = 0; i < RV_NA; i++) { for (j = 0; j < RV_ALEN[i]; j++) RV_A[i][j] = 0; rv_ai[i] = 0; }
    rv_prev = 0;
    rv_pre_write = rv_pre_count = 0;
}
/* KIT_BLOCK samples of the mix: the reverb added. send: the tracks' sends (fxbus.c, D-093), or 0: the whole mix at
 * kit_reverb's level, as before */
static void rv_block(int32_t *mix, const int32_t *send)
{
    int32_t fb = (int32_t)rv_room * 32768 / 100, wet = send ? 768 : (int32_t)rv_level * 3 * 256 / 100;   /* wet: Q8, up to x 3 */
    int32_t damp = (100 - rv_shape.damp) * 32768 / 100;
    uint32_t pre = (uint32_t)rv_shape.pre_ms * 2205u / 100u;
    int j, i;
    if (damp < 1638) damp = 1638;               /* a dark room still lets the combs decay */
    for (j = 0; j < KIT_BLOCK; j += 2) {
        int32_t in = send ? ((mix[j] + mix[j + 1]) / 100 * rv_level + send[j] + send[j + 1]) >> 4
                          : (mix[j] + mix[j + 1]) >> 4, out = 0, y;   /* the pair, at an eighth (the combs add up) */
        if (pre) {
            uint32_t w = rv_pre_write;
            y = pre <= rv_pre_count ? rv_pre[(w - pre) & (RV_PRE_LEN - 1u)] : 0;
            rv_pre[w] = sat16(in);
            rv_pre_write = (uint16_t)((w + 1u) & (RV_PRE_LEN - 1u));
            if (rv_pre_count < RV_PRE_LEN) rv_pre_count++;
            in = y;
        }
        for (i = 0; i < RV_NC; i++) {
            int16_t *b = RV_C[i];
            y = b[rv_ci[i]];
            rv_lp[i] += q15z((y - rv_lp[i]) * damp);
            b[rv_ci[i]] = sat16(in + q15z(rv_lp[i] * fb));
            if (++rv_ci[i] >= RV_CLEN[i]) rv_ci[i] = 0;
            out += y;
        }
        for (i = 0; i < RV_NA; i++) {
            int16_t *b = RV_A[i];
            y = b[rv_ai[i]];
            b[rv_ai[i]] = sat16(out + (y >> 1));
            if (++rv_ai[i] >= RV_ALEN[i]) rv_ai[i] = 0;
            out = y - out;
        }
        out = (out * wet) >> 8;
        mix[j] += (rv_prev + out) >> 1;         /* back to 44 100 Hz: halfway, then the new one */
        mix[j + 1] += out;
        rv_prev = out;
    }
}

/* ---- sound effects (API 24, D-060, D-062): one voice of its own, never on a track, as the click. PFFT: a low buzz
 * falling from 95 to 55 Hz, fluttering, with a breath of noise (half a second); PLOP: a drop, a sine rising from
 * 250 to 700 Hz as it dies away (a tenth of a second); NEIGH: a whinny, a nasal buzz up from 520 to 900 Hz and down
 * to 420, trembling 13 times a second (a second). ---- */
static volatile uint8_t sfx_id = 0xFF;
static uint32_t sfx_n, sfx_len, sfx_ph, sfx_ph2, sfx_ph3, sfx_fl, sfx_noise = 22222u;
static int32_t sfx_amp, sfx_lp, sfx_lp2;
#define HZ_INC 97391u                           /* 1 Hz: 2^32 / 44100 */

void kit_sfx(int id, int level)
{
    uint32_t k;
    if (id < 0 || id >= KIT_NSFX || level < 1) return;
    if (level > 10) level = 10;
    k = kit__lock();
    sfx_id = (uint8_t)id; sfx_n = 0; sfx_ph = sfx_ph2 = sfx_ph3 = 0; sfx_fl = 0; sfx_lp = sfx_lp2 = 0;
    sfx_len = id == KIT_SFX_PFFT ? KIT_FS * 11 / 20 : id == KIT_SFX_NEIGH ? KIT_FS : id == KIT_SFX_GULL ? KIT_FS * 11 / 20
            : id == KIT_SFX_BAA ? KIT_FS * 13 / 20 : id == KIT_SFX_CHIRP ? KIT_FS * 3 / 10
            : id == KIT_SFX_WHISTLE ? KIT_FS : id == KIT_SFX_COUGH ? KIT_FS * 9 / 20 : id == KIT_SFX_SNEEZE ? KIT_FS * 13 / 10
            : id == KIT_SFX_WOOF ? KIT_FS * 9 / 40 : id == KIT_SFX_MOO ? KIT_FS : id == KIT_SFX_HOOT ? KIT_FS * 3 / 4
            : id == KIT_SFX_HOWL ? KIT_FS * 7 / 5 : KIT_FS / 8;
    sfx_amp = level * 2600;                     /* level 10: about 0.8 of full scale */
    kit__unlock(k);
}

static void sfx_block(int32_t *mix)
{
    int32_t env, f, j;
    uint32_t inc, p = sfx_n * 1000 / sfx_len;   /* 0..1000 through it */
    if (sfx_id == KIT_SFX_PFFT) {
        env = p < 30 ? (int32_t)p * 32767 / 30 : p > 700 ? (int32_t)(1000 - p) * 32767 / 300 : 32767;
        f = 95 - (int32_t)p * 40 / 1000;
        inc = (uint32_t)f * HZ_INC;
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t saw = (int32_t)(sfx_ph >> 16) - 32768, fl, nz, s;
            sfx_noise = sfx_noise * 1664525u + 1013904223u;
            nz = (int32_t)(sfx_noise >> 17) - 16384;
            sfx_lp += ((saw + nz / 4 - sfx_lp) * 2600) >> 15;   /* softened (a low-pass near 560 Hz): a buzz, not a saw */
            fl = 20000 + (sine(sfx_fl) >> 2) + (nz >> 2);         /* the flutter, 23 Hz and a little ragged */
            s = (sfx_lp * fl) >> 15;
            mix[j] += (((s * env) >> 14) * sfx_amp) >> 15;   /* (twice: the low-pass took half the level) */
            sfx_ph += inc + (uint32_t)((nz >> 6) * 600);
            sfx_fl += 23u * HZ_INC;
        }
    } else if (sfx_id == KIT_SFX_GULL || sfx_id == KIT_SFX_BAA) {
        /* GULL: two squawks, each a bright buzz falling 1500 -> 900 Hz; BAA: a nasal buzz 330 -> 290 Hz, trembling 9 a second */
        int32_t trem, q, gate = 32767;
        if (sfx_id == KIT_SFX_GULL) {
            q = p < 500 ? (int32_t)p * 2 : ((int32_t)p - 500) * 2;   /* 0..1000 within a squawk */
            f = 1500 - q * 600 / 1000;
            env = q < 40 ? q * 32767 / 40 : (1000 - q) * 32767 / 960;
            if (q > 860) gate = 0;
            trem = 32767;
        } else {
            f = 330 - (int32_t)p * 40 / 1000;
            env = p < 60 ? (int32_t)p * 32767 / 60 : p > 700 ? (int32_t)(1000 - p) * 32767 / 300 : 32767;
            trem = 16384 + (sine(sfx_fl) >> 1);
        }
        inc = (uint32_t)f * HZ_INC;
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t saw = (int32_t)(sfx_ph >> 16) - 32768, s;
            sfx_lp += ((saw - sfx_lp) * (sfx_id == KIT_SFX_GULL ? 20000 : 9000)) >> 15;
            sfx_lp2 += ((sfx_lp - sfx_lp2) * 2500) >> 15;
            s = (((sfx_lp - sfx_lp2) * trem) >> 15) * gate >> 15;
            mix[j] += (((s * env) >> 15) * sfx_amp) >> 15;
            sfx_ph += inc;
        }
        sfx_fl += 9u * HZ_INC * KIT_BLOCK;
    } else if (sfx_id == KIT_SFX_CHIRP) {
        /* three quick high notes, each sliding down: a small bird far off */
        int32_t q = (int32_t)p * 3 % 1000;
        f = 3400 - q * 1200 / 1000;
        env = q < 60 ? q * 32767 / 60 : q > 350 ? 0 : (350 - q) * 32767 / 290;
        inc = (uint32_t)f * HZ_INC;
        for (j = 0; j < KIT_BLOCK; j++) { mix[j] += (((sine(sfx_ph) * env) >> 15) * sfx_amp) >> 16; sfx_ph += inc; }
    } else if (sfx_id == KIT_SFX_NEIGH) {
        int32_t trem, vib;
        env = p < 30 ? (int32_t)p * 32767 / 30 : p > 650 ? (int32_t)(1000 - p) * 32767 / 350 : 32767;
        f = p < 150 ? 520 + (int32_t)p * 380 / 150 : 900 - ((int32_t)p - 150) * 480 / 850;   /* up quickly, then down */
        vib = sine(sfx_fl) >> 4;                /* the tremble: the pitch about 6 % either way, 13 Hz */
        inc = (uint32_t)(f + f * vib / 32768) * HZ_INC;
        trem = 22000 + (sine(sfx_fl) >> 2);     /* and the level with it */
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t saw = (int32_t)(sfx_ph >> 16) - 32768, s;
            sfx_lp += ((saw - sfx_lp) * 12000) >> 15;              /* nasal: a saw, a little softened */
            sfx_lp2 += ((sfx_lp - sfx_lp2) * 3000) >> 15;          /* less of the low end: the throat */
            s = ((sfx_lp - sfx_lp2) * trem) >> 15;
            mix[j] += (((s * env) >> 15) * sfx_amp) >> 15;
            sfx_ph += inc;
        }
        sfx_fl += 13u * HZ_INC * KIT_BLOCK;
    } else if (sfx_id == KIT_SFX_WHISTLE) {
        /* two blasts (0..350, 420..1000): a chord of three soft triangles, 587 740 880 Hz, rising a little as each
         * starts, with a breath of noise */
        int32_t q = p < 350 ? (int32_t)p * 1000 / 350 : p < 420 ? -1 : ((int32_t)p - 420) * 1000 / 580, bend;
        env = q < 0 ? 0 : q < 60 ? q * 32767 / 60 : q > 880 ? (1000 - q) * 32767 / 120 : 30000;
        bend = q < 0 ? 970 : q < 120 ? 970 + q * 30 / 120 : 1000;
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t a = (int32_t)(sfx_ph >> 16) - 32768, b = (int32_t)(sfx_ph2 >> 16) - 32768, c = (int32_t)(sfx_ph3 >> 16) - 32768, s, nz;
            a = (a < 0 ? -a : a) * 2 - 32768; b = (b < 0 ? -b : b) * 2 - 32768; c = (c < 0 ? -c : c) * 2 - 32768;   /* triangles */
            sfx_noise = sfx_noise * 1664525u + 1013904223u; nz = (int32_t)(sfx_noise >> 17) - 16384;
            sfx_lp += ((nz - sfx_lp) * 9000) >> 15; sfx_lp2 += ((sfx_lp - sfx_lp2) * 3000) >> 15;
            s = (a + b + c) / 3 + (sfx_lp - sfx_lp2) / 3;
            mix[j] += (((s * env) >> 15) * sfx_amp) >> 15;
            sfx_ph += (uint32_t)(587 * bend / 1000) * HZ_INC; sfx_ph2 += (uint32_t)(740 * bend / 1000) * HZ_INC; sfx_ph3 += (uint32_t)(880 * bend / 1000) * HZ_INC;
        }
    } else if (sfx_id == KIT_SFX_COUGH || sfx_id == KIT_SFX_SNEEZE || sfx_id == KIT_SFX_WOOF) {
        /* COUGH: two rough bursts (0 and 0.2 s), low noise and a thump falling 110 -> 60 Hz. SNEEZE: "ah" (a softened saw
         * 170 -> 260 Hz), "ah" (220 -> 330), "aaah" (230 -> 420), then CHOO: a loud burst of noise and a thump 140 -> 45.
         * WOOF: a short nasal saw falling 420 -> 210 Hz with a little noise. */
        int32_t vo = 0, nzl = 0, th = 0, fv = 0, fth = 0, q;
        if (sfx_id == KIT_SFX_COUGH) {
            q = p < 500 ? (int32_t)p * 2 : ((int32_t)p - 500) * 2;   /* 0..1000 within a burst */
            nzl = q < 20 ? q * 32767 / 20 : q < 450 ? (450 - q) * 32767 / 430 : 0; th = nzl; fth = 110 - q * 50 / 450; if (fth < 50) fth = 50;
        } else if (sfx_id == KIT_SFX_SNEEZE) {
            if (p < 200) { q = (int32_t)p * 5; vo = q < 100 ? q * 20000 / 100 : (1000 - q) * 20000 / 900; fv = 170 + q * 90 / 1000; }
            else if (p < 250) vo = 0;
            else if (p < 460) { q = ((int32_t)p - 250) * 1000 / 210; vo = q < 100 ? q * 24000 / 100 : (1000 - q) * 24000 / 900; fv = 220 + q * 110 / 1000; }
            else if (p < 690) { q = ((int32_t)p - 460) * 1000 / 230; vo = q * 28000 / 1000; fv = 230 + q * 190 / 1000; }
            else { q = ((int32_t)p - 690) * 1000 / 310; nzl = q < 30 ? q * 32767 / 30 : (1000 - q) * 32767 / 970; th = q < 500 ? (500 - q) * 32767 / 500 : 0; fth = 140 - q * 95 / 1000; }
        } else {
            q = (int32_t)p;
            vo = q < 60 ? q * 30000 / 60 : (1000 - q) * 30000 / 940; nzl = vo / 4; fv = 420 - q * 210 / 1000;
        }
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t saw = (int32_t)(sfx_ph >> 16) - 32768, nz, s;
            sfx_noise = sfx_noise * 1664525u + 1013904223u; nz = (int32_t)(sfx_noise >> 17) - 16384;
            sfx_lp += (((vo ? saw : 0) + (nz * 2) * nzl / 32768 - sfx_lp) * (sfx_id == KIT_SFX_SNEEZE && !vo ? 16000 : 7000)) >> 15;
            s = ((sfx_lp * (vo ? vo : 32767)) >> 15) + ((sine(sfx_ph2) * th) >> 16);
            mix[j] += (s * sfx_amp) >> 15;
            sfx_ph += (uint32_t)(fv > 0 ? fv : 100) * HZ_INC; sfx_ph2 += (uint32_t)(fth > 0 ? fth : 60) * HZ_INC;
        }
    } else if (sfx_id == KIT_SFX_MOO) {
        /* a cow: a saw 150 -> 125 Hz through a low-pass that opens and closes (the mouth), slow in and out */
        int32_t cut = p < 300 ? 3000 + (int32_t)p * 5000 / 300 : 8000 - ((int32_t)p - 300) * 5500 / 700;
        env = p < 100 ? (int32_t)p * 32767 / 100 : p > 800 ? (int32_t)(1000 - p) * 32767 / 200 : 32767;
        inc = (uint32_t)(150 - (int32_t)p * 25 / 1000) * HZ_INC;
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t saw = (int32_t)(sfx_ph >> 16) - 32768;
            sfx_lp += ((saw - sfx_lp) * cut) >> 15; sfx_lp2 += ((sfx_lp - sfx_lp2) * cut) >> 15;
            mix[j] += (((sfx_lp2 * env) >> 14) * sfx_amp) >> 15;
            sfx_ph += inc;
        }
    } else if (sfx_id == KIT_SFX_HOOT || sfx_id == KIT_SFX_HOWL) {
        /* HOOT: two soft hoots, 420 -> 380 Hz (0..400) and 420 -> 370 (520..1000). HOWL: up 480 -> 820 Hz (0..300), then
         * down to 620, wavering 5 times a second */
        int32_t q, vib = 0;
        if (sfx_id == KIT_SFX_HOOT) {
            if (p < 400) { q = (int32_t)p * 1000 / 400; f = 420 - q * 40 / 1000; }
            else if (p < 520) { q = -1; f = 400; }
            else { q = ((int32_t)p - 520) * 1000 / 480; f = 420 - q * 50 / 1000; }
            env = q < 0 ? 0 : q < 150 ? q * 32767 / 150 : (1000 - q) * 32767 / 850;
        } else {
            f = p < 300 ? 480 + (int32_t)p * 340 / 300 : 820 - ((int32_t)p - 300) * 200 / 700;
            env = p < 80 ? (int32_t)p * 32767 / 80 : p > 750 ? (int32_t)(1000 - p) * 32767 / 250 : 32767;
            vib = sine(sfx_fl) >> 5;
            sfx_fl += 5u * HZ_INC * KIT_BLOCK;
        }
        inc = (uint32_t)(f + f * vib / 32768) * HZ_INC;
        for (j = 0; j < KIT_BLOCK; j++) { mix[j] += (((sine(sfx_ph) * env) >> 15) * sfx_amp) >> 15; sfx_ph += inc; }
    } else {
        env = p < 20 ? (int32_t)p * 32767 / 20 : (int32_t)(1000 - p) * (int32_t)(1000 - p) / 30;   /* up fast, dies away */
        f = 250 + (int32_t)p * (int32_t)p * 450 / 1000000;
        inc = (uint32_t)f * HZ_INC;
        for (j = 0; j < KIT_BLOCK; j++) {
            mix[j] += (((sine(sfx_ph) * env) >> 15) * sfx_amp) >> 15;
            sfx_ph += inc;
        }
    }
    sfx_n += KIT_BLOCK;
    if (sfx_n >= sfx_len) sfx_id = 0xFF;
}

void kit_click(int accent, int level)
{
    uint32_t k;
    if (level < 1) return;
    if (level > 10) level = 10;
    k = kit__lock();
    click_inc = accent ? 2u * CLICK_INC_1K : CLICK_INC_1K;
    click_ph = 0;
    click_amp = level * 1600;                   /* level 10: about half of full scale */
    kit__unlock(k);
}
#define LFO_INC 262u                            /* 5.5 Hz: 65536 * 5.5 * KIT_BLOCK / KIT_FS */
/* 2^(k / 192) in Q15 for k = -48..48 (1/16 semitones: the bend, the vibrato and the tune together) */
static const uint16_t PITCH_MUL[97] = {
    27554, 27654, 27754, 27855, 27955, 28056, 28158, 28260, 28362, 28464, 28567, 28671, 28774, 28879, 28983, 29088,
    29193, 29299, 29405, 29511, 29618, 29725, 29832, 29940, 30048, 30157, 30266, 30376, 30485, 30596, 30706, 30817,
    30929, 31041, 31153, 31266, 31379, 31492, 31606, 31720, 31835, 31950, 32066, 32182, 32298, 32415, 32532, 32650,
    32768, 32887, 33005, 33125, 33245, 33365, 33486, 33607, 33728, 33850, 33973, 34095, 34219, 34343, 34467, 34591,
    34716, 34842, 34968, 35095, 35221, 35349, 35477, 35605, 35734, 35863, 35993, 36123, 36254, 36385, 36516, 36648,
    36781, 36914, 37047, 37181, 37316, 37451, 37586, 37722, 37859, 37996, 38133, 38271, 38409, 38548, 38688, 38828,
    38968 };

void kit_track_bend(int t, int value)
{
    int steps;
    if (!okt(t)) return;
    if (value < -8192) value = -8192;
    if (value > 8191) value = 8191;
    bend_raw[t] = (int16_t)value;
    steps = kit_track_bend_range_of(t) * 16;
    bend16[t] = (int16_t)(value >= 0 ? (value * steps + 4096) / 8191 : -((-value * steps + 4096) / 8192));
}

int kit_track_bend_range_of(int t)
{
    return okt(t) && bend_range[t] ? bend_range[t] : 2;
}

void kit_track_bend_range(int t, int semitones)
{
    uint32_t lock;
    if (!okt(t)) return;
    lock = kit__lock();
    bend_range[t] = (uint8_t)(semitones < 1 ? 1 : semitones > 12 ? 12 : semitones);
    kit_track_bend(t, bend_raw[t]);
    kit__unlock(lock);
}

void kit_track_mod(int t, int depth)
{
    if (okt(t)) mod_depth[t] = (uint8_t)(depth < 0 ? 0 : depth > 127 ? 127 : depth);
}

int kit__track_pitch(int t)                     /* the bend, and the vibrato at its phase now */
{
    int tri, p;
    if (!okt(t)) return 0;
    tri = lfo_ph[t] < 32768u ? (int)lfo_ph[t] : 65535 - (int)lfo_ph[t];   /* 0..32767 */
    p = bend16[t] + tune16 + (tri - 16384) * mod_depth[t] * 8 / (16384 * 127);
    return p;
}

static uint32_t pitch_scale_step(uint32_t inc, int p)
{
    uint32_t m = PITCH_MUL[p + 48];
    return (inc >> 15) * m + (((inc & 0x7FFFu) * m) >> 15);
}

static uint32_t pitch_scale(uint32_t inc, int p)
{
    /* Extended bends fold an octave first. The default range retains the original single multiply. */
    if (p > 96) { inc <<= 1; p -= 192; }
    if (p < -96) { inc >>= 1; p += 192; }
    if (p > 48) { inc = pitch_scale_step(inc, 48); p -= 48; }
    if (p < -48) { inc = pitch_scale_step(inc, -48); p += 48; }
    return pitch_scale_step(inc, p);
}
/* ---- glide and mono (API 27, ST-46, D-073) ---- */
/* (tr_vmode, tr_slide: declared above, with the tracks' sounds) */
static uint16_t tr_glide[KIT_NTRACKS];               /* ms, 0: none */
#define HELD_MAX 8
static uint8_t held[KIT_NTRACKS][HELD_MAX], nheld[KIT_NTRACKS];   /* the keys held, in order: mono goes back to them */
static uint8_t lastc[KIT_NTRACKS][HELD_MAX], nlastc[KIT_NTRACKS]; /* the chord before: a new chord glides from it */
static uint8_t curc[KIT_NTRACKS][HELD_MAX], ncurc[KIT_NTRACKS];   /* the chord being played */
static uint32_t synth_now, tr_last_on[KIT_NTRACKS];  /* samples rendered; the last note-on of each track */
#define CHORD_WINDOW (KIT_FS / 25)                   /* notes within 40 ms of each other are one chord */
static int new_chord(int t) { return !ncurc[t] || synth_now - tr_last_on[t] > CHORD_WINDOW; }

void kit_track_voice(int t, int mode, int glide_ms, int slide)
{
    if (!okt(t)) return;
    tr_vmode[t] = (uint8_t)(mode < 0 || mode > KIT_VMODE_LEGATO ? KIT_VMODE_POLY : mode);
    tr_glide[t] = (uint16_t)(glide_ms < 0 ? 0 : glide_ms > 2000 ? 2000 : glide_ms);
    tr_slide[t] = (uint8_t)(slide == KIT_SLIDE_LEGATO);
}
int kit_track_voice_mode(int t) { return okt(t) ? tr_vmode[t] : KIT_VMODE_POLY; }
int kit_track_glide_ms(int t) { return okt(t) ? tr_glide[t] : 0; }
int kit_track_slide(int t) { return okt(t) ? tr_slide[t] : KIT_SLIDE_ALWAYS; }

static void held_add(int t, int note)
{
    int i;
    if (new_chord(t)) {                              /* a new chord: the last one is what it glides from */
        for (i = 0; i < ncurc[t]; i++) lastc[t][i] = curc[t][i];
        nlastc[t] = ncurc[t];
        ncurc[t] = 0;
    }
    tr_last_on[t] = synth_now;
    for (i = 0; i < nheld[t]; i++) if (held[t][i] == note) return;
    if (nheld[t] == HELD_MAX) { for (i = 1; i < HELD_MAX; i++) held[t][i - 1] = held[t][i]; nheld[t]--; }
    held[t][nheld[t]++] = (uint8_t)note;
    if (ncurc[t] < HELD_MAX) curc[t][ncurc[t]++] = (uint8_t)note;
}
static void held_del(int t, int note)
{
    int i, j;
    for (i = 0; i < nheld[t]; i++)
        if (held[t][i] == note) {
            for (j = i + 1; j < nheld[t]; j++) held[t][j - 1] = held[t][j];
            nheld[t]--;
            return;
        }
}
/* where a new note glides from (a note, -1: no glide). MONO: the key held last, else (SLIDE ALWAYS) the note before.
 * POLY: each note of a chord from the nearest note of the chord before (SLIDE LEGATO: only while a key is held). */
static int glide_from(int t, int note)
{
    const uint8_t *c;
    int n, i, best = -1, bd = 999;
    if (!tr_glide[t] || tr_engine[t] == KIT_ENGINE_DRUMS) return -1;
    if (tr_slide[t] == KIT_SLIDE_LEGATO && !nheld[t]) return -1;
    if (tr_vmode[t] != KIT_VMODE_POLY) {
        best = nheld[t] ? held[t][nheld[t] - 1] : ncurc[t] ? curc[t][ncurc[t] - 1] : -1;
        return best == note ? -1 : best;
    }
    if (new_chord(t)) { c = curc[t]; n = ncurc[t]; }
    else { c = lastc[t]; n = nlastc[t]; }
    for (i = 0; i < n; i++) {
        int d = c[i] > note ? c[i] - note : note - c[i];
        if (c[i] != note && d < bd) { bd = d; best = c[i]; }
    }
    return best;
}
static int32_t gstep_of(int32_t diffq, int ms)    /* 1/4096 semitones a KIT_BLOCK so the slide takes ms (|diff| < 2^19) */
{
    int32_t d = diffq < 0 ? -diffq : diffq, per = (int32_t)ms * 44, s;
    if (ms <= 0 || !d) return 0;
    s = (d * KIT_BLOCK + per - 1) / per;
    return s < 1 ? 1 : s;
}
static voice_t *basic_mono(int t)                 /* the track's sounding Basic voice: gated first, then the newest */
{
    voice_t *v = 0;
    int i;
    for (i = 0; i < KIT_NVOICES; i++)
        if (vs[i].active && !vs[i].drum && vs[i].track == t && (!v || (vs[i].gate && !v->gate) || (vs[i].gate == v->gate && vs[i].age > v->age)))
            v = &vs[i];
    return v;
}

void kit_track_note_on(int t, int note, int vel)
{
    int from = -1, mono = 0, legato = 0;
    voice_t *v = 0;
    uint32_t k;
    int i;
    if (!okt(t) || note < 0 || note > 127)
        return;
    if (!tr_level[t]) return;                        /* API 26: the fader down, nothing starts */
    vel = vel * tr_level[t] / 10;
    if (vel < 1) vel = 1;
    if (vel > 127) vel = 127;
    sus_held[t][note >> 5] &= ~(1u << (note & 31));   /* played again: no longer waiting for the pedal */
    k = kit__lock();
    tr_live[t] = (uint8_t)!kit__bg;
    if (tr_engine[t] != KIT_ENGINE_DRUMS) {           /* API 27: where it glides from, and mono */
        from = glide_from(t, note);
        mono = tr_vmode[t] != KIT_VMODE_POLY;
        legato = tr_vmode[t] == KIT_VMODE_LEGATO && nheld[t] > 0;
        held_add(t, note);
    }
    if (is_part(t)) {
        /* A second BS2 key retunes the existing mono voice, including during release. */
        int reuse = tr_engine[t] == KIT_ENGINE_BS2 && voices_on(t);
        if (!reuse && !has_note(t, note) && !make_room(t)) { kit__unlock(k); return; }
        kit__eng_note_glide(t, note, vel, from >= 0 ? from * 16 : -1, tr_glide[t], mono, legato);
        kit__unlock(k);
        return;
    }
    if (mono && (v = basic_mono(t)) != 0) {           /* one voice: it takes the note; the others let go */
        for (i = 0; i < KIT_NVOICES; i++)
            if (&vs[i] != v && vs[i].active && !vs[i].drum && vs[i].track == t) { vs[i].gate = 0; vs[i].stage = 3; }
        v->note = (uint8_t)note;
        v->vel = (uint8_t)vel;
        v->gate = 1;
        v->age = ++ages;
        v->inc = KIT_NOTE_INC[note];
        if (!legato) v->stage = 1;                    /* MONO: a new attack from where the sound is */
        v->gstep = from >= 0 ? gstep_of((note << 12) - v->curq, tr_glide[t]) : 0;
        if (!v->gstep) v->curq = note << 12;
        kit__unlock(k);
        return;
    }
    v = 0;
    if (!has_note(t, note) && !make_room(t)) {
        kit__unlock(k);
        return;
    }
    for (i = 0; i < KIT_NVOICES && !v; i++)          /* the same note on this track: retrigger it */
        if (vs[i].active && vs[i].track == t && vs[i].note == note)
            v = &vs[i];
    for (i = 0; i < KIT_NVOICES && !v; i++)          /* a free voice (make_room made sure there is one) */
        if (!vs[i].active)
            v = &vs[i];
    if (!v) {
        kit__unlock(k);
        return;
    }
    if (!(v->active && v->note == note)) {
        v->ph = 0;
        v->env = 0;
        v->prev = 0;
    }
    v->note = (uint8_t)note;
    v->track = (uint8_t)t;
    v->vel = (uint8_t)vel;
    v->gate = 1;
    v->stage = 1;
    v->age = ++ages;
    v->noise = 0x9E3779B9u ^ ages;
    if (tr_engine[t] == KIT_ENGINE_DRUMS) {      /* the pad's sound: the track's kit, or the one put on it */
        int pad = kit_drum_pad_of(note), kit, vi = (int)(v - vs);
        if (!tr_pads_init) pads_init();
        kit = tr_pad[t][pad].kit < 0 ? tr_preset[t] : tr_pad[t][pad].kit;
        drum_start(vi, kit, tr_pad[t][pad].src, tr_pad[t][pad].tune, tr_pad[t][pad].decay);
        if (dv[vi].slot == SL_HATC)                 /* the closed hat chokes the open one */
            for (i = 0; i < KIT_NVOICES; i++)
                if (i != vi && vs[i].active && vs[i].drum && vs[i].track == t && dv[i].choke) vs[i].active = 0;
        v->drum = 1;
        v->env = ONE;
        v->stage = 2;
    } else {
        v->drum = 0;
        v->inst = (uint8_t)tr_preset[t];
        v->inc = KIT_NOTE_INC[note];
        v->curq = (from >= 0 ? from : note) << 12;     /* API 27: it glides from there */
        v->gstep = from >= 0 ? gstep_of((note << 12) - v->curq, tr_glide[t]) : 0;
        if (!v->gstep) v->curq = note << 12;
    }
    v->active = 1;
    kit__unlock(k);
}

void kit_track_note_off(int t, int note)
{
    uint32_t k;
    int i;
    if (!okt(t))
        return;
    if (sus_on[t] && note >= 0 && note < 128) {     /* the pedal is down: it waits */
        sus_held[t][note >> 5] |= 1u << (note & 31);
        return;
    }
    k = kit__lock();
    if (note >= 0 && note < 128 && tr_engine[t] != KIT_ENGINE_DRUMS) {
        held_del(t, note);
        if (tr_vmode[t] != KIT_VMODE_POLY && nheld[t]) {   /* mono: back to the key still held, no new attack */
            int back = held[t][nheld[t] - 1];
            voice_t *v = is_part(t) ? 0 : basic_mono(t);
            if (is_part(t) ? kit__eng_retarget(t, note, back, tr_glide[t]) : (v && v->gate && v->note == note)) {
                if (v) {
                    v->note = (uint8_t)back;
                    v->inc = KIT_NOTE_INC[back];
                    v->gstep = gstep_of((back << 12) - v->curq, tr_glide[t]);
                    if (!v->gstep) v->curq = back << 12;
                }
                kit__unlock(k);
                return;
            }
        }
    }
    kit__eng_note_off(t, note);
    for (i = 0; i < KIT_NVOICES; i++)
        if (vs[i].active && vs[i].gate && vs[i].track == t && vs[i].note == note) {
            vs[i].gate = 0;
            if (!vs[i].drum)                        /* a drum rings out */
                vs[i].stage = 3;
        }
    kit__unlock(k);
}

void kit_track_notes_off(int t)
{
    uint32_t k;
    int i;
    if (!okt(t))
        return;
    for (i = 0; i < 4; i++)
        sus_held[t][i] = 0;
    nheld[t] = 0;                                    /* API 27: nothing held any more */
    k = kit__lock();
    kit__eng_all_off(t);
    for (i = 0; i < KIT_NVOICES; i++)
        if (vs[i].active && vs[i].track == t) {
            vs[i].gate = 0;
            if (!vs[i].drum)
                vs[i].stage = 3;
        }
    kit__unlock(k);
}

void kit_note_on(int note, int vel) { kit_track_note_on(0, note, vel); }
void kit_note_off(int note) { kit_track_note_off(0, note); }
void kit_all_notes_off(void)
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++)
        kit_track_notes_off(t);
}

static inline int32_t sine(uint32_t ph)
{
    uint32_t i = ph >> 22, f = (ph >> 6) & 0xFFFFu;
    int32_t a = KIT_SINE[i], b = KIT_SINE[i + 1];
    return a + (((b - a) * (int32_t)f) >> 16);
}

/* x (Q30, >= 0) * m (Q16) without a 64-bit multiply (felucca-bsp docs/CPU.md: keep those out) */
static inline uint32_t qmul(int32_t x, uint32_t m) { return (((uint32_t)x >> 14) * m) >> 2; }

static int32_t env_next(voice_t *v, const inst_t *in, int inst)
{
    int32_t e = v->env;
    if (v->stage == 1) {
        e += attack_step[inst];
        if (e >= ONE) { e = ONE; v->stage = 2; }
    } else if (v->stage == 2) {
        if (e > in->sustain)
            e = in->sustain + (int32_t)(qmul(e - in->sustain, in->decay_mul));
        if (in->sustain == 0 && e < (ONE >> 14)) { e = 0; v->active = 0; }
    } else {
        e = (int32_t)qmul(e, in->release_mul);
        if (e < (ONE >> 14)) { e = 0; v->active = 0; }
    }
    return e;
}

void kit_track_sustain(int t, int on)
{
    int n;
    if (!okt(t) || sus_on[t] == (on != 0))
        return;
    sus_on[t] = (uint8_t)(on != 0);
    if (on)
        return;
    for (n = 0; n < 128; n++)                   /* the pedal lifts: what was let go stops now */
        if (sus_held[t][n >> 5] >> (n & 31) & 1u) {
            sus_held[t][n >> 5] &= ~(1u << (n & 31));
            kit_track_note_off(t, n);
        }
}

/* ---- the FX (D-055): a compressor worked out once a block, a soft clip from a table, one-pole filters ---- */
static int32_t log2q8(uint32_t x)               /* log2(x), Q8 (x >= 1); the mantissa linear: within 0.09 */
{
    int n = 0;
    uint32_t y = x;
    while (y >= 2) { y >>= 1; n++; }
    return (n << 8) + (int32_t)(((x << (31 - n)) >> 23) & 0xFFu);
}
static int32_t exp2neg_q15(int32_t v)          /* 2^(-v / 256), Q15 (v >= 0); the fraction linear */
{
    int k = v >> 8, f = v & 0xFF;
    if (k > 14) return 0;
    return (int32_t)((32768u >> k) * (uint32_t)(512 - f) >> 9);
}
static int32_t soft(int32_t x, int drive_q8, int32_t norm_q15)   /* x Q15: tanh(drive x) / tanh(drive), by the table */
{
    int32_t u = x * drive_q8 >> 8, idx, fr, a, b;  /* u: Q15, -2..2 is -65536..65536 */
    if (u > 65535) u = 65535;
    if (u < -65535) u = -65535;
    idx = (u + 65536) >> 9;                     /* 0..255 */
    fr = (u + 65536) & 511;
    a = SOFT[idx]; b = SOFT[idx + 1];
    return ((a + ((b - a) * fr >> 9)) * norm_q15) >> 15;
}
typedef struct { int16_t thr; uint8_t ratio, att, rel; uint16_t makeup; uint16_t drive, norm; uint16_t lp, hp; uint8_t crush; } fx_t;
/* threshold (of 32767), ratio, attack and release (Q8 a block), makeup (Q8), soft clip drive (Q8; 0 none) and its
 * 1/tanh(drive) (Q15), low-pass and high-pass (Q15 one-pole; 0 none), bits kept (0 all) */
static const fx_t FXS[KIT_NFX] = {               /* makeup: the lab's, and the browser compressor's own (0.6 of its reduction at 0 dB) */
    { 0, 0, 0, 0, 256, 0, 0, 0, 0, 0 },
    { 1642, 4, 15, 2, 1771, 0, 0, 0, 0, 0 },                /* Punch: -26 dB, 4:1, 12 ms, 120 ms; x1.8 and +11.7 dB */
    { 2603, 6, 51, 2, 1987, 410, 35470, 0, 0, 0 },          /* MPC: -22 dB, 6:1, 3 ms, 90 ms; x2.2 and +11 dB; tanh(1.6) */
    { 4125, 3, 18, 1, 819, 0, 0, 15500, 524, 6 },           /* Lofi: 24 steps, 4.6 kHz, 110 Hz, -18 dB 3:1; x1.4 and +7.2 dB */
    { 3277, 2, 9, 1, 765, 563, 33520, 24400, 0, 0 },        /* Tape: tanh(2.2), 9.5 kHz, -20 dB 2.5:1; x1.3 and +7.2 dB */
};
/* the FX work in the lab's units (Q15: a full hit, a snare at velocity 127, near 1.0 = 25200 in the mix), so its
 * thresholds and drives mean what they meant there, then go back */
#define FX_IN 332                               /* x 1.30, Q8 */
static void fx_block(int t, int32_t *mix)
{
    const fx_t *f = &FXS[tr_fx[t]];
    int32_t *x = fxbuf[t], peak = 0, g0 = fxs[t].gain ? fxs[t].gain : 32768, g1, dg, g;
    int j, fx = tr_fx[t];
    for (j = 0; j < KIT_BLOCK; j++) {
        int32_t a;
        x[j] = x[j] * FX_IN >> 8;               /* into the lab's units */
        a = x[j] < 0 ? -x[j] : x[j];
        if (a > peak) peak = a;
    }
    if (peak > fxs[t].env) fxs[t].env += (peak - fxs[t].env) * f->att >> 8;   /* the envelope: fast up, slow down */
    else fxs[t].env += (peak - fxs[t].env) * f->rel >> 8;
    g1 = 32768;
    if (f->thr && fxs[t].env > f->thr) {        /* above the threshold: down by (1 - 1/ratio) of the excess, in dB */
        int32_t over = log2q8((uint32_t)fxs[t].env) - log2q8((uint32_t)f->thr);
        g1 = exp2neg_q15(over * (f->ratio - 1) / f->ratio);
    }
    fxs[t].gain = g1;
    dg = (g1 - g0) / KIT_BLOCK;
    g = g0;
    for (j = 0; j < KIT_BLOCK; j++) {           /* the lab's order: Punch comp; MPC comp, clip; Lofi crush, band, comp; Tape clip, low-pass, comp */
        int32_t u = x[j];
        if (u > 131071) u = 131071;
        if (u < -131071) u = -131071;
        if (fx == KIT_FX_LOFI) u = ((u * 48 + 32768) >> 16) * 1365;   /* 24 steps a side (Math.round(x * 24) / 24) */
        if (fx == KIT_FX_TAPE) u = soft(u, f->drive, f->norm);
        if (f->lp) { fxs[t].lp += ((u - fxs[t].lp) * (int32_t)f->lp) >> 15; u = fxs[t].lp; }
        if (f->hp) { fxs[t].hp += ((u - fxs[t].hp) * (int32_t)f->hp) >> 15; u -= fxs[t].hp; }
        u = (u >> 2) * (g >> 2) >> 11;          /* the compressor's gain (Q15), within 32 bits */
        u = u * f->makeup >> 8;
        if (fx == KIT_FX_MPC) u = soft(u, f->drive, f->norm);
        if (fx == KIT_FX_LOFI && peak) {        /* vinyl: sparse crackle under the drums */
            uint32_t r = fxs[t].crackle * 1664525u + 1013904223u;
            fxs[t].crackle = r;
            if ((r >> 20) < 2) u += (int32_t)(r & 0x3FFF) - 8192;
        }
        if (u > 59600) u = 59600;               /* the lab's ceiling: its output (x 0.55) clipped at 1.0 */
        if (u < -59600) u = -59600;
        mix[j] += u * 197 >> 8;                 /* back into the mix's units (256 / FX_IN, without a division) */
        g += dg;
    }
}

void kit__synth_render(int16_t *out, int n)
{
    static int32_t mix[KIT_BLOCK], tb[KIT_BLOCK];
    static uint8_t routed[KIT_NTRACKS];
    int pot = kit__pot();
    int vol = volume < vol_limit ? volume : vol_limit;
    int32_t gain = (int32_t)vol * (pot < 0 ? 1023 : pot) / 100;   /* 0..1023 */
    int b, i, j;
    for (b = 0; b < n; b += KIT_BLOCK) {
        synth_now += KIT_BLOCK;
        for (j = 0; j < KIT_NTRACKS; j++)
            if (mod_depth[j]) lfo_ph[j] = (uint16_t)(lfo_ph[j] + LFO_INC);
        for (j = 0; j < KIT_BLOCK; j++)
            mix[j] = 0;
        kit__fx_begin();
        for (i = 0; i < KIT_NTRACKS; i++) {
            routed[i] = (uint8_t)(tr_fx[i] || kit__fx_sends(i));   /* through its own buffer: its FX, its sends (D-093) */
            if (routed[i])
                for (j = 0; j < KIT_BLOCK; j++) fxbuf[i][j] = 0;
        }
        for (i = 0; i < KIT_NVOICES; i++) {
            voice_t *v = &vs[i];
            int32_t e0, e1, de, e, amp;
            uint32_t ph, inc;
            if (!v->active)
                continue;
            e0 = v->env;
            amp = v->vel * 258;                            /* vel 127 -> ~32767 */
            ph = v->ph;
            inc = v->inc;
            if (v->drum) {                                 /* the drum kits (drums.c) */
                if (!drum_render(i, routed[v->track] ? fxbuf[v->track] : mix, amp)) v->active = 0;
                continue;
            }
            {
                const inst_t *in = &INSTS[v->inst];
                int32_t *dst = routed[v->track] ? fxbuf[v->track] : mix;
                int pb = kit__track_pitch(v->track);
                if (v->curq != v->note << 12) {      /* gliding (API 27): a step a block towards the note */
                    int32_t d = (v->note << 12) - v->curq, s = v->gstep ? v->gstep : 128 << 12;
                    v->curq = d > s ? v->curq + s : d < -s ? v->curq - s : v->note << 12;
                    inc = pitch_scale(KIT_NOTE_INC[v->curq >> 12], (v->curq >> 8) & 15);
                }
                if (pb) inc = pitch_scale(inc, pb);
                e1 = env_next(v, in, v->inst);
                v->env = e1;
                de = ((e1 >> 15) - (e0 >> 15)) / KIT_BLOCK;   /* Q15 ramp per sample */
                e = e0 >> 15;
                for (j = 0; j < KIT_BLOCK; j++) {
                    int32_t s = (in->partial[0] * sine(ph)) >> 15;
                    if (in->partial[1]) s += (in->partial[1] * sine(ph * 2u)) >> 15;
                    if (in->partial[2]) s += (in->partial[2] * sine(ph * 3u)) >> 15;
                    if (in->partial[3]) s += (in->partial[3] * sine(ph * 4u)) >> 15;
                    s = (s * e) >> 15;
                    dst[j] += (s * amp) >> 15;
                    ph += inc;
                    e += de;
                }
            }
            v->ph = ph;
        }
        for (i = 0; i < KIT_NTRACKS; i++) {          /* the tracks' FX, then into the mix (and the bus: D-093) */
            if (!routed[i]) continue;
            if (!kit__fx_sends(i)) { fx_block(i, mix); continue; }
            kit__eng_render_part(i, fxbuf[i], KIT_BLOCK);
            if (tr_fx[i]) {
                for (j = 0; j < KIT_BLOCK; j++) tb[j] = 0;
                fx_block(i, tb);
                kit__fx_track(i, tb, mix);
            } else
                kit__fx_track(i, fxbuf[i], mix);
        }
        if (click_amp) {                            /* the click, on its own: never on a track */
            for (j = 0; j < KIT_BLOCK; j++) {
                mix[j] += (sine(click_ph) * click_amp) >> 15;
                click_ph += click_inc;
            }
            click_amp = (int32_t)(((uint32_t)click_amp * CLICK_DECAY) >> 16);
            if (click_amp < 40) click_amp = 0;
        }
        kit__eng_render(mix, KIT_BLOCK);
        kit__fx_buses(mix);
        if (sfx_id != 0xFF) sfx_block(mix);
        {
            const int32_t *rs = kit__fx_rev_in();
            int sent = 0;
            for (j = 0; j < KIT_BLOCK && !sent; j++) sent = rs[j] != 0;
            if (sent && !rv_sent && !rv_level) rv_clear();     /* from silence, not an old tail */
            if (sent || (rv_sent && rv_tail)) {
                rv_block(mix, rs);
                rv_tail = sent ? (uint32_t)(rv_tail_ms + rv_shape.pre_ms) * 441u / 10u
                               : rv_tail > KIT_BLOCK ? rv_tail - KIT_BLOCK : 0;
            }
            else if (rv_level) rv_block(mix, 0);
            rv_sent = (uint8_t)(sent || rv_tail);
        }
        for (j = 0; j < KIT_BLOCK; j++) {
            int32_t s = (mix[j] >> 1) * gain >> 10;       /* headroom: two full voices before the clip */
            out[b + j] = (int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s);
        }
    }
}
