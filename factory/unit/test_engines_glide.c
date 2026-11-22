/* SPDX-License-Identifier: GPL-3.0-only */
/* engines.c's glide and mono (API 27, ST-46, D-073) on the Felucca engines (a track that plays an engine):
 *   - glide from a pitch: the voice starts there and reaches the note in about the glide time (R-234)
 *   - mono: one voice takes each new note (R-235); retarget sends it back to the key still held (R-236)
 *   - mono legato: no new attack; the glide time is kept even for a long glide (fine steps) */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "kit_internal.h"

uint32_t kit__lock(void) { return 0; }             /* the bus's lock (fxbus.c, D-093) */
void kit__unlock(uint32_t k) { (void)k; }
int kit__track_pitch(int track) { (void)track; return 0; }   /* synth.c's bend and vibrato: none here */

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define FS 44100
static int32_t mix[FS];

static void render(int ms)
{
    while (ms > 0) {
        int m = ms > 500 ? 500 : ms, n = (m * FS / 1000) / 32 * 32, i;
        for (i = 0; i < n; i++) mix[i] = 0;
        kit__eng_render(mix, n);
        ms -= m;
    }
}
/* the pitch (Hz) of the next ms milliseconds, by autocorrelation between 100 and 800 Hz */
static double hz(int ms)
{
    int n = (ms * FS / 1000) / 32 * 32, lag, best = 0, i;
    double bv = -1;
    render(ms);
    for (lag = FS / 800; lag <= FS / 100; lag++) {
        double a = 0, e1 = 1e-9, e2 = 1e-9, r;
        for (i = 0; i + lag < n; i++) { a += (double)mix[i] * mix[i + lag]; e1 += (double)mix[i] * mix[i]; e2 += (double)mix[i + lag] * mix[i + lag]; }
        r = a / sqrt(e1 * e2);
        if (r > bv) { bv = r; best = lag; }
    }
    return best ? (double)FS / best : 0;
}
static int near(double f, double want) { return f > want * 0.97 && f < want * 1.03; }
#define C4 261.63
#define E4 329.63
#define G4 392.0

int main(void)
{
    double f;
    kit__eng_select(0, 0, 0);                       /* part 0: the first engine, its first preset */

    kit__eng_note_glide(0, 67, 100, -1, 0, 0, 0); f = hz(60);
    CHECK(near(f, G4), "no glide: G4 at once (%.1f Hz)", f);
    kit__eng_note_off(0, 67); render(800);

    kit__eng_note_glide(0, 67, 100, 60 * 16, 300, 0, 0); f = hz(25);
    CHECK(f > C4 * 0.97 && f < C4 * 1.12, "glide: G4 starts near C4 (%.1f Hz)", f);
    render(110); f = hz(25);
    CHECK(f > C4 * 1.12 && f < G4 * 0.95, "glide: half way it is between (%.1f Hz)", f);
    render(200); f = hz(60);
    CHECK(near(f, G4), "glide: after 300 ms it is G4 (%.1f Hz)", f);
    kit__eng_note_off(0, 67); render(800);

    kit__eng_note_glide(0, 60, 100, -1, 0, 1, 0); render(50);
    kit__eng_note_glide(0, 67, 100, 60 * 16, 0, 1, 0); f = hz(60);
    CHECK(kit__eng_nactive(0) == 1, "mono: one voice (%d)", kit__eng_nactive(0));
    CHECK(near(f, G4), "mono: the last key (%.1f Hz)", f);
    CHECK(kit__eng_retarget(0, 67, 60, 0) == 1, "mono: retarget to the key held");
    f = hz(60); CHECK(near(f, C4), "mono: back to C4 (%.1f Hz)", f);
    CHECK(kit__eng_retarget(0, 67, 64, 0) == 0, "mono: retarget only from the note sounding");
    kit__eng_note_off(0, 60); render(1500);

    kit__eng_note_glide(0, 64, 100, -1, 0, 1, 0); render(80);
    kit__eng_note_glide(0, 65, 100, 64 * 16, 1000, 1, 1);  /* a semitone over a second: still slow */
    render(300); f = hz(40);
    CHECK(kit__eng_nactive(0) == 1, "mono legato: one voice (%d)", kit__eng_nactive(0));
    CHECK(f > E4 * 1.005 && f < E4 * 1.05, "mono legato: a long glide moves slowly (%.1f Hz)", f);
    render(800); f = hz(60);
    CHECK(near(f, 349.23), "mono legato: then F4 (%.1f Hz)", f);
    kit__eng_note_off(0, 65);

    printf("%s: engine glide and mono, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
