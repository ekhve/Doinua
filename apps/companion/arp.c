/* SPDX-License-Identifier: GPL-3.0-only */
/* ARP on every track (ST-77, ST-78, D-088), as the Arp Lab the owner reviewed on 2026-11-19 (R-302..R-306, approved).
 * ARP tapped: the track's arpeggiator on or off (R-302); tapped twice within 350 ms: latch on or off, and ARP on (R-303).
 * While it is on, the keys feed it instead of playing, and the ARP page shows: K1 MODE, K2 RATE, K3 OCTAVES, K4 GATE,
 * SELECT the rhythm (a menu: number, name and its 16 steps, R-306); it plays what is held in MODE's order over OCTAVES
 * at RATE, each note GATE long, in the rhythm's steps (rests, ghosts, notes, accents), in time with the song (its ticks
 * while it plays, its tempo when stopped) (R-304). On CHORDS a key arpeggiates its chord (the key's triad in the song's
 * key); on DRUMS it is note repeat: the sound held repeats at RATE, K1 RAMP (flat, crescendo, fade) its velocity and K3
 * PITCH (flat, rise, fall) its tuning while held, two held alternate (R-305). What it plays goes through the song player,
 * so REC records it (kit_seq_note_on), and out over MIDI. Each track keeps its settings, with the song. */
#define AR_NMODES 11
#define AR_NRATES 6
#define AR_NGATES 5
#define AR_NRHY 22
#define AR_MAXN 64
static const char *const AR_MODES[AR_NMODES] = { "up", "down", "up-down", "down-up", "as played", "random", "converge",
                                                 "diverge", "pinky", "thumb", "chord" };
static const char *const AR_RATES[AR_NRATES] = { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const uint8_t AR_RTICKS[AR_NRATES] = { 24, 12, 8, 6, 4, 3 };   /* song ticks a step (24 a beat) */
static const uint8_t AR_GATES[AR_NGATES] = { 15, 30, 50, 75, 98 };   /* % of a step */
static const char *const AR_RAMPS[3] = { "flat", "crescendo", "fade" };
static const char *const AR_PITCHES[3] = { "flat", "rise", "fall" };
/* 16 steps each: 0 a rest, g a ghost, 1 a note, 2 an accent (the lab's; many are drum patterns, good on melodies too) */
static const char *const AR_RHY[AR_NRHY][2] = {
    { "straight", "1111111111111111" }, { "accents", "2111211121112111" }, { "hats 2-1", "2g1g2g1g2g1g2g1g" },
    { "ghost 16ths", "2gg12gg12gg12gg1" }, { "offbeat", "0101010101010101" }, { "house hats", "0020002000200020" },
    { "four on the floor", "2000200020002000" }, { "half-time", "2000000010000000" }, { "3-3-2", "2010012010020100" },
    { "reggaeton", "2001001020010010" }, { "son clave", "2001001000201000" }, { "rumba clave", "2001000100201000" },
    { "bossa", "2001001000100100" }, { "afrobeat", "2010011020100110" }, { "shuffle", "2g12g12g12g12g12" },
    { "skip", "2101101121011011" }, { "gallop", "2110211021102110" }, { "dotted", "2001001001001000" },
    { "pushes", "1001101001001010" }, { "trap roll", "2g1g2g1g2g1g2111" }, { "stutter", "2220000022200000" },
    { "build", "2000200020202222" } };
static int ar_level(char c) { return c == '2' ? 100 : c == '1' ? 72 : c == 'g' ? 35 : 0; }   /* % of the velocity */

typedef struct { uint8_t on, latch, mode, rate, oct, gate, rhythm, ramp, pitch; } arp_t;
static arp_t arps[KIT_NTRACKS] = { { 0, 0, 0, 3, 1, 2, 0, 0, 0 }, { 0, 0, 0, 3, 1, 2, 0, 0, 0 },
                                   { 0, 0, 0, 3, 1, 2, 0, 0, 0 }, { 0, 0, 0, 3, 1, 2, 0, 0, 0 } };
static arp_t ar_before_factory[KIT_NTRACKS];
static uint8_t ar_factory_gate[KIT_NTRACKS], ar_factory[KIT_NTRACKS];
static int8_t ar_keys[16];                      /* the keys held, as pressed; then latched while latch is on */
static uint8_t ar_vels[16];
static int ar_nkeys, ar_nheld;                  /* keys in ar_keys; of those, held now (0 with latch: latched) */
static int ar_idx, ar_step, ar_last_tick = -1, ar_running, ar_track = -1;
static uint32_t ar_next_ms, ar_hold_ms, ar_tap_ms, ar_menu_until, ar_flash_until, ar_off_at;
static const char *ar_flash;
static struct { uint8_t note, track; uint32_t off; } ar_on[16];
static int ar_non;
static uint8_t ar_shown[8];                     /* the notes of the step sounding (for the page) */
static int ar_nshown, ar_cur;

static int ar_page(void) { return mode == MODE_PRODUCER && arps[track].on && sq_view < 0 && !song_page && !help; }

static int ar_triad(int n, int *out)            /* the key's chord: its triad in the song's key (a major one off the key) */
{
    static const int8_t MAJ[7] = { 0, 2, 4, 5, 7, 9, 11 }, MIN[7] = { 0, 2, 3, 5, 7, 8, 10 };
    const int8_t *sc = pads[track].minor ? MIN : MAJ;
    int pc = ((n - pads[track].tonic) % 12 + 12) % 12, d, k;
    for (d = 0; d < 7 && sc[d] != pc; d++) ;
    if (d == 7) { out[0] = n; out[1] = n + 4; out[2] = n + 7; return 3; }
    for (k = 0; k < 3; k++) {
        int dd = d + 2 * k, up = sc[dd % 7] + 12 * (dd / 7) - sc[d];
        out[k] = n + up;
    }
    return 3;
}

static int ar_source(int *notes)                /* what is held (or latched), as notes, in the order played */
{
    int i, n = 0, j, k, t[3];
    for (i = 0; i < ar_nkeys && n < AR_MAXN - 3; i++) {
        int base = is_drums() ? KIT_DRUM_LO + ar_keys[i] : key_note(ar_keys[i]);
        if (is_chords() && !is_drums()) {
            ar_triad(base, t);
            for (k = 0; k < 3; k++) { for (j = 0; j < n && notes[j] != t[k]; j++) ; if (j == n) notes[n++] = t[k]; }
        } else {
            for (j = 0; j < n && notes[j] != base; j++) ;
            if (j == n) notes[n++] = base;
        }
    }
    return n;
}

static int ar_order(int *seq)                   /* the notes in MODE's order over OCTAVES (CHORD: one step of all) */
{
    int src[AR_MAXN], up[AR_MAXN], n = ar_source(src), nu = 0, i, j, o, out = 0, oc = is_drums() ? 1 : arps[track].oct;
    const arp_t *a = &arps[track];
    if (!n) return 0;
    for (o = 0; o < oc; o++) for (i = 0; i < n && nu < AR_MAXN; i++) up[nu++] = src[i] + 12 * o;   /* as played, over the octaves */
    if (a->mode == 4 || is_drums()) { for (i = 0; i < nu; i++) seq[out++] = up[i]; return out; }
    for (i = 1; i < nu; i++) { int v = up[i]; for (j = i; j > 0 && up[j - 1] > v; j--) up[j] = up[j - 1]; up[j] = v; }   /* low to high */
    switch (a->mode) {
    case 0: case 5: case 10: for (i = 0; i < nu; i++) seq[out++] = up[i]; break;          /* up; random and chord draw from it */
    case 1: for (i = nu - 1; i >= 0; i--) seq[out++] = up[i]; break;                       /* down */
    case 2: for (i = 0; i < nu; i++) seq[out++] = up[i]; for (i = nu - 2; i >= 1; i--) seq[out++] = up[i]; break;
    case 3: for (i = nu - 1; i >= 0; i--) seq[out++] = up[i]; for (i = 1; i <= nu - 2; i++) seq[out++] = up[i]; break;
    case 6: { int lo = 0, hi = nu - 1; while (lo <= hi) { seq[out++] = up[lo++]; if (lo <= hi) seq[out++] = up[hi--]; } break; }
    case 7: { int mid = (nu - 1) / 2, k; for (k = 0; out < nu; k++) { if (mid - k >= 0) seq[out++] = up[mid - k]; if (k && mid + k < nu) seq[out++] = up[mid + k]; } break; }
    case 8: for (i = 0; i < nu - 1; i++) { seq[out++] = up[i]; seq[out++] = up[nu - 1]; } if (!out) seq[out++] = up[0]; break;
    case 9: for (i = 1; i < nu; i++) { seq[out++] = up[0]; seq[out++] = up[i]; } if (!out) seq[out++] = up[0]; break;
    default: break;
    }
    return out;
}

static void ar_note_off_all(void)
{
    int i;
    for (i = 0; i < ar_non; i++) {
        kit_seq_note_off(ar_on[i].track, ar_on[i].note);
        kit_midi_note_off(channel_of(ar_on[i].track), ar_on[i].note);
    }
    ar_non = 0;
    ar_nshown = 0;
}

static void ar_stop(void)                       /* nothing to play: what sounds lets go, the order starts over */
{
    ar_note_off_all();
    if (ar_track >= 0 && ar_track < KIT_NTRACKS && arps[ar_track].pitch) dr_apply(ar_track, 0);
    ar_running = 0;
    ar_idx = 0;
    ar_step = 0;
    ar_last_tick = -1;
}

static void ar_clear(void) { ar_stop(); ar_nkeys = ar_nheld = 0; }

static void ar_load_preset(int t, int engine, int preset)
{
    static const uint8_t modes[] = {0, 1, 2, 4, 5};
    kit_preset_arp_t factory;
    int rate;
    if (!kit_preset_arp(engine, preset, &factory)) {
        if (ar_factory[t]) {
            if (t == ar_track) ar_clear();
            arps[t] = ar_before_factory[t];
            ar_factory[t] = ar_factory_gate[t] = 0;
        }
        return;
    }
    for (rate = 0; rate < AR_NRATES && AR_RTICKS[rate] != factory.ticks; ++rate) {}
    if (rate == AR_NRATES) return;
    if (t == ar_track) ar_clear();
    int gate = 0, error = 12800;
    for (int i = 0; i < AR_NGATES; ++i) {
        int delta = AR_GATES[i] * 128 - factory.gate * 100;
        if (delta < 0) delta = -delta;
        if (delta < error) { gate = i; error = delta; }
    }
    if (!ar_factory[t]) ar_before_factory[t] = arps[t];
    arps[t] = (arp_t){1, 0, modes[factory.mode], (uint8_t)rate, factory.octaves,
                       (uint8_t)gate, 0, 0, 0};
    ar_factory[t] = 1;
    ar_factory_gate[t] = factory.gate;
}


static uint32_t ar_step_ms(void) { return 60000u * AR_RTICKS[arps[track].rate] / (24u * (uint32_t)(song.bpm ? song.bpm : 90)); }

static void ar_play_step(uint32_t now)          /* one step of the rhythm: a note (or the chord, or a rest) */
{
    const arp_t *a = &arps[track];
    int seq[AR_MAXN * 2], n = ar_order(seq), lvl = ar_level(AR_RHY[a->rhythm][1][ar_step % 16]), i;
    ar_step++;
    if (!n || !lvl) return;
    {
        uint32_t held = now - ar_hold_ms, off = now + (ar_factory_gate[track] ? ar_step_ms() * ar_factory_gate[track] / 128u
                                                           : ar_step_ms() * AR_GATES[a->gate] / 100u);
        int vel = (ar_nkeys ? ar_vels[0] : 100) * lvl / 100, cnt = a->mode == 10 && !is_drums() ? n : 1, first = 0;
        if (is_drums()) {                       /* note repeat: the ramp on the velocity, the pitch on the tune */
            int r = (int)(held / 10u);          /* centiseconds held */
            if (a->ramp == 1) vel = vel * (25 + (r * 40 / 100 > 75 ? 75 : r * 40 / 100)) / 100;
            if (a->ramp == 2) vel = vel * (100 - (r * 35 / 100 > 88 ? 88 : r * 35 / 100)) / 100;
        }
        if (vel < 1) vel = 1;
        if (vel > 127) vel = 127;
        if (a->mode == 5 && !is_drums()) first = (int)(kit_random() % (uint32_t)n);   /* random: any note of them */
        else if (cnt == 1) first = ar_idx % n;
        ar_cur = cnt == 1 ? first : 0;
        ar_nshown = 0;
        for (i = 0; i < cnt; i++) {
            int note = seq[(first + i) % n];
            if (note < 0 || note > 127) continue;
            if (is_drums() && a->pitch) {       /* PITCH: up or down an octave over 4 s held (the pad's tune) */
                int key = note - KIT_DRUM_LO, st = (int)(held / 333u);
                dr_pad_t *d = &dr_pads[track][key < 0 ? 0 : key];
                if (st > 12) st = 12;
                if (key >= 0 && key < KIT_DRUM_PADS)
                    kit_drum_pad_set(track, key, d->kit, d->src, d->tune + (a->pitch == 1 ? st : -st), DR_DECAYS[d->decay]);
            }
            kit_seq_note_on(track, note, vel);
            kit_midi_note_on(channel_of(track), note, vel);
            if (ar_non < 16) { ar_on[ar_non].note = (uint8_t)note; ar_on[ar_non].track = (uint8_t)track; ar_on[ar_non].off = off; ar_non++; }
            if (ar_nshown < 8) ar_shown[ar_nshown++] = (uint8_t)note;
        }
        ar_idx++;
        dirty = 1;
    }
}

static void ar_frame(uint32_t now)
{
    int i;
    for (i = 0; i < ar_non; ) {                 /* the gate: each note lets go GATE into its step */
        if ((int32_t)(now - ar_on[i].off) >= 0) {
            kit_seq_note_off(ar_on[i].track, ar_on[i].note);
            kit_midi_note_off(channel_of(ar_on[i].track), ar_on[i].note);
            ar_on[i] = ar_on[--ar_non];
        } else i++;
    }
    if (ar_menu_until && (int32_t)(now - ar_menu_until) >= 0) { ar_menu_until = 0; dirty = 1; }
    if (ar_flash_until && (int32_t)(now - ar_flash_until) >= 0) { ar_flash_until = 0; dirty = 1; }
    if (ar_off_at && (int32_t)(now - ar_off_at) >= 0) { ar_off_at = 0; if (!arps[track].on) ar_clear(); }
    if (ar_track != track) { ar_clear(); ar_track = track; }
    if (mode != MODE_PRODUCER || !arps[track].on || !ar_nkeys) { if (ar_running) ar_stop(); return; }
    if (kit_seq_playing()) {                    /* in time with the song: a step each RATE of its ticks */
        int tk = kit_seq_tick(), r = AR_RTICKS[arps[track].rate], st = tk / r;
        if (tk < 0) return;
        if (!ar_running || st != ar_last_tick) { ar_last_tick = st; ar_running = 1; ar_step = st % 16; ar_play_step(now); }
    } else {                                    /* stopped: its own clock at the song's tempo */
        if (!ar_running) { ar_running = 1; ar_next_ms = now; ar_last_tick = -1; }
        if ((int32_t)(now - ar_next_ms) >= 0) { ar_play_step(now); ar_next_ms += ar_step_ms(); if ((int32_t)(now - ar_next_ms) > 0) ar_next_ms = now + ar_step_ms(); }
    }
}

static int ar_event(const kit_event_t *e)
{
    arp_t *a = &arps[track];
    if (mode != MODE_PRODUCER || picker || song_page || help || sq_view >= 0)
        return 0;
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_ARP) {
        uint32_t now = kit_ms();
        if (now - ar_tap_ms < 350u) {           /* twice: latch (R-303), and ARP on (the first tap undone: the keys stay) */
            ar_off_at = 0;
            a->latch = !a->latch;
            a->on = 1;
            ar_flash = a->latch ? "LATCH" : "latch off";
            ar_flash_until = now + 900u;
            if (!a->latch && !ar_nheld) ar_clear();
        } else {                                /* once: on or off (R-302) */
            a->on = !a->on;
            if (!a->on) { ar_stop(); ar_off_at = now + 350u; }   /* off: what it holds goes 350 ms later, unless a second tap */
            else { ar_off_at = 0; chord_release(); kit_track_notes_off(track); }
        }
        ar_tap_ms = now;
        dirty = 1;
        return 1;
    }
    if (!a->on && !(ar_off_at && (e->type == KIT_EV_KEY_UP)))
        return 0;
    if (!a->on && e->type == KIT_EV_KEY_UP) { if (ar_nheld > 0) ar_nheld--; return 0; }   /* (between a double tap's taps) */
    if (e->type == KIT_EV_KEY_DOWN) {
        if (!ar_nheld) { ar_nkeys = 0; ar_hold_ms = kit_ms(); ar_idx = 0; }   /* a new hold replaces what was latched */
        if (ar_nkeys < 16) { ar_keys[ar_nkeys] = (int8_t)e->id; ar_vels[ar_nkeys] = (uint8_t)(e->vel ? e->vel : 100); ar_nkeys++; }
        ar_nheld++;
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_KEY_UP) {
        int i, j;
        if (ar_nheld > 0) ar_nheld--;
        if (!a->latch)                          /* without latch the key leaves the arpeggio */
            for (i = 0; i < ar_nkeys; i++) if (ar_keys[i] == e->id) { for (j = i; j < ar_nkeys - 1; j++) { ar_keys[j] = ar_keys[j + 1]; ar_vels[j] = ar_vels[j + 1]; } ar_nkeys--; break; }
        if (!ar_nkeys) ar_stop();
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_ENC) {
        int v;
        if (e->id == KIT_ENC_KNOB1) { v = is_drums() ? a->ramp : a->mode; clampi(&v, e->delta, 0, is_drums() ? 2 : AR_NMODES - 1); if (is_drums()) a->ramp = (uint8_t)v; else { a->mode = (uint8_t)v; ar_idx = 0; } }
        else if (e->id == KIT_ENC_KNOB2) { v = a->rate; clampi(&v, e->delta, 0, AR_NRATES - 1); a->rate = (uint8_t)v; }
        else if (e->id == KIT_ENC_KNOB3) { if (is_drums()) { v = a->pitch; clampi(&v, e->delta, 0, 2); a->pitch = (uint8_t)v; } else { v = a->oct; clampi(&v, e->delta, 1, 4); a->oct = (uint8_t)v; } }
        else if (e->id == KIT_ENC_KNOB4) { v = a->gate; clampi(&v, e->delta, 0, AR_NGATES - 1); a->gate = (uint8_t)v; ar_factory_gate[track] = 0; }
        else if (e->id == KIT_ENC_SELECT) { v = a->rhythm; clampi(&v, e->delta, 0, AR_NRHY - 1); a->rhythm = (uint8_t)v; ar_menu_until = kit_ms() + 1600u; }   /* R-306 */
        else return 0;                          /* PRESETS, ALGORITHM: as on any page */
        dirty = 1;
        return 1;
    }
    return 0;
}

static const char *ar_name(int n, char *s)      /* C4, a drum's number on DRUMS */
{
    static const char *const NN[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (is_drums()) { cat_int(cat(s, "#"), n - KIT_DRUM_LO + 1); return s; }
    cat_int(cat(s, NN[n % 12]), n / 12 - 1);
    return s;
}

#define AR_REST KIT_RGB(17, 21, 29)
#define AR_ACC2 KIT_RGB(77, 111, 174)
#define AR_GHOST KIT_RGB(28, 35, 51)
#define AR_NOTE KIT_RGB(43, 54, 80)
static void ar_bars(int x0, int base, int step10, int w, int hmax, const char *pat, int lit, int on_menu)   /* 16 steps */
{
    int k;
    for (k = 0; k < 16; k++) {
        int lv = ar_level(pat[k]), h = (on_menu ? 2 : 4) + hmax * lv / 100, x = x0 + k * step10 / 10;
        kit_color_t c = k == lit ? ACC : pat[k] == '0' ? AR_REST : on_menu ? (on_menu > 1 ? KIT_RGB(156, 192, 255) : KIT_RGB(93, 107, 134))
                      : pat[k] == '2' ? AR_ACC2 : pat[k] == 'g' ? AR_GHOST : AR_NOTE;
        if (on_menu && pat[k] == '0') h = 2;
        if (on_menu) kit_rect(x, base - h, w, h, c);
        else kit_round_rect(x, base - h, w, h, 2, c);
    }
}

static void ar_draw(void)                       /* the ARP page, as the lab */
{
    const arp_t *a = &arps[track];
    char s[40], v[16], *d;
    int seq[AR_MAXN * 2], n = ar_order(seq), i, r;
    kit_clear(BG);
    ch_top("ARP");
    {                                           /* ARP lit while on, LATCH beside it (the lab) */
        int w = kit_text_width(track_name(track), KIT_FONT_XSMALL), x = 6 + w + 8 > 56 ? 6 + w + 8 : 56;
        x += kit_text(x, 5, "ARP", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, a->on ? ACC : MUTED);
        if (a->latch) ch_small(x + 18, 12, "LATCH", WARN);   /* (small capitals: the top line's right holds key, tempo, octave) */
    }
    for (i = 0; i < 4; i++) {                   /* the knobs, as the lab's cards */
        const char *lb = i == 0 ? (is_drums() ? "RAMP" : "MODE") : i == 1 ? "RATE" : i == 2 ? (is_drums() ? "PITCH" : "OCTAVES") : "GATE";
        const char *vl = v;
        if (i == 0) vl = is_drums() ? AR_RAMPS[a->ramp] : AR_MODES[a->mode];
        else if (i == 1) vl = AR_RATES[a->rate];
        else if (i == 2) { if (is_drums()) vl = AR_PITCHES[a->pitch]; else cat_int(v, a->oct); }
        else cat(cat_int(v, ar_factory_gate[track] ? (ar_factory_gate[track] * 100 + 64) / 128
                                                  : AR_GATES[a->gate] > 95 ? 100 : AR_GATES[a->gate]), "%");
        exp_card(i, lb, vl, 0);
        kit_round_rect(4 + i * 58, 26, 54, 30, 5, CARD);
        ctext(4 + i * 58 + 27, 34, lb, KIT_FONT_TINY, MUTED);
        ctext(4 + i * 58 + 27, 47, vl, KIT_FONT_XSMALL, ACC);
    }
    d = cat_int(cat(s, "RHYTHM "), a->rhythm + 1);
    cat(cat(d, "  "), AR_RHY[a->rhythm][0]);
    kit_text(8, 63, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, FG);
    ar_bars(6, 98, 143, 12, 16, AR_RHY[a->rhythm][1], ar_running && ar_step ? (ar_step - 1) % 16 : -1, 0);
    d = cat(s, is_drums() ? "NOTE REPEAT" : "ORDER  ");
    if (!is_drums()) { d = cat(d, AR_MODES[a->mode]); if (a->oct > 1) { d = cat(d, "  "); d = cat_int(d, a->oct); cat(d, " oct"); } }
    kit_text(8, 107, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, FG);
    if (!n) ctext(120, 160, a->on ? "hold some keys" : "ARP is off: tap ARP", KIT_FONT_XSMALL, MUTED);
    else {
        int shown = a->mode == 10 && !is_drums() ? 1 : n > 16 ? 16 : n, w = shown ? 228 / shown : 28, cur = ar_running ? ar_cur % (shown ? shown : 1) : -1;
        if (w > 28) w = 28;
        for (i = 0; i < shown; i++) {
            int x = 6 + i * w, on = i == cur, k, cnt = a->mode == 10 && !is_drums() ? n : 1;
            kit_round_rect(x, 124, w - 3, 60, 4, on ? ACC : CARD);
            for (k = 0; k < cnt && k < 4; k++) {
                char nm[8];
                nm[0] = 0;
                ar_name(seq[cnt == 1 ? i : k], nm);
                ctext(x + (w - 3) / 2, 136 + k * 12, nm, KIT_FONT_TINY, on ? KIT_WHITE : MUTED);
            }
        }
    }
    d = cat(s, is_drums() ? "hold a sound: it repeats at RATE" : "REC: what it plays goes into SEQ ");
    if (!is_drums()) cat_int(d, cur_seq + 1);
    kit_text(8, 195, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
    if (ar_menu_until) {                        /* SELECT: the rhythms, number, name and their 16 steps (R-306) */
        kit_round_rect(4, 22, 232, 214, 8, ACC);
        kit_round_rect(5, 23, 230, 212, 7, KIT_RGB(10, 13, 19));
        d = cat_int(cat(s, "RHYTHM  "), a->rhythm + 1);
        cat_int(cat(d, " / "), AR_NRHY);
        kit_text(12, 28, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
        for (r = -3; r <= 3; r++) {
            int k = a->rhythm + r, y = 56 + (r + 3) * 25, on = r == 0;
            if (k < 0 || k >= AR_NRHY) continue;
            if (on) kit_round_rect(8, y - 11, 224, 22, 4, ACC_D);
            cat_int(v, k + 1);
            kit_text(14, y - 6, v, KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? KIT_WHITE : MUTED);
            kit_text(32, y - 6, AR_RHY[k][0], KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? KIT_WHITE : FG);
            ar_bars(130, y + 7, 63, 4, 12, AR_RHY[k][1], -1, on ? 2 : 1);
        }
    }
    if (ar_flash_until) { kit_round_rect(80, 210, 80, 20, 6, ACC_D); ctext(120, 214, ar_flash, KIT_FONT_TINY, KIT_WHITE); }
}

static void ar_lights(void)                     /* the keys held (or latched) lit */
{
    int k, i;
    for (k = 0; k < KIT_NKEYS; k++) {
        int on = 0;
        for (i = 0; i < ar_nkeys; i++) if (ar_keys[i] == k) on = 1;
        kit_key_light(k, on ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    }
    kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_ON);
    kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF);
}
