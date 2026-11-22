/* SPDX-License-Identifier: GPL-3.0-only */
/* engines.c's FMINI (ST-70, D-086), fmini-1's engine with the owner's 66 picks:
 *   - every pick sounds (a held C4 is heard within its attack), stays below full scale, and is silent after its release
 *   - its pitch follows the key (SINE SOLO: C4 within 15 cents; an octave up, twice the frequency)
 *   - at most four voices a part (Felucca's cap for FMINI beside other engines): a fifth note takes the oldest
 *   - the presets' mono flag is what kit_track_sound turns into MONO legato */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "kit_internal.h"

uint32_t kit__lock(void) { return 0; }             /* the bus's lock (fxbus.c, D-093) */
void kit__unlock(uint32_t k) { (void)k; }
int kit__track_pitch(int track) { (void)track; return 0; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define FS 44100
#define E 6                                 /* the host's FMINI (the kit's engine 8) */
static int32_t mix[FS * 2];                 /* up to 2 s a render */

static void render(int ms, double *rms, int32_t *peak)
{
    int n = (ms * FS / 1000) / 32 * 32, i;
    double s = 0;
    int32_t p = 0;
    memset(mix, 0, sizeof mix);
    kit__eng_render(mix, n);
    for (i = 0; i < n; i++) { s += (double)mix[i] * mix[i]; if (mix[i] > p) p = mix[i]; if (-mix[i] > p) p = -mix[i]; }
    if (rms) *rms = sqrt(s / n);
    if (peak) *peak = p;
}
static double hz(int ms)                    /* the pitch of the next ms, by autocorrelation between 60 and 1200 Hz */
{
    int n = (ms * FS / 1000) / 32 * 32, lag, best = 0, i;
    double bv = -1;
    static double rs[FS / 60 + 2];
    memset(mix, 0, sizeof mix);
    kit__eng_render(mix, n);
    for (lag = FS / 1200; lag <= FS / 60; lag++) {
        double a = 0, e1 = 1e-9, e2 = 1e-9, r;
        for (i = 0; i + lag < n; i++) { a += (double)mix[i] * mix[i + lag]; e1 += (double)mix[i] * mix[i]; e2 += (double)mix[i + lag] * mix[i + lag]; }
        r = a / sqrt(e1 * e2);
        rs[lag] = r;
        if (r > bv) bv = r;
    }
    for (lag = FS / 1200; lag <= FS / 60; lag++)    /* the shortest lag near the best: not one of its multiples */
        if (rs[lag] >= bv * 0.97 && rs[lag] >= rs[lag - 1] && rs[lag] >= rs[lag + 1]) { best = lag; break; }
    return best ? (double)FS / best : 0;
}
static int find(const char *name)
{
    int p;
    for (p = 0; p < kit__eng_npresets(E); p++)
        if (!strcmp(kit__eng_preset_name(E, p), name)) return p;
    return -1;
}

int main(void)
{
    int p, i, sine = find("SINE SOLO"), pad = find("LUSH STRINGS");
    double r, f;
    int32_t pk;
    CHECK(!strcmp(kit__eng_name(E), "FMini") && kit__eng_npresets(E) == 66, "FMini with the 66 picks (%s, %d)", kit__eng_name(E), kit__eng_npresets(E));
    for (p = 0; p < kit__eng_npresets(E); p++) {
        double held, after;
        kit__eng_select(0, E, p);
        kit__eng_note_on(0, 60, 100);
        render(1500, &held, &pk);
        CHECK(held > 200, "%s sounds while held (rms %.0f)", kit__eng_preset_name(E, p), held);
        CHECK(pk < 60000, "%s stays below 60000, the mix's clip less a margin (peak %d)", kit__eng_preset_name(E, p), pk);
        kit__eng_note_off(0, 60);
        for (i = 0; i < 20 && kit__eng_nactive(0); i++) render(500, &after, 0);
        CHECK(!kit__eng_nactive(0), "%s ends after its release", kit__eng_preset_name(E, p));
    }
    CHECK(sine >= 0 && pad >= 0, "SINE SOLO and LUSH STRINGS are picks");
    kit__eng_select(0, E, sine);
    kit__eng_note_on(0, 60, 100);
    render(300, 0, 0);
    f = hz(400);
    CHECK(fabs(1200 * log2(f / 261.63)) < 15, "SINE SOLO's C4 is C4 (%.1f Hz)", f);
    kit__eng_note_off(0, 60);
    for (i = 0; i < 20 && kit__eng_nactive(0); i++) render(500, &r, 0);
    kit__eng_note_on(0, 72, 100);
    render(300, 0, 0);
    f = hz(400);
    CHECK(fabs(1200 * log2(f / 523.25)) < 15, "and C5 is an octave up (%.1f Hz)", f);
    kit__eng_all_off(0);
    for (i = 0; i < 20 && kit__eng_nactive(0); i++) render(500, &r, 0);
    kit__eng_select(0, E, pad);
    for (i = 0; i < 6; i++) kit__eng_note_on(0, 60 + i * 2, 100);
    CHECK(kit__eng_nactive(0) == 4, "six notes on LUSH STRINGS: four sound (%d)", kit__eng_nactive(0));
    CHECK(kit__eng_has(0, 70) && !kit__eng_has(0, 60), "the newest kept, the oldest taken");
    CHECK(kit__eng_preset_mono(E, sine) && !kit__eng_preset_mono(E, pad), "SINE SOLO is MONO, LUSH STRINGS POLY");
    printf("engines fmini: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
