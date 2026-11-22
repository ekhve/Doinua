/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit bank: one big record (up to KIT_BANK_MAX bytes, the companion's song) that survives power-off (API 31, D-085).
 * Portable: the backend gives three primitives over KIT_BANK_SECTORS 4 KiB sectors (kit__bank_*); the format is here,
 * the same on the FM-1 and the host (kit__bank_read, kit__bank_erase, kit__bank_write_at). The settings store (store.c) is its small sibling and works the same way.
 *
 *   two slots of KIT_BANK_SECTORS / 2 sectors; a record is written to the slot the newest record is not in, in as many
 *   sectors (parts) as it needs, each sector: [header 16 B][up to PART bytes of the record]
 *   header: magic "FKSG" | sequence | part | parts | record length (2) | CRC-16 of the part | CRC-16 of bytes 0..13
 *
 * Other firmwares' data (D-022, D-085, Decision #115). The bank's flash (0x97000..0x9EFFF on the FM-1) is shared with
 * other FM-1 firmwares' project areas. As the store does (Optimist's "keep what you cannot read"), the bank writes a
 * sector only if it is blank (all 0xFF) or starts with our magic; if the slot it needs holds anything else, the save
 * fails with KIT_BANK_ERR_FULL and nothing is touched.
 *
 * Power cuts. The newest record's slot is never written, so it stays whole until the new one is complete. Each part is
 * written magic first, then its data, then the rest of its header, so a cut leaves sectors that are ours (and
 * reclaimable) and a record that does not load (a part missing or failing its CRC); the previous record loads.
 * factory/unit/test_bank.c cuts the power at every step of a save and puts foreign data in the way.
 *
 * Pacing. kit_bank_save only queues the record; the main loop's kit__bank_poll writes it once it has stayed the same
 * for KIT_BANK_SETTLE_MS, at least KIT_BANK_GAP_MS after the last write, while nothing plays or is held (erasing stops
 * the CPU, and the sound, for a moment), and not at all if it equals the newest record. */
#include "kit_internal.h"

#define MAGIC 0x47534B46u           /* "FKSG" */
#define HDR 16
#define SECTOR 4096
#define PART (SECTOR - HDR)
#define SLOT (KIT_BANK_SECTORS / 2)

static uint16_t crc16_upd(uint16_t c, const uint8_t *d, int n)
{
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

static uint8_t tmp[256];            /* (in RAM: the flash driver reads and writes RAM only) */
static uint8_t pend[KIT_BANK_MAX] KIT_BIG;   /* the record queued, in the RAM pool */

enum { OTHER = 0, BLANK, OURS };    /* what a sector holds: not ours, all 0xFF, starts with our magic */

static int kind(int sector)
{
    int off, i;
    if (kit__bank_read(sector, 0, tmp, 4))
        return OTHER;
    if (rd32(tmp) == MAGIC)
        return OURS;
    for (i = 0; i < 4; i++)         /* blank but for part of our magic: a cut while the magic was written (ours) */
        if (tmp[i] != 0xFF && tmp[i] != (uint8_t)(MAGIC >> (8 * i)))
            return OTHER;
    for (off = 0; off < SECTOR; off += (int)sizeof tmp) {
        if (kit__bank_read(sector, off, tmp, (int)sizeof tmp))
            return OTHER;
        for (i = off ? 0 : 4; i < (int)sizeof tmp; i++)
            if (tmp[i] != 0xFF)
                return OTHER;
    }
    return BLANK;
}

static int parts_of(int len) { return len <= 0 ? 1 : (len + PART - 1) / PART; }

/* the header of a slot's part p, checked: its sequence, parts and length; 0 when it is not a valid header */
static int header(int slot, int p, uint32_t *seq, int *parts, int *len, uint16_t *crc)
{
    uint8_t h[HDR];
    int i;
    if (kit__bank_read(slot * SLOT + p, 0, h, HDR) || rd32(h) != MAGIC)
        return 0;
    if ((h[14] | h[15] << 8) != crc16_upd(0xFFFF, h, 14) || h[8] != p)
        return 0;
    *seq = rd32(h + 4);
    *parts = h[9];
    *len = h[10] | h[11] << 8;
    *crc = (uint16_t)(h[12] | h[13] << 8);
    if (*parts < 1 || *parts > SLOT || *len > KIT_BANK_MAX || parts_of(*len) != *parts)
        return 0;
    (void)i;
    return 1;
}

/* the slot's record, checked through: its sequence and length if every part is there and whole */
static int valid(int slot, uint32_t *seq, int *len)
{
    uint32_t s0, s;
    int n0, parts0, p, parts, n, off, k;
    uint16_t crc, c;
    if (!header(slot, 0, &s0, &parts0, &n0, &crc))
        return 0;
    for (p = 0; p < parts0; p++) {
        if (p && (!header(slot, p, &s, &parts, &n, &crc) || s != s0 || parts != parts0 || n != n0))
            return 0;
        if (!p && !header(slot, 0, &s, &parts, &n, &crc))
            return 0;
        k = n0 - p * PART;
        if (k > PART) k = PART;
        c = 0xFFFF;
        for (off = 0; off < k; off += (int)sizeof tmp) {
            int m = k - off < (int)sizeof tmp ? k - off : (int)sizeof tmp;
            if (kit__bank_read(slot * SLOT + p, HDR + off, tmp, m))
                return 0;
            c = crc16_upd(c, tmp, m);
        }
        if (c != crc)
            return 0;
    }
    *seq = s0;
    *len = n0;
    return 1;
}

static int newest(uint32_t *seq, int *len)  /* the slot of the newest whole record, or -1 */
{
    int k, best = -1;
    for (k = 0; k < 2; k++) {
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

int kit_bank_load(void *dst, int len)
{
    uint32_t seq = 0;
    int n = 0, s = newest(&seq, &n), p, k;
    uint8_t *d = dst;
    if (s < 0 || len <= 0)
        return 0;
    if (n > len)
        n = len;
    for (p = 0; p * PART < n; p++) {
        k = n - p * PART;
        if (k > PART) k = PART;
        if (kit__bank_read(s * SLOT + p, HDR, d + p * PART, k))
            return 0;
    }
    return n;
}

int kit__bank_write(const void *src, int len)
{
    uint32_t seq = 0;
    int n = 0, s, slot, parts = parts_of(len), p, k;
    const uint8_t *d = src;
    uint8_t h[HDR];
    uint16_t c;
    if (len < 0 || len > KIT_BANK_MAX)
        return KIT_BANK_ERR_SIZE;
    s = newest(&seq, &n);
    slot = s < 0 ? -1 : 1 - s;                  /* never the slot of the newest record */
    if (slot < 0) {                             /* none yet: the first slot we can use */
        for (k = 0; k < 2 && slot < 0; k++) {
            for (p = 0; p < parts && kind(k * SLOT + p) != OTHER; p++) ;
            if (p == parts) slot = k;
        }
        if (slot < 0)
            return KIT_BANK_ERR_FULL;
    }
    for (p = 0; p < parts; p++)                 /* every sector it needs: blank or ours, or nothing is touched */
        if (kind(slot * SLOT + p) == OTHER)
            return KIT_BANK_ERR_FULL;
    for (p = 0; p < parts; p++) {
        int sec = slot * SLOT + p;
        k = len - p * PART;
        if (k > PART) k = PART;
        if (k < 0) k = 0;
        if (kit__bank_erase(sec))
            return KIT_BANK_ERR_FLASH;
        for (c = 0; c < HDR; c++)
            h[c] = 0xFF;
        wr32(h, MAGIC);
        if (kit__bank_write_at(sec, 0, h, 4))      /* the magic first: from here on the sector is recognisably ours */
            return KIT_BANK_ERR_FLASH;
        if (k && kit__bank_write_at(sec, HDR, d + p * PART, k))
            return KIT_BANK_ERR_FLASH;
        wr32(h + 4, s < 0 ? 1u : seq + 1u);
        h[8] = (uint8_t)p;
        h[9] = (uint8_t)parts;
        h[10] = (uint8_t)len; h[11] = (uint8_t)(len >> 8);
        c = crc16_upd(0xFFFF, d + p * PART, k);
        h[12] = (uint8_t)c; h[13] = (uint8_t)(c >> 8);
        c = crc16_upd(0xFFFF, h, 14);
        h[14] = (uint8_t)c; h[15] = (uint8_t)(c >> 8);
        if (kit__bank_write_at(sec, 4, h + 4, HDR - 4))   /* the rest of the header last: until then it doesn't load */
            return KIT_BANK_ERR_FLASH;
    }
    return 0;
}

/* ---- pacing ---- */
static int pend_len = -1;               /* -1: nothing queued */
static uint32_t pend_ms, last_ms;
static int wrote_once, last_rc;

int kit_bank_save(const void *src, int len)
{
    const uint8_t *p = src;
    int i;
    if (len < 0 || len > KIT_BANK_MAX)
        return KIT_BANK_ERR_SIZE;
    for (i = 0; i < len; i++)
        pend[i] = p[i];
    pend_len = len;
    pend_ms = kit_ms();
    return 0;
}

int kit_bank_pending(void) { return pend_len >= 0; }
int kit_bank_status(void) { return last_rc; }

static int same_as_newest(void)
{
    uint32_t seq = 0;
    int n = 0, s = newest(&seq, &n), off, m, i;
    if (s < 0 || n != pend_len)
        return 0;
    for (off = 0; off < n; off += m) {
        int p = off / PART, in = off % PART;
        m = PART - in;
        if (m > (int)sizeof tmp) m = (int)sizeof tmp;
        if (m > n - off) m = n - off;
        if (kit__bank_read(s * SLOT + p, HDR + in, tmp, m))
            return 0;
        for (i = 0; i < m; i++)
            if (tmp[i] != pend[off + i])
                return 0;
    }
    return 1;
}

/* the main loop, every pass: write the queued record when it is due (backends call it; never from an interrupt) */
void kit__bank_poll(uint32_t now)
{
    if (pend_len < 0 || (int32_t)(now - pend_ms) < (int32_t)KIT_BANK_SETTLE_MS)
        return;
    if (wrote_once && (int32_t)(now - last_ms) < (int32_t)KIT_BANK_GAP_MS)
        return;
    if (kit__core_busy() || kit__synth_active() || kit_playing() || kit_seq_playing())
        return;                         /* something plays or is held: the erase would be heard */
    if (!same_as_newest()) {
        last_rc = kit__bank_write(pend, pend_len);
        last_ms = now;
        wrote_once = 1;
        if (last_rc == KIT_BANK_ERR_FLASH)
            return;                     /* keep it queued: the next try is a gap later */
    }
    pend_len = -1;
}
