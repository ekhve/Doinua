/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1-scales adapter for the CoralFlower VA voice renderer and preset bank. */
#include "va_voice.h"

typedef struct {
    int8_t patch[VA_NP];
    cf_va_part_t control;
    cf_va_voice_t voices[NVOICE];
    unsigned latest;
} kit_va_part_t;

static kit_va_part_t kit_va[NPART] __attribute__((section(".pool")));

static void kit_va_select(unsigned part_index, unsigned preset)
{
    kit_va_part_t *state = &kit_va[part_index];
    memset(state, 0, sizeof *state);
    cf_va_preset_patch(preset, state->patch);
}

static void kit_va_note_on(track_t *track, voice_t *voice)
{
    kit_va_part_t *state = &kit_va[track - trk];
    unsigned index = (unsigned)(voice - track->v);
    int first_held = 1;
    for (unsigned i = 0; i < NVOICE; ++i)
        first_held &= (i == index || !track->v[i].gate || !track->v[i].active);
    int fresh = !voice->env || cf_va_voice_done(&state->voices[index]);
    if (fresh) state->voices[index].oscillators.noise_seed = 0;
    cf_va_note_on(&state->control, &state->voices[index], voice->age, fresh, first_held);
    voice->env = 1;
    state->latest = index;
}

static void kit_va_block(track_t *track)
{
    kit_va_part_t *state = &kit_va[track - trk];
    /* All nine selected factory patches use free-running LFOs. */
    cf_va_part_tick(&state->control, state->patch, 120);
}

static int kit_va_done(track_t *track, voice_t *voice)
{
    return cf_va_voice_done(&kit_va[track - trk].voices[voice - track->v]);
}

static void kit_va_render(track_t *track, voice_t *voice, int32_t *out, uint32_t n, const vmod_t *m)
{
    kit_va_part_t *state = &kit_va[track - trk];
    unsigned index = (unsigned)(voice - track->v);
    cf_va_note_t note = {
        .index = (uint8_t)index, .key = voice->note, .velocity = voice->mvel,
        .wheel = (uint8_t)track->mw, .gate = voice->gate, .latest = index == state->latest,
        .random = (int16_t)(voice->age * 0x9e3779b9u >> 16)
    };
    cf_va_render(&state->control, &state->voices[index], state->patch, &note, m, out, 0, n);
}

static const engine_t ENG_CF_VA = {
    .name = "CoralFlower VA", .presets = VA_PRESETS, .npresets = CF_VA_PRESET_COUNT,
    .note_on = kit_va_note_on, .block = kit_va_block, .render = kit_va_render,
    .ownenv = 1, .done = kit_va_done,
};
