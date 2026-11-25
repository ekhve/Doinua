/* SPDX-License-Identifier: GPL-3.0-only */
/* engines.c's Juno-106 (ST-71b, D-090), FM-106's engine through juno.c with the owner's 28 picks:
 *   - every pick sounds (a held C4 is heard; the three slow swells excepted), stays below full scale, and ends after its
 *     release
 *   - its pitch follows the key (Flutes: C4 within 15 cents; an octave up, twice the frequency)
 *   - five voices a track (the Juno has six: D-090); a voice can be taken; each track has its own Juno */
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
#define E 7                                 /* the host's Juno-106 (the kit's engine 9) */
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

static void check_live_filter(int preset)
{
    kit_sound_filter_t original, edit, readback;
    uint8_t bands[KIT_SPECTRUM_BANDS];
    double open, closed;
    int i, energy = 0;

    CHECK(preset >= 0, "Combo Organ is in the factory bank");
    if (preset < 0) return;
    kit__eng_select(0, E, preset);
    kit__eng_select(1, E, preset);
    CHECK(kit_track_filter_of(0, &original), "read the Juno's native filter");
    edit = (kit_sound_filter_t){127, 0, 3, 110};
    CHECK(kit_track_filter(0, &edit), "edit the Juno's native filter");
    CHECK(kit_track_filter_of(0, &readback) && !memcmp(&edit, &readback, sizeof edit), "native filter reads back all four values");
    CHECK(kit_track_filter_of(1, &readback) && !memcmp(&original, &readback, sizeof original), "another track retains the factory filter");
    CHECK(kit_track_spectrum_position(0, 127) > kit_track_spectrum_position(0, 0), "Juno cutoff advances on the spectrum axis");
    kit_track_spectrum(0);
    kit__eng_note_on(0, 72, 100);
    render(320, &open, 0);
    CHECK(kit_track_spectrum_read(bands), "the Juno produced a spectrum window");
    for (i = 0; i < KIT_SPECTRUM_BANDS; i++) energy += bands[i];
    CHECK(energy > 0, "a Juno note appears on the selected track's spectrum");
    edit.cutoff = 0;
    kit_track_filter(0, &edit);
    render(400, &closed, 0);
    CHECK(open > 200 && closed < open / 8, "closing the filter changes a held note (%.0f to %.0f)", open, closed);
    kit_track_spectrum(-1);
    kit__eng_select(0, E, preset);
    CHECK(kit_track_filter_of(0, &readback) && !memcmp(&original, &readback, sizeof original), "reloading recalls the factory filter");
}

int main(void)
{
    int p, i, flute = find("Flutes"), pad = find("Synth Pad"), n0;
    double r, f;
    int32_t pk;
    CHECK(!strcmp(kit__eng_name(E), "Juno-106") && kit__eng_npresets(E) == 28, "Juno-106 with the 28 picks (%s, %d)", kit__eng_name(E), kit__eng_npresets(E));
    CHECK(kit__eng_count() > E + 1 && !strcmp(kit__eng_name(E + 1), "Bass Something 2"), "BS2 follows the Juno (%d)", kit__eng_count());
    CHECK(!strcmp(kit__eng_preset_name(E, 20), "Orchestral"), "names without the bank number or trailing spaces (%s)", kit__eng_preset_name(E, 20));
    for (p = 0; p < kit__eng_npresets(E); p++) {
        double held, after;
        kit__eng_select(0, E, p);
        CHECK(!kit__eng_nactive(0), "%s starts silent", kit__eng_preset_name(E, p));
        kit__eng_note_on(0, 60, 100);
        render(1500, &held, &pk);
        if (kit__eng_npresets(E) && strcmp(kit__eng_preset_name(E, p), "Orchestral") && strcmp(kit__eng_preset_name(E, p), "FX Rise 2") && strcmp(kit__eng_preset_name(E, p), "FX Sweep 1"))
            CHECK(held > 200, "%s sounds while held (rms %.0f)", kit__eng_preset_name(E, p), held);
        CHECK(pk < 60000, "%s stays below 60000, the mix's clip less a margin (peak %d)", kit__eng_preset_name(E, p), pk);
        kit__eng_note_off(0, 60);
        for (i = 0; i < 40 && kit__eng_nactive(0); i++) render(500, &after, 0);
        CHECK(!kit__eng_nactive(0), "%s ends after its release", kit__eng_preset_name(E, p));
    }
    CHECK(flute >= 0 && pad >= 0, "Flutes and Synth Pad are picks");
    kit__eng_select(0, E, flute);
    kit__eng_note_on(0, 60, 100);
    render(300, 0, 0);
    f = hz(400);
    CHECK(fabs(1200 * log2(f / 261.63)) < 15, "Flutes' C4 is C4 (%.1f Hz)", f);
    kit__eng_all_off(0);
    for (i = 0; i < 40 && kit__eng_nactive(0); i++) render(500, &r, 0);
    kit__eng_note_on(0, 72, 100);
    render(300, 0, 0);
    f = hz(400);
    CHECK(fabs(1200 * log2(f / 523.25)) < 15, "and C5 is an octave up (%.1f Hz)", f);
    kit__eng_note_on(0, 76, 100);
    render(300, 0, 0);
    CHECK(kit__eng_has(0, 72) && kit__eng_has(0, 76) && kit__eng_nactive(0) == 2, "two keys, two voices");
    kit__eng_note_on(0, 60, 30);
    render(300, &r, 0);
    kit__eng_all_off(0);
    for (i = 0; i < 40 && kit__eng_nactive(0); i++) render(500, 0, 0);
    CHECK(!kit__eng_nactive(0), "all off ends every voice");
    kit__eng_select(0, E, pad);
    for (i = 0; i < 6; i++) kit__eng_note_on(0, 60 + i * 2, 100);
    n0 = kit__eng_nactive(0);
    CHECK(n0 == 5, "six notes on Synth Pad: five voices, the cap (D-090) (%d)", n0);
    CHECK(kit__eng_steal(0) && kit__eng_nactive(0) == 4, "a voice taken leaves four (%d)", kit__eng_nactive(0));
    kit__eng_select(0, E, flute);
    kit__eng_select(1, E, flute);
    kit__eng_note_on(0, 60, 100);
    kit__eng_note_on(1, 60, 20);
    render(300, 0, 0);
    kit__eng_all_off(1);
    render(300, &r, 0);
    kit__eng_all_off(0);
    for (i = 0; i < 40 && kit__eng_nactive(0); i++) render(500, 0, 0);
    CHECK(r > 200, "two tracks with their own Junos: one's all off leaves the other (rms %.0f)", r);
    kit__eng_select(0, E - 1, 0);
    kit__eng_note_on(0, 60, 100);
    render(300, &r, 0);
    CHECK(r > 100 && !kit__eng_has(1, 60), "another engine on the track takes the Juno off it (rms %.0f)", r);
    check_live_filter(find("Combo Organ"));
    printf("engines juno: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
