/* SPDX-License-Identifier: GPL-3.0-only */
/* The REC layer, the same on every track (ST-40..42, D-051), included by app.c after its state. Felucca's way: REC
 * held opens it (REC again closes it). On it the knobs are this track's GRID and FEEL (they change the playback only:
 * what was played is kept), the CLICK and the COUNT-IN; OCT+ clears the track, asking first. Anywhere: EDIT held with a
 * key erases that key's notes of the track as the song plays past them; SAVE undoes the last recording, erase or clear.
 * REC tapped while stopped with a COUNT-IN counts in and records. */

#define RL_NGRIDS 5
static const char *const RL_GRIDS[RL_NGRIDS] = { "off", "1/16", "1/8", "1/16T", "1/8T" };
static const uint8_t RL_GRID_TICKS[RL_NGRIDS] = { 0, 6, 12, 4, 8 };
/* the feels: swing, shift and looseness for the kick, snare, hats and the rest (a pitched track: the rest's) */
typedef struct { const char *name, *what; uint8_t sw[4]; int8_t sh[4]; uint8_t jit[4]; } rl_feel_t;
enum { GK = 0, GS, GH, GP };
static const rl_feel_t RL_FEELS[] = {
    { "Straight", "on the grid", { 50, 50, 50, 50 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { "Swing 55", "a touch of swing", { 55, 55, 55, 55 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { "Swing 62", "a clear shuffle", { 62, 62, 62, 62 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { "MPC 66", "the MPC's heavy swing", { 66, 66, 66, 66 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { "Dilla", "kick swung, snare late", { 72, 50, 60, 64 }, { -4, 32, 6, 10 }, { 12, 6, 12, 8 } },
    { "Laid back", "behind the beat", { 50, 50, 50, 50 }, { 0, 20, 12, 10 }, { 2, 3, 4, 3 } },
    { "Pushed", "ahead of the beat", { 50, 50, 50, 50 }, { 0, -6, -12, -8 }, { 2, 3, 3, 3 } },
    { "Dembow", "the snares lean late", { 50, 54, 54, 54 }, { 0, 9, 0, 6 }, { 2, 3, 3, 4 } },
    { "Shatta", "a bounce, the kick early", { 60, 60, 60, 60 }, { -9, 0, 0, 0 }, { 2, 2, 4, 3 } },
    { "Drunk", "loose on purpose", { 50, 50, 50, 50 }, { 0, 0, 0, 0 }, { 18, 18, 22, 20 } },
};
#define RL_NFEELS ((int)(sizeof RL_FEELS / sizeof RL_FEELS[0]))
static const char *const RL_CLICKS[3] = { "off", "rec", "on" };
static const char *const RL_COUNTS[3] = { "off", "1 bar", "2 bars" };

static int rl_open, rl_ask, rl_long;            /* the layer; asking to clear; REC held long enough */
static int sv_long;                             /* SAVE held long enough: it undid */
static uint32_t sv_at;                          /* when SAVE went down */
static void rl_close(void) { rl_open = 0; rl_ask = 0; }
static uint32_t rl_at;                          /* when REC went down */
static uint8_t rl_grid[KIT_NTRACKS], rl_feel[KIT_NTRACKS];
static int rl_ngrids(void) { return RL_NGRIDS; }
static const char *rl_grid_name(int t) { return RL_GRIDS[rl_grid[t]]; }
static const char *rl_feel_name(int t) { return RL_FEELS[rl_feel[t]].name; }
static int edit_used;                           /* EDIT: a key was held with it (no lights toggle) */
static uint32_t erase_keys;                     /* keys held with EDIT: they erase, they do not play */
static int erase_from = -1, erase_snap;         /* the playhead the erase has reached; snapshot taken this hold */
static uint32_t undo_until;                     /* the "undone" note */
static const char *undo_msg;

/* undo: the track's notes before each change, the last four */
#define RL_NUNDO 4
static struct { int8_t track; uint8_t count; kit_song_note_t notes[KIT_SONG_MAX_NOTES]; } undo[RL_NUNDO];
static int nundo, rec_snap = -1;                /* the track whose recording snapshot is on top, or -1 */

static void rl_copy(kit_song_note_t *to, const kit_song_note_t *from, int n) { while (n-- > 0) *to++ = *from++; }   /* (apps use fm1kit.h alone: no C library) */
static int rl_same(const kit_song_note_t *a, const kit_song_note_t *b, int n)
{
    for (; n > 0; n--, a++, b++)
        if (a->start != b->start || a->len != b->len || a->note != b->note || a->vel != b->vel) return 0;
    return 1;
}

static void undo_push(int t)
{
    if (nundo == RL_NUNDO) {                    /* full: the oldest goes */
        int j;
        for (j = 0; j < RL_NUNDO - 1; j++) {
            undo[j].track = undo[j + 1].track;
            undo[j].count = undo[j + 1].count;
            rl_copy(undo[j].notes, undo[j + 1].notes, undo[j + 1].count);
        }
        nundo--;
    }
    undo[nundo].track = (int8_t)t;
    undo[nundo].count = song.count[t];
    rl_copy(undo[nundo].notes, song.notes[t], song.count[t]);
    nundo++;
}
static int undo_same(int t)                     /* the top snapshot is track t as it is now */
{
    return nundo && undo[nundo - 1].track == t && undo[nundo - 1].count == song.count[t]
        && rl_same(undo[nundo - 1].notes, song.notes[t], song.count[t]);
}
static void rec_snap_end(void)                  /* a recording pass ends: nothing recorded, no undo step */
{
    if (rec_snap >= 0 && undo_same(rec_snap)) nundo--;
    rec_snap = -1;
}
static void rec_snap_begin(void) { rec_snap_end(); undo_push(track); rec_snap = track; }

static void undo_pop(void)
{
    int t;
    rec_snap_end();                             /* a pass that recorded nothing is no change */
    if (!nundo) { undo_msg = "Nothing to undo"; undo_until = kit_ms() + 1500; if (armed) rec_snap_begin(); return; }
    nundo--;
    t = undo[nundo].track;
    kit_track_notes_off(t);
    song.count[t] = undo[nundo].count;
    rl_copy(song.notes[t], undo[nundo].notes, undo[nundo].count);
    if (armed) rec_snap_begin();                /* still recording: a new pass from here */
    undo_msg = "The last change is undone";
    undo_until = kit_ms() + 1500;
    update_key();
}
static kit_feel_t rl_feel_of(const rl_feel_t *r, int g)
{
    kit_feel_t f;
    f.swing = r->sw[g];
    f.shift_ms = r->sh[g];
    f.loose_ms = r->jit[g];
    return f;
}
static void rl_apply(int t)                     /* the track's GRID and FEEL to the player */
{
    const rl_feel_t *r = &RL_FEELS[rl_feel[t]];
    int n;
    kit_seq_grid(t, RL_GRID_TICKS[rl_grid[t]]);
    for (n = 0; n < 128; n++) {                 /* a drums track: each pad its drum's part of the feel; else the rest's */
        int g = GP;
        if (set.role[t] == ROLE_DRUMS) {
            int pad = kit_drum_pad_of(n);
            g = kit_drum_group(dr_src_kit(t, pad), dr_pads[t][pad].src);
        }
        kit_seq_feel_note(t, n, rl_feel_of(r, g));
    }
}

static int rl_nfeels(void) { return RL_NFEELS; }
static int rl_grid_ticks(int t) { return RL_GRID_TICKS[rl_grid[t]]; }
static const char *rl_feel_word(int t)          /* the feel's first word: it fits a card */
{
    static char w[12];
    const char *f = RL_FEELS[rl_feel[t]].name;
    char *d = w;
    while (*f && *f != ' ' && d < w + 11) *d++ = *f++;
    *d = 0;
    return w;
}
static int rl_feel_fine(int t, int g, int tick) /* where the track's FEEL puts a hit on tick (of a bar), in sixteenths of a tick */
{
    const rl_feel_t *r = &RL_FEELS[rl_feel[t]];
    int pos = tick * 16;
    if (tick % 6 == 0 && (tick / 6) % 2 == 1 && r->sw[g] > 50) pos += (r->sw[g] * 2 - 100) * 6 * 16 / 100;
    pos += r->sh[g] * song.bpm * 8 / 1250;
    return pos;
}

static void rl_knob(int id, int d)
{
    int v;
    if (id == KIT_ENC_KNOB1) { v = rl_grid[track]; wrap(&v, d, RL_NGRIDS); rl_grid[track] = (uint8_t)v; rl_apply(track); }
    if (id == KIT_ENC_KNOB2) { v = rl_feel[track]; wrap(&v, d, RL_NFEELS); rl_feel[track] = (uint8_t)v; rl_apply(track); }
    if (id == KIT_ENC_KNOB3) { v = set.click; clampi(&v, d, 0, 2); set.click = (uint8_t)v; settings_save(); }
    if (id == KIT_ENC_KNOB4) { v = set.count_in; clampi(&v, d, 0, 2); set.count_in = (uint8_t)v; settings_save(); }
}

static void rec_tap(void)                       /* REC: arm or disarm; while stopped with a COUNT-IN, count in and record */
{
    armed = !armed;
    kit_seq_record(armed ? track : -1);
    if (armed) {
        rec_snap_begin();
        if (!kit_seq_playing() && !kit_seq_counting() && set.count_in) {
            kit_seq_follow(0);
            kit_seq_count_in(&song, set.count_in, set.click_lvl);
        }
    } else {
        rec_snap_end();
        if (!kit_seq_playing()) update_key();
    }
}

static void erase_poll(void)                    /* EDIT + keys: the track's notes of those keys, as the playhead passes */
{
    int tick, len, i, k, n;
    uint16_t pcs = 0;
    if (!erase_keys || !kit_btn_held(KIT_BTN_EDIT) || !kit_seq_playing()) { erase_from = -1; return; }
    for (k = 0; k < KIT_NKEYS; k++) if (erase_keys >> k & 1u) pcs |= (uint16_t)(1u << pc12(key_note(k)));
    len = kit_song_length(&song);
    tick = (kit_seq_tick() + 3) % len;          /* a little ahead of the playhead, so an erased note does not sound first */
    if (erase_from < 0) { erase_from = (kit_seq_tick() + len - 1) % len; }
    n = song.count[track];
    for (i = 0; i < n; i++) {
        int at = kit_seq_place(&song, track, i) / 16, hit;
        if (!(pcs >> pc12(song.notes[track][i].note) & 1u) || song.notes[track][i].start >= len) continue;
        hit = tick >= erase_from ? at > erase_from && at <= tick : at > erase_from || at <= tick;   /* (from, tick], over the loop's end too */
        if (!hit) continue;
        if (!erase_snap) { undo_push(track); erase_snap = 1; }
        kit_track_note_off(track, song.notes[track][i].note);
        for (k = i; k < n - 1; k++) song.notes[track][k] = song.notes[track][k + 1];   /* (no memmove on the FM-1) */
        n--; i--;
        song.count[track] = (uint8_t)n;
        dirty = 1;
    }
    erase_from = tick;
}

/* the panel's events for the layer, EDIT and SAVE; 1 when the event was theirs */
static int rl_cleared;                          /* REC went down with EDIT: it cleared (its release does nothing else) */
static void rl_clear_track(void)                /* the track's notes gone, undoable with SAVE held (R-286, R-287) */
{
    rec_snap_end();
    undo_push(track);
    kit_track_notes_off(track);
    kit_song_clear(&song, track);
    if (armed) rec_snap_begin();
    update_key();
}

static int rl_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_REC && kit_btn_held(KIT_BTN_EDIT)) {   /* EDIT + REC: clear (ST-86) */
        edit_used = 1;
        rl_cleared = 1;
        rl_clear_track();
        undo_msg = "Track cleared: hold SAVE to undo";
        undo_until = kit_ms() + 2000;
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_REC) { rl_at = kit_ms(); rl_long = 0; rl_cleared = 0; return 1; }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_REC && rl_cleared) { rl_cleared = 0; return 1; }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_REC) {
        if (!rl_long) { if (rl_open || rl_ask) { rl_open = 0; rl_ask = 0; } else rec_tap(); }
        dirty = 1;
        return 1;
    }
    if (rl_ask && (e->type == KIT_EV_ENC || e->type == KIT_EV_KEY_DOWN))
        return 1;                               /* the question takes the knobs and new keys until it is answered (R-242) */
    if (rl_ask && e->type == KIT_EV_BTN_DOWN) {   /* clear this track? OCT+ again: yes; any other: no */
        if (e->id == KIT_BTN_OCT_UP) rl_clear_track();
        rl_ask = 0;
        dirty = 1;
        return 1;
    }
    if (rl_open && e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_OCT_UP) { rl_ask = 1; dirty = 1; return 1; }
    if (rl_open && e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_OCT_UP) return 1;
    if (rl_open && e->type == KIT_EV_ENC && e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB4) { rl_knob(e->id, e->delta); dirty = 1; return 1; }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_EDIT) {
        edit_used = 0;
        erase_snap = 0;
        erase_from = -1;
        sound_edit_press();
        return 1;
    }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_EDIT) {
        if (!edit_used) {
            if (is_manual()) lights_offs[track] = !lights_offs[track];
            else if (is_drums()) dr_edit_tap();   /* a drums track: EDIT builds the kit (ST-96, R-378) */
            else sound_edit_release();
        }
        erase_keys = 0;
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_KEY_DOWN && kit_btn_held(KIT_BTN_EDIT)) {   /* EDIT + key: it erases, it does not play */
        edit_used = 1;
        erase_keys |= 1u << e->id;
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_KEY_UP && (erase_keys >> e->id & 1u)) { erase_keys &= ~(1u << e->id); return 1; }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_SAVE && !song_page) { sv_at = kit_ms(); sv_long = 0; return 1; }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_SAVE && !song_page) {   /* tapped: it says how; held: rl_frame undid (R-147) */
        if (!sv_long) exp_say("HOLD", "");
        sv_long = 0;
        return 1;
    }
    return 0;
}

static void rl_frame(uint32_t now)
{
    if (kit_btn_held(KIT_BTN_REC) && !rl_long && now - rl_at >= HOLD_MS) {
        rl_long = 1;                            /* REC held: the layer opens, or closes */
        rl_open = !rl_open;
        rl_ask = 0;
        dirty = 1;
    }
    if (kit_btn_held(KIT_BTN_SAVE) && !sv_long && !song_page && now - sv_at >= HOLD_MS) {
        sv_long = 1;                            /* SAVE held: undo (R-147) */
        undo_pop();
        dirty = 1;
    }
    if (undo_until && (int32_t)(now - undo_until) >= 0) { undo_until = 0; dirty = 1; }
    erase_poll();
}

static void draw_rec_layer(void)                /* over the page: the layer's cards and what the buttons do */
{
    static const char *const keys[4] = { "OCT+", "EDIT + key", "SAVE", "REC" };
    static const char *const does[4] = { "clear the track", "erase as it plays", "undo", "close" };
    const char *fn = RL_FEELS[rl_feel[track]].name;
    char f1[12], *d = f1;
    int i;
    if (undo_until && sq_view >= 0 && mode == MODE_PRODUCER) {   /* the sequencer's pages: the lab's message (D-084) */
        char cl[24];
        const char *m;
        cat(cat(cl, track_name(track)), " cleared");
        m = undo_msg[0] == 'N' ? "nothing to undo" : undo_msg[4] == 'l' ? "take undone" : cl;
        int w = kit_text_width(m, KIT_FONT_XSMALL) + 16;
        kit_round_rect(120 - w / 2, 214, w, 20, 6, SQ_ACCD);
        ctext(120, 224, m, KIT_FONT_XSMALL, FG);
    } else if (undo_until) {
        kit_round_rect(6, 92, 228, 40, 8, CARD2);
        kit_text(120, 98, "UNDO", KIT_FONT_SMALL, KIT_ALIGN_CENTER, FG);
        kit_text(120, 116, undo_msg, KIT_FONT_XSMALL, KIT_ALIGN_CENTER, MUTED);
    }
    if (!rl_open && !rl_ask) return;
    kit_rect(0, 28, KIT_W, 112, BG);
    while (*fn && *fn != ' ' && d < f1 + 11) *d++ = *fn++;   /* the feel's first word fits the card */
    *d = 0;
    card(0, "GRID", RL_GRIDS[rl_grid[track]], rl_grid[track] > 0);
    card(1, "FEEL", f1, rl_feel[track] > 0);
    card(2, "CLICK", RL_CLICKS[set.click], set.click > 0);
    card(3, "COUNT-IN", RL_COUNTS[set.count_in], set.count_in > 0);
    if (rl_ask) {
        kit_round_rect(6, 70, 228, 50, 8, REC);
        kit_text(120, 76, "Clear this track?", KIT_FONT_SMALL, KIT_ALIGN_CENTER, FG);
        kit_text(120, 100, "OCT+ again: yes    any other: no", KIT_FONT_XSMALL, KIT_ALIGN_CENTER, FG);
        return;
    }
    if (rl_feel[track] && !rl_grid[track]) kit_text(232, 66, "FEEL needs a GRID", KIT_FONT_TINY, KIT_ALIGN_RIGHT, WARN);
    for (i = 0; i < 4; i++) {
        kit_text(10, 74 + i * 16, keys[i], KIT_FONT_XSMALL, KIT_ALIGN_LEFT, ACC);
        kit_text(92, 74 + i * 16, does[i], KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    }
}
