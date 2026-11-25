/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit engines: a host for the six Felucca-Engines (MIT: ANALOG, PHASE, LOFI, TRIO, WHEEL, NOISE), one part
 * per track, up to KIT_NTRACKS parts (D-029); the kit's allocator (synth.c) keeps the voices of all parts within
 * KIT_NVOICES. Its own translation unit: the engines' tables (SINE, FS, CTL, ...) stay out of the
 * rest of the kit. The host's side of the contract is felucca_engine.h's; tests/fe_host.c there is the model.
 * Each preset's A D S R (0..127) drive a block-rate envelope (KIT_ENV_STEP / KIT_ENV_MUL, tools/gen_tables.py).
 * Integer only, no 64-bit multiplies (felucca-bsp docs/CPU.md). */
#include <stdint.h>
#include "kit_internal.h"
#ifdef __PI32V2__
void *memset(void *d, int c, unsigned n);       /* felucca-bsp chip/libc.c (no C library on the device) */
void *memcpy(void *d, const void *s, unsigned n);
#else
#include <string.h>
#endif

#define NVOICE 8                    /* a part (track) at most; the kit allocator keeps all parts within KIT_NVOICES */
#define NPART 4                     /* == KIT_NTRACKS */
#include "fe_tables.h"              /* generated: felucca-engines tools/gen_engine_tables.py */
#include "felucca_engine.h"
int kit__track_pitch(int track);               /* synth.c: the track's bend and vibrato, 1/16 semitones */

enum { P_ATK, P_REL, P_FTYPE, P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7, P_COUNT };
typedef struct track {
    int16_t p[P_COUNT];
    voice_t v[NVOICE];
    int32_t lfo_val, lfo_fade, mw;  /* the part's LFO this block (Q15), its fade-in (Q15), the mod wheel (FMINI reads them) */
    uint32_t lfo_ph;
} track_t;
static track_t trk[NPART];

#include "dsp.c"
#include "eng_analog.c"
#include "eng_phase.c"
#include "eng_lofi.c"
#include "eng_trio.c"
#include "eng_wheel.c"
#include "eng_noise.c"

#include "kit_tables.h"             /* KIT_ENV_STEP, KIT_ENV_MUL; FM_ENV_LIN, FM_ENV_EXP, FM_LFO_INC, FM_LEVEL_Q12 (FMINI) */
#define GQ 8                                    /* glide: pitch_cur's extra fraction bits */
#include "fmini/eng_fmini.c"                    /* fmini-1's Minimoog engine (GPL-3.0-only, D-086) */

#include "coralflower/eng_va.c"

#define ENV_ONE (1 << 24)

static const struct { const engine_t *e; const char *name; } ENGS[] = {
    { &ENG_ANALOG, "Analog" }, { &ENG_PHASE, "Phase" }, { &ENG_LOFI, "Lo-fi" },
    { &ENG_TRIO, "Trio" }, { &ENG_WHEEL, "Organ wheel" }, { &ENG_NOISE, "Noise" },
    { &ENG_FMINI, "FMini" },                    /* the kit's engine 8 (synth.c eng_of): API 32, D-086 */
};
#define ENG_I_FMINI 6
/* the Juno-106 (juno.c, D-090): host engine NENGS (the kit's 9), one FM-106 a track: these route to it */
#include "bs2_hooks.h"
#include "obervave_hooks.h"
#define ENG_I_CF_VA (NENGS + 3)
#define ENG_I_OBV (NENGS + 2)
#define ENG_I_BS2 (NENGS + 1)
#define ENG_I_JUNO NENGS
int kit__juno_npresets(void);
const char *kit__juno_preset_name(int p);
void kit__juno_select(int t, int p);
int kit__juno_on(int t);
void kit__juno_note_on(int t, int note, int vel);
void kit__juno_note_off(int t, int note);
void kit__juno_all_off(int t);
int kit__juno_nactive(int t);
int kit__juno_nreleasing(int t);
int kit__juno_has(int t, int note);
int kit__juno_steal(int t);
void kit__juno_render(int t, int32_t *mix, int n);
void kit__juno_sends(int t, uint8_t sends[4]);
int kit__fx_sends(int track);                    /* fxbus.c: the track sends to the bus (D-093) */
#define FM_KNEE 40000                           /* the mix's level the other engines' loudest presets reach (49000 at most) */
#define NENGS ((int)(sizeof(ENGS) / sizeof(ENGS[0])))

typedef struct {                    /* a part's sound: its engine, preset and envelope */
    const engine_t *cur;
    const preset_t *pre;
    uint32_t atk_step, dec_mul, rel_mul;
    int32_t sus_level;
    int32_t last_amp[NVOICE];
} part_t;
static part_t part[NPART];
static uint32_t vage;
/* FMINI's own host (fmini-1's voice.c, its FMini-only build, which the owner auditioned): its patch's extras (the LFO,
 * LFO -> pitch / amplitude, contour -> pitch, LEVEL, the keys' octave) and its amplitude envelope: the Minimoog's
 * capacitor attack, decay to the sustain, the release at the DECAY's time (a factory patch has no own), fmini-1's
 * times (1 ms .. 12 s). Velocity as the other engines (fmini-1's default is off; here it also carries the level) */
static const fmini_x_t *fmx[NPART];             /* the part's extras while it plays FMINI, else 0 */
static int okp(int pt) { return pt >= 0 && pt < NPART; }

int kit_track_filter_of(int pt, kit_sound_filter_t *filter)
{
    const int16_t *p;
    if (!okp(pt) || !fmx[pt] || !filter) return 0;
    p = trk[pt].p;
    *filter = (kit_sound_filter_t){p[P_E0], p[P_E1], p[P_E2], fmini_patch[pt][FG_KBD]};
    return 1;
}

int kit_track_filter(int pt, const kit_sound_filter_t *filter)
{
    uint32_t k;
    int16_t *p;
    if (!okp(pt) || !fmx[pt] || !filter) return 0;
    k = kit__lock();
    p = trk[pt].p;
    p[P_E0] = clamp(filter->cutoff, 0, 127);
    p[P_E1] = clamp(filter->resonance, 0, 127);
    p[P_E2] = clamp(filter->amount, 0, 127);
    fmini_patch[pt][FG_KBD] = clamp(filter->keytrack, 0, 127);
    kit__unlock(k);
    return 1;
}
static int32_t pitch_of(int pt, int note)          /* 1/16 semitones: FMINI's patch may move the keys an octave */
{
    int n = note + (fmx[pt] ? fmx[pt]->oct * 12 : 0);
    return (n < 0 ? 0 : n > 127 ? 127 : n) * 16;
}

int kit__eng_count(void) { return NENGS + 4; }   /* Juno-106, BS2, OberVave and CoralFlower VA */
int kit__eng_nactive(int pt)                        /* voices sounding (or releasing) on part pt */
{
    if (okp(pt) && kit__obv_on(pt)) return kit__obv_nactive(pt);
    if (okp(pt) && kit__bs2_on(pt)) return kit__bs2_nactive(pt);
    if (okp(pt) && kit__juno_on(pt)) return kit__juno_nactive(pt);
    uint32_t i;
    int n = 0;
    if (!okp(pt) || !part[pt].cur)
        return 0;
    for (i = 0; i < NVOICE; i++)
        n += trk[pt].v[i].active != 0;
    return n;
}
int kit__eng_active(void)
{
    int pt;
    for (pt = 0; pt < NPART; pt++)
        if (kit__eng_nactive(pt))
            return 1;
    return 0;
}
int kit__eng_has(int pt, int note)                  /* note sounding (gate on) on part pt */
{
    if (okp(pt) && kit__obv_on(pt)) return kit__obv_has(pt, note);
    if (okp(pt) && kit__bs2_on(pt)) return kit__bs2_has(pt, note);
    if (okp(pt) && kit__juno_on(pt)) return kit__juno_has(pt, note);
    uint32_t i;
    if (!okp(pt))
        return 0;
    for (i = 0; i < NVOICE; i++)
        if (trk[pt].v[i].active && trk[pt].v[i].gate && trk[pt].v[i].note == note)
            return 1;
    return 0;
}
int kit__eng_steal(int pt)                          /* silence one voice of part pt: a releasing one, else the oldest */
{
    if (okp(pt) && kit__obv_on(pt)) return kit__obv_steal(pt);
    if (okp(pt) && kit__bs2_on(pt)) return kit__bs2_steal(pt);
    if (okp(pt) && kit__juno_on(pt)) return kit__juno_steal(pt);
    voice_t *best = 0;
    uint32_t i;
    if (!okp(pt))
        return 0;
    for (i = 0; i < NVOICE; i++) {
        voice_t *v = &trk[pt].v[i];
        if (!v->active)
            continue;
        if (!best || (v->gate == 0 && best->gate != 0) || (v->gate == best->gate && v->age < best->age))
            best = v;
    }
    if (!best)
        return 0;
    best->active = 0;
    best->stage = 0;
    return 1;
}
int kit__eng_nreleasing(int pt)
{
    if (okp(pt) && kit__obv_on(pt)) return kit__obv_nreleasing(pt);
    if (okp(pt) && kit__bs2_on(pt)) return kit__bs2_nreleasing(pt);
    if (okp(pt) && kit__juno_on(pt)) return kit__juno_nreleasing(pt);
    uint32_t i;
    int n = 0;
    if (!okp(pt))
        return 0;
    for (i = 0; i < NVOICE; i++)
        n += trk[pt].v[i].active && !trk[pt].v[i].gate;
    return n;
}
static const engine_t *engine_of(int i)
{
    if (i == ENG_I_CF_VA) return &ENG_CF_VA;
    return i >= 0 && i < NENGS ? ENGS[i].e : 0;
}

static const preset_t *engine_preset(int i, int p)
{
    const engine_t *engine = engine_of(i);
    return engine && p >= 0 && p < engine->npresets ? &engine->presets[p] : 0;
}

int kit__eng_preset_mono(int i, int p)
{
    const preset_t *preset = engine_preset(i, p);
    return preset ? preset->mono : 0;
}

const char *kit__eng_name(int i)
{
    if (i == ENG_I_CF_VA) return ENG_CF_VA.name;
    if (i == ENG_I_OBV) return "OberVave";
    if (i == ENG_I_BS2) return "Bass Something 2";
    if (i == ENG_I_JUNO) return "Juno-106";
    return i >= 0 && i < NENGS ? ENGS[i].name : "?";
}

int kit__eng_npresets(int i)
{
    if (i == ENG_I_OBV) return kit__obv_npresets();
    if (i == ENG_I_BS2) return kit__bs2_npresets();
    if (i == ENG_I_JUNO) return kit__juno_npresets();
    const engine_t *engine = engine_of(i);
    return engine ? engine->npresets : 0;
}

const char *kit__eng_preset_name(int i, int p)
{
    if (i == ENG_I_OBV) return kit__obv_preset_name(p);
    if (i == ENG_I_BS2) return kit__bs2_preset_name(p);
    if (i == ENG_I_JUNO) return kit__juno_preset_name(p);
    const preset_t *preset = engine_preset(i, p);
    return preset ? preset->name : "?";
}

int kit_preset_arp(int engine, int preset, kit_preset_arp_t *arp)
{
    /* fmini-1 at 649a8bb: seq.c AM_* and fx.c div_samples(). */
    static const uint8_t modes[] = {255, KIT_PRESET_ARP_UP, KIT_PRESET_ARP_DOWN,
        KIT_PRESET_ARP_UP_DOWN, KIT_PRESET_ARP_RANDOM, KIT_PRESET_ARP_AS_PLAYED,
        KIT_PRESET_ARP_REPEAT, 255, KIT_PRESET_ARP_UP_OCTAVE, 255, KIT_PRESET_ARP_DIVERGE};
    static const uint8_t ticks[] = {24, 12, 6, 3, 8, 4, 48, 96, 192, 0, 32, 16};
    if (engine == KIT_ENGINE_OBERVAVE) return kit__obv_preset_arp(preset, arp);
    if (engine != KIT_ENGINE_FMINI || (unsigned)preset >= FMB_N || !arp) return 0;
    const uint8_t *p = FMB_X[preset].arp;
    if (!p[0] || p[1] >= sizeof modes || modes[p[1]] == 255 ||
        p[2] >= sizeof ticks || !ticks[p[2]] || !p[3] || p[3] > 4)
        return 0;
    *arp = (kit_preset_arp_t){modes[p[1]], ticks[p[2]], p[3], p[4], p[5], p[6]};
    return 1;
}

/* the caller holds the kit's lock */
void kit__eng_select(int pt, int i, int p)          /* i < 0: the part plays no engine */
{
    track_t *t;
    part_t *P;
    uint32_t k;
    if (!okp(pt))
        return;
    kit__obv_select(pt, i == ENG_I_OBV ? p : -1);
    kit__bs2_select(pt, i == ENG_I_BS2 ? p : -1);
    kit__juno_select(pt, i == ENG_I_JUNO ? p : -1);   /* the Juno (or none) */
    t = &trk[pt];
    P = &part[pt];
    memset(t, 0, sizeof *t);
    memset(&drw_t[pt], 0, sizeof drw_t[pt]);       /* WHEEL's own state for this part: as from power-on */
    memset(drw_v[pt], 0, sizeof drw_v[pt]);
    memset(P, 0, sizeof *P);
    fmx[pt] = 0;
    P->pre = engine_preset(i, p);
    if (!P->pre) return;
    P->cur = engine_of(i);
    if (i == ENG_I_CF_VA) kit_va_select((unsigned)pt, (unsigned)p);
    if (i == ENG_I_FMINI) {                         /* its patch and extras */
        memcpy(fmini_patch[pt], FMB_PATCH[p], FG_SIZE);
        fmx[pt] = &FMB_X[p];
        t->lfo_fade = 32767;
    }
    for (k = 0; k < 8u; k++)
        t->p[P_E0 + k] = P->pre->e[k];
    t->p[P_ATK] = P->pre->env[0];
    t->p[P_REL] = P->pre->env[3];
    t->p[P_FTYPE] = 0;
    P->atk_step = KIT_ENV_STEP[P->pre->env[0] & 127u];
    P->dec_mul = KIT_ENV_MUL[P->pre->env[1] & 127u];
    P->sus_level = (int32_t)((P->pre->env[2] & 127u) * (uint32_t)(ENV_ONE / 127));
    P->rel_mul = KIT_ENV_MUL[P->pre->env[3] & 127u];
}

void kit__eng_note_on(int pt, int note, int vel)
{
    if (okp(pt) && kit__obv_on(pt)) { kit__obv_note_on(pt, note, vel); return; }
    if (okp(pt) && kit__bs2_on(pt)) { kit__bs2_note_on(pt, note, vel); return; }
    if (okp(pt) && kit__juno_on(pt)) { kit__juno_note_on(pt, note, vel); return; }
    track_t *t;
    part_t *P;
    voice_t *v = 0;
    uint32_t i, ph0, ph1, ph2;
    int32_t s[8];
    int sounding;
    if (!okp(pt) || !part[pt].cur)
        return;
    t = &trk[pt];
    P = &part[pt];
    for (i = 0; i < NVOICE && !v; i++)              /* the same note again: retrigger it */
        if (t->v[i].active && t->v[i].note == note)
            v = &t->v[i];
    if (!v && P->cur->poly && kit__eng_nactive(pt) >= P->cur->poly)
        kit__eng_steal(pt);                         /* the engine's own cap (FMINI: 4, as Felucca's multi-engine build) */
    for (i = 0; i < NVOICE && !v; i++)
        if (!t->v[i].active)
            v = &t->v[i];
    if (!v) {                                       /* steal the oldest */
        v = &t->v[0];
        for (i = 1; i < NVOICE; i++)
            if (t->v[i].age < v->age)
                v = &t->v[i];
    }
    sounding = v->active && v->stage != 0 && v->note == note;
    ph0 = v->ph[0]; ph1 = v->ph[1]; ph2 = v->ph[2];
    memcpy(s, v->s, sizeof s);
    if (!sounding) {
        memset(v, 0, sizeof *v);
        P->last_amp[v - t->v] = 0;
    }
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->mvel = (uint8_t)vel;
    v->gate = 1;
    v->active = 1;
    v->stage = 1;
    v->age = ++vage;
    v->pitch16 = pitch_of(pt, note);
    v->pitch_cur = v->pitch16 << GQ;
    v->fine = 0;
    if (fmx[pt] && !sounding && kit__eng_nactive(pt) <= 1)
        t->lfo_fade = fmx[pt]->lfo[2] ? 0 : 32767;   /* FMINI: the LFO fades in from the first note */
    P->cur->note_on(t, v);
    if (sounding) {                                 /* a retrigger keeps the phases and the keep slots */
        v->ph[0] = ph0; v->ph[1] = ph1; v->ph[2] = ph2;
        for (i = 0; i < 8u; i++)
            if (i != 2u && i != 3u && (P->cur->keep >> i & 1u))
                v->s[i] = s[i];
    }
}

/* glide (API 27, ST-46): here pitch_cur is in 1/16 semitone << GQ (the engines never read it) and moves to
 * pitch16 << GQ by gstep a CTL block, so the slide takes glide_ms whatever its size, even 2 s for a semitone
 * (44 samples a millisecond; no 64-bit multiply: |diff| < 2^19, CTL 32) */
static int32_t glide_step(int32_t diffq, int glide_ms)
{
    int32_t d = diffq < 0 ? -diffq : diffq, per = (int32_t)glide_ms * 44, s;
    if (glide_ms <= 0 || d == 0) return 0;
    s = (d * CTL + per - 1) / per;
    return s < 1 ? 1 : s;
}

static voice_t *mono_voice(track_t *t)          /* the part's sounding voice: gated first, then the newest */
{
    voice_t *v = 0;
    uint32_t i;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && (!v || (t->v[i].gate && !v->gate) || (t->v[i].gate == v->gate && t->v[i].age > v->age)))
            v = &t->v[i];
    return v;
}

int kit__eng_note_glide(int pt, int note, int vel, int from16, int glide_ms, int mono, int legato)
{
    if (okp(pt) && kit__obv_on(pt)) { kit__obv_note_on(pt, note, vel); return 1; }
    if (okp(pt) && kit__bs2_on(pt)) { kit__bs2_note_on(pt, note, vel); return 1; }
    if (okp(pt) && kit__juno_on(pt)) { kit__juno_note_on(pt, note, vel); return 1; }   /* (the Juno glides and stacks its own way) */
    track_t *t;
    voice_t *v;
    uint32_t i;
    if (!okp(pt) || !part[pt].cur)
        return 0;
    t = &trk[pt];
    v = mono ? mono_voice(t) : 0;
    if (mono && v) {                               /* one voice: it takes the new note */
        int32_t cur = v->pitch_cur;
        for (i = 0; i < NVOICE; i++)               /* any other voice of the part lets go */
            if (&t->v[i] != v && t->v[i].active) { t->v[i].gate = 0; t->v[i].stage = 3; }
        v->note = (uint8_t)note;
        v->gate = 1;
        v->active = 1;
        v->age = ++vage;
        if (!legato) {                             /* MONO: a new attack on the same voice, its phases kept */
            part_t *P = &part[pt];
            uint32_t ph0 = v->ph[0], ph1 = v->ph[1], ph2 = v->ph[2];
            int32_t ks[8];
            memcpy(ks, v->s, sizeof ks);
            v->vel = (uint8_t)vel;
            v->mvel = (uint8_t)vel;
            v->stage = 1;
            v->pitch16 = pitch_of(pt, note);
            P->cur->note_on(t, v);
            v->ph[0] = ph0; v->ph[1] = ph1; v->ph[2] = ph2;
            for (i = 0; i < 8u; i++)
                if (i != 2u && i != 3u && (P->cur->keep >> i & 1u))
                    v->s[i] = ks[i];
        }
        v->pitch16 = pitch_of(pt, note);
        v->pitch_cur = from16 >= 0 ? cur : v->pitch16 << GQ;
        v->gstep = glide_step((v->pitch16 << GQ) - v->pitch_cur, glide_ms);
        if (!v->gstep) v->pitch_cur = v->pitch16 << GQ;
        return 1;
    }
    kit__eng_note_on(pt, note, vel);
    for (i = 0; i < NVOICE; i++)                   /* the voice just started: glide it from the given pitch */
        if (t->v[i].active && t->v[i].gate && t->v[i].note == note && t->v[i].age == vage) {
            voice_t *w = &t->v[i];
            if (from16 >= 0 && glide_ms > 0) {
                w->pitch_cur = from16 << GQ;
                w->gstep = glide_step((w->pitch16 << GQ) - w->pitch_cur, glide_ms);
                if (!w->gstep) w->pitch_cur = w->pitch16 << GQ;
            }
            break;
        }
    return 1;
}

int kit__eng_retarget(int pt, int from_note, int note, int glide_ms)
{
    if (okp(pt) && kit__obv_on(pt)) return 0;
    if (okp(pt) && kit__bs2_on(pt)) return 0;
    if (okp(pt) && kit__juno_on(pt)) return 0;
    voice_t *v;
    if (!okp(pt) || !part[pt].cur)
        return 0;
    v = mono_voice(&trk[pt]);
    if (!v || !v->gate || v->note != from_note)
        return 0;
    v->note = (uint8_t)note;
    v->pitch16 = pitch_of(pt, note);
    v->gstep = glide_step((v->pitch16 << GQ) - v->pitch_cur, glide_ms);
    if (!v->gstep) v->pitch_cur = v->pitch16 << GQ;
    return 1;
}

void kit__eng_note_off(int pt, int note)
{
    if (okp(pt) && kit__obv_on(pt)) { kit__obv_note_off(pt, note); return; }
    if (okp(pt) && kit__bs2_on(pt)) { kit__bs2_note_off(pt, note); return; }
    if (okp(pt) && kit__juno_on(pt)) { kit__juno_note_off(pt, note); return; }
    uint32_t i;
    if (!okp(pt))
        return;
    for (i = 0; i < NVOICE; i++) {
        voice_t *v = &trk[pt].v[i];
        if (v->active && v->gate && v->note == note) {
            v->gate = 0;
            v->stage = 3;
        }
    }
}

void kit__eng_all_off(int pt)
{
    if (okp(pt) && kit__obv_on(pt)) { kit__obv_all_off(pt); return; }
    if (okp(pt) && kit__bs2_on(pt)) { kit__bs2_all_off(pt); return; }
    if (okp(pt) && kit__juno_on(pt)) { kit__juno_all_off(pt); return; }
    uint32_t i;
    if (!okp(pt))
        return;
    for (i = 0; i < NVOICE; i++) {
        voice_t *v = &trk[pt].v[i];
        if (v->active) {
            v->gate = 0;
            v->stage = 3;
        }
    }
}

static inline int32_t env_mul(int32_t x, uint32_t m)   /* x (Q24, >= 0) * m (Q16) */
{
    return (int32_t)((((uint32_t)x >> 8) * m) >> 8);
}

static int32_t env_tick_fmini(const part_t *P, voice_t *v)   /* FMINI's (fmini-1 voice.c, FMini-only build) */
{
    const uint8_t *e = P->pre->env;
    int32_t sus = (int32_t)(e[2] & 127) << 17;
    switch (v->stage) {
    case 1:
        {   /* the capacitor charges toward a level above full: a concave rise ending in a corner (no 64-bit multiply) */
            int32_t d = fm_charge(((5 << 22) - v->env) >> 1, (int32_t)(FM_ENV_LIN[e[0] & 127] >> 1)) << 1;
            v->env += d < 1 ? 1 : d;
        }
        if (v->env >= ENV_ONE) { v->env = ENV_ONE; v->stage = 2; }
        break;
    case 2:
        v->env += mulq16(sus - v->env, FM_ENV_EXP[e[1] & 127]);
        break;
    case 3:
        v->env -= mulq16(v->env, FM_ENV_EXP[e[1] & 127]);   /* the release: the DECAY's time */
        if (v->env < (1 << 12)) { v->env = 0; v->stage = 0; v->active = 0; }
        break;
    default:
        break;
    }
    v->env_out = v->env >> 9;
    return v->env >> 9;
}

static int32_t env_tick(const part_t *P, voice_t *v)  /* once a block; returns Q15 */
{
    switch (v->stage) {
    case 1:
        v->env += (int32_t)P->atk_step;
        if (v->env >= ENV_ONE) { v->env = ENV_ONE; v->stage = 2; }
        break;
    case 2:
        if (v->env > P->sus_level)
            v->env = P->sus_level + env_mul(v->env - P->sus_level, P->dec_mul);
        if (P->sus_level == 0 && v->env < ENV_ONE / 1000) { v->env = 0; v->stage = 0; v->active = 0; }
        break;
    case 3:
        v->env = env_mul(v->env, P->rel_mul);
        if (v->env < ENV_ONE / 1000) { v->env = 0; v->stage = 0; v->active = 0; }
        break;
    default:
        break;
    }
    v->env_out = v->env >> 9;
    return v->env >> 9;
}

/* adds n samples (n a multiple of CTL) of part pt into mix (Q15) */
static void render_part(int pt, int32_t *mix, int n)
{
    track_t *t = &trk[pt];
    part_t *P = &part[pt];
    const engine_t *cur = P->cur;
    int32_t out[CTL];
    int b, i, k, pb = kit__track_pitch(pt);     /* the track's bend and vibrato (synth.c) */
    if (!cur)
        return;
    for (b = 0; b < n; b += CTL) {
        const fmini_x_t *x = fmx[pt];
        int32_t lfo = 0;
        if (x) {                                    /* FMINI: the part's LFO, a block at a time (fmini-1 voice.c) */
            uint32_t ph;
            t->lfo_ph += FM_LFO_INC[x->lfo[0] & 127];
            ph = t->lfo_ph;
            t->lfo_val = x->lfo[1] == 1 ? osc_tri(ph) : x->lfo[1] == 2 ? (int32_t)(ph >> 16) - 32768
                       : x->lfo[1] == 3 ? (ph < 0x80000000u ? 32767 : -32767) : osc_sine(ph);
            if (t->lfo_fade < 32767) {
                int32_t st = (int32_t)(FM_ENV_LIN[x->lfo[2] & 127] >> 9);
                t->lfo_fade = x->lfo[2] ? clamp(t->lfo_fade + (st ? st : 1), 0, 32767) : 32767;
            }
            lfo = mulq15(t->lfo_val, t->lfo_fade);
        }
        if (cur->block)
            cur->block(t);
        memset(out, 0, sizeof out);
        for (k = 0; k < NVOICE; k++) {
            voice_t *v = &t->v[k];
            vmod_t m;
            int32_t a;
            uint32_t base;
            if (!v->active)
                continue;
            if (cur->ownenv && cur->done && cur->done(t, v)) {
                v->active = 0;
                v->stage = 0;
                continue;
            }
            a = cur->ownenv ? 32767 : x ? env_tick_fmini(P, v) : env_tick(P, v);
            if (!cur->ownenv) a = (a * v->vel) / 127;                 /* (FMINI too: on the FM-1 the velocity also carries the track's level) */
            if (x && x->mod[1])                     /* LFO -> amplitude */
                a = mulq15(a, 32767 - mulq15((lfo + 32768) >> 1, x->mod[1] * 258));
            if (cur->amp)
                a = cur->amp(t, v, a);
            if (!v->active)
                continue;
            if (v->pitch_cur != v->pitch16 << GQ) {  /* gliding (API 27): a step a block towards the note */
                int32_t d = (v->pitch16 << GQ) - v->pitch_cur, s = v->gstep ? v->gstep : 2047 << GQ;
                v->pitch_cur = d > s ? v->pitch_cur + s : d < -s ? v->pitch_cur - s : v->pitch16 << GQ;
            }
            base = pitch_inc((uint32_t)clamp((v->pitch_cur >> GQ) + pb, 0, 2047));
            memset(&m, 0, sizeof m);
            m.pitch16 = (v->pitch_cur >> GQ) + pb;
            if (x && (x->mod[0] || x->mod[2])) {    /* LFO -> pitch, contour -> pitch */
                m.pitch16 = clamp(m.pitch16 + ((lfo * x->mod[0] * 3) >> 15) + ((a * x->mod[2] * 3) >> 15), 0, 2047);
                base = pitch_inc((uint32_t)m.pitch16);
            }
            m.inc = base;
            m.amp0 = P->last_amp[k];
            m.amp1 = a;
            m.envq15 = cur->ownenv ? 0 : a;
            m.cutoff = 0;
            m.shape = 64 << 8;
            P->last_amp[k] = a;
            cur->render(t, v, out, CTL, &m);
        }
        if (cur->post)
            cur->post(t, out, CTL);
        if (x) {                                    /* FMINI: the patch's LEVEL (112 = 0 dB, half a dB a step), and its rare
                                                     * peaks above the other engines' (a few patches reach twice: MINI BASS)
                                                     * taken down 4:1 over FM_KNEE, so one note never clips the mix */
            int32_t lv = FM_LEVEL_Q12[x->level & 127];
            for (i = 0; i < CTL; i++) {
                int32_t o = ((out[i] >> 4) * lv) >> 8;
                if (o > FM_KNEE) o = FM_KNEE + ((o - FM_KNEE) >> 2);
                else if (o < -FM_KNEE) o = -FM_KNEE + ((o + FM_KNEE) >> 2);
                mix[b + i] += o;
            }
        } else if (cur == &ENG_CF_VA) {
            for (i = 0; i < CTL; i++) {
                int32_t value = out[i];
                if (value > FM_KNEE) value = FM_KNEE + ((value - FM_KNEE) >> 2);
                else if (value < -FM_KNEE) value = -FM_KNEE + ((value + FM_KNEE) >> 2);
                mix[b + i] += value;
            }
        } else
            for (i = 0; i < CTL; i++)
                mix[b + i] += out[i];
    }
}

void kit__eng_render_part(int pt, int32_t *buf, int n)
{
    if (!okp(pt)) return;
    if (kit__obv_on(pt)) kit__obv_render(pt, buf, n);
    else if (kit__bs2_on(pt)) kit__bs2_render(pt, buf, n);
    else if (kit__juno_on(pt)) kit__juno_render(pt, buf, n);       /* D-090 */
    else render_part(pt, buf, n);
}

void kit__eng_render(int32_t *mix, int n)        /* the parts without sends (synth.c takes the others, D-093) */
{
    int pt;
    for (pt = 0; pt < NPART; pt++)
        if (!kit__fx_sends(pt)) kit__eng_render_part(pt, mix, n);
}

/* the part's preset's sends (DRIVE CHORUS DELAY REVERB; stored + 1, 0 the default: fmini-1's FX_DEF) and, for an
 * FMini patch, its twelve bus settings (D-093). The Juno: its reverb on (preset p[45]) sends its MIX (p[46]) */
int kit__eng_fx(int pt, uint8_t sends[4], uint8_t bus12[12])
{
    static const uint8_t FX_DEF[4] = { 0, 24, 28, 36 };
    int k;
    for (k = 0; k < 4; k++) sends[k] = 0;
    if (!okp(pt)) return 0;
    if (kit__obv_on(pt)) { kit__obv_sends(pt, sends); return 0; }
    if (kit__juno_on(pt)) { kit__juno_sends(pt, sends); return 0; }
    if (!part[pt].pre) return 0;                 /* every engine's sound plays its preset's sends (the owner, D-094) */
    for (k = 0; k < 4; k++) sends[k] = part[pt].pre->fx[k] ? (uint8_t)(part[pt].pre->fx[k] - 1u) : FX_DEF[k];
    if (!fmx[pt]) return 0;
    for (k = 0; k < 12; k++) bus12[k] = fmx[pt]->fx[k];
    return 1;
}
