/* SPDX-License-Identifier: GPL-3.0-only */
/* The drums' styles (apps/companion/drumstyle.c, D-054), as the drum lab makes them: the main beat on its sounds' keys,
 * busy, fill, break and drop made from it; GHOSTS off leaves the ghost notes out; FILLS 4 bars plays the fill in the
 * fourth bar of each four; a roll is two hits a sixteenth, a burst three; every style the owner asked for is there. */
#include <stdio.h>
#include <string.h>
#include "../../apps/companion/drumstyle.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static int has(const ds_hit_t *h, int n, int slot, int tick) { int i; for (i = 0; i < n; i++) if (h[i].slot == slot && (tick < 0 || h[i].tick == tick)) return 1; return 0; }
static int count(const ds_hit_t *h, int n, int slot) { int i, c = 0; for (i = 0; i < n; i++) c += h[i].slot == slot; return c; }

int main(void)
{
    ds_hit_t h[96];
    int n, s, i;
    const char *want[] = { "Reggae", "Afrobeats", "Shatta", "Reggaeton", "Drum & bass", "Boom bap", "Dilla" };
    for (i = 0; i < 7; i++) {
        int found = 0;
        for (s = 0; s < DS_NSTYLES; s++) found |= !strcmp(DS_STYLES[s].name, want[i]);
        CHECK(found, "the style %s", want[i]);
    }
    n = ds_hits(0, DV_MAIN, 1, h, 96);              /* Boom bap */
    CHECK(has(h, n, DS_KICK, 0) && has(h, n, DS_KICK, 42) && has(h, n, DS_SNARE, 24) && has(h, n, DS_SNARE, 72), "boom bap: kick 1, a 7th-sixteenth kick, snares on 2 and 4");
    CHECK(count(h, n, DS_HATC) == 8 && has(h, n, DS_GHOST, 84), "eight hats and a ghost");
    for (i = 0; i < n; i++) if (h[i].slot == DS_KICK) CHECK(h[i].pad == 0, "the kick plays kick1 (F3)");
    n = ds_hits(0, DV_MAIN, 0, h, 96);
    CHECK(!has(h, n, DS_GHOST, -1), "GHOSTS off: no ghost");
    n = ds_hits(0, DV_BUSY, 1, h, 96);
    CHECK(count(h, n, DS_HATC) == 16 && has(h, n, DS_KICK, 84), "busy: the hats' gaps filled, a pickup kick");
    n = ds_hits(0, DV_FILL, 1, h, 96);
    CHECK(has(h, n, DS_TOMH, 54) && has(h, n, DS_TOML, 78) && !has(h, n, DS_HATC, 60), "fill: toms down the second half, no hats there");
    n = ds_hits(0, DV_BREAK, 1, h, 96);
    CHECK(count(h, n, DS_KICK) == 1 && !has(h, n, DS_SNARE, -1) && count(h, n, DS_HATC) == 8, "break: the hats and one kick");
    n = ds_hits(0, DV_DROP, 1, h, 96);
    CHECK(!has(h, n, DS_KICK, -1) && has(h, n, DS_SNARE, 24), "drop: all but the kick");
    for (s = 0; s < DS_NSTYLES; s++) if (!strcmp(DS_STYLES[s].name, "Trap")) break;
    n = ds_hits(s, DV_MAIN, 1, h, 96);
    CHECK(has(h, n, DS_HATC, 72) && has(h, n, DS_HATC, 75), "trap: a roll is two hits a sixteenth");
    for (s = 0; s < DS_NSTYLES; s++) if (!strcmp(DS_STYLES[s].name, "Reggaeton")) break;
    n = ds_hits(s, DV_BUSY, 1, h, 96);
    CHECK(has(h, n, DS_CLAP, 72) && has(h, n, DS_PERC, 12), "reggaeton's own busy beat");
    CHECK(ds_variant_at(DV_MAIN, 1, 3) == DV_FILL && ds_variant_at(DV_MAIN, 1, 2) == DV_MAIN && ds_variant_at(DV_MAIN, 1, 7) == DV_FILL, "FILLS 4: the fourth bar of four");
    CHECK(ds_variant_at(DV_MAIN, 2, 3) == DV_MAIN && ds_variant_at(DV_MAIN, 2, 7) == DV_FILL && ds_variant_at(DV_BUSY, 1, 3) == DV_BUSY, "FILLS 8; only the main beat fills");
    printf("%s: drum styles, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
