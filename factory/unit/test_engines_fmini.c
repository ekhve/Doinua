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

static void check_factory_arp(void)
{
    static const struct { int preset; uint8_t mode, ticks, octaves, gate, swing; } expected[] = {
        {0, KIT_PRESET_ARP_UP_DOWN, 6, 3, 64, 0},
        {6, KIT_PRESET_ARP_UP_DOWN, 12, 1, 64, 0},
        {14, KIT_PRESET_ARP_UP_DOWN, 8, 2, 102, 0},
        {16, KIT_PRESET_ARP_UP_DOWN, 12, 2, 89, 10},
        {19, KIT_PRESET_ARP_UP_OCTAVE, 12, 1, 70, 0},
        {20, KIT_PRESET_ARP_AS_PLAYED, 6, 1, 44, 25},
        {23, KIT_PRESET_ARP_DIVERGE, 12, 2, 114, 0},
        {27, KIT_PRESET_ARP_UP_DOWN, 6, 2, 57, 0},
        {28, KIT_PRESET_ARP_UP, 6, 3, 64, 0},
        {29, KIT_PRESET_ARP_UP_DOWN, 12, 2, 38, 0},
        {30, KIT_PRESET_ARP_UP, 3, 4, 114, 0},
        {36, KIT_PRESET_ARP_REPEAT, 48, 1, 95, 0},
        {39, KIT_PRESET_ARP_UP, 6, 2, 57, 0},
        {40, KIT_PRESET_ARP_UP, 6, 1, 64, 0},
        {47, KIT_PRESET_ARP_UP, 6, 1, 64, 0},
        {49, KIT_PRESET_ARP_UP, 6, 1, 64, 0},
        {50, KIT_PRESET_ARP_UP, 6, 1, 64, 0},
        {55, KIT_PRESET_ARP_UP, 6, 1, 64, 0},
        {64, KIT_PRESET_ARP_UP, 6, 1, 64, 0},
    };
    unsigned next = 0, count = sizeof expected / sizeof expected[0];
    for (int preset = 0; preset < kit__eng_npresets(E); ++preset) {
        kit_preset_arp_t actual;
        int enabled = kit_preset_arp(KIT_ENGINE_FMINI, preset, &actual);
        int want = next < count && expected[next].preset == preset;
        CHECK(enabled == want, "factory ARP enable for FMini preset %d", preset);
        if (enabled && want) {
            CHECK(actual.mode == expected[next].mode && actual.ticks == expected[next].ticks &&
                  actual.octaves == expected[next].octaves && actual.gate == expected[next].gate &&
                  actual.swing == expected[next].swing && actual.probability == 127,
                  "factory ARP settings for FMini preset %d", preset);
        }
        next += want;
    }
    CHECK(next == count, "all selected FMini factory arpeggios checked");
}

static void check_live_filter(int preset)
{
    kit_sound_filter_t original, current, high = {255, 0, 0, 0}, low = {0, 0, 0, 0};
    double open, closed;
    kit__eng_select(0, E, preset);
    kit__eng_select(1, E, preset);
    CHECK(kit_track_filter_of(0, &original), "read the native filter of an FMini preset");
    CHECK(kit_track_filter(0, &high) && kit_track_filter_of(0, &current) && current.cutoff == 127,
          "the live cutoff is clamped to the engine dial");
    kit__eng_note_on(0, 72, 100);
    render(1300, 0, 0);
    render(200, &open, 0);
    CHECK(kit_track_filter(0, &low), "close the native filter while a note is held");
    render(1500, 0, 0);
    render(200, &closed, 0);
    CHECK(open > 200 && closed < open / 4, "native cutoff changes held-note audio (%.1f to %.1f RMS)", open, closed);
    CHECK(kit__eng_has(0, 72), "filter editing keeps the held note");
    low.resonance = 41;
    low.amount = 83;
    low.keytrack = 111;
    CHECK(kit_track_filter(0, &low) && kit_track_filter_of(0, &current) && !memcmp(&low, &current, sizeof low),
          "all four native filter controls round-trip");
    CHECK(kit_track_filter_of(1, &current) && !memcmp(&original, &current, sizeof current),
          "filter edits leave the other track's patch intact");
    kit__eng_select(0, E, preset);
    CHECK(kit_track_filter_of(0, &current) && !memcmp(&original, &current, sizeof current),
          "reselecting a preset restores its native filter settings");
    kit__eng_select(0, -1, 0);
    kit__eng_select(1, -1, 0);
    CHECK(!kit_track_filter_of(0, &current) && !kit_track_filter(0, &low), "unsupported tracks reject filter edits");
}

static void check_live_osc(int preset)
{
    kit_sound_osc_t original, current;
    double sine, square;
    kit__eng_select(0, E, preset);
    CHECK(kit_track_osc_of(0, &original), "read FMini's native oscillator settings");
    CHECK(kit_track_osc(0, KIT_OSC_MIX, 0) && kit_track_osc(0, KIT_OSC_WAVE1, KIT_OSC_SINE),
          "isolate oscillator 1 on its sine wave");
    kit__eng_note_on(0, 72, 100);
    render(1300, 0, 0);
    render(200, &sine, 0);
    CHECK(kit_track_osc(0, KIT_OSC_WAVE1, KIT_OSC_SQR), "switch the held oscillator to square");
    render(200, &square, 0);
    CHECK(sine > 200 && square > 200 && fabs(square - sine) > sine * 0.1 && kit__eng_has(0, 72),
          "wave change reaches the held note (%.1f to %.1f RMS)", sine, square);
    CHECK(kit_track_osc(0, KIT_OSC_DETUNE, -245) && kit_track_osc(0, KIT_OSC_MIX, 127) &&
          kit_track_osc_of(0, &current) && current.detune == -245 && current.mix == 127,
          "detune and the oscillator 2 balance endpoint round-trip");
    kit__eng_select(0, E, preset);
    CHECK(kit_track_osc_of(0, &current) && current.wave1 == original.wave1 && current.wave2 == original.wave2 &&
          current.detune == original.detune && current.mix == original.mix, "preset selection restores oscillator settings");
    kit__eng_select(0, -1, 0);
    CHECK(!kit_track_osc_of(0, &current) && !kit_track_osc(0, KIT_OSC_MIX, 0), "unsupported tracks reject oscillator edits");
}

static void check_live_envelopes(int preset)
{
    uint8_t amp[4], filter[4];
    double quick, slow;
    kit__eng_select(0, E, preset);
    CHECK(kit_track_envelope_of(0, KIT_ENV_AMP, amp) && amp[KIT_ENV_RELEASE] == amp[KIT_ENV_DECAY],
          "the factory amp release follows its decay");
    CHECK(kit_track_envelope_of(0, KIT_ENV_FILTER, filter) && filter[KIT_ENV_RELEASE] == filter[KIT_ENV_DECAY],
          "the factory filter release follows its decay");
    kit_track_envelope(0, KIT_ENV_FILTER, KIT_ENV_RELEASE, 121);
    kit_track_envelope(0, KIT_ENV_FILTER, KIT_ENV_DECAY, 31);
    CHECK(kit_track_envelope_of(0, KIT_ENV_FILTER, filter) && filter[KIT_ENV_RELEASE] == 121 && filter[KIT_ENV_DECAY] == 31,
          "editing filter release makes it independent of decay");
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_ATTACK, 0);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_SUSTAIN, 127);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_RELEASE, 0);
    kit__eng_note_on(0, 60, 100);
    render(200, 0, 0);
    kit__eng_note_off(0, 60);
    render(40, 0, 0);
    render(60, &quick, 0);
    kit__eng_note_on(0, 60, 100);
    render(200, 0, 0);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_RELEASE, 98);
    kit__eng_note_off(0, 60);
    render(40, 0, 0);
    render(60, &slow, 0);
    CHECK(quick < 20 && slow > 1000, "amp release time applies to audio (%.1f vs %.1f RMS)", quick, slow);
    kit__eng_select(0, E, preset);
    CHECK(kit_track_envelope_of(0, KIT_ENV_AMP, amp) && amp[KIT_ENV_RELEASE] == amp[KIT_ENV_DECAY],
          "reselecting the preset restores its coupled release");
    CHECK(kit_track_envelope_ms(0, 0) == 1 && kit_track_envelope_ms(0, 127) == 12000,
          "the displayed time uses the engine's envelope dial scale");
    kit__eng_select(0, -1, 0);
}

static void check_live_lfo(int preset)
{
    kit_sound_lfo_t lfo, factory;
    kit_sound_filter_t filter = { 65, 0, 0, 0 };
    double first, second;
    kit__eng_select(0, E, preset);
    CHECK(kit_track_lfo_of(0, &factory) && !factory.depth, "the added LFO route starts at zero depth");
    lfo = (kit_sound_lfo_t){ 57, 120, KIT_LFO_VOLUME, KIT_LFO_SQUARE };
    CHECK(kit_track_lfo(0, &lfo) && kit_track_lfo_of(0, &factory) && !memcmp(&factory, &lfo, sizeof lfo),
          "the rate, depth, target and wave round-trip");
    CHECK(kit_track_lfo_centihz(0, 57) >= 95 && kit_track_lfo_centihz(0, 57) <= 105,
          "the displayed rate is near one Hz");
    kit__eng_note_on(0, 72, 100);
    render(200, 0, 0); render(150, &first, 0);
    render(250, 0, 0); render(150, &second, 0);
    CHECK(first > 0 && second > first * 5, "square LFO modulates held-note volume (%.1f to %.1f)", first, second);

    kit__eng_select(0, E, preset);
    lfo.target = KIT_LFO_CUTOFF;
    kit_track_filter(0, &filter);
    kit_track_lfo(0, &lfo);
    kit__eng_note_on(0, 72, 100);
    render(200, 0, 0); render(150, &first, 0);
    render(250, 0, 0); render(150, &second, 0);
    CHECK(first > 100 && first > second * 2, "square LFO reaches the held note's filter (%.1f to %.1f)", first, second);
    CHECK(kit_track_filter_of(0, &filter) && filter.cutoff == 65, "LFO leaves the stored cutoff untouched");

    kit__eng_select(0, E, preset);
    lfo.target = KIT_LFO_PITCH;
    kit_track_lfo(0, &lfo);
    kit__eng_note_on(0, 60, 100);
    render(200, 0, 0); first = hz(130);
    render(300, 0, 0); second = hz(130);
    CHECK(first > 264 && second < 259 && second > 235, "square LFO modulates pitch (%.1f to %.1f Hz)", first, second);
    kit__eng_select(0, E, preset);
    CHECK(kit_track_lfo_of(0, &lfo) && !lfo.depth, "preset reload clears the added LFO route");
    kit__eng_select(0, -1, 0);
}

static void check_wheel_filter(int preset)
{
    kit_sound_filter_t filter;
    kit_sound_mod_t route = {.cutoff = -100};
    double open, closed;
    kit__eng_select(0, E, preset);
    kit__eng_note_on(0, 72, 100);
    render(700, 0, 0); render(100, &open, 0);
    kit_track_mod_shape(0, &route);
    render(1400, 0, 0); render(100, &closed, 0);
    CHECK(open > 100 && closed < open / 2, "signed cutoff route darkens a held note (%.1f to %.1f RMS)", open, closed);
    CHECK(kit_track_filter_of(0, &filter) && filter.cutoff == 127, "wheel offset keeps the patch's cutoff");
    kit__eng_select(0, -1, 0);
}

static void check_live_spectrum(int preset)
{
    uint8_t bands[KIT_SPECTRUM_BANDS];
    int i, peak = 0;
    kit__eng_select(0, E, preset);
    kit__eng_select(1, E, preset);
    kit_track_spectrum(0);
    kit__eng_note_on(1, 72, 100);
    render(45, 0, 0);
    CHECK(kit_track_spectrum_read(bands), "spectrum publishes a window from the selected track");
    for (i = 0; i < KIT_SPECTRUM_BANDS; i++) peak |= bands[i];
    CHECK(!peak, "a different track has no energy in the selected spectrum");
    peak = 0;
    kit__eng_note_on(0, 60, 100);
    render(700, 0, 0);
    kit_track_spectrum_read(bands);
    render(40, 0, 0);
    CHECK(kit_track_spectrum_read(bands), "spectrum publishes a held note's capture");
    for (i = 0; i < KIT_SPECTRUM_BANDS; i++) if (bands[i] > bands[peak]) peak = i;
    CHECK(peak >= 4 && peak <= 6 && bands[peak] > 45,
          "a C4 sine peaks near 262 Hz (band %d, level %d)", peak, bands[peak]);
    kit_track_spectrum(-1);
    render(40, 0, 0);
    CHECK(!kit_track_spectrum_read(bands), "no spectrum window is reported when capture is off");
    kit__eng_select(0, -1, 0);
    kit__eng_select(1, -1, 0);
}

int main(void)
{
    check_factory_arp();
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
    check_live_filter(sine);
    check_live_osc(sine);
    check_live_envelopes(sine);
    check_live_lfo(sine);
    check_wheel_filter(sine);
    check_live_spectrum(sine);
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
