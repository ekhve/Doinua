/* SPDX-License-Identifier: GPL-3.0-only */
/* bank.c (API 31, D-085), the song's big record:
 *   - save and load records of 0 bytes, one sector, several sectors and KIT_BANK_MAX; too big is refused
 *   - a power cut at any point of a save keeps a whole record (the old one or the new one), and the bank keeps working
 *   - another firmware's data is never erased or written, through many saves and cuts; with the slot it needs taken,
 *     a save fails with KIT_BANK_ERR_FULL and changes nothing
 *   - the pacing: nothing is written until the record has stayed the same for KIT_BANK_SETTLE_MS, writes are at least
 *     KIT_BANK_GAP_MS apart, none while something plays, and an unchanged record is not written again
 * The flash model clears bits on a write (NOR) and loses power at the Nth operation, part-way through. */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

#define NS KIT_BANK_SECTORS
static uint8_t fl[NS][4096];
static int ops, cut_at = -1, erases, writes;
static uint32_t now;
static int busy;

static int cut(void) { return cut_at >= 0 && ops++ >= cut_at; }
int kit__bank_read(int s, int off, uint8_t *d, int n) { memcpy(d, &fl[s][off], (size_t)n); return 0; }
int kit__bank_erase(int s)
{
    erases++;
    if (cut()) { memset(fl[s] + 1000, 0xFF, 1000); return -1; }
    memset(fl[s], 0xFF, 4096);
    return 0;
}
int kit__bank_write_at(int s, int off, const uint8_t *src, int n)
{
    int i, m = n;
    writes++;
    if (cut()) m = n / 2;
    for (i = 0; i < m; i++) fl[s][off + i] &= src[i];
    return m == n ? 0 : -1;
}

uint32_t kit_ms(void) { return now; }
int kit__core_busy(void) { return busy; }
int kit__synth_active(void) { return 0; }
int kit_playing(void) { return 0; }
int kit_seq_playing(void) { return 0; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static uint8_t a[KIT_BANK_MAX], b[KIT_BANK_MAX], got[KIT_BANK_MAX + 100];

static void foreign(int s, uint8_t seed)
{
    int i;
    for (i = 0; i < 4096; i++) fl[s][i] = (uint8_t)(seed + i * 7);
}

static int loads(const uint8_t *want, int n)
{
    int m = kit_bank_load(got, sizeof got);
    return m == n && !memcmp(got, want, (size_t)n);
}

int main(void)
{
    int i, n, cutp, s, sizes[] = { 0, 1, 4000, 4080, 4081, 9000, KIT_BANK_MAX };
    uint8_t keep[4096];
    for (i = 0; i < KIT_BANK_MAX; i++) { a[i] = (uint8_t)(i * 13 + 1); b[i] = (uint8_t)(255 - i * 3); }

    /* empty, then each size saves and loads back, alternating slots */
    memset(fl, 0xFF, sizeof fl);
    CHECK(kit_bank_load(got, sizeof got) == 0, "an empty bank loads nothing");
    for (i = 0; i < (int)(sizeof sizes / sizeof sizes[0]); i++) {
        const uint8_t *src = i % 2 ? b : a;
        CHECK(kit__bank_write(src, sizes[i]) == 0, "save %d bytes", sizes[i]);
        CHECK(loads(src, sizes[i]), "load %d bytes back", sizes[i]);
    }
    CHECK(kit__bank_write(a, KIT_BANK_MAX + 1) == KIT_BANK_ERR_SIZE, "too big is refused");
    CHECK(kit_bank_save(a, KIT_BANK_MAX + 1) == KIT_BANK_ERR_SIZE, "too big is not queued");
    n = kit_bank_load(got, 100);
    CHECK(n == 100 && !memcmp(got, a, 100), "a short buffer gets the start");

    /* a power cut at every step of saving b over a: a or b loads, whole; then the bank works again */
    for (s = 0; s < 3; s++) {
        int size = s == 0 ? 3000 : s == 1 ? 9000 : KIT_BANK_MAX;
        for (cutp = 0; cutp < 40; cutp++) {
            memset(fl, 0xFF, sizeof fl);
            cut_at = -1;
            kit__bank_write(a, size);
            ops = 0; cut_at = cutp;
            kit__bank_write(b, size);
            cut_at = -1;
            CHECK(loads(a, size) || loads(b, size), "cut at %d (%d B): a whole record loads", cutp, size);
            CHECK(kit__bank_write(b, size) == 0 && loads(b, size), "cut at %d (%d B): the next save works", cutp, size);
            CHECK(kit__bank_write(a, size) == 0 && loads(a, size), "cut at %d (%d B): and the one after", cutp, size);
        }
    }

    /* another firmware's data: never touched; a save that needs it fails and changes nothing */
    memset(fl, 0xFF, sizeof fl);
    foreign(5, 3);
    memcpy(keep, fl[5], 4096);
    for (i = 0; i < 20; i++) {
        int rc = kit__bank_write(i % 2 ? a : b, 3000);
        CHECK(rc == 0, "small records fit beside foreign data (save %d)", i);
    }
    CHECK(!memcmp(keep, fl[5], 4096), "foreign data in a slot's second sector survives small saves");
    CHECK(kit__bank_write(a, 9000) == KIT_BANK_ERR_FULL || !memcmp(keep, fl[5], 4096), "a big save never writes it");
    CHECK(!memcmp(keep, fl[5], 4096), "foreign data still there");
    memset(fl, 0xFF, sizeof fl);
    foreign(0, 9); foreign(4, 11);
    CHECK(kit__bank_write(a, 10) == KIT_BANK_ERR_FULL, "both slots taken by others: full");
    for (i = 0; i < NS; i++) if (i != 0 && i != 4) { int k; for (k = 0; k < 4096; k++) if (fl[i][k] != 0xFF) break; CHECK(k == 4096, "nothing written to sector %d", i); }
    CHECK(kit_bank_load(got, sizeof got) == 0, "and nothing loads");
    memset(fl, 0xFF, sizeof fl);
    foreign(4, 11);
    CHECK(kit__bank_write(a, 100) == 0 && loads(a, 100), "first save beside foreign data");
    CHECK(kit__bank_write(b, 100) == KIT_BANK_ERR_FULL && loads(a, 100), "the other slot taken: full, the record stays");

    /* pacing */
    memset(fl, 0xFF, sizeof fl);
    now = 1000; erases = writes = 0;
    kit_bank_save(a, 5000);
    for (now = 1000; now < 1000 + KIT_BANK_SETTLE_MS - 10; now += 10) kit__bank_poll(now);
    CHECK(erases == 0, "nothing written before it settles");
    for (; now < 1000 + KIT_BANK_SETTLE_MS + 20; now += 10) kit__bank_poll(now);
    CHECK(erases > 0 && loads(a, 5000) && !kit_bank_pending(), "written once settled");
    erases = 0;
    kit_bank_save(b, 5000);
    { uint32_t t0 = now; for (; now < t0 + KIT_BANK_SETTLE_MS + 100; now += 10) kit__bank_poll(now); }
    CHECK(erases == 0, "not again within the gap");
    for (; now < 1000 + KIT_BANK_SETTLE_MS + KIT_BANK_GAP_MS + 100; now += 10) kit__bank_poll(now);
    CHECK(erases > 0 && loads(b, 5000), "written after the gap");
    erases = 0;
    now += KIT_BANK_GAP_MS;
    kit_bank_save(b, 5000);
    { uint32_t t0 = now; for (; now < t0 + KIT_BANK_SETTLE_MS + 100; now += 10) kit__bank_poll(now); }
    CHECK(erases == 0 && !kit_bank_pending(), "an unchanged record is not written again");
    now += KIT_BANK_GAP_MS;
    busy = 1;
    kit_bank_save(a, 5000);
    { uint32_t t0 = now; for (; now < t0 + KIT_BANK_SETTLE_MS + 500; now += 10) kit__bank_poll(now); }
    CHECK(erases == 0 && kit_bank_pending(), "nothing written while a key is held");
    busy = 0;
    kit__bank_poll(now);
    CHECK(erases > 0 && loads(a, 5000), "written once it is let go");

    printf("bank: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
