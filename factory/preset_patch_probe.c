/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test of the complete preset payload using a model of track controls. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../fm1kit/include/fm1kit.h"
#include "../apps/companion/presetcatalog.c"
#include "../apps/companion/presetosc.c"
#include "../apps/companion/presetenv.c"

enum { SED_NMOD = 7 };
typedef struct { uint8_t target[2]; int8_t amount[2]; } sed_mod_t;
static sed_mod_t sed_mod[KIT_NTRACKS];
static uint8_t sed_osc_edits[KIT_NTRACKS], sed_env_edits[KIT_NTRACKS], fx_mod[KIT_NTRACKS];
static int mod_applies, selections;

typedef struct {
    int engine, base, voice, glide, slide, bend, sends[4];
    kit_dirt_t dirt;
    kit_sound_filter_t filter;
    kit_sound_osc_t osc;
    uint8_t levels[2], envelopes[2][4];
    kit_sound_lfo_t lfo;
} track_t;

static track_t tracks[KIT_NTRACKS];
static const track_t factory_patch = {
    .bend = 2, .osc = {KIT_OSC_TRI, KIT_OSC_SINE, 71, 15},
    .levels = {50, 66}, .envelopes = {{4, 20, 34, 255}, {6, 10, 30, 255}}
};

int kit_engine_count(void) { return KIT_ENGINE_CF_VA + 1; }
int kit_preset_count(int e) { (void)e; return 4; }
int kit_track_engine(int t) { return tracks[t].engine; }
int kit_track_preset(int t) { return tracks[t].base; }
void kit_track_sound_local(int t, int e, int p) { tracks[t] = factory_patch; tracks[t].engine = e; tracks[t].base = p; ++selections; }
int kit_track_send_of(int t, int i) { return tracks[t].sends[i]; }
void kit_track_sends(int t, int a, int b, int c, int d)
{
    tracks[t].sends[0] = a; tracks[t].sends[1] = b;
    tracks[t].sends[2] = c; tracks[t].sends[3] = d;
}
void kit_track_dirt_of(int t, kit_dirt_t *d) { *d = tracks[t].dirt; }
void kit_track_dirt(int t, const kit_dirt_t *d) { tracks[t].dirt = *d; }
int kit_track_filter_of(int t, kit_sound_filter_t *f) { *f = tracks[t].filter; return 1; }
int kit_track_filter(int t, const kit_sound_filter_t *f) { tracks[t].filter = *f; return 1; }
int kit_track_voice_mode(int t) { return tracks[t].voice; }
int kit_track_glide_ms(int t) { return tracks[t].glide; }
int kit_track_slide(int t) { return tracks[t].slide; }
int kit_track_bend_range_of(int t) { return tracks[t].bend; }
void kit_track_voice(int t, int v, int g, int s) { tracks[t].voice = v; tracks[t].glide = g; tracks[t].slide = s; }
void kit_track_bend_range(int t, int b) { tracks[t].bend = b; }
int kit_track_lfo_of(int t, kit_sound_lfo_t *f) { *f = tracks[t].lfo; return 1; }
int kit_track_lfo(int t, const kit_sound_lfo_t *f) { tracks[t].lfo = *f; return 1; }
int kit_track_osc_of(int t, kit_sound_osc_t *o) { *o = tracks[t].osc; return tracks[t].engine == KIT_ENGINE_FMINI; }
int kit_track_osc_levels_of(int t, uint8_t l[2]) { memcpy(l, tracks[t].levels, 2); return 1; }
int kit_track_osc_levels(int t, const uint8_t l[2]) { memcpy(tracks[t].levels, l, 2); return 1; }
int kit_track_osc(int t, int control, int value)
{
    kit_sound_osc_t *o = &tracks[t].osc;
    if (control == KIT_OSC_WAVE1) o->wave1 = value;
    if (control == KIT_OSC_WAVE2) o->wave2 = value;
    if (control == KIT_OSC_DETUNE) o->detune = value;
    return 1;
}
int kit_track_envelope_of(int t, int kind, uint8_t values[4])
{
    memcpy(values, tracks[t].envelopes[kind], 4);
    if (values[KIT_ENV_RELEASE] == 255) values[KIT_ENV_RELEASE] = values[KIT_ENV_DECAY];
    return 1;
}
int kit_track_envelope(int t, int kind, int control, int value)
{
    tracks[t].envelopes[kind][control] = value;
    return 1;
}
static void sound_mod_apply(int t, int value) { assert(value == fx_mod[t]); ++mod_applies; }
static void sound_mod_reset(int t)
{
    sed_osc_edits[t] = sed_env_edits[t] = 0;
    sed_mod[t] = (sed_mod_t){0};
}

#include "../apps/companion/presetpatch.c"

int main(void)
{
    preset_record_t preset = {0}, malformed;
    track_t *edited = &tracks[0], other;
    unsigned n;
    kit_track_sound_local(0, KIT_ENGINE_FMINI, 3);
    edited->filter = (kit_sound_filter_t){80, 50, 29, 11};
    edited->lfo = (kit_sound_lfo_t){16, 53, KIT_LFO_CUTOFF, KIT_LFO_SAW};
    kit_track_voice(0, KIT_VMODE_LEGATO, 2000, KIT_SLIDE_LEGATO);
    kit_track_bend_range(0, 12);
    kit_track_sends(0, 2, 48, 58, 127);
    kit_track_dirt(0, &(kit_dirt_t){4, 30, 44});
    kit_track_osc(0, KIT_OSC_WAVE1, KIT_OSC_SQR);
    kit_track_osc(0, KIT_OSC_DETUNE, -700);
    kit_track_osc_levels(0, (uint8_t[]){100, 31});
    kit_track_envelope(0, KIT_ENV_FILTER, KIT_ENV_DECAY, 74);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_RELEASE, 92);
    sed_mod[0] = (sed_mod_t){{2, 6}, {-127, 127}};
    sed_osc_edits[0] = 13;
    sed_env_edits[0] = preset_env_bit(edited->engine, KIT_ENV_FILTER, KIT_ENV_DECAY) |
                       preset_env_bit(edited->engine, KIT_ENV_AMP, KIT_ENV_RELEASE);
    assert(preset_patch_capture(0, &preset));
    malformed = preset;
    for (n = 0; n < preset.data_len; ++n) {
        malformed.data_len = n;
        assert(!preset_patch_recall(1, &malformed));
    }
    malformed = preset;
    malformed.data[1] = 128;
    assert(!preset_patch_recall(1, &malformed) && selections == 1);
    other = tracks[0];
    assert(preset_patch_recall(1, &preset) && selections == 2 && mod_applies == 1);
    assert(!memcmp(&other, &tracks[0], sizeof other));
    assert(tracks[1].engine == KIT_ENGINE_FMINI && tracks[1].base == 3);
    assert(tracks[1].filter.cutoff == 80 && tracks[1].lfo.depth == 53);
    assert(tracks[1].voice == KIT_VMODE_LEGATO && tracks[1].glide == 2000 && tracks[1].bend == 12);
    assert(tracks[1].sends[3] == 127 && tracks[1].dirt.mix == 44);
    assert(tracks[1].osc.detune == -700 && tracks[1].osc.wave1 == KIT_OSC_SQR &&
           tracks[1].osc.wave2 == KIT_OSC_SINE && tracks[1].levels[0] == 100 && tracks[1].levels[1] == 31);
    assert(tracks[1].envelopes[0][1] == 74 && tracks[1].envelopes[0][3] == 255 &&
           tracks[1].envelopes[1][3] == 92);
    assert(sed_osc_edits[1] == 13 && sed_env_edits[1] == 130 && sed_mod[1].amount[0] == -127);

    sed_osc_edits[0] = 15;
    sed_env_edits[0] = 255;
    assert(preset_patch_capture(0, &preset) && preset.data_len == PRESET_DATA_MAX);
    assert(preset_patch_recall(1, &preset));
    kit_track_sound_local(0, KIT_ENGINE_JUNO, 1);
    sound_mod_reset(0);
    assert(preset_patch_capture(0, &preset) && preset_patch_recall(1, &preset));
    kit_track_sound_local(0, KIT_ENGINE_CF_VA, 0);
    assert(preset_patch_capture(0, &preset) && preset.data_len == 8 && preset_patch_recall(1, &preset));
    kit_track_sound_local(0, KIT_ENGINE_DRUMS, 0);
    assert(!preset_patch_capture(0, &preset));
    puts("PASS: track capture, recall, full payload, and invalid records rejected before selection.");
    return 0;
}
