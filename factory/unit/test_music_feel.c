/* SPDX-License-Identifier: GPL-3.0-only */
/* FEEL (apps/companion/feel.c, ST-81, R-260): for a 4-note chord at 90 BPM, many times over, each feel's note times and
 * velocities stay in their ranges: pianist plays the bass first, the inner notes 10 to 40 ms later and softer, the top
 * note louder than them; tight together; strum up low to high, strum down high to low; rolled and laid back as the lab. */
#include <stdio.h>
#include "../../apps/companion/feel.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    uint32_t rng = 12345u;
    int k, i, n = 4, q = 60000 / 90 / 4;      /* a sixteenth at 90 BPM: 166 ms */
    for (k = 0; k < 500; k++) {
        int d[4], v[4];
        for (i = 0; i < n; i++) feel_note(FEEL_PIANIST, i, n, q, &rng, &d[i], &v[i]);
        for (i = 1; i < n; i++) CHECK(d[i] > d[0], "pianist: note %d at %d ms, not after the bass (%d)", i, d[i], d[0]);
        for (i = 1; i < n - 1; i++) {
            CHECK(d[i] - d[0] >= 10 && d[i] - d[0] <= 40, "pianist: inner note %d %d ms after the bass", i, d[i] - d[0]);
            CHECK(v[i] < v[0] && v[i] < v[n - 1], "pianist: inner note %d at %d%%, not softer (bass %d%%, top %d%%)", i, v[i], v[0], v[n - 1]);
        }
        for (i = 0; i < n; i++) feel_note(FEEL_TIGHT, i, n, q, &rng, &d[i], &v[i]);
        for (i = 0; i < n; i++) CHECK(d[i] <= 3, "tight: note %d at %d ms", i, d[i]);
        for (i = 0; i < n; i++) feel_note(FEEL_STRUM_UP, i, n, q, &rng, &d[i], &v[i]);
        for (i = 1; i < n; i++) CHECK(d[i] > d[i - 1] && v[i] < v[i - 1], "strum up: note %d not after and softer", i);
        for (i = 0; i < n; i++) feel_note(FEEL_STRUM_DOWN, i, n, q, &rng, &d[i], &v[i]);
        for (i = 1; i < n; i++) CHECK(d[i] < d[i - 1] && v[i] > v[i - 1], "strum down: note %d not before and louder", i);
        for (i = 0; i < n; i++) feel_note(FEEL_ROLLED, i, n, q, &rng, &d[i], &v[i]);
        for (i = 1; i < n; i++) CHECK(d[i] > d[i - 1] && v[i] > v[i - 1], "rolled: note %d not after and louder", i);
        CHECK(d[n - 1] <= 3 * q * 28 / 100 + 1, "rolled: %d ms for the whole chord", d[n - 1]);
        for (i = 0; i < n; i++) feel_note(FEEL_LAIDBACK, i, n, q, &rng, &d[i], &v[i]);
        CHECK(d[0] >= q * 30 / 100 && d[0] <= q * 40 / 100, "laid back: the bass %d ms late", d[0]);
        for (i = 0; i < n; i++) feel_note(FEEL_SLOPPY, i, n, q, &rng, &d[i], &v[i]);
        for (i = 0; i < n; i++) CHECK(d[i] <= q * 45 / 100 && v[i] >= 35 && v[i] <= 95, "sloppy: note %d at %d ms, %d%%", i, d[i], v[i]);
    }
    printf("%s: feel, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
