/* SPDX-License-Identifier: GPL-3.0-only */
/* seqsong.c: the sequencer's SONG page (ST-85, D-081, R-274..R-282), as the owner tried it in the Sequencer Lab.
 * app.c includes it after chauto.c; its 16 sequences are app.c's seqs[] (the song being the one playing).
 *
 * SEQ opens the sequencer's pages (SONG first; SEQ again steps them), HOME goes back to the track. On SONG:
 *   - the 16 white keys are the 16 sequences (R-274): lit when they hold notes, dark when empty (R-280), the one
 *     playing full;
 *   - stopped, a lit key adds that sequence to the song after the cursor's block (R-275); playing, it plays that
 *     sequence from the next bar (R-276); a lit key held and a dark key pressed copies it there (R-277);
 *   - EDIT and a lit key ask "Delete sequence n?" (R-278); EDIT and the same key again within 3 s delete it and its
 *     blocks (R-279); EDIT tapped alone removes the cursor's block;
 *   - K1 the cursor, K2 the block's repeats (drawn as dashed copies, R-281), K3 moves the block, K4 the sequence it
 *     plays; PLAY plays the song from the cursor's block, each block its repeats, then the next, round to the top.
 * A new song is sequence 1, empty, as its only block (R-282). */

#define SQ_MAXB 64
static struct { uint8_t seq, reps; } sq_blocks[SQ_MAXB];
static int sq_nb, sq_cur;                       /* the blocks; the cursor */
static int sq_view = -1;                        /* -1: the track pages; 0: SONG (ST-86..ST-88 add the others) */
enum { SQV_SONG = 0, SQV_SEQ, SQV_STEPS, SQV_N };   /* SONG (ST-85), SEQUENCE (ST-86), STEPS (ST-87); MOTION follows */
static int sq_songplay, sq_pblock, sq_prep;     /* playing the song's order: the block playing, its repeat */
static int sq_queued = -1;                      /* a sequence pressed while playing (R-276) */
static int sq_del = -1;                         /* the sequence the delete question is about */
static uint32_t sq_del_at, sq_msg_until;
static const char *sq_msg;
static char sq_msgbuf[40];
static int sq_edit_used;
static int sq_add_key = -1;                    /* a lit key pressed: its block is added when it is let go, unless it copied */

static int sq_white_index(int key)              /* the 16 white keys F3..G5: sequences 0..15 (-1: a black key) */
{
    int k, w = 0;
    if (key < 0 || key >= KIT_NKEYS || !kit_is_white(KIT_KEY0_NOTE + key)) return -1;
    for (k = 0; k < key; k++) if (kit_is_white(KIT_KEY0_NOTE + k)) w++;
    return w < KIT_NSEQS ? w : -1;
}

static int sq_used(int s)
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++) if (seqs[s].count[t]) return 1;
    return 0;
}

static void sq_say(const char *m) { sq_msg = m; sq_msg_until = kit_ms() + 1100; if (!sq_msg_until) sq_msg_until = 1; dirty = 1; }
static void sq_sayn(const char *a, int n, const char *b)   /* "sequence 3 deleted" */
{
    char *d = cat(sq_msgbuf, a);
    if (n >= 0) d = cat_int(d, n);
    cat(d, b);
    sq_say(sq_msgbuf);
}

/* ---- drawn as the Sequencer Lab draws them (its draw(): the owner's contract, D-084) */
#define SQ_ACC KIT_RGB(77, 141, 255)
#define SQ_ACCD KIT_RGB(29, 58, 110)
#define SQ_CARD KIT_RGB(22, 27, 36)
#define SQ_GRID KIT_RGB(28, 34, 48)
#define SQ_MUTE KIT_RGB(127, 135, 150)
#define SQ_USED KIT_RGB(42, 79, 143)
#define SQ_EMPTY KIT_RGB(17, 21, 29)
#define SQ_EMPTYT KIT_RGB(58, 65, 80)
static const kit_color_t SQ_TC[KIT_NTRACKS] = { KIT_RGB(77, 141, 255), KIT_RGB(242, 163, 60), KIT_RGB(60, 207, 122), KIT_RGB(215, 124, 194) };
static int sq_knob_i = -1;                      /* the knob turned last: its card lit for a moment */
static uint32_t sq_knob_until;
static void sq_turned(int id) { sq_knob_i = id - KIT_ENC_KNOB1; sq_knob_until = kit_ms() + 900; if (!sq_knob_until) sq_knob_until = 1; }

static kit_color_t sq_mix(kit_color_t c, kit_color_t b, int a)   /* c over b, a/100 of c */
{
    int r = (((c >> 11) & 31) * a + ((b >> 11) & 31) * (100 - a) + 50) / 100;
    int g = (((c >> 5) & 63) * a + ((b >> 5) & 63) * (100 - a) + 50) / 100;
    int bl = ((c & 31) * a + (b & 31) * (100 - a) + 50) / 100;
    return (kit_color_t)((r << 11) | (g << 5) | bl);
}

/* while the delete pop-up shows, the page under it is drawn dimmed (the lab's shade over it): from here on in this
 * file the drawing calls take their colour through SQD */
static int sq_dimmed;
#define SQD(c) (sq_dimmed ? sq_mix((c), BG, 40) : (c))
#define kit_rect(x, y, w, h, c) kit_rect(x, y, w, h, SQD(c))
#define kit_round_rect(x, y, w, h, r, c) kit_round_rect(x, y, w, h, r, SQD(c))
#define kit_rect_outline(x, y, w, h, t, c) kit_rect_outline(x, y, w, h, t, SQD(c))
#define kit_circle(x, y, r, c) kit_circle(x, y, r, SQD(c))
#define kit_text(x, y, s, f, a, c) kit_text(x, y, s, f, a, SQD(c))
#define ctext(x, y, s, f, c) ctext(x, y, s, f, SQD(c))

static void sq_tri(int x, int cy, int sz, int dir, kit_color_t c)   /* a filled triangle pointing left (-1) or right (1) */
{
    int i;
    for (i = 0; i <= sz; i++) {
        int h = dir > 0 ? sz - i : i;
        kit_rect(x + i, cy - h, 1, 2 * h + 1, c);
    }
}

/* the top line: the page, then the sequence and track; the tempo on the right, red with a dot while REC is on,
 * a triangle while playing; the track's colour along the top */
static void sq_top(const char *page)
{
    char v[32], r[32], *d;
    int w, x, xl, wr, n;
    kit_rect(0, 0, KIT_W, 2, SQ_TC[track]);
    w = kit_text(6, 5, page, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, FG);
    x = kit_seq_playing() ? 224 : 234;
    if (kit_seq_playing()) sq_tri(228, 12, 4, 1, armed ? REC : SQ_MUTE);
    d = cat_int(r, song.bpm);
    if ((sq_view == SQV_SEQ || sq_view == SQV_STEPS) && !is_drums()) {   /* the keys play notes here: their octave (the owner, 2026-11-21) */
        d = cat(d, octave > 0 ? "   Oct +" : "   Oct ");
        cat_int(d, octave);
    }
    wr = kit_text(x, 5, r, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, armed ? REC : SQ_MUTE);
    if (armed) { kit_circle(x - wr - 7, 12, 3, REC); wr += 12; }
    if (sq_view != SQV_SONG) {                  /* the sequence and track; the track's name shortened when it would touch the right side */
        xl = 6 + w + 10 > 72 ? 6 + w + 10 : 72;
        d = cat_int(cat(v, "SEQ "), cur_seq + 1);
        cat(cat(d, "  "), track_name(track));
        n = 0; while (v[n]) n++;
        while (n > 6 && xl + kit_text_width(v, KIT_FONT_XSMALL) > x - wr - 6) v[--n] = 0;
        kit_text(xl, 5, v, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, SQ_MUTE);
    }
}

/* a knob's card, at the lab's place; a value with '|' shows a middle dot there, "<>" two arrows, "-" is quiet */
static void sq_card(int i, const char *label, const char *value)
{
    int x = 4 + i * 58, lit = sq_knob_i == i && sq_knob_until;
    exp_card(i, label, value, 0);
    kit_round_rect(x, 22, 54, 28, 5, lit ? SQ_ACCD : SQ_CARD);
    ctext(x + 27, 30, label, KIT_FONT_TINY, SQ_MUTE);
    if (value[0] == '<' && value[1] == '>') {
        sq_tri(x + 17, 42, 4, -1, SQ_ACC);
        sq_tri(x + 33, 42, 4, 1, SQ_ACC);
    } else {
        char a[16];
        const char *bar = value;
        int n = 0;
        while (*bar && *bar != '|') bar++;
        if (*bar) {                                 /* "beat 1|1/4": two parts and a dot */
            int wa, wb, x0;
            while (value + n < bar && n < 15) { a[n] = value[n]; n++; }
            a[n] = 0;
            wa = kit_text_width(a, KIT_FONT_XSMALL); wb = kit_text_width(bar + 1, KIT_FONT_XSMALL);
            x0 = x + 27 - (wa + 9 + wb) / 2;
            kit_text(x0, 42 - kit_font_height(KIT_FONT_XSMALL) / 2, a, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, SQ_ACC);
            kit_rect(x0 + wa + 3, 42, 2, 2, SQ_ACC);
            kit_text(x0 + wa + 9, 42 - kit_font_height(KIT_FONT_XSMALL) / 2, bar + 1, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, SQ_ACC);
        } else {
            ctext(x + 27, 42, value, KIT_FONT_XSMALL, value[0] == '-' && !value[1] ? SQ_MUTE : SQ_ACC);
        }
    }
}

static void sq_toast(void)                      /* the lab's message: a pill at the bottom, for a second */
{
    int w;
    if (!sq_msg_until || !sq_msg) return;
    w = kit_text_width(sq_msg, KIT_FONT_XSMALL) + 16;
    kit_round_rect(120 - w / 2, 214, w, 20, 6, SQ_ACCD);
    ctext(120, 224, sq_msg, KIT_FONT_XSMALL, FG);
}

static void sq_new_song(void)                   /* R-282: sequence 1, empty, the only block */
{
    sq_nb = 1; sq_cur = 0;
    sq_blocks[0].seq = 0; sq_blocks[0].reps = 1;
}

/* the song's settings (tempo, key, which tracks are drums) are the same in every sequence */
static void sq_sync(void)
{
    int i;
    for (i = 0; i < KIT_NSEQS; i++) {
        if (i == cur_seq) continue;
        seqs[i].bpm = song.bpm; seqs[i].key_auto = song.key_auto;
        seqs[i].key_tonic = song.key_tonic; seqs[i].key_minor = song.key_minor; seqs[i].unpitched = song.unpitched;
    }
}

static void sq_go(int s)                        /* the track pages show (and REC records) sequence s */
{
    if (s == cur_seq) return;
    sq_sync();
    cur_seq = s;
    dirty = 1;
}

/* PLAY on SONG: the song's order from the cursor's block */
static int sq_play_song(void)
{
    if (sq_view != SQV_SONG || !sq_nb) return 0;
    sq_songplay = 1; sq_pblock = sq_cur; sq_prep = 0; sq_queued = -1;
    sq_go(sq_blocks[sq_pblock].seq);
    return 1;
}

/* each frame: follow the chain (the player moved on: a new block, or the sequence pressed), count the repeats (the
 * loop came round), and queue the next block during the last bar of the block's last repeat */
static kit_song_t *sq_last_p;
static int sq_last_tick = -1, sq_pending;       /* sq_pending: the next block is queued */
static void sq_poll(uint32_t now)
{
    kit_song_t *p = kit_seq_song();
    int tick = kit_seq_tick();
    if (sq_msg_until && (int32_t)(now - sq_msg_until) >= 0) { sq_msg_until = 0; dirty = 1; }
    if (sq_del >= 0 && now - sq_del_at > 3000) { sq_del = -1; dirty = 1; }
    if (!p) { sq_songplay = 0; sq_queued = -1; sq_pending = 0; sq_last_p = 0; sq_last_tick = -1; return; }
    if (sq_last_p && (p != sq_last_p || tick < sq_last_tick)) {   /* a new start: a block, a repeat or a sequence */
        if (sq_queued >= 0) { sq_queued = -1; sq_songplay = 0; }
        else if (sq_songplay && sq_pending) { sq_pblock = (sq_pblock + 1) % sq_nb; sq_prep = 0; }
        else if (sq_songplay) sq_prep++;
        sq_pending = 0;
        if (p >= seqs && p < seqs + KIT_NSEQS) sq_go((int)(p - seqs));
        dirty = 1;
    }
    if (sq_view == SQV_SEQ && tick / 6 != sq_last_tick / 6) dirty = 1;   /* SEQUENCE's playhead, a 16th at a time */
    sq_last_p = p;
    sq_last_tick = tick;
    if (sq_songplay && sq_queued < 0 && !sq_pending && sq_nb && tick / KIT_TICKS_PER_BAR == song.bars - 1
        && sq_prep + 1 >= sq_blocks[sq_pblock].reps) {
        kit_seq_next(&seqs[sq_blocks[(sq_pblock + 1) % sq_nb].seq]);
        sq_pending = 1;
    }
}

static void sq_key_down(int key)
{
    int s = sq_white_index(key), k, held = -1;
    if (s < 0) return;
    if (kit_btn_held(KIT_BTN_EDIT)) {           /* EDIT + a sequence: delete, after a second press (R-278, R-279) */
        sq_edit_used = 1;
        if (!sq_used(s)) return;
        if (sq_del == s && kit_ms() - sq_del_at < 3000) {
            int b;
            kit_song_init(&seqs[s], song.bpm, SONG_BARS);
            for (b = sq_nb - 1; b >= 0; b--)
                if (sq_blocks[b].seq == s) { for (k = b + 1; k < sq_nb; k++) sq_blocks[k - 1] = sq_blocks[k]; sq_nb--; }
            if (!sq_nb) sq_new_song();
            if (sq_cur >= sq_nb) sq_cur = sq_nb - 1;
            sq_del = -1;
            sq_sayn("sequence ", s + 1, " deleted");
            sq_sync();
            cur_seq = sq_blocks[sq_cur].seq;      /* the cursor's block's sequence, not the one deleted */
        } else {
            sq_del = s; sq_del_at = kit_ms(); dirty = 1;
        }
        return;
    }
    for (k = 0; k < KIT_NKEYS; k++)             /* a lit sequence held */
        if (k != key && kit_key_held(k) && sq_white_index(k) >= 0 && sq_used(sq_white_index(k))) held = sq_white_index(k);
    if (!sq_used(s)) {                          /* a dark key: a held sequence copies itself there (R-277) */
        if (held >= 0) { seqs[s] = seqs[held]; cat_int(cat(cat_int(sq_msgbuf, held + 1), " copied to "), s + 1); sq_say(sq_msgbuf); sq_add_key = -1; }
        else sq_say("empty: record it on SEQUENCE");
        return;
    }
    if (held >= 0) return;
    if (kit_seq_playing()) {                    /* playing: from the next bar (R-276) */
        sq_queued = s;
        kit_seq_next(&seqs[s]);
        sq_sayn("next: ", s + 1, "");
        return;
    }
    sq_add_key = key;                           /* stopped: added when let go (unless a copy took it) */
    dirty = 1;
}

static void sq_key_up(int key)                  /* R-275: the lit key let go adds its sequence after the cursor's block */
{
    int s = sq_white_index(key), k;
    if (key != sq_add_key || s < 0) return;
    sq_add_key = -1;
    if (sq_nb < SQ_MAXB && !kit_seq_playing()) {
        for (k = sq_nb; k > sq_cur + 1; k--) sq_blocks[k] = sq_blocks[k - 1];
        sq_blocks[sq_cur + 1].seq = (uint8_t)s; sq_blocks[sq_cur + 1].reps = 1;
        sq_nb++; sq_cur++;
        sq_go(s);
    }
    dirty = 1;
}

static void sq_edit_up(void)                    /* EDIT tapped alone: the cursor's block goes */
{
    int k;
    if (!sq_edit_used && sq_nb > 1) {
        for (k = sq_cur + 1; k < sq_nb; k++) sq_blocks[k - 1] = sq_blocks[k];
        sq_nb--;
        if (sq_cur >= sq_nb) sq_cur = sq_nb - 1;
        sq_say("block removed");
    }
    sq_edit_used = 0;
}

static void sq_knob(int id, int d)
{
    if (!sq_nb) return;
    if (id >= KIT_ENC_KNOB1 && id <= KIT_ENC_KNOB4) sq_turned(id);
    if (id == KIT_ENC_KNOB1 || id == KIT_ENC_SELECT) { clampi(&sq_cur, d, 0, sq_nb - 1); if (!kit_seq_playing()) sq_go(sq_blocks[sq_cur].seq); }
    if (id == KIT_ENC_KNOB2) { int r = sq_blocks[sq_cur].reps; clampi(&r, d, 1, 16); sq_blocks[sq_cur].reps = (uint8_t)r; }
    if (id == KIT_ENC_KNOB3) {
        int to = sq_cur + (d > 0 ? 1 : -1);
        if (to >= 0 && to < sq_nb) { uint8_t a = sq_blocks[to].seq, r = sq_blocks[to].reps; sq_blocks[to] = sq_blocks[sq_cur]; sq_blocks[sq_cur].seq = a; sq_blocks[sq_cur].reps = r; sq_cur = to; }
    }
    if (id == KIT_ENC_KNOB4) { int v = sq_blocks[sq_cur].seq; clampi(&v, d, 0, KIT_NSEQS - 1); sq_blocks[sq_cur].seq = (uint8_t)v; if (!kit_seq_playing()) sq_go(v); }
    dirty = 1;
}

static void sq_lights(void)
{
    int k;
    for (k = 0; k < KIT_NKEYS; k++) {
        int s = sq_white_index(k), lv = KIT_LIGHT_OFF;
        if (s >= 0 && sq_used(s)) lv = s == cur_seq || s == sq_queued ? KIT_LIGHT_ON : KIT_LIGHT_HALF;   /* R-280, as the lab */
        if (kit_key_held(k)) lv = KIT_LIGHT_ON;
        kit_key_light(k, lv);
    }
    kit_btn_light(KIT_BTN_SEQ, KIT_LIGHT_ON);
    kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_EDIT, sq_del >= 0 ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
}

static void sq_dashed(int x, int y, int w, int h, kit_color_t c)   /* a repeat: a dashed outline */
{
    int i;
    for (i = 0; i < w; i += 4) { kit_rect(x + i, y, 2, 1, c); kit_rect(x + i, y + h - 1, 2, 1, c); }
    for (i = 0; i < h; i += 4) { kit_rect(x, y + i, 1, 2, c); kit_rect(x + w - 1, y + i, 1, 2, c); }
}

static void sq_draw(void)                       /* the lab's SONG: blocks a bar wide, a lane per track, the 16 keys */
{
    char v[40], *d;
    int i, t, Y = 56, LX = 40, W = 194, bars = 0, BW, x, cx = 0, cw = 0, scroll;
    kit_clear(BG);
    sq_dimmed = sq_del >= 0;
    sq_top("SONG");
    cat_int(v, sq_nb ? sq_cur + 1 : 0);
    sq_card(0, "CURSOR", sq_nb ? v : "-");
    cat_int(cat(v, "x"), sq_nb ? sq_blocks[sq_cur].reps : 0);
    sq_card(1, "REPEATS", sq_nb ? v : "-");
    sq_card(2, "MOVE", "<>");
    cat_int(v, sq_nb ? sq_blocks[sq_cur].seq + 1 : 0);
    sq_card(3, "SEQUENCE", sq_nb ? v : "-");
    for (i = 0; i < sq_nb; i++) bars += seqs[sq_blocks[i].seq].bars * sq_blocks[i].reps;
    if (!bars) bars = 1;
    BW = W / bars; if (BW < 6) BW = 6; if (BW > 48) BW = 48;   /* big while the song is short, 6 px a bar when long */
    for (i = 0, x = 0; i < sq_nb; i++) {
        int w = seqs[sq_blocks[i].seq].bars * sq_blocks[i].reps * BW;
        if (i == sq_cur) { cx = x; cw = w; }
        x += w;
    }
    scroll = cx + cw - W + 20; if (scroll > x - W) scroll = x - W; if (scroll < 0) scroll = 0;
    for (i = 0, x = 0; i < sq_nb; i++) {        /* the blocks, repeats as dashed copies (R-281), and their lanes */
        int sq = sq_blocks[i].seq, reps = sq_blocks[i].reps, w = seqs[sq].bars * reps * BW, one = w / reps, r;
        int X = LX + x - scroll, on = i == sq_cur, pl = sq_songplay && i == sq_pblock && kit_seq_playing();
        x += w;
        if (X < LX || X + w > LX + W + 2) continue;   /* (only whole blocks: the song scrolls to the cursor) */
        kit_round_rect(X + 1, Y + 2, w - 2, 22, 4, pl ? SQ_ACC : on ? SQ_ACCD : SQ_CARD);
        if (on) kit_rect_outline(X + 1, Y + 2, w - 2, 22, 1, SQ_ACC);
        for (r = 0; r < reps; r++) {
            cat_int(v, sq + 1);
            ctext(X + r * one + one / 2, Y + 13, v, r ? KIT_FONT_TINY : KIT_FONT_XSMALL, r ? SQ_MUTE : FG);
            if (r) { int yy; for (yy = Y + 4; yy < Y + 82; yy += 4) kit_rect(X + r * one, yy, 1, 2, SQ_MUTE); }
        }
        for (t = 0; t < KIT_NTRACKS; t++)
            kit_rect(X + 1, Y + 30 + t * 13, w - 2, 9, seqs[sq].count[t] ? sq_mix(SQ_TC[t], BG, 80) : SQ_GRID);
    }
    for (t = 0; t < KIT_NTRACKS; t++) {          /* the legend: each lane is a track, by its template */
        char n[6];
        const char *nm = track_name(t);
        int k;
        for (k = 0; k < 5 && nm[k]; k++) n[k] = nm[k];
        n[k] = 0;
        kit_text(4, Y + 30 + t * 13 + 4 - kit_font_height(KIT_FONT_TINY) / 2, n, KIT_FONT_TINY, KIT_ALIGN_LEFT, SQ_TC[t]);
    }
    if (scroll > 0) sq_tri(LX - 8, Y + 13, 3, -1, SQ_MUTE);
    if (x - scroll > W) sq_tri(232, Y + 13, 3, 1, SQ_MUTE);
    if (sq_del < 0 && kit_seq_playing() && sq_queued >= 0) {
        cat_int(cat(v, "next: "), sq_queued + 1);
        kit_text(8, Y + 95 - kit_font_height(KIT_FONT_XSMALL) / 2, v, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, SQ_ACC);
    }
    for (i = 0; i < KIT_NSEQS; i++) {            /* the sequence keys, two rows of eight: lit when they hold notes (R-280) */
        int bx = 6 + (i % 8) * 286 / 10, by = Y + 108 + (i / 8) * 22, u = sq_used(i), c = i == cur_seq;
        kit_round_rect(bx, by, 26, 18, 3, c && u ? SQ_ACC : u ? SQ_USED : SQ_EMPTY);
        cat_int(v, i + 1);
        ctext(bx + 13, by + 9, v, KIT_FONT_TINY, u ? FG : SQ_EMPTYT);
    }
    if (sq_del >= 0) {                          /* R-278: a pop-up over the song, for attention (the owner, 2026-11-20) */
        sq_dimmed = 0;                          /* (the page under it was drawn dimmed) */
        kit_round_rect(14, 82, 212, 52, 8, REC);
        d = cat_int(cat(v, "Delete sequence "), sq_del + 1);
        cat(d, "?");
        ctext(120, 101, v, KIT_FONT_SMALL, FG);
        d = cat_int(cat(v, "EDIT + "), sq_del + 1);
        cat(d, " again: delete");
        ctext(120, 122, v, KIT_FONT_XSMALL, FG);
    }
    sq_toast();
}

static void sq_seq_knob(int id, int d);
static const char *dr_name(int t, int p);
static void st_key_down(int key, int vel);
static void st_key_up(int key);
static void st_knob(int id, int d);

/* the sequencer's pages own the keys, the knobs (not ALGORITHM or PRESETS), EDIT and SEQ; the rest goes on as usual */
static int sq_event(const kit_event_t *e)
{
    if (mode != MODE_PRODUCER || picker || song_page || rl_open || rl_ask) return 0;   /* the REC layer's buttons stay its own */
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_SEQ) {
        sq_view = sq_view < 0 ? SQV_SONG : (sq_view + 1) % SQV_N;
        chord_release();
        kit_track_notes_off(track);
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_SEQ) return 1;
    if (sq_view < 0) return 0;
    if (sq_view == SQV_STEPS) {                 /* STEPS: the keys write at the cursor; the knobs and EDIT are its own */
        if (e->type == KIT_EV_KEY_DOWN) { st_key_down(e->id, e->vel); return 1; }
        if (e->type == KIT_EV_KEY_UP) { st_key_up(e->id); return 1; }
        if (e->type == KIT_EV_ENC && ((e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB4) || e->id == KIT_ENC_SELECT)) { st_knob(e->id, e->delta); return 1; }
        if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_EDIT) { sq_edit_used = 0; return 1; }
        if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_EDIT) return 1;
        return 0;
    }
    if (sq_view == SQV_SEQ) {                   /* SEQUENCE: the knobs are its own; the keys, REC, EDIT, SAVE as on the track */
        if (e->type == KIT_EV_ENC && e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB4) { sq_seq_knob(e->id, e->delta); return 1; }
        return 0;
    }
    switch (e->type) {
    case KIT_EV_KEY_DOWN: sq_key_down(e->id); dirty = 1; return 1;
    case KIT_EV_KEY_UP: sq_key_up(e->id); dirty = 1; return 1;
    case KIT_EV_ENC:
        if (e->id == KIT_ENC_ALGORITHM || e->id == KIT_ENC_PRESETS) return 0;
        sq_knob(e->id, e->delta);
        return 1;
    case KIT_EV_BTN_DOWN: if (e->id == KIT_BTN_EDIT) { sq_edit_used = 0; dirty = 1; return 1; } return 0;
    case KIT_EV_BTN_UP: if (e->id == KIT_BTN_EDIT) { sq_edit_up(); dirty = 1; return 1; } return 0;
    default: return 0;
    }
}

/* ---- SEQUENCE (ST-86, D-082, R-283..R-288): the sequence's four tracks, the track's QUANTIZE, SWING, STRENGTH, the
 * sequence's LENGTH. The keys, REC, EDIT and SAVE work as on the track's pages: REC records as played (R-283), SAVE held
 * undoes the take (R-285), EDIT + REC clears the track (R-286), SAVE held brings it back (R-287); only the playback
 * moves, by QUANTIZE and STRENGTH (R-284). */
static const char *const SQ_QN[8] = { "off", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" };
static const uint16_t SQ_QF[8] = { 0, 384, 192, 128, 96, 64, 48, 24 };   /* in sixteenths of a tick (API 30) */
static const uint8_t SQ_SW[7] = { 50, 54, 58, 62, 66, 70, 75 };
static uint8_t sq_q[KIT_NTRACKS], sq_sw[KIT_NTRACKS], sq_str[KIT_NTRACKS] = { 10, 10, 10, 10 };

static void sq_apply(int t)
{
    kit_feel_t f[12];
    int i;
    kit_seq_quantize(t, SQ_QF[sq_q[t]], sq_str[t] * 10);
    for (i = 0; i < 12; i++) { f[i].swing = SQ_SW[sq_sw[t]]; f[i].shift_ms = 0; f[i].loose_ms = 0; }
    kit_seq_feel(t, sq_sw[t] ? f : 0);
}

static void sq_seq_knob(int id, int d)
{
    int v;
    sq_turned(id);
    if (id == KIT_ENC_KNOB1) { v = sq_q[track]; clampi(&v, d, 0, 7); sq_q[track] = (uint8_t)v; sq_apply(track); }
    if (id == KIT_ENC_KNOB2) { v = sq_sw[track]; clampi(&v, d, 0, 6); sq_sw[track] = (uint8_t)v; sq_apply(track); }
    if (id == KIT_ENC_KNOB3) { v = sq_str[track]; clampi(&v, d, 0, 10); sq_str[track] = (uint8_t)v; sq_apply(track); }
    if (id == KIT_ENC_KNOB4) { v = song.bars; clampi(&v, d, 1, KIT_SONG_MAX_BARS); kit_song_set_bars(&song, v); }
    dirty = 1;
}


/* the sequence, a lane per track, as the lab draws it: from tick ws to we (x 6..234), top..top+H; the track's lane
 * tinted; each note at its pitch within its lane, its velocity its strength; on SEQUENCE a faint mark where it was
 * played when QUANTIZE moved it */
static void sq_lanes(int ws, int we, int top, int H, int ghosts, int cx, int cw)   /* cw > 0: STEPS' cursor cell, lighter */
{
    int t, i, rowH = H / KIT_NTRACKS, span = we - ws;
    if (cw > 0) kit_rect(cx, top, cw, H, sq_mix(KIT_WHITE, KIT_RGB(18, 26, 42), 20));
    for (t = 0; t < KIT_NTRACKS; t++) {
        int y0 = top + t * H / KIT_NTRACKS, lo = 127, hi = 0, sp;
        if (t == track) { kit_rect(6, y0, 228, rowH, KIT_RGB(17, 24, 38)); if (cw > 0) kit_rect(cx, y0, cw, rowH, sq_mix(KIT_WHITE, KIT_RGB(17, 24, 38), 20)); }
        for (i = 0; i < song.count[t]; i++) { int n = song.notes[t][i].note; if (n < lo) lo = n; if (n > hi) hi = n; }
        sp = hi - lo < 6 ? 6 : hi - lo;
        for (i = 0; i < song.count[t]; i++) {
            const kit_song_note_t *n = &song.notes[t][i];
            int at = kit_seq_place(&song, t, i) / 16, x, w, yy, a = 35 + 65 * n->vel / 127;
            if (at + n->len < ws || at >= we) continue;
            x = 6 + (at - ws) * 228 / span; w = n->len * 228 / span; if (w < 2) w = 2;
            if (x + w > 234) w = 234 - x;
            yy = y0 + rowH - 4 - (rowH - 8) * (n->note - lo) / sp;
            if (ghosts && n->start != at) {
                int px = 6 + (n->start - ws) * 228 / span;
                if (px >= 6 && px < 234) kit_rect(px, yy - 1, w, 2, sq_mix(SQ_TC[t], BG, 22));
            }
            if (x >= 6 && x < 234) kit_rect(x, yy - 1, w, 3, sq_mix(SQ_TC[t], BG, a));
        }
        {
            char l[2];
            l[0] = track_name(t)[0]; l[1] = 0;
            kit_text(9, y0 + 2, l, KIT_FONT_TINY, KIT_ALIGN_LEFT, t == track ? FG : SQ_MUTE);
        }
    }
}

static void sq_seq_draw(void)                   /* the lab's SEQUENCE: QUANTIZE, SWING, STRENGTH, LENGTH over the lanes */
{
    char v[16];
    int i, len = song.bars * KIT_TICKS_PER_BAR, top = 58, H = 130, q = SQ_QF[sq_q[track]];
    kit_clear(BG);
    sq_top("SEQUENCE");
    sq_card(0, "QUANTIZE", SQ_QN[sq_q[track]]);
    cat(cat_int(v, SQ_SW[sq_sw[track]]), "%");
    sq_card(1, "SWING", v);
    cat(cat_int(v, sq_str[track] * 10), "%");
    sq_card(2, "STRENGTH", v);
    cat(cat_int(v, song.bars), song.bars > 1 ? " bars" : " bar");
    sq_card(3, "LENGTH", v);
    if (q && (long)228 * q / (len * 16) >= 3)   /* the quantize's grid, when its lines are apart */
        for (i = 0; (long)i * q <= (long)len * 16; i++)
            kit_rect(6 + (int)((long)i * q * 228 / ((long)len * 16)), top, 1, H, KIT_RGB(22, 28, 40));
    for (i = 0; i <= song.bars * 4; i++)        /* the beats; the bars brighter */
        kit_rect(6 + i * 228 / (song.bars * 4), top, 1, H, i % 4 ? KIT_RGB(35, 43, 60) : KIT_RGB(52, 64, 90));
    sq_lanes(0, len, top, H, 1, 0, 0);
    if (kit_seq_playing())                      /* the playhead */
        kit_rect(6 + kit_seq_tick() * 228 / len, top, 2, H, FG);
    sq_toast();
}

/* ---- STEPS (ST-87, D-083, R-289..R-293): one bar, its four beats numbered, the cursor's beat divided by K1 GRID (R-289);
 * K2 STEP moves the cursor (SELECT too), K3 LENGTH and K4 VELOCITY of the notes at the cursor and of the next ones
 * written. REC off: the keys only sound (R-290); REC on: they write their notes at the cursor (R-291); EDIT + a key
 * removes that key's note at the cursor (R-292); the keys of the notes at the cursor are lit (R-293). The grids are
 * whole ticks (24 a beat), so STEPS stops at 1/32 (SEQUENCE's QUANTIZE goes to 1/64). */
static const char *const ST_GN[6] = { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const uint8_t ST_GT[6] = { 24, 12, 8, 6, 4, 3 };
static int st_gi = 3, st_pos, st_len = 1, st_vel = 100;   /* the grid; the cursor (ticks); the next note's length (cells), velocity */
static uint8_t st_sound[KIT_NKEYS];              /* the note each key sounds, + 1 (0: none) */

static int st_note(int key) { return is_drums() ? KIT_DRUM_LO + key : key_note(key); }

static int st_at_cursor(int t, int i)           /* note i of track t starts in the cursor's cell */
{
    int st = song.notes[t][i].start;
    return st >= st_pos && st < st_pos + ST_GT[st_gi];
}

static void st_remove(int t, int i)
{
    int k;
    for (k = i + 1; k < song.count[t]; k++) song.notes[t][k - 1] = song.notes[t][k];
    song.count[t]--;
}

static void st_key_down(int key, int vel)
{
    int n = st_note(key), i;
    if (kit_btn_held(KIT_BTN_EDIT)) {           /* EDIT + key: that key's note at the cursor goes (R-292) */
        sq_edit_used = 1;
        for (i = song.count[track] - 1; i >= 0; i--)
            if (song.notes[track][i].note == n && st_at_cursor(track, i)) st_remove(track, i);
        sq_say("removed");
        return;
    }
    st_sound[key] = (uint8_t)(n + 1);
    kit_track_note_on(track, n, vel);           /* it sounds, REC on or off (R-290) */
    if (!armed) sq_say("REC to write");
    if (armed) {                                /* REC on: written at the cursor (R-291) */
        for (i = song.count[track] - 1; i >= 0; i--)   /* (the same key there again: replaced) */
            if (song.notes[track][i].note == n && st_at_cursor(track, i)) st_remove(track, i);
        kit_song_add(&song, track, st_pos, st_len * ST_GT[st_gi], n, st_vel);
        update_key();
    }
    dirty = 1;
}

static void st_key_up(int key)
{
    if (st_sound[key]) { kit_track_note_off(track, st_sound[key] - 1); st_sound[key] = 0; }
    dirty = 1;
}

static void st_knob(int id, int d)
{
    int i, len = song.bars * KIT_TICKS_PER_BAR;
    if (id >= KIT_ENC_KNOB1 && id <= KIT_ENC_KNOB4) sq_turned(id);
    if (id == KIT_ENC_KNOB1) { clampi(&st_gi, d, 0, 5); st_pos = st_pos / ST_GT[st_gi] * ST_GT[st_gi]; }
    if (id == KIT_ENC_KNOB2 || id == KIT_ENC_SELECT) { st_pos = (st_pos + d * ST_GT[st_gi]) % len; if (st_pos < 0) st_pos += len; st_pos = st_pos / ST_GT[st_gi] * ST_GT[st_gi]; }
    if (id == KIT_ENC_KNOB3) {
        clampi(&st_len, d, 1, 16);
        for (i = 0; i < song.count[track]; i++) if (st_at_cursor(track, i)) song.notes[track][i].len = (uint16_t)(st_len * ST_GT[st_gi]);
    }
    if (id == KIT_ENC_KNOB4) {
        clampi(&st_vel, d * 5, 5, 127);
        for (i = 0; i < song.count[track]; i++) if (st_at_cursor(track, i)) song.notes[track][i].vel = (uint8_t)st_vel;
    }
    dirty = 1;
}

static void st_lights(void)                     /* the keys of the notes at the cursor (R-293), what is held */
{
    int k, i;
    for (k = 0; k < KIT_NKEYS; k++) {
        int lv = kit_key_held(k) ? KIT_LIGHT_ON : is_drums() ? KIT_LIGHT_DIM : KIT_LIGHT_OFF, n = st_note(k);   /* the drums' sounds dim, as the lab */
        for (i = 0; i < song.count[track]; i++) if (song.notes[track][i].note == n && st_at_cursor(track, i)) lv = KIT_LIGHT_ON;
        kit_key_light(k, lv);
    }
    kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF);
}

static void st_draw(void)                       /* the lab's STEPS: one bar of the four lanes, the cursor's beat in GRID cells */
{
    char v[48], *d;
    int i, b, top = 68, H = 120, ws = st_pos / KIT_TICKS_PER_BAR * KIT_TICKS_PER_BAR, we = ws + KIT_TICKS_PER_BAR;
    int beat = (st_pos - ws) / KIT_TICKS_PER_BEAT, per = KIT_TICKS_PER_BEAT / ST_GT[st_gi], at = -1;
    kit_clear(BG);
    sq_top("STEPS");
    sq_card(0, "GRID", ST_GN[st_gi]);
    d = cat_int(cat(v, "beat "), beat + 1);
    if (per > 1) cat_int(cat(cat_int(cat(d, "|"), (st_pos - ws - beat * KIT_TICKS_PER_BEAT) / ST_GT[st_gi] + 1), "/"), per);
    sq_card(1, "STEP", v);
    for (i = 0; i < song.count[track]; i++) if (st_at_cursor(track, i)) { at = i; break; }
    if (at >= 0) {                              /* what is at the cursor: its length in GRID cells, its velocity */
        int l10 = song.notes[track][at].len * 10 / ST_GT[st_gi];
        d = cat_int(v, l10 / 10);
        if (l10 % 10) d = cat_int(cat(d, "."), l10 % 10);
        cat(cat(d, " x "), ST_GN[st_gi]);
        sq_card(2, "LENGTH", v);
        cat_int(v, song.notes[track][at].vel);
        sq_card(3, "VELOCITY", v);
    } else {
        sq_card(2, "LENGTH", "-");
        sq_card(3, "VELOCITY", "-");
    }
    kit_rect(6 + beat * 57, top, 57, H, KIT_RGB(18, 26, 42));   /* the cursor's beat, divided by GRID (R-289) */
    for (i = 0; i < per; i++) kit_rect(6 + beat * 57 + i * 57 / per, top, 1, H, KIT_RGB(42, 53, 80));
    for (b = 0; b <= 4; b++) kit_rect(6 + b * 57, top, 1, H, b % 4 ? KIT_RGB(35, 43, 60) : KIT_RGB(52, 64, 90));
    for (b = 0; b < 4; b++) {
        cat_int(cat(v, "beat "), b + 1);
        kit_text(6 + b * 57 + 3, top - 12, v, KIT_FONT_TINY, KIT_ALIGN_LEFT, KIT_RGB(138, 150, 173));
    }
    i = ST_GT[st_gi] * 228 / KIT_TICKS_PER_BAR; if (i < 2) i = 2;   /* the cursor's cell, lighter */
    sq_lanes(ws, we, top, H, 0, 6 + (st_pos - ws) * 228 / KIT_TICKS_PER_BAR, i);
    if (kit_seq_playing()) { int ph = kit_seq_tick(); if (ph >= ws && ph < we) kit_rect(6 + (ph - ws) * 228 / KIT_TICKS_PER_BAR, top, 2, H, FG); }
    d = cat_int(cat(v, "BAR "), ws / KIT_TICKS_PER_BAR + 1);
    cat_int(cat(d, " of "), song.bars);
    kit_text(234, top + H + 3, v, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, FG);
    d = cat(v, armed ? "" : "REC to write - ");  /* what is at the cursor, by name */
    {
        int any = 0;
        for (i = 0; i < song.count[track] && d < v + 34; i++) {
            int n = song.notes[track][i].note;
            if (!st_at_cursor(track, i)) continue;
            if (any) d = cat(d, " ");
            d = cat(d, is_drums() ? dr_name(track, n - KIT_DRUM_LO) : kit_note_name(n, KIT_NAMES_LETTERS));
            any = 1;
        }
        if (!any) cat(d, "empty");
    }
    kit_text(8, top + H + 3, v, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, armed ? SQ_MUTE : REC);
    sq_toast();
}
#undef kit_rect
#undef kit_round_rect
#undef kit_rect_outline
#undef kit_circle
#undef kit_text
#undef ctext
