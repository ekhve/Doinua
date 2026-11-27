/* SPDX-License-Identifier: GPL-3.0-only */
/* synth.c's glide and mono (API 27, ST-46, D-073), heard on a Basic sine:
 *   - with a glide time a note slides from the last one, in about that time (R-234); without, it jumps
 *   - SLIDE LEGATO slides only while a key is held; SLIDE ALWAYS (portamento) also after the keys were let go
 *   - MONO sounds one note, the last key (R-235); letting it go goes back to the key still held (R-236)
 *   - MONO LEGATO does the same without a new attack; drums never glide and are never mono */
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
int kit__eng_fx(int p, uint8_t s[4], uint8_t b[12]) { (void)p; (void)b; s[0] = s[1] = s[2] = s[3] = 0; return 0; }
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
static int16_t buf[KIT_BLOCK * 1000];

/* the pitch of what plays over the next ms milliseconds (Hz), from its first and last rising zero crossings */
static double hz(int ms)
{
    int n = (ms * KIT_FS / 1000) / KIT_BLOCK * KIT_BLOCK, i, periods = -1;
    double first = 0, last = 0;
    kit__synth_render(buf, n);
    for (i = 1; i < n; i++)
        if (buf[i - 1] < 0 && buf[i] >= 0) {
            double at = i - 1 + (double)-buf[i - 1] / (buf[i] - buf[i - 1]);
            if (periods < 0) first = at;
            last = at;
            periods++;
        }
    return periods > 0 ? periods / ((last - first) / KIT_FS) : 0;
}
static void run(int ms) { while (ms > 0) { int m = ms > 500 ? 500 : ms; kit__synth_render(buf, (m * KIT_FS / 1000) / KIT_BLOCK * KIT_BLOCK); ms -= m; } }
static int near(double f, double want) { return f > want * 0.97 && f < want * 1.03; }
#define C4 261.63
#define G4 392.0
#define E4 329.63

int main(void)
{
    double f;
    kit__synth_init();
    kit_track_sound(KIT_TRACK_MELODY, KIT_ENGINE_BASIC, KIT_INST_SINE);
    CHECK(kit_track_voice_mode(KIT_TRACK_MELODY) == KIT_VMODE_POLY && kit_track_glide_ms(KIT_TRACK_MELODY) == 0, "a track starts POLY with no glide");

    kit_track_note_on(KIT_TRACK_MELODY, 60, 100); run(80);
    kit_track_bend(KIT_TRACK_MELODY, 8191);
    f = hz(60);
    CHECK(near(f, 293.66), "default positive bend stays two semitones (%.1f Hz)", f);
    kit_track_bend_range(KIT_TRACK_MELODY, 12);
    f = hz(60);
    CHECK(near(f, C4 * 2), "an already-held wheel follows the new range (%.1f Hz)", f);
    CHECK(kit_track_bend_range_of(KIT_TRACK_CHORDS) == 2, "bend range belongs to one track");
    kit_track_bend(KIT_TRACK_MELODY, -8192);
    f = hz(60);
    CHECK(near(f, C4 / 2), "negative octave bend (%.1f Hz)", f);
    kit_track_bend(KIT_TRACK_MELODY, 6144);
    f = hz(60);
    CHECK(near(f, 440.0), "octave folding preserves a nine-semitone bend (%.1f Hz)", f);
    kit_track_bend(KIT_TRACK_MELODY, 0);
    kit_track_bend_range(KIT_TRACK_MELODY, 2);
    kit_track_note_off(KIT_TRACK_MELODY, 60); run(500);

    /* no glide: a new note is at its pitch at once */
    kit_track_note_on(KIT_TRACK_MELODY, 60, 100); run(100); kit_track_note_off(KIT_TRACK_MELODY, 60); run(300);
    kit_track_note_on(KIT_TRACK_MELODY, 67, 100); f = hz(30);
    CHECK(near(f, G4), "no glide: G4 at once (%.1f Hz)", f);
    kit_track_note_off(KIT_TRACK_MELODY, 67); run(400);

    /* SLIDE ALWAYS (portamento) 200 ms: after C4 is let go, G4 still slides up from it */
    kit_track_voice(KIT_TRACK_MELODY, KIT_VMODE_POLY, 200, KIT_SLIDE_ALWAYS);
    kit_track_note_on(KIT_TRACK_MELODY, 60, 100); run(100); kit_track_note_off(KIT_TRACK_MELODY, 60); run(300);
    kit_track_note_on(KIT_TRACK_MELODY, 67, 100); f = hz(25);
    CHECK(f < C4 * 1.12 && f > C4 * 0.97, "glide: G4 starts near C4 (%.1f Hz)", f);
    run(80); f = hz(20);
    CHECK(f > C4 * 1.1 && f < G4 * 0.97, "glide: half way it is between (%.1f Hz)", f);
    run(150); f = hz(40);
    CHECK(near(f, G4), "glide: after 200 ms it is G4 (%.1f Hz)", f);
    kit_track_note_off(KIT_TRACK_MELODY, 67); run(400);

    /* SLIDE LEGATO: after the keys were let go it jumps; with C4 held it slides */
    kit_track_voice(KIT_TRACK_MELODY, KIT_VMODE_POLY, 200, KIT_SLIDE_LEGATO);
    kit_track_note_on(KIT_TRACK_MELODY, 60, 100); run(100); kit_track_note_off(KIT_TRACK_MELODY, 60); run(300);
    kit_track_note_on(KIT_TRACK_MELODY, 67, 100); f = hz(25);
    CHECK(near(f, G4), "legato slide: let go first, G4 at once (%.1f Hz)", f);
    kit_track_note_off(KIT_TRACK_MELODY, 67); run(400);

    /* MONO: one voice, the last key; letting it go returns to the key held (R-235, R-236) */
    kit_track_voice(KIT_TRACK_MELODY, KIT_VMODE_MONO, 0, KIT_SLIDE_ALWAYS);
    kit_track_note_on(KIT_TRACK_MELODY, 60, 100); run(60);
    kit_track_note_on(KIT_TRACK_MELODY, 67, 100); run(60);
    CHECK(kit__track_voices(KIT_TRACK_MELODY) == 1, "mono: one voice for two keys (%d)", kit__track_voices(KIT_TRACK_MELODY));
    f = hz(40); CHECK(near(f, G4), "mono: the last key sounds (%.1f Hz)", f);
    kit_track_note_off(KIT_TRACK_MELODY, 67); f = hz(40);
    CHECK(near(f, C4), "mono: letting G4 go returns to C4, still held (%.1f Hz)", f);
    CHECK(kit__track_voices(KIT_TRACK_MELODY) == 1, "mono: still one voice (%d)", kit__track_voices(KIT_TRACK_MELODY));
    kit_track_note_off(KIT_TRACK_MELODY, 60); run(1500);
    CHECK(kit__track_voices(KIT_TRACK_MELODY) == 0, "mono: let go, it ends (%d)", kit__track_voices(KIT_TRACK_MELODY));

    /* MONO LEGATO with glide: E4 held, then G4 slides there without a new attack, and back */
    kit_track_voice(KIT_TRACK_MELODY, KIT_VMODE_LEGATO, 100, KIT_SLIDE_LEGATO);
    kit_track_note_on(KIT_TRACK_MELODY, 64, 100); run(120);
    kit_track_note_on(KIT_TRACK_MELODY, 67, 100); f = hz(20);
    CHECK(f > E4 * 0.97 && f < G4 * 0.98, "mono legato: G4 slides from E4 (%.1f Hz)", f);
    run(120); f = hz(30); CHECK(near(f, G4), "mono legato: then G4 (%.1f Hz)", f);
    kit_track_note_off(KIT_TRACK_MELODY, 67); run(150); f = hz(30);
    CHECK(near(f, E4), "mono legato: back to E4 held (%.1f Hz)", f);
    kit_track_note_off(KIT_TRACK_MELODY, 64); run(500);

    /* drums: never mono, never glide */
    kit_track_voice(KIT_TRACK_DRUMS, KIT_VMODE_MONO, 500, KIT_SLIDE_ALWAYS);
    kit_track_note_on(KIT_TRACK_DRUMS, 53, 100); kit_track_note_on(KIT_TRACK_DRUMS, 55, 100); run(10);
    CHECK(kit__track_voices(KIT_TRACK_DRUMS) == 2, "drums: two hits, two voices (%d)", kit__track_voices(KIT_TRACK_DRUMS));

    printf("%s: glide and mono, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
