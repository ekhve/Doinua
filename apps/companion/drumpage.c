/* SPDX-License-Identifier: GPL-3.0-only */
/* The drums template's PLAY page (ST-30, ST-38, ST-39; D-053), included by app.c before app_frame. Every key is its
 * own sound of the track's kit (27 a kit, fm1kit's drum kits), named on the screen as it plays; the octave does not
 * move them. PRESETS changes the kit, named on the top line; a key held and PRESETS puts the next sound of any kit
 * on that key (My kit). K1 REPEAT, K2 TUNE and K3 DECAY are the key played last's own: a held key with a repeat
 * plays again at that interval of the song's tempo (recorded as it plays), a tuned pitched sound names its note. */

#define DR_NREPEATS 7
static const char *const DR_REPEATS[DR_NREPEATS] = { "off", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const uint8_t DR_REPEAT_TICKS[DR_NREPEATS] = { 0, 24, 12, 8, 6, 4, 3 };
#define DR_NDECAYS 9
static const uint8_t DR_DECAYS[DR_NDECAYS] = { 16, 24, 32, 48, 64, 90, 128, 180, 255 };   /* x/64 of the sound's */
static const char *const DR_DECAY_NAMES[DR_NDECAYS] = { "25 %", "38 %", "50 %", "75 %", "100 %", "140 %", "200 %", "280 %", "400 %" };

typedef struct { int8_t kit, src, tune; uint8_t decay, repeat; } dr_pad_t;   /* kit -1: the track's own kit */
static dr_pad_t dr_pads[KIT_NTRACKS][KIT_DRUM_PADS];
static int dr_last[KIT_NTRACKS];                /* the pad played last (the knobs' pad) */
static uint32_t dr_next[KIT_DRUM_PADS];         /* a held key's next repeat, ms (0: none) */
static uint32_t dr_shown_until;                 /* a held key's sound changed: say so */

/* IDEAS and AUTO (ST-32, ST-33, D-054): the styles of drumstyle.c */
enum { DR_PLAY = 0, DR_IDEAS, DR_AUTO, DR_NPAGES };
static const char *const DR_PAGE[DR_NPAGES] = { "PLAY", "IDEAS", "AUTO" };
static const char *const DR_FILLS[3] = { "off", "4 bars", "8 bars" };
static const char *const DR_GHOSTS[3] = { "off", "soft", "more" };
typedef struct { uint8_t page, istyle, ivar, speed, hear, astyle, fills, ghosts; } dr_track_t;
static dr_track_t drt[KIT_NTRACKS];
static int dr_pad_var = -1;                     /* AUTO: the variant pad held (-1: none) */
static int dr_auto_started, dr_prev_tick = -1, dr_bars, dr_loops;
static const int8_t DR_VAR_KEY[DV_N] = { 0, 2, 4, 6, 7 };   /* AUTO's pads: F3 G3 A3 B3 C4: main busy fill break drop */

static const char *const DR_FX_NAMES[KIT_NFX] = { "off", "Punch", "MPC", "Lofi", "Tape" };
static int is_drums(void) { return set.role[track] == ROLE_DRUMS; }
static int dr_kit(int t) { int e, p; sound_of(snd[t], &e, &p); return e == KIT_ENGINE_DRUMS ? p : 0; }

static void dr_init(void)
{
    int t, p;
    for (t = 0; t < KIT_NTRACKS; t++) {
        for (p = 0; p < KIT_DRUM_PADS; p++) { dr_pads[t][p].kit = -1; dr_pads[t][p].src = (int8_t)p; dr_pads[t][p].tune = 0; dr_pads[t][p].decay = 4; dr_pads[t][p].repeat = 0; }
        dr_last[t] = 0;
        drt[t].page = DR_PLAY; drt[t].istyle = 0; drt[t].ivar = 0; drt[t].speed = MEL_NSPEEDS - 1; drt[t].hear = HEAR_LISTEN;
        drt[t].astyle = 0; drt[t].fills = 0; drt[t].ghosts = 1;
    }
}
static void dr_apply(int t, int p)              /* the pad's sound to fm1kit */
{
    dr_pad_t *d = &dr_pads[t][p];
    kit_drum_pad_set(t, p, d->kit, d->src, d->tune, DR_DECAYS[d->decay]);
}
static int dr_custom(int t)                     /* a key plays another sound than the kit's: My kit */
{
    int p;
    for (p = 0; p < KIT_DRUM_PADS; p++) if (dr_pads[t][p].kit >= 0 || dr_pads[t][p].src != p) return 1;
    return 0;
}
static int dr_src_kit(int t, int p) { return dr_pads[t][p].kit < 0 ? dr_kit(t) : dr_pads[t][p].kit; }
static const char *dr_name(int t, int p) { return kit_drum_name(dr_src_kit(t, p), dr_pads[t][p].src); }
static const char *dr_kit_title(int t) { return dr_custom(t) ? "My kit" : kit_drum_kit_name(dr_kit(t)); }

static uint32_t dr_repeat_ms(int ticks) { return (uint32_t)ticks * 60000u / ((uint32_t)song.bpm * KIT_TICKS_PER_BEAT); }

static void dr_hit(int key, int vel)
{
    int note = KIT_DRUM_LO + key;
    kit_seq_note_on(track, note, vel);
    kit_midi_note_on(channel_of(track), note, vel);
}

/* the drums' events: 1 when the event was theirs */
static int dr_event(const kit_event_t *e)
{
    if (!is_drums() || song_page || help) return 0;
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_SCL) {   /* the pages: PLAY, IDEAS, AUTO */
        int v = drt[track].page;
        wrap(&v, 1, DR_NPAGES);
        drt[track].page = (uint8_t)v;
        dr_pad_var = -1;
        dirty = 1;
        return 1;
    }
    if (drt[track].page == DR_AUTO && (e->type == KIT_EV_KEY_DOWN || e->type == KIT_EV_KEY_UP)) {
        int v, which = -1;
        for (v = 0; v < DV_N; v++) if (DR_VAR_KEY[v] == e->id) which = v;
        if (e->type == KIT_EV_KEY_DOWN && which >= 0) {   /* a pad: its beat; the song starts with it */
            dr_pad_var = which;
            if (!kit_seq_playing() && !kit_seq_counting()) { kit_seq_follow(0); kit_seq_play(&song); dr_auto_started = 1; dr_bars = 0; }
        } else if (e->type == KIT_EV_KEY_UP && which == dr_pad_var) {
            int k, other = -1;
            for (v = 0; v < DV_N; v++) for (k = 0; k < KIT_NKEYS; k++) if (k == DR_VAR_KEY[v] && k != e->id && kit_key_held(k)) other = v;
            dr_pad_var = other;
            if (other < 0 && dr_auto_started && !armed) { kit_seq_stop(); bar = -1; dr_auto_started = 0; }
        }
        dirty = 1;
        return 1;
    }
    if (drt[track].page != DR_PLAY && e->type == KIT_EV_ENC && ((e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB4) || e->id == KIT_ENC_SELECT)) {
        dr_track_t *d = &drt[track];
        int v;
        if (d->page == DR_IDEAS) {
            if (e->id == KIT_ENC_KNOB1) { v = d->istyle; wrap(&v, e->delta, DS_NSTYLES); d->istyle = (uint8_t)v; }
            if (e->id == KIT_ENC_KNOB2 || e->id == KIT_ENC_SELECT) { v = d->ivar; wrap(&v, e->delta > 0 ? 1 : -1, DV_N); d->ivar = (uint8_t)v; }
            if (e->id == KIT_ENC_KNOB3) { v = d->speed; clampi(&v, e->delta > 0 ? 1 : -1, 0, MEL_NSPEEDS - 1); d->speed = (uint8_t)v; }
            if (e->id == KIT_ENC_KNOB4) { v = d->hear; wrap(&v, e->delta, BASS_NHEARS); d->hear = (uint8_t)v; }
        } else {
            if (e->id == KIT_ENC_KNOB1 || e->id == KIT_ENC_SELECT) { v = d->astyle; wrap(&v, e->delta > 0 ? 1 : -1, DS_NSTYLES); d->astyle = (uint8_t)v; }
            if (e->id == KIT_ENC_KNOB2) { v = d->fills; clampi(&v, e->delta, 0, 2); d->fills = (uint8_t)v; }
            if (e->id == KIT_ENC_KNOB4) { v = d->ghosts; clampi(&v, e->delta, 0, 2); d->ghosts = (uint8_t)v; }   /* ST-84, R-273: K3 FEEL, K4 GHOSTS */
            if (e->id == KIT_ENC_KNOB3) { v = rl_feel[track]; wrap(&v, e->delta, rl_nfeels()); rl_feel[track] = (uint8_t)v; rl_apply(track); }
        }
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_KEY_DOWN) {
        dr_last[track] = e->id;
        last_note = KIT_DRUM_LO + e->id;
        last_channel = 0;
        dr_hit(e->id, e->vel);
        dr_next[e->id] = dr_pads[track][e->id].repeat ? kit_ms() + dr_repeat_ms(DR_REPEAT_TICKS[dr_pads[track][e->id].repeat]) : 0;
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_KEY_UP) {
        int note = KIT_DRUM_LO + e->id;
        dr_next[e->id] = 0;
        kit_seq_note_off(track, note);
        kit_midi_note_off(channel_of(track), note);
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_ENC && e->id == KIT_ENC_PRESETS) {
        int k, held = -1;
        for (k = 0; k < KIT_NKEYS; k++) if (kit_key_held(k)) held = k;
        if (held >= 0) {                        /* a key held: the next sound of any kit on that key (My kit) */
            dr_pad_t *d = &dr_pads[track][held];
            int i = dr_src_kit(track, held) * KIT_DRUM_PADS + d->src, n = kit_preset_count(KIT_ENGINE_DRUMS) * KIT_DRUM_PADS;
            i = ((i + (e->delta > 0 ? 1 : -1)) % n + n) % n;
            d->kit = (int8_t)(i / KIT_DRUM_PADS);
            d->src = (int8_t)(i % KIT_DRUM_PADS);
            if (d->kit == dr_kit(track) && d->src == held) d->kit = -1;   /* back to the kit's own */
            dr_apply(track, held);
            rl_apply(track);                    /* its drum's part of the feel */
            dr_last[track] = held;
            dr_shown_until = kit_ms() + 1500;
            dr_hit(held, 100);                  /* hear it */
        } else {                                /* the kit */
            int kit = dr_kit(track);
            wrap(&kit, e->delta > 0 ? 1 : -1, kit_preset_count(KIT_ENGINE_DRUMS));
            snd[track] = sound_index(KIT_ENGINE_DRUMS, kit);
            pick_sound(track);
            rl_apply(track);
        }
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_ENC && e->id == KIT_ENC_KNOB4) {   /* the drums' FX: the whole track */
        int v = kit_track_fx_of(track);
        clampi(&v, e->delta, 0, KIT_NFX - 1);
        kit_track_fx(track, v);
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_ENC && e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB3) {
        int p = dr_last[track], v;
        dr_pad_t *d = &dr_pads[track][p];
        if (e->id == KIT_ENC_KNOB1) { v = d->repeat; clampi(&v, e->delta, 0, DR_NREPEATS - 1); d->repeat = (uint8_t)v; }
        if (e->id == KIT_ENC_KNOB2) { v = d->tune; clampi(&v, e->delta, -12, 12); d->tune = (int8_t)v; }
        if (e->id == KIT_ENC_KNOB3) { v = d->decay; clampi(&v, e->delta, 0, DR_NDECAYS - 1); d->decay = (uint8_t)v; }
        dr_apply(track, p);
        dirty = 1;
        return 1;
    }
    return 0;
}

static void dr_frame(uint32_t now)
{
    int k;
    if (!is_drums()) return;
    for (k = 0; k < KIT_NKEYS; k++)             /* a held key's repeat, at the song's tempo */
        if (dr_next[k] && kit_key_held(k) && (int32_t)(now - dr_next[k]) >= 0) {
            int ticks = DR_REPEAT_TICKS[dr_pads[track][k].repeat];
            if (!ticks) { dr_next[k] = 0; continue; }
            kit_seq_note_off(track, KIT_DRUM_LO + k);
            dr_hit(k, 96);
            dr_next[k] += dr_repeat_ms(ticks);
            if ((int32_t)(now - dr_next[k]) >= 0) dr_next[k] = now + dr_repeat_ms(ticks);
        }
    if (dr_shown_until && (int32_t)(now - dr_shown_until) >= 0) { dr_shown_until = 0; dirty = 1; }
}

/* the keys, coloured by what they are: kicks, snares and claps, hats and shakers, toms and percussion */
static const kit_color_t DR_GROUP_COL[4] = { KIT_RGB(235, 110, 90), KIT_RGB(242, 175, 70), KIT_RGB(110, 200, 210), KIT_RGB(170, 140, 245) };
static void dr_keys(int y, int h)
{
    int k, wi = 0, last = dr_last[track];
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_DRUM_LO + k, g = kit_drum_group(dr_src_kit(track, k), dr_pads[track][k].src);
        kit_color_t col = kit_key_held(k) ? FG : DR_GROUP_COL[g];
        if (!kit_is_white(n)) continue;
        kit_rect(wi * 15, y, 14, h, k == last ? col : KEY_W);
        kit_rect(wi * 15, y + h - 6, 14, 6, col);   /* the group's colour at the foot of each key */
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_DRUM_LO + k, g = kit_drum_group(dr_src_kit(track, k), dr_pads[track][k].src), x, bh = h * 58 / 100;
        if (kit_is_white(n)) { wi++; continue; }
        x = wi * 15 - 5;
        kit_rect(x, y, 9, bh, kit_key_held(k) ? FG : k == last ? DR_GROUP_COL[g] : KEY_B);
        kit_rect(x + 2, y + bh - 5, 5, 4, DR_GROUP_COL[g]);
    }
}

static void dr_rec_graph(int y);
static void draw_drums(void)
{
    int p = dr_last[track], note;
    dr_pad_t *d = &dr_pads[track][p];
    char v[16], s[40], *q;
    kit_clear(BG);
    top("PLAY");
    card(0, "REPEAT", DR_REPEATS[d->repeat], d->repeat != 0);
    q = v;
    note = kit_drum_note(dr_src_kit(track, p), d->src, d->tune);
    if (d->tune || note < 0) {
        if (d->tune > 0) q = cat(q, "+");
        q = cat_int(q, d->tune);
        if (note < 0) q = cat(q, " st");
    }
    if (note >= 0) {                            /* a pitched sound: the note it sounds (an 808 tuned to the song) */
        if (q != v) q = cat(q, " ");
        cat_int(cat(q, kit_note_name(note, KIT_NAMES_LETTERS)), kit_octave(note));
    }
    card(1, "TUNE", v, d->tune != 0);
    card(2, "DECAY", DR_DECAY_NAMES[d->decay], d->decay != 4);
    card(3, "FX", DR_FX_NAMES[kit_track_fx_of(track)], kit_track_fx_of(track) != KIT_FX_OFF);
    q = cat(s, kit_drum_kit_name(dr_src_kit(track, p)));
    if (dr_pads[track][p].kit >= 0 || dr_pads[track][p].src != p) q = cat(q, "  on this key");
    if (song.count[track]) {                    /* a beat recorded: its bar, each hit where the feel puts it (R-143) */
        int w = kit_text(8, 68, dr_name(track, p), KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, FG);
        kit_text(16 + w, 74, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, dr_shown_until ? ACC : MUTED);
        dr_rec_graph(96);
        dr_keys(150, 90);
        return;
    }
    kit_text(8, 74, dr_name(track, p), KIT_FONT_LARGE, KIT_ALIGN_LEFT, FG);
    kit_text(8, 116, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, dr_shown_until ? ACC : MUTED);
    kit_text(8, 134, "PRESETS: the kit   key + PRESETS: its sound", KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
    dr_keys(150, 90);
}

static void dr_ideas_lights(void);
static void drums_lights(void)
{
    int k;
    if (drt[track].page == DR_IDEAS) { dr_ideas_lights(); return; }
    if (drt[track].page == DR_AUTO) {           /* the five pads, the one held lit */
        for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, KIT_LIGHT_OFF);
        for (k = 0; k < DV_N; k++) kit_key_light(DR_VAR_KEY[k], k == dr_pad_var ? KIT_LIGHT_ON : KIT_LIGHT_DIM);
        return;
    }
    for (k = 0; k < KIT_NKEYS; k++)
        kit_key_light(k, kit_key_held(k) ? KIT_LIGHT_ON : k == dr_last[track] ? KIT_LIGHT_BREATH : KIT_LIGHT_DIM);
}

/* the beat as the song plays: IDEAS (heard as HEAR says, not recorded) and AUTO (a pad held; REC records it) */
static void dr_play_hit(const ds_hit_t *h, int rec)
{
    int note = KIT_DRUM_LO + h->pad;
    if (rec) { kit_seq_note_on(track, note, h->vel); kit_seq_note_off(track, note); }
    else { kit_track_note_on(track, note, h->vel); kit_track_note_off(track, note); }
}
static void dr_beat_at(int tick)                /* the hits on this tick of the loop */
{
    dr_track_t *d = &drt[track];
    ds_hit_t h[96];
    int n, i, bt = tick % KIT_TICKS_PER_BAR, b = tick / KIT_TICKS_PER_BAR;
    if (d->page == DR_IDEAS) {
        int hear = d->hear == HEAR_ON || (d->hear == HEAR_LISTEN && dr_loops % 2 == 0);
        if (!hear) return;
        n = ds_hits(d->istyle, d->ivar, 1, h, 96);
        for (i = 0; i < n; i++) if (h[i].tick == bt) dr_play_hit(&h[i], 0);
    } else if (d->page == DR_AUTO && dr_pad_var >= 0) {
        (void)b;
        n = ds_hits(d->astyle, ds_variant_at(dr_pad_var, d->fills, dr_bars), d->ghosts, h, 96);
        for (i = 0; i < n; i++) if (h[i].tick == bt) dr_play_hit(&h[i], armed);
    }
}
static void dr_poll(void)
{
    int tick = kit_seq_playing() ? kit_seq_tick() : -1, i;
    if (!is_drums() || drt[track].page == DR_PLAY || tick < 0) {
        if (tick < 0) { dr_prev_tick = -1; dr_loops = 0; }
        return;
    }
    if (dr_prev_tick < 0) dr_prev_tick = tick - 1;
    if (tick < dr_prev_tick) {                  /* round the loop */
        int len = song.bars * KIT_TICKS_PER_BAR;
        for (i = dr_prev_tick + 1; i < len; i++) {
            if (i % KIT_TICKS_PER_BAR == 0 && i) dr_bars++;
            dr_beat_at(i);
        }
        dr_prev_tick = -1;
        dr_loops++;
        dr_bars++;
    }
    for (i = dr_prev_tick + 1; i <= tick; i++) {
        if (i % KIT_TICKS_PER_BAR == 0 && i > 0) dr_bars++;
        dr_beat_at(i);
    }
    if (tick / 6 != dr_prev_tick / 6) dirty = 1;
    dr_prev_tick = tick;
}

/* the feel, drawn (R-143): four rows (kick, snare, hats, the rest) over a bar; each hit where the track's FEEL puts
 * it, a line from its place on the grid */
static void dr_feel_graph(int y, int style, int variant, int ghosts)
{
    static const char *const ROW[4] = { "kick", "snare", "hats", "perc" };
    ds_hit_t h[96];
    int n = ds_hits(style, variant, ghosts, h, 96), i, r, x0 = 36, w = 196;
    for (r = 0; r < 4; r++) {
        kit_text(4, y + r * 12 + 1, ROW[r], KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
        kit_rect(x0, y + r * 12 + 5, w, 1, LINE);
    }
    for (i = 0; i <= 16; i += 4) kit_rect(x0 + i * w / 16, y, 1, 46, LINE);
    for (i = 0; i < n; i++) {
        int g = kit_drum_group(dr_src_kit(track, h[i].pad), dr_pads[track][h[i].pad].src);
        int fx = rl_feel_fine(track, g, h[i].tick), gx = x0 + h[i].tick * w / 96, x = x0 + fx * w / (96 * 16);
        int yy = y + g * 12 + 5;
        if (x != gx) kit_rect(gx < x ? gx : x, yy, (x > gx ? x - gx : gx - x) + 1, 1, MUTED);   /* from the grid */
        kit_rect(gx, yy - 3, 1, 7, MUTED);
        kit_round_rect(x - 2, yy - 3, 5, 7, 2, DR_GROUP_COL[g]);
    }
}

static void dr_falling(int y0, int y1, int style, int variant)   /* IDEAS: the next hits falling onto their keys */
{
    ds_hit_t h[96];
    int n = ds_hits(style, variant, 1, h, 96), i, tick = kit_seq_playing() ? kit_seq_tick() : -1, k;
    int span = KIT_TICKS_PER_BAR;               /* a bar ahead */
    if (tick < 0) return;
    for (k = 0; k < 2; k++)
        for (i = 0; i < n; i++) {
            int at = (tick / KIT_TICKS_PER_BAR + k) * KIT_TICKS_PER_BAR + h[i].tick, d = at - tick, key = h[i].pad, wi = 0, j, x, yy;
            if (d < 0 || d > span) continue;
            for (j = 0; j < key; j++) if (kit_is_white(KIT_DRUM_LO + j)) wi++;
            x = kit_is_white(KIT_DRUM_LO + key) ? wi * 15 + 3 : wi * 15 - 4;
            yy = y1 - (y1 - y0) * d / span - 6;
            kit_round_rect(x, yy, 8, 6, 2, DR_GROUP_COL[kit_drum_group(dr_src_kit(track, key), dr_pads[track][key].src)]);
        }
    kit_rect(0, y1, KIT_W, 1, MUTED);
}

static void draw_drums_ideas(void)
{
    dr_track_t *d = &drt[track];
    char v[12];
    kit_clear(BG);
    top("IDEAS");
    card(0, "STYLE", DS_STYLES[d->istyle].name, 0);
    card(1, "BEAT", DV_NAMES[d->ivar], d->ivar != 0);
    cat(cat_int(v, MEL_SPEEDS[d->speed]), " %");
    card(2, "SPEED", v, d->speed != MEL_NSPEEDS - 1);
    card(3, "HEAR", BASS_HEAR[d->hear], d->hear != HEAR_ON);
    kit_text(8, 68, kit_seq_playing() ? (d->hear == HEAR_LISTEN ? (dr_loops % 2 == 0 ? "Listen" : "Your turn") : d->hear == HEAR_ON ? "Play along" : "On your own") : "PLAY: the beat falls onto its keys",
             KIT_FONT_XSMALL, KIT_ALIGN_LEFT, kit_seq_playing() ? ACC : MUTED);
    dr_feel_graph(84, d->istyle, d->ivar, 1);
    dr_falling(134, 168, d->istyle, d->ivar);
    dr_keys(170, 70);
}

static void draw_drums_auto(void)
{
    dr_track_t *d = &drt[track];
    int v, now = dr_pad_var >= 0 ? ds_variant_at(dr_pad_var, d->fills, dr_bars) : -1;
    kit_clear(BG);
    top("AUTO");
    card(0, "STYLE", DS_STYLES[d->astyle].name, 0);
    card(1, "FILLS", DR_FILLS[d->fills], d->fills != 0);
    card(2, "FEEL", rl_feel_word(track), rl_feel[track] != 0);
    card(3, "GHOSTS", DR_GHOSTS[d->ghosts], d->ghosts != 1);
    for (v = 0; v < DV_N; v++) {                /* the five pads, the one playing lit */
        int x = 4 + v * 47;
        kit_round_rect(x, 68, 44, 20, 5, v == now ? ACC : v == dr_pad_var ? ACC_D : CARD);
        ctext(x + 22, 78, DV_NAMES[v], KIT_FONT_TINY, v == now ? BG : FG);
    }
    dr_feel_graph(98, d->astyle, now >= 0 ? now : DV_MAIN, d->ghosts);
    kit_text(8, 152, dr_pad_var >= 0 ? "" : "hold a pad: F3 main, G3 busy, A3 fill, B3 break, C4 drop", KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
    dr_keys(170, 70);
}

static void dr_ideas_lights(void)               /* IDEAS: the key of the next hit breathes, a hit now is lit */
{
    ds_hit_t h[96];
    int n, i, k, tick = kit_seq_playing() ? kit_seq_tick() : -1, best = 9999, next = -1;
    uint8_t lv[KIT_NKEYS];
    for (k = 0; k < KIT_NKEYS; k++) lv[k] = KIT_LIGHT_OFF;
    if (tick >= 0) {
        n = ds_hits(drt[track].istyle, drt[track].ivar, 1, h, 96);
        for (i = 0; i < n; i++) {
            int d = (h[i].tick - tick % KIT_TICKS_PER_BAR + KIT_TICKS_PER_BAR) % KIT_TICKS_PER_BAR;
            if (d <= 2 || d >= KIT_TICKS_PER_BAR - 3) lv[h[i].pad] = KIT_LIGHT_ON;   /* landing, and just landed */
            else if (d < best) { best = d; next = h[i].pad; }
        }
        if (next >= 0 && lv[next] != KIT_LIGHT_ON) lv[next] = KIT_LIGHT_BREATH;
    }
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, kit_key_held(k) ? KIT_LIGHT_ON : lv[k]);
}

/* the recorded beat's bar (the one playing, else the first): each hit where it plays (kit_seq_place: GRID and FEEL),
 * a tick at its place on the grid and a line between (R-143) */
static void dr_rec_graph(int y)
{
    static const char *const ROW[4] = { "kick", "snare", "hats", "perc" };
    int i, r, x0 = 36, w = 196, b = kit_seq_playing() ? kit_seq_tick() / KIT_TICKS_PER_BAR : 0, g0 = rl_grid_ticks(track);
    for (r = 0; r < 4; r++) {
        kit_text(4, y + r * 12 + 1, ROW[r], KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
        kit_rect(x0, y + r * 12 + 5, w, 1, LINE);
    }
    for (i = 0; i <= 16; i += 4) kit_rect(x0 + i * w / 16, y, 1, 46, LINE);
    for (i = 0; i < song.count[track]; i++) {
        const kit_song_note_t *v = &song.notes[track][i];
        int pad = kit_drum_pad_of(v->note), g = kit_drum_group(dr_src_kit(track, pad), dr_pads[track][pad].src);
        int at = kit_seq_place(&song, track, i), gt = g0 ? (v->start + g0 / 2) / g0 * g0 : v->start, yy = y + g * 12 + 5, x, gx;
        if (at / 16 / KIT_TICKS_PER_BAR != b && gt / KIT_TICKS_PER_BAR != b) continue;
        x = x0 + (at - b * KIT_TICKS_PER_BAR * 16) * w / (KIT_TICKS_PER_BAR * 16);
        gx = x0 + (gt - b * KIT_TICKS_PER_BAR) * w / KIT_TICKS_PER_BAR;
        if (x < x0 - 4 || x > x0 + w + 4) continue;
        if (x != gx) kit_rect(gx < x ? gx : x, yy, (x > gx ? x - gx : gx - x) + 1, 1, MUTED);
        kit_rect(gx, yy - 3, 1, 7, MUTED);
        kit_round_rect(x - 2, yy - 3, 5, 7, 2, DR_GROUP_COL[g]);
    }
}
