/* SPDX-License-Identifier: GPL-3.0-only */
/* fxbus.c, the effects bus (ST-69, D-093):
 *   - a track without sends adds its block to the mix unchanged; with sends, also into the buses
 *   - the DELAY repeats a click after its TIME (fmini-1's dly_ms), quieter each time, and ends (no DC, no hum)
 *   - the DRIVE is a soft clip: a full-scale square stays under 1.0 and keeps its sign; silence stays silence
 *   - the CHORUS makes a delayed, moving copy and stops when its input does */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kit_internal.h"

uint32_t kit__lock(void) { return 0; }
void kit__unlock(uint32_t k) { (void)k; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define FS 44100
#define N (FS * 6)
static int32_t out[N];

static void run(int track, const int32_t *in, int n)   /* n samples of track's input through the bus into out */
{
    int b, i;
    static int32_t x[KIT_BLOCK], mix[KIT_BLOCK];
    for (b = 0; b + KIT_BLOCK <= n; b += KIT_BLOCK) {
        kit__fx_begin();
        for (i = 0; i < KIT_BLOCK; i++) { x[i] = in ? in[b + i] : 0; mix[i] = 0; }
        kit__fx_track(track, x, mix);
        kit__fx_buses(mix);
        memcpy(&out[b], mix, sizeof mix);
    }
}

int main(void)
{
    static int32_t in[N];
    kit_fx_t f = { 0, 60, 40, 1, 64, 60, 127 };     /* DIGITAL, TIME 64, FEEDBACK 60, MIX 127 */
    int i, peak1 = 0, at1 = -1, peak2 = 0, at2 = -1, late = 0, d = 10 + 64 * 64 * 1390 / 16129;   /* ms */
    int dn = d * FS / 1000;

    memset(in, 0, sizeof in);
    in[100] = 30000;
    run(0, in, N);
    CHECK(out[100] == 30000, "no sends: the block passes unchanged (%d)", (int)out[100]);
    for (i = 101; i < N; i++) CHECK(out[i] == 0, "no sends: nothing after (%d at %d)", (int)out[i], i);

    kit_fx_bus(&f);
    kit_track_sends(0, 0, 0, 127, 0);
    memset(out, 0, sizeof out);
    run(0, in, N);
    for (i = 200; i < N; i++) {
        int a = abs(out[i]);
        if (i < 100 + dn + dn / 2 && a > peak1) { peak1 = a; at1 = i; }
        if (i > 100 + dn + dn / 2 && i < 100 + 2 * dn + dn / 2 && a > peak2) { peak2 = a; at2 = i; }
    }
    CHECK(abs(at1 - (100 + dn)) < 4, "DELAY's first echo after its TIME, %d ms (%d samples, at %d)", d, dn, at1 - 100);
    CHECK(abs(at2 - (100 + 2 * dn)) < 6 && peak2 < peak1 && peak2 > 0, "a second, quieter (%d < %d)", peak2, peak1);
    for (i = N - FS / 2; i < N; i++) late |= out[i];
    CHECK(!late, "and it ends: silence half a second before the end (no DC, no hum)");

    kit_track_sends(0, 127, 0, 0, 0);
    for (i = 0; i < N; i++) in[i] = i < FS / 2 ? ((i / 50) & 1 ? 32767 : -32767) : 0;
    memset(out, 0, sizeof out);
    run(0, in, N);
    for (i = 0; i < FS / 2; i++)
        CHECK(abs(out[i]) <= 32768 && (out[i] > 0) == (in[i] > 0), "DRIVE: a full square is clipped softly under 1.0, its sign kept (%d at %d)", (int)out[i], i);
    for (i = FS / 2; i < N; i++) CHECK(out[i] == 0, "DRIVE: silence stays silence (%d at %d)", (int)out[i], i);

    kit_track_sends(0, 0, 127, 0, 0);
    memset(in, 0, sizeof in);
    for (i = 0; i < FS; i++) in[i] = (i / 100) & 1 ? 8000 : -8000;
    memset(out, 0, sizeof out);
    run(0, in, N);
    {
        int diff = 0, tail = 0;
        for (i = FS / 4; i < FS; i++) diff |= out[i] != in[i];
        for (i = N - FS / 2; i < N; i++) tail |= out[i];
        CHECK(diff, "CHORUS: a delayed, moving copy is added");
        CHECK(!tail, "CHORUS: it stops when its input does");
    }
    printf("fxbus: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
