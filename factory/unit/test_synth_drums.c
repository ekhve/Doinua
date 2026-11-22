/* SPDX-License-Identifier: GPL-3.0-only */
/* The drum kits (drums.c in synth.c, API 23, D-053): four kits of 27 sounds named as sound files, each distinct in a
 * kit; F3..G5 are the pads, General MIDI drums map onto them; the groups; a pitched sound names its note (the owner's
 * example: the 808's kick4 tuned up to A1); every sound of every kit plays, within the output, and falls silent by
 * itself; a pad can take any kit's sound (My kit); the closed hat chokes the open one. */
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
static int16_t buf[KIT_BLOCK * 64];
static long run(int blocks, int *peak)          /* render; the summed |sample| */
{
    long e = 0;
    int b, j;
    for (b = 0; b < blocks; b++) {
        kit__synth_render(buf, KIT_BLOCK);
        for (j = 0; j < KIT_BLOCK; j++) { int a = buf[j] < 0 ? -buf[j] : buf[j]; e += a; if (peak && a > *peak) *peak = a; }
    }
    return e;
}

int main(void)
{
    int k, p, q, t = KIT_TRACK_DRUMS;
    kit__synth_init();
    kit_volume(100);
    for (k = 0; k < KIT_DRUM_KITS; k++)
        for (p = 0; p < KIT_DRUM_PADS; p++)
            for (q = p + 1; q < KIT_DRUM_PADS; q++)
                CHECK(strcmp(kit_drum_name(k, p), kit_drum_name(k, q)), "%s: %s twice", kit_drum_kit_name(k), kit_drum_name(k, p));
    CHECK(!strcmp(kit_drum_kit_name(0), "Dusty") && !strcmp(kit_drum_kit_name(1), "808") && kit_preset_count(KIT_ENGINE_DRUMS) == 4, "the four kits");
    CHECK(!strcmp(kit_drum_name(0, 0), "kick1") && !strcmp(kit_drum_name(3, 22), "conga1") && !strcmp(kit_drum_name(1, 26), "crash1"), "names as files");
    CHECK(kit_drum_pad_of(53) == 0 && kit_drum_pad_of(79) == 26 && kit_drum_pad_of(36) == 0 && kit_drum_pad_of(38) == 4 && kit_drum_pad_of(42) == 12,
          "F3..G5 the pads; GM 36 kick, 38 snare, 42 hat");
    CHECK(kit_drum_group(0, 0) == 0 && kit_drum_group(0, 4) == 1 && kit_drum_group(0, 12) == 2 && kit_drum_group(0, 19) == 3, "groups");
    CHECK(kit_drum_note(1, 3, 0) == 28 && kit_drum_note(1, 3, 5) == 33, "808 kick4: E1 (28), tuned up five A1 (33) (%d %d)", kit_drum_note(1, 3, 0), kit_drum_note(1, 3, 5));
    CHECK(kit_drum_note(0, 12, 0) < 0, "a hat has no note");
    for (k = 0; k < KIT_DRUM_KITS; k++) {
        kit_track_sound(t, KIT_ENGINE_DRUMS, k);
        for (p = 0; p < KIT_DRUM_PADS; p++) {
            int peak = 0;
            long e;
            kit_track_note_on(t, KIT_DRUM_LO + p, 127);
            e = run(20, &peak);
            kit_track_note_off(t, KIT_DRUM_LO + p);
            CHECK(e > 20000, "%s %s sounds (%ld)", kit_drum_kit_name(k), kit_drum_name(k, p), e);
            CHECK(peak < 30000, "%s %s within the output (%d)", kit_drum_kit_name(k), kit_drum_name(k, p), peak);
            run(KIT_FS / KIT_BLOCK * 3, 0);     /* three seconds: it must have stopped */
            CHECK(kit__track_voices(t) == 0, "%s %s falls silent by itself", kit_drum_kit_name(k), kit_drum_name(k, p));
        }
    }
    /* My kit: pad 0 plays the Latin kit's conga1 */
    kit_track_sound(t, KIT_ENGINE_DRUMS, 0);
    kit_drum_pad_set(t, 0, 3, 22, 0, 64);
    {
        long a, b;
        kit_track_note_on(t, KIT_DRUM_LO, 127); a = run(40, 0); run(KIT_FS / KIT_BLOCK * 3, 0);
        kit_drum_pad_set(t, 0, -1, 0, 0, 64);
        kit_track_note_on(t, KIT_DRUM_LO, 127); b = run(40, 0); run(KIT_FS / KIT_BLOCK * 3, 0);
        CHECK(a != b, "a pad plays another kit's sound, and back (%ld %ld)", a, b);
    }
    /* the closed hat chokes the open one */
    kit_track_note_on(t, KIT_DRUM_LO + 15, 127);   /* open1 */
    run(4, 0);
    kit_track_note_on(t, KIT_DRUM_LO + 12, 127);   /* hat1 */
    CHECK(kit__track_voices(t) == 1, "the closed hat chokes the open (%d)", kit__track_voices(t));
    printf("%s: drum kits, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
