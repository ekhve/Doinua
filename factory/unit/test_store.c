/* SPDX-License-Identifier: GPL-3.0-only */
/* store.c's writer, kit__store_write (D-019, D-022); the pacing in front of it is test_store_pacing.c:
 *   - a power cut at any point of a save keeps a whole record (the old one, or the new one), and the store keeps
 *     working after it
 *   - another firmware's data in the candidate sectors is never erased or written, through many saves and cuts
 *   - with every candidate taken by others, a save fails with KIT_STORE_ERR_FULL and changes nothing
 *   - too big is refused; a short buffer gets the start
 * The flash model clears bits on a write (NOR) and loses power at the Nth operation, part-way through. */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

#define NS KIT_STORE_SECTORS
static uint8_t fl[NS][4096];
static int ops, cut_at = -1;            /* the operation the power fails in (-1: never) */

static int cut(void) { return cut_at >= 0 && ops++ >= cut_at; }
int kit__flash_read(int s, int off, uint8_t *d, int n) { memcpy(d, &fl[s][off], (size_t)n); return 0; }
int kit__flash_erase(int s)
{
    if (cut()) { memset(fl[s] + 1000, 0xFF, 1000); return -1; }   /* part of the sector erased */
    memset(fl[s], 0xFF, 4096);
    return 0;
}
int kit__flash_write(int s, int off, const uint8_t *src, int n)
{
    int i, m = n;
    if (cut()) m = n / 2;                                 /* half the bytes made it */
    for (i = 0; i < m; i++) fl[s][off + i] &= src[i];
    return m == n ? 0 : -1;
}

uint32_t kit_ms(void) { return 0; }
int kit__core_busy(void) { return 0; }
int kit__synth_active(void) { return 0; }
int kit_playing(void) { return 0; }
int kit_seq_playing(void) { return 0; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static void foreign(int s, uint8_t seed)                  /* another firmware's sector: anything but blank or ours */
{
    int i;
    for (i = 0; i < 4096; i++) fl[s][i] = (uint8_t)(seed + i * 7);
}

int main(void)
{
    uint8_t a[300], b[300], got[400], keep0[4096], keep2[4096];
    int cutp, n, i, s;
    for (i = 0; i < 300; i++) { a[i] = (uint8_t)i; b[i] = (uint8_t)(255 - i); }

    /* empty, save, load */
    memset(fl, 0xFF, sizeof fl);
    CHECK(kit_store_load(got, sizeof got) == 0, "an empty store loads nothing");
    CHECK(kit__store_write(a, 300) == 0, "save a");
    n = kit_store_load(got, sizeof got);
    CHECK(n == 300 && !memcmp(got, a, 300), "load a back");

    /* a power cut at every operation of saving b, then the store still works */
    for (cutp = 0; cutp < 6; cutp++) {
        uint8_t was[NS][4096];
        memcpy(was, fl, sizeof fl);
        ops = 0; cut_at = cutp;
        kit__store_write(b, 300);
        cut_at = -1;
        n = kit_store_load(got, sizeof got);
        CHECK(n == 300 && (!memcmp(got, a, 300) || !memcmp(got, b, 300)), "cut at op %d: a whole record survives (n %d)", cutp, n);
        CHECK(kit__store_write(b, 300) == 0, "cut at op %d: the next save works", cutp);
        n = kit_store_load(got, sizeof got);
        CHECK(n == 300 && !memcmp(got, b, 300), "cut at op %d: and loads", cutp);
        memcpy(fl, was, sizeof fl);
    }

    /* other firmwares' data in sectors 0 and 2: never touched, through saves and cuts */
    memset(fl, 0xFF, sizeof fl);
    foreign(0, 3); foreign(2, 99);
    memcpy(keep0, fl[0], 4096); memcpy(keep2, fl[2], 4096);
    for (i = 0; i < 12; i++) {
        if (i % 4 == 3) { ops = 0; cut_at = i % 5; }
        kit__store_write(i & 1 ? b : a, 300);
        cut_at = -1;
    }
    CHECK(!memcmp(keep0, fl[0], 4096) && !memcmp(keep2, fl[2], 4096), "foreign sectors are never erased or written");
    CHECK(kit__store_write(a, 300) == 0, "with two sectors free, saves work around the foreign ones");
    n = kit_store_load(got, sizeof got);
    CHECK(n == 300 && !memcmp(got, a, 300), "and load");

    /* every candidate taken by others: refuse, change nothing */
    memset(fl, 0xFF, sizeof fl);
    for (s = 0; s < NS; s++) foreign(s, (uint8_t)(s * 31 + 1));
    {
        uint8_t was[NS][4096];
        memcpy(was, fl, sizeof fl);
        CHECK(kit__store_write(a, 300) == KIT_STORE_ERR_FULL, "all foreign: the save says FULL");
        CHECK(!memcmp(was, fl, sizeof fl), "all foreign: nothing changed");
        CHECK(kit_store_load(got, sizeof got) == 0, "all foreign: nothing loads");
    }

    /* one blank sector only (the rest foreign): the first save works; the next has nowhere else to go */
    memset(fl, 0xFF, sizeof fl);
    for (s = 1; s < NS; s++) foreign(s, (uint8_t)(s * 13));
    CHECK(kit__store_write(a, 300) == 0, "one blank sector: the first save works");
    CHECK(kit__store_write(b, 300) == KIT_STORE_ERR_FULL, "one blank sector: a second save would overwrite the only record, so it says FULL");
    n = kit_store_load(got, sizeof got);
    CHECK(n == 300 && !memcmp(got, a, 300), "one blank sector: the first record still loads");

    /* newest of many; limits */
    memset(fl, 0xFF, sizeof fl);
    for (i = 0; i < 7; i++)
        CHECK(kit__store_write(i & 1 ? b : a, 300) == 0, "save %d", i);
    n = kit_store_load(got, sizeof got);
    CHECK(n == 300 && !memcmp(got, a, 300), "the newest of seven saves loads");
    for (s = 0, n = 0; s < NS; s++) n += fl[s][0] == 'F';
    CHECK(n == 2, "the store keeps to two sectors (uses %d)", n);
    CHECK(kit__store_write(a, KIT_STORE_MAX + 1) == KIT_STORE_ERR_SIZE, "too big is refused");
    n = kit_store_load(got, 10);
    CHECK(n == 10 && !memcmp(got, a, 10), "a short buffer gets the start");

    printf("%s: store, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
