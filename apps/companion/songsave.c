/* SPDX-License-Identifier: GPL-3.0-only */
/* The song kept over power-off (ST-89, R-297, D-085): the sequences' notes, tempo, length and key, the song's order,
 * each track's sound, level, GRID and FEEL, the drums' pads, MY KIT 1-3, and each track's page settings, in fm1kit's bank
 * (kit_bank_*, API 31). Written as tagged chunks, so a chunk the next firmware lays out differently is left out (its
 * defaults stay) and the notes, the precious part, always come back.
 *
 *   "FMSG" | version | chunks: tag (1) | length (2, little-endian) | data
 *
 * Kept while the producer mode shows: twice a second the song is put into a record; when the record changes it goes
 * to the bank, which writes it to flash once it has stayed the same for 3 s while the song is stopped and nothing is
 * held (bank.c). So a song is kept a few seconds after STOP; while it plays, it waits. */
#define SG_VERSION 1
enum { SG_SEQ = 1, SG_ORDER, SG_TRACKS, SG_PADS, SG_MANS, SG_BASSES, SG_MELS, SG_DRPADS, SG_DRTRACKS, SG_CHORDS, SG_PAGES, SG_ARPS, SG_MYKITS, SG_DRMY, SG_DRFX };
static uint8_t sg_buf[KIT_BANK_MAX] KIT_BIG;
static int sg_len, sg_over;
static uint32_t sg_crc, sg_at;
static int sg_loaded;                           /* the song came back at power-on (for the log and the tests) */

static uint8_t *sg_p;
static int sg_room(int n) { return (int)(sg_p - sg_buf) + n <= KIT_BANK_MAX; }
static void sg_u8(int v) { if (sg_room(1)) *sg_p++ = (uint8_t)v; else sg_over = 1; }
static void sg_u16(int v) { sg_u8(v & 0xFF); sg_u8((v >> 8) & 0xFF); }
static uint8_t *sg_open(int tag) { sg_u8(tag); sg_u16(0); return sg_p; }
static void sg_close(uint8_t *start) { int n = (int)(sg_p - start); start[-2] = (uint8_t)n; start[-1] = (uint8_t)(n >> 8); }
static void sg_raw(int tag, const void *v, int n)
{
    const uint8_t *s = v;
    uint8_t *c = sg_open(tag);
    int i;
    for (i = 0; i < n; i++) sg_u8(s[i]);
    sg_close(c);
}

static int sg_build(void)                       /* the song into sg_buf; its length (0 when it does not fit) */
{
    int i, t, k;
    uint8_t *c;
    sg_p = sg_buf;
    sg_over = 0;
    sg_u8('F'); sg_u8('M'); sg_u8('S'); sg_u8('G'); sg_u8(SG_VERSION);
    for (i = 0; i < KIT_NSEQS; i++) {
        const kit_song_t *q = &seqs[i];
        int used = 0;
        for (t = 0; t < KIT_NTRACKS; t++) used += q->count[t];
        if (!used && q->bpm == seqs[0].bpm && q->bars == SONG_BARS && q->key_auto) continue;   /* untouched: left out */
        c = sg_open(SG_SEQ);
        sg_u8(i); sg_u8(q->key_auto); sg_u8(q->key_tonic); sg_u8(q->key_minor); sg_u8(q->bpm); sg_u8(q->bars); sg_u8(q->unpitched);
        for (t = 0; t < KIT_NTRACKS; t++) sg_u8(q->count[t]);
        for (t = 0; t < KIT_NTRACKS; t++)
            for (k = 0; k < q->count[t]; k++) {
                const kit_song_note_t *v = &q->notes[t][k];
                sg_u16(v->start); sg_u16(v->len); sg_u8(v->note); sg_u8(v->vel);
            }
        sg_close(c);
    }
    c = sg_open(SG_ORDER);
    sg_u8(cur_seq); sg_u8(sq_nb); sg_u8(sq_cur);
    for (i = 0; i < sq_nb; i++) { sg_u8(sq_blocks[i].seq); sg_u8(sq_blocks[i].reps); }
    sg_close(c);
    c = sg_open(SG_TRACKS);
    for (t = 0; t < KIT_NTRACKS; t++) {
        int e, p;
        sound_of(snd[t], &e, &p);
        sg_u8(e); sg_u8(p); sg_u8(kit_track_level_of(t)); sg_u8(rl_grid[t]); sg_u8(rl_feel[t]);
    }
    sg_close(c);
    sg_raw(SG_PADS, pads, (int)sizeof pads);
    sg_raw(SG_MANS, mans, (int)sizeof mans);
    sg_raw(SG_BASSES, basses, (int)sizeof basses);
    sg_raw(SG_MELS, mels, (int)sizeof mels);
    sg_raw(SG_DRPADS, dr_pads, (int)sizeof dr_pads);
    sg_raw(SG_DRTRACKS, drt, (int)sizeof drt);
    {
        int v[KIT_NTRACKS * 8];
        for (t = 0; t < KIT_NTRACKS; t++) {
            v[t * 8 + 0] = play_helps[t]; v[t * 8 + 1] = play_prog[t]; v[t * 8 + 2] = ph_lock[t]; v[t * 8 + 3] = play_feel[t];
            v[t * 8 + 4] = play_trans[t]; v[t * 8 + 5] = play_colour[t]; v[t * 8 + 6] = man_arps[t]; v[t * 8 + 7] = lights_offs[t];
        }
        sg_raw(SG_CHORDS, v, (int)sizeof v);
    }
    sg_raw(SG_PAGES, pages, (int)sizeof pages);
    sg_raw(SG_ARPS, arps, (int)sizeof arps);    /* each track's ARP (ST-77) */
    sg_raw(SG_MYKITS, dr_mykit, (int)sizeof dr_mykit);   /* MY KIT 1-3, kept with the song (R-390, D-127) */
    sg_raw(SG_DRMY, dr_my, (int)sizeof dr_my);
    sg_raw(SG_DRFX, dr_kfx, (int)sizeof dr_kfx);   /* the kit's FX (ST-97) */
    return sg_over ? 0 : (int)(sg_p - sg_buf);
}

static int sg_get(const uint8_t **p, const uint8_t *end, int n) { if (*p + n > end) return -1; *p += n; return 0; }
static int sg_rd8(const uint8_t **p) { return *(*p)++; }
static int sg_rd16(const uint8_t **p) { int v = (*p)[0] | (*p)[1] << 8; *p += 2; return v; }
static void sg_copy(void *to, int size, const uint8_t *from, int n)
{
    uint8_t *d = to;
    int i;
    if (n != size) return;                      /* laid out differently by another firmware: the defaults stay */
    for (i = 0; i < n; i++) d[i] = from[i];
}

static int sg_parse(const uint8_t *b, int n)    /* 1 when a song was taken from b */
{
    const uint8_t *p = b + 5, *end = b + n;
    int t, k;
    if (n < 5 || b[0] != 'F' || b[1] != 'M' || b[2] != 'S' || b[3] != 'G' || b[4] != SG_VERSION)
        return 0;
    while (p + 3 <= end) {
        int tag = p[0], len = p[1] | p[2] << 8;
        const uint8_t *d = p + 3, *q = d, *de = d + len;
        if (de > end) break;
        if (tag == SG_SEQ && len >= 7 + KIT_NTRACKS) {
            int i = sg_rd8(&q), cnt[KIT_NTRACKS], tot = 0;
            kit_song_t *s;
            if (i >= KIT_NSEQS) goto next;
            s = &seqs[i];
            s->key_auto = (uint8_t)sg_rd8(&q); s->key_tonic = (uint8_t)sg_rd8(&q); s->key_minor = (uint8_t)sg_rd8(&q);
            s->bpm = (uint8_t)kit_tempo_clamp(sg_rd8(&q));
            s->bars = (uint8_t)sg_rd8(&q);
            if (s->bars < 1 || s->bars > KIT_SONG_MAX_BARS) s->bars = SONG_BARS;
            s->unpitched = (uint8_t)sg_rd8(&q);
            for (t = 0; t < KIT_NTRACKS; t++) { cnt[t] = sg_rd8(&q); if (cnt[t] > KIT_SONG_MAX_NOTES) cnt[t] = 0; tot += cnt[t]; }
            if (q + tot * 6 > de) { for (t = 0; t < KIT_NTRACKS; t++) s->count[t] = 0; goto next; }
            for (t = 0; t < KIT_NTRACKS; t++) {
                for (k = 0; k < cnt[t]; k++) {
                    kit_song_note_t *v = &s->notes[t][k];
                    v->start = (uint16_t)sg_rd16(&q); v->len = (uint16_t)sg_rd16(&q); v->note = (uint8_t)sg_rd8(&q); v->vel = (uint8_t)sg_rd8(&q);
                }
                s->count[t] = (uint8_t)cnt[t];
            }
        } else if (tag == SG_ORDER && len >= 3) {
            int c = sg_rd8(&q), nb = sg_rd8(&q), cur = sg_rd8(&q), i;
            if (nb < 1 || nb > SQ_MAXB || len < 3 + nb * 2) goto next;
            for (i = 0; i < nb; i++) {
                int s = sg_rd8(&q), r = sg_rd8(&q);
                sq_blocks[i].seq = (uint8_t)(s < KIT_NSEQS ? s : 0);
                sq_blocks[i].reps = (uint8_t)(r < 1 ? 1 : r);
            }
            sq_nb = nb;
            sq_cur = cur < nb ? cur : 0;
            cur_seq = c < KIT_NSEQS ? c : 0;
        } else if (tag == SG_TRACKS && len >= KIT_NTRACKS * 5) {
            for (t = 0; t < KIT_NTRACKS; t++) {
                int e = sg_rd8(&q), pr = sg_rd8(&q), lv = sg_rd8(&q), g = sg_rd8(&q), f = sg_rd8(&q);
                if (e < kit_engine_count() && pr < kit_preset_count(e)) snd[t] = sound_index(e, pr);
                kit_track_level(t, lv);
                if (g < RL_NGRIDS) rl_grid[t] = (uint8_t)g;
                if (f < RL_NFEELS) rl_feel[t] = (uint8_t)f;
            }
        } else if (tag == SG_PADS) sg_copy(pads, (int)sizeof pads, d, len);
        else if (tag == SG_MANS) sg_copy(mans, (int)sizeof mans, d, len);
        else if (tag == SG_BASSES) sg_copy(basses, (int)sizeof basses, d, len);
        else if (tag == SG_MELS) sg_copy(mels, (int)sizeof mels, d, len);
        else if (tag == SG_DRPADS) sg_copy(dr_pads, (int)sizeof dr_pads, d, len);
        else if (tag == SG_DRTRACKS) sg_copy(drt, (int)sizeof drt, d, len);
        else if (tag == SG_PAGES) sg_copy(pages, (int)sizeof pages, d, len);
        else if (tag == SG_ARPS) sg_copy(arps, (int)sizeof arps, d, len);
        else if (tag == SG_MYKITS) sg_copy(dr_mykit, (int)sizeof dr_mykit, d, len);   /* MY KIT 1-3 (R-390) */
        else if (tag == SG_DRMY) sg_copy(dr_my, (int)sizeof dr_my, d, len);
        else if (tag == SG_DRFX) sg_copy(dr_kfx, (int)sizeof dr_kfx, d, len);
        else if (tag == SG_CHORDS && len == KIT_NTRACKS * 8 * (int)sizeof(int)) {
            int v[KIT_NTRACKS * 8];
            sg_copy(v, (int)sizeof v, d, len);
            for (t = 0; t < KIT_NTRACKS; t++) {
                play_helps[t] = v[t * 8 + 0]; play_prog[t] = v[t * 8 + 1]; ph_lock[t] = v[t * 8 + 2]; play_feel[t] = v[t * 8 + 3];
                play_trans[t] = v[t * 8 + 4]; play_colour[t] = v[t * 8 + 5]; man_arps[t] = v[t * 8 + 6]; lights_offs[t] = v[t * 8 + 7];
            }
        }
    next:
        (void)sg_get;
        p = de;
    }
    return 1;
}

static uint32_t sg_sum(const uint8_t *b, int n)  /* FNV-1a: has the record changed */
{
    uint32_t h = 2166136261u;
    int i;
    for (i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}

/* power-on: the song as it was kept, on top of app_init's defaults; then the player and the sounds follow it */
static void song_restore(void)
{
    int n = kit_bank_load(sg_buf, KIT_BANK_MAX), t, p;
    if (n <= 0 || !sg_parse(sg_buf, n))
        return;
    for (t = 0; t < KIT_NTRACKS; t++) {
        pick_sound(t);
        rl_apply(t);
        if (t == KIT_TRACK_DRUMS || set.role[t] == ROLE_DRUMS) {
            for (p = 0; p < KIT_DRUM_PADS; p++) dr_apply(t, p);
            kit_drum_fx(t, &dr_kfx[t].f);       /* the kit's FX as kept (ST-97) */
        }
    }
    update_key();
    sg_len = sg_build();
    sg_crc = sg_sum(sg_buf, sg_len);            /* what was loaded is what is kept: no write until it changes */
    sg_loaded = 1;
    kit_log("companion: song kept, loaded");
}

/* every frame in the producer mode: the record queued when the song has changed */
static void song_keep(uint32_t now)
{
    uint32_t c;
    if ((int32_t)(now - sg_at) < 500) return;
    sg_at = now;
    sg_len = sg_build();
    if (!sg_len) return;                        /* too big for the bank: the last kept song stays (D-085) */
    c = sg_sum(sg_buf, sg_len);
    if (c == sg_crc) return;
    sg_crc = c;
    kit_bank_save(sg_buf, sg_len);
}
