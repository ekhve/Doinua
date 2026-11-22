/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit store: one record of up to KIT_STORE_MAX bytes that survives power-off. Portable: the backend gives
 * three primitives over KIT_STORE_SECTORS candidate 4 KiB sectors (kit__flash_*); the format is here, the same on
 * the FM-1 and the host.
 *
 *   sector: [header 16 B][data len B]
 *   header: magic "FKST" | sequence | length | CRC-16 of the data | CRC-16 of bytes 0..9 | 0xFF 0xFF 0xFF 0xFF
 *
 * Other firmwares' data (D-022). The candidate sectors are shared flash: another firmware may have data there. As
 * Optimist does (its "keep what you cannot read", after it measured its first start erasing SLOOP's projects), this
 * store never erases a sector it did not write: only a blank one (all 0xFF) or one that starts with our magic. If no
 * candidate is free, a save fails and nothing is touched.
 *
 * Power cuts. A save goes to a sector other than the one holding the newest record, so the previous record stays
 * whole. After the erase, the magic is written first, then the data, then the rest of the header: a cut at any point
 * after the magic leaves a sector that is recognisably ours (and reclaimable), with CRCs that fail (so it is not
 * loaded). factory/unit/test_store.c cuts the power at every step of a save and puts foreign data in the way.
 *
 * Pacing (D-023, as Felucca's autosave and Optimist pace theirs). kit_store_save only queues the record; the main
 * loop's kit__store_poll writes it once it has stayed the same for KIT_STORE_SETTLE_MS, at least KIT_STORE_GAP_MS
 * after the last write, and while nothing plays (an erase stops the CPU, and the sound, for tens of ms), and not at
 * all if it equals the stored record. So an app that saves every frame still writes flash at most once a minute,
 * and a power-off loses at most the changes of the last minute. factory/unit/test_store_pacing.c checks it. */
#include "kit_internal.h"

#define MAGIC 0x54534B46u           /* "FKST" */
#define HDR 16
#define SECTOR 4096

static uint16_t crc16(const uint8_t *d, int n)
{
    uint16_t c = 0xFFFF;
    int i, b;
    for (i = 0; i < n; i++) {
        c ^= (uint16_t)(d[i] << 8);
        for (b = 0; b < 8; b++)
            c = (uint16_t)(c & 0x8000 ? (c << 1) ^ 0x1021 : c << 1);
    }
    return c;
}

static uint32_t rd32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

static uint8_t buf[KIT_STORE_MAX];  /* (in RAM: the flash driver reads and writes RAM only) */

enum { OTHER = 0, BLANK, OURS };    /* what a sector holds: not ours, all 0xFF, starts with our magic */

static int kind(int sector)
{
    uint8_t h[4];
    int off, i;
    if (kit__flash_read(sector, 0, h, 4))
        return OTHER;
    if (rd32(h) == MAGIC)
        return OURS;
    /* blank, but for part of our magic at the start: a power cut while the magic was being written (still ours) */
    for (i = 0; i < 4; i++)
        if (h[i] != 0xFF && h[i] != (uint8_t)(MAGIC >> (8 * i)))
            return OTHER;
    for (off = 0; off < SECTOR; off += KIT_STORE_MAX) {
        if (kit__flash_read(sector, off, buf, KIT_STORE_MAX))
            return OTHER;
        for (i = off ? 0 : 4; i < KIT_STORE_MAX; i++)
            if (buf[i] != 0xFF)
                return OTHER;
    }
    return BLANK;
}

/* the sector's record: its sequence and length if valid (both CRCs), else 0; its data is left in buf */
static int valid(int sector, uint32_t *seq, int *len)
{
    uint8_t h[HDR];
    int n;
    if (kit__flash_read(sector, 0, h, HDR))
        return 0;
    if (rd32(h) != MAGIC || (h[12] | h[13] << 8) != crc16(h, 10))
        return 0;
    n = h[8] | h[9] << 8;
    if (n > KIT_STORE_MAX || kit__flash_read(sector, HDR, buf, n))
        return 0;
    if ((h[10] | h[11] << 8) != crc16(buf, n))
        return 0;
    *seq = rd32(h + 4);
    *len = n;
    return 1;
}

static int newest(uint32_t *seq, int *len)  /* the sector of the newest valid record, or -1 */
{
    int k, best = -1;
    for (k = 0; k < KIT_STORE_SECTORS; k++) {
        uint32_t s;
        int n;
        if (valid(k, &s, &n) && (best < 0 || (int32_t)(s - *seq) > 0)) {
            best = k;
            *seq = s;
            *len = n;
        }
    }
    return best;
}

int kit_store_load(void *dst, int len)
{
    uint32_t seq = 0;
    int n = 0, s = newest(&seq, &n), i;
    uint8_t *d = dst;
    if (s < 0 || len <= 0)
        return 0;
    if (!valid(s, &seq, &n))                /* (buf holds the newest record's data again) */
        return 0;
    if (n > len)
        n = len;
    for (i = 0; i < n; i++)
        d[i] = buf[i];
    return n;
}

int kit__store_write(const void *src, int len)
{
    uint32_t seq = 0;
    int n = 0, s, target = -1, k, i;
    const uint8_t *p = src;
    uint8_t h[HDR];
    if (len < 0 || len > KIT_STORE_MAX)
        return KIT_STORE_ERR_SIZE;
    s = newest(&seq, &n);
    for (k = 0; k < KIT_STORE_SECTORS && target < 0; k++)   /* first choice: a sector of ours, not the newest */
        if (k != s && kind(k) == OURS)
            target = k;
    for (k = 0; k < KIT_STORE_SECTORS && target < 0; k++)   /* then a blank one */
        if (k != s && kind(k) == BLANK)
            target = k;
    if (target < 0)
        return KIT_STORE_ERR_FULL;          /* only other firmwares' data left: touch none of it */
    for (i = 0; i < len; i++)
        buf[i] = p[i];
    if (kit__flash_erase(target))
        return KIT_STORE_ERR_FLASH;
    for (i = 0; i < HDR; i++)
        h[i] = 0xFF;
    wr32(h, MAGIC);
    if (kit__flash_write(target, 0, h, 4))  /* the magic first: from here on the sector is recognisably ours */
        return KIT_STORE_ERR_FLASH;
    if (len && kit__flash_write(target, HDR, buf, len))
        return KIT_STORE_ERR_FLASH;
    wr32(h + 4, s < 0 ? 1u : seq + 1u);
    h[8] = (uint8_t)len; h[9] = (uint8_t)(len >> 8);
    {
        uint16_t c = crc16(buf, len);
        h[10] = (uint8_t)c; h[11] = (uint8_t)(c >> 8);
    }
    {
        uint16_t c = crc16(h, 10);
        h[12] = (uint8_t)c; h[13] = (uint8_t)(c >> 8);
    }
    if (kit__flash_write(target, 4, h + 4, HDR - 4))   /* the rest of the header last: until then it doesn't load */
        return KIT_STORE_ERR_FLASH;
    return 0;
}

/* ---- pacing ---- */
static uint8_t pend[KIT_STORE_MAX];
static int pend_len = -1;               /* -1: nothing queued */
static uint32_t pend_ms, last_ms;
static int wrote_once, last_rc;

int kit_store_save(const void *src, int len)
{
    const uint8_t *p = src;
    int i;
    if (len < 0 || len > KIT_STORE_MAX)
        return KIT_STORE_ERR_SIZE;
    for (i = 0; i < len; i++)
        pend[i] = p[i];
    pend_len = len;
    pend_ms = kit_ms();
    return 0;
}

int kit_store_pending(void) { return pend_len >= 0; }
int kit_store_status(void) { return last_rc; }

/* the main loop, every pass: write the queued record when it is due (backends call it; never from an interrupt) */
void kit__store_poll(uint32_t now)
{
    uint32_t seq = 0;
    int n = 0, s, i, same;
    /* signed differences: the caller's `now` can be a little older than a save made later in the same loop pass,
     * and an unsigned now - pend_ms would then wrap to ~4e9 and look long past (D-018's bug, the other way round) */
    if (pend_len < 0 || (int32_t)(now - pend_ms) < (int32_t)KIT_STORE_SETTLE_MS)
        return;
    if (wrote_once && (int32_t)(now - last_ms) < (int32_t)KIT_STORE_GAP_MS)
        return;
    if (kit__core_busy() || kit__synth_active() || kit_playing() || kit_seq_playing())
        return;                         /* something plays or is held: the erase would be heard */
    s = newest(&seq, &n);
    same = s >= 0 && valid(s, &seq, &n) && n == pend_len;
    for (i = 0; same && i < n; i++)
        same = buf[i] == pend[i];
    if (!same) {
        last_rc = kit__store_write(pend, pend_len);
        last_ms = now;
        wrote_once = 1;
        if (last_rc == KIT_STORE_ERR_FLASH)
            return;                     /* keep it queued: the next try is a gap later */
    }
    pend_len = -1;
}
