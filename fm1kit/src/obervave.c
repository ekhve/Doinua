/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave a1f660c voice allocator and mono mix adapter for fm1kit. */
#include "kit_internal.h"
#include "obervave_hooks.h"
#include "obervave/voice.h"
#include "obervave/bank.h"
#ifdef __PI32V2__
void *memset(void *dst, int value, unsigned bytes);
#else
#include <string.h>
#endif


#define OBV_VOICES 8
/* Drone Uni at eight unison voices reaches 81% in Hello's mixed-track sweep. */
#define OBV_UNISON_MAX 6
#define OBV_PRESETS (sizeof obervave_bank / sizeof obervave_bank[0])
enum { OBV_POLY, OBV_DUAL, OBV_UNISON, OBV_MONO };

typedef struct {
    obv_voice_t voice;
    uint32_t age;
    uint8_t track;
} slot_t;

typedef struct {
    const uint8_t *params;
    obv_part_t part;
    uint32_t random;
    uint8_t held[128], velocity[128], count;
    int32_t last_pitch;
    int32_t samples[OBV_CONTROL_FRAMES];
    unsigned cursor;
} obv_track_t;

static slot_t voices[OBV_VOICES];
static obv_track_t tracks[KIT_NTRACKS];
static uint32_t next_age;

int kit__obv_npresets(void) { return (int)OBV_PRESETS; }
const char *kit__obv_preset_name(int preset)
{
    return (unsigned)preset < OBV_PRESETS ? obervave_bank[preset].name : "?";
}

static int alive(const slot_t *slot) { return slot->voice.ae.stage != E_IDLE; }
int kit__obv_on(int track)
{
    return (unsigned)track < KIT_NTRACKS && tracks[track].params;
}

void kit__obv_select(int track, int preset)
{
    if ((unsigned)track >= KIT_NTRACKS) return;
    for (unsigned i = 0; i < OBV_VOICES; ++i)
        if (voices[i].track == track) memset(&voices[i], 0, sizeof voices[i]);
    obv_track_t *state = &tracks[track];
    memset(state, 0, sizeof *state);
    state->cursor = OBV_CONTROL_FRAMES;
    state->random = 0x9e3779b9u ^ (uint32_t)track;
    state->part.lfo_rnd_a = xorshift(&state->random);
    state->part.lfo_rnd_b = xorshift(&state->random);
    if ((unsigned)preset < OBV_PRESETS) state->params = obervave_bank[preset].param;
}

static slot_t *oldest(int track, uint32_t first_new_age)
{
    slot_t *best = 0;
    for (unsigned i = 0; i < OBV_VOICES; ++i) {
        slot_t *slot = &voices[i];
        if (track >= 0 && (slot->track != track || !alive(slot))) continue;
        if (!alive(slot)) return slot;
        if (slot->age >= first_new_age) continue;
        if (!best || slot->voice.gate < best->voice.gate ||
            (slot->voice.gate == best->voice.gate && slot->age < best->age)) best = slot;
    }
    return best;
}

int kit__obv_steal(int track)
{
    slot_t *slot = oldest(track, UINT32_MAX);
    if (!slot) return 0;
    memset(slot, 0, sizeof *slot);
    return 1;
}

static void trigger(int track, int note, int velocity)
{
    static const int8_t pan_card[OBV_VOICES] = {-40, 40, -24, 24, -56, 56, -10, 10};
    obv_track_t *state = &tracks[track];
    const uint8_t *p = state->params;
    unsigned mode = p[P_MODE];
    int count = mode == OBV_UNISON ? clampi(p[P_UNI_N], 1, OBV_UNISON_MAX) :
                mode == OBV_DUAL ? 2 : 1;
    unsigned reused = 0;
    uint32_t first_new_age = next_age + 1;
    int spread = p[P_SPREAD], detune = (p[P_UNI_DET] * p[P_UNI_DET]) >> 6;
    int had_gate = 0;
    if (mode >= OBV_UNISON) {
        for (unsigned i = 0; i < OBV_VOICES; ++i) {
            slot_t *slot = &voices[i];
            if (slot->track != track || !slot->voice.gate) continue;
            obv_voice_legato(&slot->voice, p, (uint8_t)note, slot->voice.det8,
                             count > 1 ? ((128 * (int)reused / (count - 1) - 64) * spread) >> 7 : 0);
            ++reused;
        }
        had_gate = reused != 0;
    } else {
        for (unsigned i = 0; i < OBV_VOICES; ++i)
            had_gate |= voices[i].track == track && voices[i].voice.gate;
    }
    for (int i = (int)reused; i < count; ++i) {
        slot_t *slot = oldest(-1, first_new_age);
        if (!slot) break;
        int det = mode == OBV_DUAL ? (i ? detune / 2 : -detune / 2) :
                  count > 1 ? 2 * detune * i / (count - 1) - detune : 0;
        int pan = mode == OBV_DUAL ? (i ? 48 : -48) :
                  count > 1 ? 128 * i / (count - 1) - 64 :
                  mode == OBV_POLY ? pan_card[slot - voices] : 0;
        memset(slot, 0, sizeof *slot);
        slot->track = (uint8_t)track;
        slot->age = ++next_age;
        slot->voice.render.noise = xorshift(&state->random);
        obv_voice_start(&slot->voice, p, (uint8_t)note, (uint8_t)velocity, state->last_pitch,
                        det, (pan * spread) >> 7);
    }
    state->last_pitch = note << 16;
    if (!had_gate && p[P_LFO_PHASE] == LP_SYNC) state->part.lfo_ph = 0;
}

void kit__obv_note_on(int track, int note, int velocity)
{
    if (!kit__obv_on(track) || (unsigned)note >= 128) return;
    obv_track_t *state = &tracks[track];
    for (unsigned i = 0; i < state->count; ++i) {
        if (state->held[i] != note) continue;
        --state->count;
        for (; i < state->count; ++i) state->held[i] = state->held[i + 1];
        break;
    }
    state->held[state->count++] = (uint8_t)note;
    state->velocity[note] = (uint8_t)clampi(velocity, 1, 127);
    trigger(track, note, state->velocity[note]);
}

void kit__obv_note_off(int track, int note)
{
    if (!kit__obv_on(track) || (unsigned)note >= 128) return;
    obv_track_t *state = &tracks[track];
    unsigned last = state->count && state->held[state->count - 1] == note;
    for (unsigned i = 0; i < state->count; ++i) {
        if (state->held[i] != note) continue;
        --state->count;
        for (; i < state->count; ++i) state->held[i] = state->held[i + 1];
        break;
    }
    if (state->params[P_MODE] >= OBV_UNISON && last && state->count) {
        int next = state->held[state->count - 1];
        trigger(track, next, state->velocity[next]);
        return;
    }
    for (unsigned i = 0; i < OBV_VOICES; ++i)
        if (voices[i].track == track && voices[i].voice.note == note) voices[i].voice.gate = 0;
}

void kit__obv_all_off(int track)
{
    if (!kit__obv_on(track)) return;
    tracks[track].count = 0;
    for (unsigned i = 0; i < OBV_VOICES; ++i)
        if (voices[i].track == track) voices[i].voice.gate = 0;
}

static int count_voices(int track, int note, int releasing)
{
    int count = 0;
    for (unsigned i = 0; i < OBV_VOICES; ++i) {
        const slot_t *slot = &voices[i];
        count += slot->track == track && alive(slot) &&
                 (note < 0 || slot->voice.note == note) && (!releasing || !slot->voice.gate);
    }
    return count;
}

int kit__obv_nactive(int track) { return count_voices(track, -1, 0); }
int kit__obv_nreleasing(int track) { return count_voices(track, -1, 1); }
int kit__obv_has(int track, int note) { return count_voices(track, note, 0) != 0; }

void kit__obv_render(int track, int32_t *mix, int count)
{
    if (!kit__obv_on(track)) return;
    obv_track_t *state = &tracks[track];
    while (count-- > 0) {
        if (state->cursor == OBV_CONTROL_FRAMES) {
            int32_t stereo[2 * OBV_CONTROL_FRAMES] = {0};
            obv_part_tick(&state->part, state->params, &state->random, 0);
            for (unsigned i = 0; i < OBV_VOICES; ++i) {
                slot_t *slot = &voices[i];
                if (slot->track != track || !alive(slot)) continue;
                obv_voice_block(&slot->voice, &state->part, state->params, i, 0);
                obv_voice_render(&slot->voice.render, &state->part.render, stereo);
            }
            for (unsigned i = 0; i < OBV_CONTROL_FRAMES; ++i)
                /* Sum the stereo pair to match the kit's mono playback level. */
                state->samples[i] = stereo[2 * i] + stereo[2 * i + 1];
            state->cursor = 0;
        }
        *mix++ += state->samples[state->cursor++];
    }
}


/* The filter stage already applies OberVave's drive. Feed its three wet
 * levels into Doinua's existing shared chorus, delay and reverb returns. */
void kit__obv_sends(int track, uint8_t sends[4])
{
    if (!kit__obv_on(track)) return;
    const uint8_t *params = tracks[track].params;
    sends[0] = 0;
    sends[1] = params[P_CHO_MIX];
    sends[2] = params[P_DLY_MIX];
    sends[3] = params[P_REV_MIX];
}

int kit__obv_preset_arp(int preset, kit_preset_arp_t *arp)
{
    static const uint8_t modes[] = {0, 0, 1, 2, 4, 3};
    static const uint8_t ticks[] = {24, 12, 8, 6, 4, 3, 2};
    if ((unsigned)preset >= OBV_PRESETS || !arp) return 0;
    const uint8_t *p = obervave_bank[preset].param;
    if (!p[P_ARP_MODE] || p[P_ARP_MODE] >= sizeof modes || p[P_ARP_DIV] >= sizeof ticks) return 0;
    *arp = (kit_preset_arp_t){modes[p[P_ARP_MODE]], ticks[p[P_ARP_DIV]], p[P_ARP_OCT],
                             (uint8_t)(p[P_ARP_GATE] + 1u), 0, 127};
    return 1;
}
