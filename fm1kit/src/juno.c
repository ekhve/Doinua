/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit's Juno-106: FM-106's engine (https://github.com/PatttF/fm-106 at 48ca1b6, GPL-3.0-only; fm1kit/src/juno/),
 * one instance a track that plays it, with the owner's 28 picks (juno/juno_bank.h) (ST-71b, D-090). FM-106 runs one
 * Juno on both cores with an effects chain; here each track's Juno renders its voices on the audio core, each voice
 * scaled by its velocity (the Juno has none; on the FM-1 the velocity also carries the track's level), then FM-106's
 * own HPF, VCA law and BBD chorus, its two sides summed to the mono mix. FM-106's arpeggiator, step sequencer and
 * insert effects are not used (ARP is the kit's, D-088; the effects bus is ST-69). Its own translation unit: the
 * engine's floats and names stay here. engines.c is the only caller (kit__juno_*), under the kit's lock. */
#include <stdint.h>
#include "kit_internal.h"

#define FM106_RAM_FUNC
#define FM106_NUM_VOICES 5u                     /* five a track (the owner: "at least 4 or 5", a Cm7 and its bass): a voice costs ~10 % of the CPU (D-090) */
#define FM106_LEAN                              /* no insert FX, Dimension, reverb, FM-106 audio ISR: the kit has its own bus (ST-69) */
#define JUNO_TRACKS KIT_NTRACKS
#define JUNO_KNEE 40000                         /* and above it 4:1, as FMini (engines.c FM_KNEE) */
#define JUNO_GAIN 12                            /* FM-106's output (its headroom is for six voices at 16 bits) to the kit's
                                                 * mix: one note as loud as the other engines' (D-090) */
static inline void fm1_irq_off(void) {}         /* (engines.c holds the kit's lock) */
static inline void fm1_irq_on(void) {}
static inline uint32_t fm1_ticks(void) { return 0; }
#include "juno/fm106_presets.h"
#include "juno/juno_bank.h"                     /* JUNO_N, JUNO_PICKS: tools/gen_juno.py */
#include "juno/fm106_engine.h"

static fm106_engine_t jn[JUNO_TRACKS] KIT_BIG;
static uint8_t jn_on[JUNO_TRACKS];             /* the track plays a Juno */
static uint16_t jn_quiet[JUNO_TRACKS];         /* samples since its last voice ended (the chorus's tail done: no work) */
static uint8_t jn_preset[JUNO_TRACKS];          /* the preset it plays */
static int32_t jn_tmp[256];

static int okj(int t) { return t >= 0 && t < JUNO_TRACKS; }

int kit__juno_npresets(void) { return JUNO_N; }
const char *kit__juno_preset_name(int p)
{
    static char nm[16];
    const char *s;
    int k = 0;
    if (p < 0 || p >= JUNO_N) return "?";
    s = JUNO_PICKS[p].name;
    if (s[0] && s[1] && s[2] && s[3] == ' ') s += 4;   /* "A11 Brass": the bank's number off */
    while (s[k] && k < 15) { nm[k] = s[k]; k++; }
    while (k > 0 && nm[k - 1] == ' ') k--;      /* and the spaces after ("Orchestral ") */
    nm[k] = 0;
    return nm;
}

void kit__juno_select(int t, int p)             /* p < 0: the track plays no Juno */
{
    if (!okj(t)) return;
    jn_on[t] = 0;
    if (p < 0 || p >= JUNO_N) return;
    fm106_engine_init(&jn[t]);
    fm106_load_preset(&jn[t], (uint32_t)p);
    jn_preset[t] = (uint8_t)p;
    jn_on[t] = 1;
}

int kit__juno_on(int t) { return okj(t) && jn_on[t]; }

enum { JN_ENV_ATTACK = 16 };                   /* D, S and R follow in the native table */

int kit__juno_envelope_of(int t, uint8_t values[4])
{
    int i;
    if (!values) return 0;
    for (i = 0; i < 4; i++) values[i] = fm106_param_get(&jn[t], JN_ENV_ATTACK + i);
    return 1;
}

int kit__juno_envelope(int t, int control, int value)
{
    uint32_t lock = kit__lock();
    fm106_param_set(&jn[t], JN_ENV_ATTACK + control, value);
    kit__unlock(lock);
    return 1;
}

int kit__juno_envelope_ms(int control, int dial)
{
    unsigned ticks = 0;
    if (control == KIT_ENV_ATTACK) {
        unsigned step = FM106_ENV_ATK[dial];
        if (step == FM106_ENV_MAX) return 0;    /* attack switches immediately to decay */
        ticks = (FM106_ENV_MAX + step - 1) / step;
    } else {
        unsigned level = FM106_ENV_MAX;
        /* Nominal time constant, measured with the native decay arithmetic. */
        while (level > 6027u) {
            level = fm106_adsr_calc_decay(level, FM106_ENV_DEC_REL[dial]);
            ticks++;
        }
    }
    return (ticks * 4234u + 500) / 1000;       /* firmware envelope tick, in microseconds */
}

int kit__juno_motion_of(int t, kit_sound_motion_t *motion)
{
    const fm106_voice_t *latest = 0;
    uint32_t lock;
    int i;
    if (!motion) return 0;
    lock = kit__lock();
    for (i = 0; i < FM106_NUM_VOICES; i++) {
        const fm106_voice_t *v = &jn[t].voice[i];
        if (v->adsr.stage != FM106_ENV_IDLE && (!latest || (int32_t)(v->age - latest->age) > 0)) latest = v;
    }
    if (latest) {
        uint8_t stage = latest->adsr.stage == FM106_ENV_RELEASE ? 3 :
                        latest->adsr.stage == FM106_ENV_ATTACK ? 1 : 2;
        uint8_t level = latest->adsr.env14 >> 7;
        *motion = (kit_sound_motion_t){stage, level, stage, level, 0};
    }
    kit__unlock(lock);
    return latest != 0;
}

/* Native FM-106 parameter IDs for EDIT FILTER. */
enum { JN_FILTER_CUTOFF = 10, JN_FILTER_RESONANCE = 11, JN_FILTER_ENV_AMOUNT = 12, JN_FILTER_KEY_TRACK = 14 };

int kit__juno_filter_of(int t, kit_sound_filter_t *filter)
{
    const fm106_engine_t *e;
    if (!kit__juno_on(t) || !filter) return 0;
    e = &jn[t];
    *filter = (kit_sound_filter_t){fm106_param_get(e, JN_FILTER_CUTOFF), fm106_param_get(e, JN_FILTER_RESONANCE),
                                   fm106_param_get(e, JN_FILTER_ENV_AMOUNT), fm106_param_get(e, JN_FILTER_KEY_TRACK)};
    return 1;
}

int kit__juno_filter(int t, const kit_sound_filter_t *filter)
{
    fm106_engine_t *e;
    uint32_t lock;
    if (!kit__juno_on(t) || !filter) return 0;
    lock = kit__lock();
    e = &jn[t];
    fm106_param_set(e, JN_FILTER_CUTOFF, filter->cutoff > 127 ? 127 : filter->cutoff);
    fm106_param_set(e, JN_FILTER_RESONANCE, filter->resonance > 127 ? 127 : filter->resonance);
    fm106_param_set(e, JN_FILTER_ENV_AMOUNT, filter->amount > 127 ? 127 : filter->amount);
    fm106_param_set(e, JN_FILTER_KEY_TRACK, filter->keytrack > 127 ? 127 : filter->keytrack);
    kit__unlock(lock);
    return 1;
}

unsigned kit__juno_cutoff_bin(int cutoff)
{
    unsigned g = FM106_VCF_G1[(unsigned)(cutoff < 0 ? 0 : cutoff > 127 ? 127 : cutoff) * 32];
    unsigned tangent, angle;
    if (g >= 9600) return 129u * 256;             /* at or above the spectrum's Nyquist frequency */
    /* Invert the ladder coefficient g = tan(w)/(1+tan(w)) for w <= pi/8. */
    tangent = g * 4096 / (32767 - g);
    angle = tangent * 4096 / (4096 + (tangent * tangent >> 12) * 28 / 100);
    return angle * 2608 >> 7;                    /* w in Q12 to spectrum bin in Q8 */
}

void kit__juno_note_on(int t, int note, int vel)
{
    if (!kit__juno_on(t) || note < 0 || note > 127) return;
    fm106_key_on(&jn[t], (uint32_t)note, (uint32_t)(vel < 1 ? 1 : vel));
}
void kit__juno_note_off(int t, int note)
{
    if (!kit__juno_on(t) || note < 0 || note > 127) return;
    fm106_key_off(&jn[t], (uint32_t)note);
}
void kit__juno_all_off(int t)
{
    uint32_t n;
    if (!kit__juno_on(t)) return;
    for (n = 0; n < 128u; n++) if (fm106_bit_get(jn[t].keys_down, n)) fm106_key_off(&jn[t], n);
    for (n = 0; n < FM106_NUM_VOICES; n++) if (jn[t].voice[n].gate) fm106_note_off(&jn[t], jn[t].voice[n].note);
}

static int jn_sounding(const fm106_voice_t *v) { return !(v->adsr.stage == FM106_ENV_IDLE && !v->gate && v->vca_slew <= 10); }

int kit__juno_nactive(int t)
{
    uint32_t i;
    int n = 0;
    if (!kit__juno_on(t)) return 0;
    for (i = 0; i < FM106_NUM_VOICES; i++) n += jn_sounding(&jn[t].voice[i]);
    return n;
}
int kit__juno_nreleasing(int t)
{
    uint32_t i;
    int n = 0;
    if (!kit__juno_on(t)) return 0;
    for (i = 0; i < FM106_NUM_VOICES; i++) n += jn_sounding(&jn[t].voice[i]) && !jn[t].voice[i].gate;
    return n;
}
int kit__juno_has(int t, int note)
{
    uint32_t i;
    if (!kit__juno_on(t)) return 0;
    for (i = 0; i < FM106_NUM_VOICES; i++) if (jn[t].voice[i].gate && jn[t].voice[i].note == note) return 1;
    return 0;
}
int kit__juno_steal(int t)                      /* silence one voice: a releasing one, else the oldest */
{
    fm106_voice_t *best = 0;
    uint32_t i;
    if (!kit__juno_on(t)) return 0;
    for (i = 0; i < FM106_NUM_VOICES; i++) {
        fm106_voice_t *v = &jn[t].voice[i];
        if (!jn_sounding(v)) continue;
        if (!best || (!v->gate && best->gate) || (v->gate == best->gate && v->age < best->age)) best = v;
    }
    if (!best) return 0;
    best->gate = 0;
    best->adsr.stage = FM106_ENV_IDLE;
    best->vca_slew = 0;
    return 1;
}

/* adds n samples (n <= 256) of track t's Juno into mix */
static void jn_render(int t, int32_t *mix, int n)
{
    fm106_engine_t *e;
    uint32_t i, vi, any = 0, lfo_ph, lfo_step, delay_samples, ramp_samples = 11025u;
    int keys;
    int capture = kit__spectrum_wants(t);
    float vca_scale;
    e = &jn[t];
    /* the LFO, as FM-106's audio render makes it (its delay and the quarter-second ramp) */
    lfo_step = (uint32_t)e->lfo_rate * 12000u;
    lfo_ph = e->lfo_ph;
    delay_samples = (uint32_t)e->lfo_delay * 520u;
    keys = fm106_bit_count(e->keys_down) > 0;
    for (vi = 0; vi < FM106_NUM_VOICES && !keys; vi++) if (e->voice[vi].gate) keys = 1;
    for (i = 0; i < (uint32_t)n; i++) {
        int32_t tri, raw;
        lfo_ph += lfo_step;
        tri = (int32_t)(lfo_ph >> 16) - 32768;
        if (tri < 0) tri = -tri;
        raw = (int16_t)((tri - 16384) << 1);
        if (e->lfo_delay > 0) {
            int32_t amp = 0;
            if (keys) {
                uint32_t tt = e->lfo_delay_timer;
                amp = tt < delay_samples ? 0 : tt < delay_samples + ramp_samples ? (int32_t)(((tt - delay_samples) * 194783u) >> 16) : 32767;
                if (e->lfo_delay_timer < 0xFFFFFFFFu) e->lfo_delay_timer++;
            }
            g_lfo_buf[i] = (int16_t)((raw * amp) >> 15);
        } else
            g_lfo_buf[i] = (int16_t)raw;
    }
    e->lfo_ph = lfo_ph;
    e->lfo_val = g_lfo_buf[n - 1];
    for (vi = 0; vi < FM106_NUM_VOICES && !any; vi++) any = jn_sounding(&e->voice[vi]);
    if (!any) {                                 /* silent: once the chorus's and HPF's tails are out (~0.1 s), nothing to do */
        if (jn_quiet[t] >= 4096u) {
            if (capture) kit__spectrum_capture(0, n);
            return;
        }
        jn_quiet[t] = (uint16_t)(jn_quiet[t] + (uint32_t)n);
    } else
        jn_quiet[t] = 0;
    any = 0;
    for (i = 0; i < (uint32_t)n; i++) g_mix0[i] = 0;
    for (vi = 0; vi < FM106_NUM_VOICES; vi++) {  /* each voice at its velocity */
        fm106_voice_t *v = &e->voice[vi];
        int32_t vel;
        if (!jn_sounding(v)) { v->vca_slew = 0; continue; }
        fm106_render_voice_ex(e, vi, jn_tmp, (uint32_t)n, 0);
        vel = v->vel ? v->vel : 100;
        for (i = 0; i < (uint32_t)n; i++) g_mix0[i] += (jn_tmp[i] >> 7) * vel;
        any = 1;
    }
    vca_scale = (float)FM106_VCA_LAW[e->vca_level > 127u ? 127u : e->vca_level] * (1.0f / 4096.0f);
    for (i = 0; i < (uint32_t)n; i++) {         /* FM-106's post: headroom, HPF, VCA, the BBD chorus (mono here) */
        int32_t sum = any ? (g_mix0[i] * 3) >> 6 : 0, h, a, sl, sr, o;
        h = fm106_hpf_process(&e->hpf, sum);
        a = (int32_t)((float)h * vca_scale);
        fm106_chorus_process(&e->chorus, a, &sl, &sr);
        o = ((sl >> 1) + (sr >> 1)) * JUNO_GAIN;
        if (o > JUNO_KNEE) o = JUNO_KNEE + ((o - JUNO_KNEE) >> 2);           /* FMini's 4:1 knee (D-086) */
        else if (o < -JUNO_KNEE) o = -JUNO_KNEE + ((o + JUNO_KNEE) >> 2);
        mix[i] += o;
        if (capture) jn_tmp[i] = o;
    }
    if (capture) kit__spectrum_capture(jn_tmp, n);
}

/* adds n samples of track t's Juno into mix, in FM-106's blocks of at most 256 */
void kit__juno_render(int t, int32_t *mix, int n)
{
    if (!kit__juno_on(t)) return;
    for (; n > 0; n -= 256, mix += 256) jn_render(t, mix, n < 256 ? n : 256);
}

/* the Juno's sends to the kit's bus (D-093): FM-106's reverb, when the preset has it on (p[45]), as the reverb send at
 * its MIX (p[46]); its chorus is its own (above) */
void kit__juno_sends(int t, uint8_t sends[4])
{
    const fm106_preset_t *p;
    int i;
    for (i = 0; i < 4; i++) sends[i] = 0;
    if (!kit__juno_on(t) || jn_preset[t] >= JUNO_N) return;
    p = &JUNO_PICKS[jn_preset[t]];
    if (p->p[45]) sends[3] = (uint8_t)(p->p[46] > 127 ? 127 : p->p[46]);
}
