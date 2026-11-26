/* SPDX-License-Identifier: GPL-3.0-only */
/* synth.c's tracks (D-029, API 8), with a model of the engine parts:
 *   - four tracks sound at once, within KIT_NVOICES voices and the CPU budget (D-047)
 *   - with no room, a note takes a releasing voice, else the oldest of another track with two or more (the song's
 *     before the live ones), else its own oldest; a track never loses its last note; each counts as stolen
 *   - a note sounding again takes no new voice; changing one track's sound leaves the others sounding
 *   - the drum kit sounds, rings out after note-off and falls silent by itself */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

/* the backend */
uint32_t kit__lock(void) { return 0; }
void kit__unlock(uint32_t k) { (void)k; }
int kit__pot(void) { return 1023; }

/* a model of engines.c: 8 slots a part, note numbers, gates and ages */
static struct { int note, gate, age, on; } ev[4][8];
static int engsel[4] = { -1, -1, -1, -1 }, eage;
int kit__eng_count(void) { return 6; }
const char *kit__eng_name(int i) { (void)i; return "E"; }
int kit__eng_npresets(int i) { (void)i; return 4; }
const char *kit__eng_preset_name(int i, int p) { (void)i; (void)p; return "P"; }
void kit__eng_select(int pt, int i, int p) { (void)p; engsel[pt] = i; memset(ev[pt], 0, sizeof ev[pt]); }
void kit__eng_note_on(int pt, int note, int vel)
{
    int i, f = -1;
    (void)vel;
    for (i = 0; i < 8; i++) if (ev[pt][i].on && ev[pt][i].note == note) f = i;
    for (i = 0; i < 8 && f < 0; i++) if (!ev[pt][i].on) f = i;
    if (f < 0) { printf("FAIL: the model's part %d overflowed\n", pt); return; }
    ev[pt][f].on = 1; ev[pt][f].gate = 1; ev[pt][f].note = note; ev[pt][f].age = ++eage;
}
int kit__eng_note_glide(int pt, int note, int vel, int from16, int glide_ms, int mono, int legato)   /* API 27: the model plays it plainly */
{ (void)from16; (void)glide_ms; (void)mono; (void)legato; kit__eng_note_on(pt, note, vel); return 1; }
int kit__eng_retarget(int pt, int from_note, int note, int glide_ms) { (void)pt; (void)from_note; (void)note; (void)glide_ms; return 0; }
void kit__eng_note_off(int pt, int note) { int i; for (i = 0; i < 8; i++) if (ev[pt][i].on && ev[pt][i].note == note) ev[pt][i].gate = 0; }
void kit__eng_all_off(int pt) { int i; for (i = 0; i < 8; i++) ev[pt][i].gate = 0; }
void kit__eng_render(int32_t *mix, int n) { (void)mix; (void)n; }
int kit__eng_nactive(int pt) { int i, n = 0; if (engsel[pt] < 0) return 0; for (i = 0; i < 8; i++) n += ev[pt][i].on; return n; }
int kit__eng_active(void) { int p; for (p = 0; p < 4; p++) if (kit__eng_nactive(p)) return 1; return 0; }
int kit__eng_has(int pt, int note) { int i; for (i = 0; i < 8; i++) if (ev[pt][i].on && ev[pt][i].gate && ev[pt][i].note == note) return 1; return 0; }
int kit__eng_nreleasing(int pt) { int i, n = 0; for (i = 0; i < 8; i++) n += ev[pt][i].on && !ev[pt][i].gate; return n; }
int kit__eng_preset_mono(int e, int p) { (void)e; (void)p; return 0; }   /* (API 32: only FMINI's presets bring a voice mode) */
static int engine_has_shared_fx;
int kit__eng_fx(int p, uint8_t s[4], uint8_t b[12])
{
    (void)p;
    s[0] = s[1] = s[2] = s[3] = 0;
    if (!engine_has_shared_fx) return 0;
    s[0] = 21; s[1] = 32; s[2] = 43; s[3] = 54;
    memset(b, 0, 12);
    b[1] = 45;
    b[4] = 52;
    b[7] = 11;
    return 1;
}
void kit__eng_render_part(int p, int32_t *b, int n) { (void)p; (void)b; (void)n; }
int kit__eng_steal(int pt)
{
    int i, b = -1;
    for (i = 0; i < 8; i++)
        if (ev[pt][i].on && (b < 0 || (!ev[pt][i].gate && ev[pt][b].gate) || (ev[pt][i].gate == ev[pt][b].gate && ev[pt][i].age < ev[pt][b].age)))
            b = i;
    if (b < 0) return 0;
    ev[pt][b].on = 0;
    return 1;
}

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define V(t) kit__track_voices(t)
static int total(void) { int t, n = 0; for (t = 0; t < KIT_NTRACKS; t++) n += V(t); return n; }
static int16_t buf[KIT_BLOCK * 64];
static long energy(int blocks)
{
    long e = 0;
    int i;
    kit__synth_render(buf, KIT_BLOCK * blocks);
    for (i = 0; i < KIT_BLOCK * blocks; i++) e += buf[i] < 0 ? -buf[i] : buf[i];
    return e;
}

int main(void)
{
    int i;
    kit__synth_init();
    kit_track_sound(KIT_TRACK_CHORDS, KIT_ENGINE_BASIC, KIT_INST_ORGAN);
    kit_track_sound(KIT_TRACK_BASS, 1, 0);                       /* an engine part */
    kit_track_sound(KIT_TRACK_MELODY, KIT_ENGINE_BASIC, KIT_INST_SINE);
    CHECK(kit_track_engine(KIT_TRACK_DRUMS) == KIT_ENGINE_DRUMS, "the drums track starts on the drum kit");
    CHECK(kit_engine_count() == 8 && kit_engine_name(KIT_ENGINE_DRUMS)[0] == 'D', "Drums is the last engine (%d)", kit_engine_count());

    /* the owner's example (D-047): chords with an arpeggio's note, plain chords, four melody keys, a bass, a hat */
    for (i = 0; i < 5; i++) kit_track_note_on(KIT_TRACK_CHORDS, 60 + i * 3, 100);
    kit_track_note_on(KIT_TRACK_BASS, 36, 100);
    for (i = 0; i < 4; i++) kit_track_note_on(KIT_TRACK_MELODY, 72 + i, 100);
    kit_track_note_on(KIT_TRACK_DRUMS, 42, 100);
    CHECK(V(0) == 5 && V(1) == 1 && V(2) == 4 && V(3) == 1 && kit_voices_stolen() == 0, "eleven voices at once, nothing taken (%d %d %d %d)", V(0), V(1), V(2), V(3));
    CHECK(kit_voices_used() == 11 && kit_voices_load() < KIT_CPU_LIMIT, "eleven used, the load %d %% under the limit", kit_voices_load());
    energy(2);
    CHECK(kit__synth_active(), "and sounding");
    kit_track_note_on(KIT_TRACK_MELODY, 76, 100);
    CHECK(total() == 12 && V(2) == 5 && kit_voices_stolen() == 0, "the twelfth fits");
    kit_track_note_on(KIT_TRACK_MELODY, 77, 100);
    CHECK(total() == 12 && V(0) == 4 && V(2) == 6 && kit_voices_stolen() == 1, "the thirteenth takes the oldest of another track with most voices (%d %d)", V(0), V(2));
    CHECK(!kit__track_sounding(KIT_TRACK_CHORDS, 60) && kit__track_sounding(KIT_TRACK_CHORDS, 63), "the oldest chord note went");
    kit_track_note_on(KIT_TRACK_MELODY, 76, 110);
    CHECK(total() == 12 && kit_voices_stolen() == 1, "a note sounding again takes no new voice");
    CHECK(V(1) == 1 && V(3) == 1, "the bass and the hat keep their only note");

    kit_all_notes_off();
    for (i = 0; i < 300 && kit__synth_active(); i++) energy(8);
    for (i = 0; i < 8; i++) ev[1][i].on = 0;                     /* (the model's part has no envelope) */
    CHECK(total() == 0, "all released (%d left)", total());

    /* the song's notes give way before the ones played live */
    kit__bg = 1;
    for (i = 0; i < 4; i++) kit_track_note_on(KIT_TRACK_CHORDS, 60 + i, 100);   /* the song plays the chords */
    kit__bg = 0;
    for (i = 0; i < 6; i++) kit_track_note_on(KIT_TRACK_MELODY, 72 + i, 100);   /* you play six melody keys */
    kit_track_note_on(KIT_TRACK_DRUMS, 42, 100);
    kit_track_note_on(KIT_TRACK_DRUMS, 44, 100);
    kit_track_note_on(KIT_TRACK_BASS, 36, 100);
    CHECK(total() == 12 && V(0) == 3 && V(2) == 6, "a bass note takes one of the song's chord notes, not a live melody key (%d %d)", V(0), V(2));
    kit_all_notes_off();
    for (i = 0; i < 300 && kit__synth_active(); i++) energy(8);
    for (i = 0; i < 8; i++) ev[1][i].on = 0;

    /* a releasing voice goes first */
    for (i = 0; i < 6; i++) kit_track_note_on(KIT_TRACK_CHORDS, 60 + i, 100);
    for (i = 0; i < 6; i++) kit_track_note_on(KIT_TRACK_MELODY, 72 + i, 100);
    kit_track_note_off(KIT_TRACK_CHORDS, 62);                    /* one chord note releasing */
    kit_track_note_on(KIT_TRACK_MELODY, 80, 100);
    CHECK(V(0) == 5 && V(2) == 7, "the melody note took a chord voice (%d %d)", V(0), V(2));
    CHECK(!kit__track_sounding(KIT_TRACK_CHORDS, 62) && kit__track_sounding(KIT_TRACK_CHORDS, 60),
          "the releasing chord voice is the one taken, not the oldest held one");

    kit_track_sound(KIT_TRACK_CHORDS, KIT_ENGINE_BASIC, KIT_INST_PIANO);
    CHECK(V(0) == 0 && V(2) == 7, "changing the chords' sound stops only the chords (%d %d)", V(0), V(2));
    kit_all_notes_off();
    for (i = 0; i < 300 && kit__synth_active(); i++) energy(8);

    /* the CPU budget: heavy sounds fit fewer voices */
    kit_track_sound(KIT_TRACK_CHORDS, 5, 0);                     /* engine 5 is the Organ wheel's place: 5.3 % a voice */
    kit_track_sound(KIT_TRACK_MELODY, 5, 0);
    for (i = 0; i < 8; i++) kit_track_note_on(KIT_TRACK_CHORDS, 48 + i, 100);
    for (i = 0; i < 4; i++) kit_track_note_on(KIT_TRACK_MELODY, 72 + i, 100);
    CHECK(total() == 11 && kit_voices_load() <= KIT_CPU_LIMIT, "twelve heavy voices asked, eleven fit within %d %% (10 + 2 x 3 + 11 x 5.3; %d voices, %d %%)", KIT_CPU_LIMIT, total(), kit_voices_load());
    for (i = 0; i < 8; i++) { ev[0][i].on = 0; ev[2][i].on = 0; }
    kit_track_sound(KIT_TRACK_CHORDS, KIT_ENGINE_BASIC, KIT_INST_PIANO);
    kit_track_sound(KIT_TRACK_MELODY, KIT_ENGINE_BASIC, KIT_INST_SINE);


    kit_track_note_on(KIT_TRACK_DRUMS, 36, 127);
    kit_track_note_off(KIT_TRACK_DRUMS, 36);
    CHECK(energy(8) > 1000, "a kick sounds, and rings out after note-off");
    for (i = 0; i < 60 && kit__synth_active(); i++) energy(32);
    CHECK(!kit__synth_active(), "and falls silent by itself");
    kit_track_note_on(KIT_TRACK_DRUMS, 38, 127);
    CHECK(energy(8) > 1000, "a snare sounds");
    kit_track_note_on(KIT_TRACK_DRUMS, 42, 127);
    CHECK(V(3) == 2, "a hat alongside it");

    {
        enum { N = KIT_BLOCK * 256 };
        static int16_t dry[N], room[N], later[N], brighter[N];
        kit_reverb_shape_t shape;
        int early, delayed;
        kit_all_notes_off();
        for (i = 0; i < 300 && kit__synth_active(); i++) energy(8);
        kit_reverb_shape_of(&shape);
        CHECK(shape.damp == 70 && shape.pre_ms == 0 && kit_reverb_tail_ms_of() == 500, "original room defaults");
        kit_sfx(KIT_SFX_PLOP, 8);
        kit__synth_render(dry, N);
        kit_reverb(75);
        kit_sfx(KIT_SFX_PLOP, 8);
        kit__synth_render(room, N);
        kit_reverb(0);
        shape.pre_ms = 100;
        kit_reverb_shape(&shape);
        kit_reverb(75);
        kit_sfx(KIT_SFX_PLOP, 8);
        kit__synth_render(later, N);
        for (early = 0; early < N && room[early] == dry[early]; early++) {}
        for (delayed = 0; delayed < N && later[delayed] == dry[delayed]; delayed++) {}
        CHECK(early < N && delayed < N && delayed - early == KIT_FS / 10,
              "100 ms pre-delay moves only the wet onset (%d -> %d)", early, delayed);
        kit_reverb(0);
        shape.damp = 10;
        kit_reverb_shape(&shape);
        kit_reverb(75);
        kit_sfx(KIT_SFX_PLOP, 8);
        kit__synth_render(brighter, N);
        CHECK(memcmp(later, brighter, sizeof later) != 0, "damping changes the reflected sound");
        kit_reverb(0);
        shape.pre_ms = 255;
        shape.damp = 255;
        shape.size = 127;
        kit_reverb_shape(&shape);
        kit_reverb_shape_of(&shape);
        CHECK(shape.pre_ms == 120 && shape.damp == 100 && kit_reverb_tail_ms_of() == 4400, "room controls' upper bounds");
    }

    {
        kit_fx_t song_bus = {2, 73, 19, 3, 77, 68, 51}, bus;
        kit_reverb_shape_t song_room = {92, 38, 27}, room;
        int track = KIT_TRACK_BASS, other = KIT_TRACK_MELODY;

        kit_fx_bus(&song_bus);
        kit_reverb_shape(&song_room);
        kit_delay_tone(26);
        kit_track_sends(other, 11, 22, 33, 44);
        kit_track_note_on(other, 64, 100);
        engine_has_shared_fx = 1;

        kit_track_sound_local(track, 1, 2);
        kit_fx_bus_of(&bus);
        kit_reverb_shape_of(&room);
        CHECK(!memcmp(&bus, &song_bus, sizeof bus) && kit_delay_tone_of() == 26,
              "local sound selection preserves the song's chorus and delay");
        CHECK(!memcmp(&room, &song_room, sizeof room), "local sound selection preserves the song's room");
        CHECK(kit_track_engine(track) == 1 && kit_track_preset(track) == 2,
              "local sound selection loads the requested engine and preset");
        CHECK(kit_track_send_of(track, KIT_SEND_DRIVE) == 21 && kit_track_send_of(track, KIT_SEND_CHORUS) == 32 &&
              kit_track_send_of(track, KIT_SEND_DELAY) == 43 && kit_track_send_of(track, KIT_SEND_REVERB) == 54,
              "local sound selection loads the selected track's sends");
        CHECK(kit_track_send_of(other, KIT_SEND_DELAY) == 33 && V(other) > 0,
              "local sound selection leaves the other sounding track and its sends alone");

        kit_track_sound(track, 1, 3);
        kit_fx_bus_of(&bus);
        kit_reverb_shape_of(&room);
        CHECK(bus.dly_time == 52 && bus.cho_depth == 11 && room.size == 45,
              "ordinary sound selection still loads the preset's shared effects");
    }

    printf("%s: synth tracks, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
