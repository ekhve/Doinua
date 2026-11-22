/* SPDX-License-Identifier: GPL-3.0-only */
/* store.c pacing (D-023, R-039..R-042), on a simulated clock:
 *   - an app saving every 10 ms for 10 minutes erases flash at most once a minute, and the last record wins
 *   - nothing is written until the record has stayed the same for 2 s
 *   - nothing is written while a key is held or a voice sounds; it is written once they stop
 *   - a record equal to the stored one is never written */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

static uint8_t fl[KIT_STORE_SECTORS][4096];
static int erases;
static uint32_t now;
static int busy, sounding;

uint32_t kit_ms(void) { return now; }
int kit__core_busy(void) { return busy; }
int kit__synth_active(void) { return sounding; }
int kit_playing(void) { return 0; }
int kit_seq_playing(void) { return 0; }
int kit__flash_read(int s, int off, uint8_t *d, int n) { memcpy(d, &fl[s][off], (size_t)n); return 0; }
int kit__flash_erase(int s) { erases++; memset(fl[s], 0xFF, 4096); return 0; }
int kit__flash_write(int s, int off, const uint8_t *src, int n)
{
    int i;
    for (i = 0; i < n; i++) fl[s][off + i] &= src[i];
    return 0;
}

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static void run(uint32_t ms)                       /* the main loop for ms, polled every ms */
{
    uint32_t end = now + ms;
    while (now < end) { now++; kit__store_poll(now); }
}

int main(void)
{
    uint8_t rec[16], got[16];
    uint32_t i;
    memset(fl, 0xFF, sizeof fl);

    /* an app saving a new record every 10 ms for 10 minutes */
    for (i = 0; i < 60000u; i++) {
        memset(rec, 0, sizeof rec);
        memcpy(rec, &i, sizeof i);
        CHECK(kit_store_save(rec, sizeof rec) == 0, "save queues");
        run(10);
    }
    CHECK(erases == 0, "saving without pause never settles: no write yet (%d erases)", erases);
    run(2100);
    CHECK(erases == 1, "2 s after the last save it is written once (%d erases)", erases);
    CHECK(kit_store_load(got, sizeof got) == 16 && !memcmp(got, rec, 16), "the last record saved is the one stored");
    CHECK(!kit_store_pending(), "nothing left queued");

    /* the gap: a new record 10 s later waits until 60 s after the last write */
    run(10000);
    rec[0] ^= 0xFF;
    kit_store_save(rec, sizeof rec);
    run(30000);
    CHECK(erases == 1, "within 60 s of the last write: still not written (%d)", erases);
    run(25000);
    CHECK(erases == 2, "60 s after the last write: written (%d)", erases);

    /* a key held or a voice sounding holds the write back */
    run(61000);
    rec[1] ^= 0xFF;
    kit_store_save(rec, sizeof rec);
    busy = 1;
    run(5000);
    CHECK(erases == 2, "while a key is held: not written (%d)", erases);
    busy = 0; sounding = 1;
    run(5000);
    CHECK(erases == 2, "while a voice sounds: not written (%d)", erases);
    sounding = 0;
    run(10);
    CHECK(erases == 3, "once quiet: written (%d)", erases);

    /* the device's order: the loop reads `now`, then the app saves later in the same pass (kit_ms() a ms later) */
    run(61000);
    rec[2] ^= 0xFF;
    now++;                                         /* the save sees the next millisecond ... */
    kit_store_save(rec, sizeof rec);
    kit__store_poll(now - 1);                      /* ... the poll still has the older one */
    CHECK(erases == 3, "a poll with an older 'now' than the save: not written early (%d)", erases);
    run(2100);
    CHECK(erases == 4, "and written 2 s later (%d)", erases);

    /* an unchanged record is not written */
    run(61000);
    kit_store_save(rec, sizeof rec);
    run(3000);
    CHECK(erases == 4, "the same record again: no write (%d)", erases);
    CHECK(!kit_store_pending(), "and nothing left queued");

    CHECK(kit_store_save(rec, KIT_STORE_MAX + 1) == KIT_STORE_ERR_SIZE, "too big is refused at once");
    printf("%s: store pacing, %d failed (%d erases in %u simulated s)\n", fails ? "FAIL" : "PASS", fails, erases, now / 1000u);
    return fails != 0;
}
