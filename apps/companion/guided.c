/* SPDX-License-Identifier: GPL-3.0-only */
/* The guided mode (ST-57, #98, D-068): lessons for a grown-up who never made a beat, as the beat-guide lab the owner
 * shaped. Its own simple room (#94): K1-K4 the mixer, ALGORITHM the tempo, PRESETS the drums, OCT the key, SELECT the
 * step, HOME tapped the lessons. The screen is layout E (the owner, 2026-11-16): the lesson and step, small; what to
 * do, the largest text, its colour saying to do (amber), done (green) or a warning (red); the step's words; the
 * sequencer of the lesson's song with the step's rows lit; one line for the knobs. The lesson's song plays on the
 * kit's sequencer, on its own kit_song_t; each track's level (API 26) is the mixer. The lesson and step are kept over
 * power-off (the settings record, version 6). Lesson 1 here; the others follow, a story each. */

enum { GS_PLAY, GS_K1, GS_K2, GS_K4, GS_K3OFF, GS_K3ON, GS_SOLO, GS_PARTS, GS_STYLES, GS_TEMPO, GS_KEY, GS_OCT, GS_MELODY,
       GS_FREE, G_STEPS };
#define G_LESSONS 9
static const char *const G_TITLES[G_LESSONS] = { "WHAT'S IN A BEAT", "TEMPO, YOUR FIRST BEAT", "GROOVE", "KEYS AND CHORDS",
    "CHORD COLOURS", "THE BASS", "THE MELODY", "WAYS TO START", "MAKE YOUR OWN BEAT" };
static const uint8_t G_NSTEPS[G_LESSONS] = { G_STEPS, 9, 10, 10, 8, 6, 6, 11, 9 };
/* each step's words (short: the screen carries everything, the owner, #98) */
static const char *const G_WORDS[G_STEPS] = {
    "This beat has 4 tracks: drums, bass, chords, melody. For now only the chords sound.",
    "The knobs K1-K4 are a mixer: each is the volume of one track. K1 is the drums.",
    "K2 is the bass: it locks to the kick and makes you nod.",
    "K4 is the melody: the part you would hum.",
    "Take the chords out. Hear how thin it gets?",
    "Bring them back. Chords are the colour: Am sad, F warm.",
    "Just the drums: the kick (low), the snare (the crack), the hats (the ticking).",
    "PRESETS changes the drums: same song, same speed.",
    "A style is the drums and the speed: each has its own.",
    "BPM, beats per minute, is the song's speed.",
    "A song has a key: a home note and a family of notes that fit. Ours is A minor.",
    "OCT moves the whole song to another key: every note up or down together.",
    "Melody out: you play it. The lit keys are the key: they all work.",
    "Your style, your speed, your mix, your melody.",
};

/* the drums: a pattern a style, 16 steps a bar, a digit a step (0 none, 1..9 soft to loud) */
typedef struct { const char *name; uint8_t bpm, kit, snare_pad; const char *k, *s, *g, *h, *o; } g_style_t;
#define G_PARTS 3
#define G_NSTYLES 5
static const g_style_t G_PART[G_PARTS] = {
    { "HIP-HOP 1", 88, KIT_DRUM_USER, 4, "9000000000800000", "0000800000008000", "0000000000000000", "8040804080408040", "0000000000000000" },
    { "HIP-HOP 2", 88, KIT_DRUM_USER, 4, "9005000500800000", "0000800000008000", "0000000002000002", "7252725272527250", "0000000000000003" },
    { "HALF-TIME", 88, KIT_DRUM_USER, 4, "9000000507000000", "0000000080000000", "0000000000000000", "8040804080408040", "0000000000000000" },
};
static const g_style_t G_STYLE[G_NSTYLES] = {
    { "HIP-HOP", 88, KIT_DRUM_USER, 4, "9000000000800000", "0000800000008000", "0000000000000000", "8040804080408040", "0000000000000000" },
    { "LO-FI", 76, 0, 4, "8000000000700000", "0000700000007000", "0000000002000000", "5030503050305030", "0000000000000000" },
    { "HOUSE", 120, 2, 9, "8000800080008000", "0000500000005000", "0000000000000000", "0050005000500050", "0000000000000000" },
    { "REGGAETON", 94, 3, 4, "8000800080008000", "0007007000070070", "0000000000000000", "3020302030203020", "0000000000000000" },
    { "TRAP", 140, 1, 4, "9000000000800000", "0000000090000000", "0000000000000000", "7050705070507055", "0000000000000000" },
};
#define G_KICK 0                                /* the pads: kick1, ghost1, hat1, open1 */
#define G_GHOST 11
#define G_HAT 12
#define G_OPEN 15
/* the song (the lab's first): Am7 | Fmaj7 | Am7 | Fmaj7, the bass on root and octave, a sparse hook */
static const uint8_t G_CHORD[2][4] = { { 57, 60, 64, 67 }, { 53, 57, 60, 64 } };
static const uint8_t G_BASS[2][7][3] = {           /* step (1..16), note, length in steps */
    { { 1, 45, 3 }, { 4, 45, 1 }, { 6, 57, 1 }, { 8, 45, 2 }, { 11, 45, 1 }, { 12, 57, 1 }, { 14, 43, 2 } },
    { { 1, 41, 3 }, { 4, 41, 1 }, { 6, 53, 1 }, { 8, 41, 2 }, { 11, 41, 1 }, { 12, 53, 1 }, { 14, 40, 2 } } };
static const uint8_t G_MEL[12][4] = { { 0, 3, 76, 2 }, { 0, 5, 74, 2 }, { 0, 7, 72, 8 }, { 1, 3, 69, 2 }, { 1, 5, 72, 10 },
    { 2, 3, 76, 2 }, { 2, 5, 74, 2 }, { 2, 7, 76, 2 }, { 2, 9, 79, 8 }, { 3, 5, 76, 2 }, { 3, 7, 74, 2 }, { 3, 9, 72, 8 } };
#define G_SIXT (KIT_TICKS_PER_BEAT / 4)         /* a sixteenth, in ticks */
#define G_TA KIT_RGB(255, 181, 71)              /* amber: to do */
#define G_TG KIT_RGB(79, 192, 127)              /* green: done */
#define G_TR KIT_RGB(255, 100, 100)             /* red: a warning */
#define G_INK KIT_RGB(241, 239, 232)
#define G_DIMT KIT_RGB(143, 151, 179)
#define G_OFF KIT_RGB(59, 66, 86)
static const kit_color_t G_TRK_COL[4] = { KIT_RGB(255, 181, 71), KIT_RGB(169, 139, 255), KIT_RGB(69, 214, 194), KIT_RGB(255, 122, 184) };

static kit_song_t g_song;
static struct {
    int on, step, list, list_sel, playing;
    int vol[4];                                 /* the mixer: drums, bass, chords, melody (K1..K4) */
    int part, style, styles_on, bpm, key, solo; /* the drums, the speed, the key (semitones from A minor), one drum alone */
    uint8_t solo_seen, parts_seen, styles_seen;
    int bpm_moved, key_moved, home_hit, dim_hits, lit_notes;
    int was_done;
    uint32_t chime_at, note_at, out_at;
    int out_note;
    const char *note;
    int saved_e[4], saved_p[4], saved_lv[4], saved_rv;
    uint8_t held[27];
} G;

static int g_scale_has(int n) { int pc = ((n - 57 - G.key) % 12 + 12) % 12; return pc == 0 || pc == 2 || pc == 3 || pc == 5 || pc == 7 || pc == 8 || pc == 10; }
static int g_is_home(int n) { return ((n - 57 - G.key) % 12 + 12) % 12 == 0; }
static const g_style_t *g_drums(void) { return G.styles_on ? &G_STYLE[G.style] : &G_PART[G.part]; }
static int g_melody_yours(void) { return G.step >= GS_KEY; }   /* from the key on, the melody track is the keys' */

/* ---- the song ---- */
static void g_add_drum(const char *pat, int pad, char which)
{
    int s, bar;
    if (G.solo && G.solo != which) return;      /* one drum alone (step 7) */
    for (bar = 0; bar < 4; bar++)
        for (s = 0; s < 16; s++)
            if (pat[s] > '0') kit_song_add(&g_song, KIT_TRACK_DRUMS, bar * KIT_TICKS_PER_BAR + s * G_SIXT, G_SIXT, KIT_DRUM_LO + pad, 30 + (pat[s] - '0') * 10);
}
static void g_build(void)
{
    const g_style_t *d = g_drums();
    int bar, i, t = G.key;
    kit_song_init(&g_song, G.bpm, 4);
    kit_song_lock_key(&g_song, (9 + G.key + 12) % 12, 1);
    for (bar = 0; bar < 4; bar++) {
        const uint8_t *ch = G_CHORD[bar % 2], *late = bar == 3 ? G_CHORD[0] : ch;   /* the last bar's late hit is the next chord: a push */
        for (i = 0; i < 4; i++) {
            kit_song_add(&g_song, KIT_TRACK_CHORDS, bar * KIT_TICKS_PER_BAR, 13 * G_SIXT + 3, ch[i] + t, 96);
            kit_song_add(&g_song, KIT_TRACK_CHORDS, bar * KIT_TICKS_PER_BAR + 14 * G_SIXT, 2 * G_SIXT - 1, late[i] + t, 70);
        }
        for (i = 0; i < 7; i++) { const uint8_t *b = G_BASS[bar % 2][i]; kit_song_add(&g_song, KIT_TRACK_BASS, bar * KIT_TICKS_PER_BAR + (b[0] - 1) * G_SIXT, b[2] * G_SIXT - 1, b[1] + t, 110); }
    }
    if (!g_melody_yours())
        for (i = 0; i < 12; i++) kit_song_add(&g_song, KIT_TRACK_MELODY, G_MEL[i][0] * KIT_TICKS_PER_BAR + (G_MEL[i][1] - 1) * G_SIXT, G_MEL[i][3] * G_SIXT - 1, G_MEL[i][2] + t, 100);
    g_add_drum(d->k, G_KICK, 'k');
    g_add_drum(d->s, d->snare_pad, 's');
    g_add_drum(d->g, G_GHOST, 's');
    g_add_drum(d->h, G_HAT, 'h');
    g_add_drum(d->o, G_OPEN, 'h');
    if (kit_track_engine(KIT_TRACK_DRUMS) != KIT_ENGINE_DRUMS || kit_track_preset(KIT_TRACK_DRUMS) != d->kit)
        kit_track_sound(KIT_TRACK_DRUMS, KIT_ENGINE_DRUMS, d->kit);   /* only when it changes: switching stops what sounds */
}
static void g_levels(void)
{
    int i;
    for (i = 0; i < 4; i++) kit_track_level(i == 0 ? KIT_TRACK_DRUMS : i == 1 ? KIT_TRACK_BASS : i == 2 ? KIT_TRACK_CHORDS : KIT_TRACK_MELODY, G.vol[i]);
    if (g_melody_yours()) kit_track_level(KIT_TRACK_MELODY, 10);   /* the keys' melody: always heard (K4 shows "you") */
}
static void g_mix(int d, int b, int c, int m) { G.vol[0] = d; G.vol[1] = b; G.vol[2] = c; G.vol[3] = m; g_levels(); }
static void g_restart(void) { if (G.playing) kit_seq_play(&g_song); }

/* ---- the steps: what each sets up, and when it is done ---- */
static void g_enter_step(void)
{
    G.solo = 0; G.note = 0;
    if (G.step <= GS_K1 || G.step == GS_PLAY) { G.styles_on = 0; G.part = 0; G.bpm = 88; }
    switch (G.step) {
    case GS_PLAY: g_mix(0, 0, 8, 0); break;
    case GS_SOLO: g_mix(8, 0, 0, 0); G.solo_seen = 0; break;
    case GS_PARTS: G.styles_on = 0; G.part = 0; G.bpm = 88; G.parts_seen = 1; g_mix(8, 7, 8, 6); break;
    case GS_STYLES: G.styles_on = 1; G.style = 0; G.bpm = G_STYLE[0].bpm; G.styles_seen = 1; break;
    case GS_TEMPO: G.bpm_moved = 0; break;
    case GS_KEY: G.styles_on = 1; G.style = 1; G.bpm = G_STYLE[1].bpm; G.home_hit = 0; G.dim_hits = 0; g_mix(0, 0, 8, 0); break;
    case GS_OCT: G.key_moved = 0; g_mix(6, 6, 8, 6); break;
    case GS_MELODY: G.lit_notes = 0; g_mix(6, 6, 8, 0); break;
    default: break;
    }
    g_build(); g_levels(); g_restart();
    G.was_done = 1;                             /* a step done as it opens does not chime */
    set.g_lesson = 0; set.g_step = (uint8_t)G.step; settings_save();
}
static int g_done(void)
{
    switch (G.step) {
    case GS_PLAY: return G.playing;
    case GS_K1: return G.vol[0] >= 6;
    case GS_K2: return G.vol[1] >= 6;
    case GS_K4: return G.vol[3] >= 6;
    case GS_K3OFF: return G.vol[2] == 0;
    case GS_K3ON: return G.vol[2] >= 6;
    case GS_SOLO: return G.solo_seen == 7;
    case GS_PARTS: return G.parts_seen == 7;
    case GS_STYLES: return (G.styles_seen & 1) + ((G.styles_seen >> 1) & 1) + ((G.styles_seen >> 2) & 1) + ((G.styles_seen >> 3) & 1) + ((G.styles_seen >> 4) & 1) >= 3;
    case GS_TEMPO: return G.bpm_moved;
    case GS_KEY: return G.home_hit && G.dim_hits >= 3;
    case GS_OCT: return G.key_moved && G.key == 0;
    case GS_MELODY: return G.lit_notes >= 12;
    default: return 1;
    }
}
static int g_bits(uint8_t b) { int n = 0; while (b) { n += b & 1; b >>= 1; } return n; }
static void g_cat(char *d, const char *s) { while (*d) d++; while (*s) *d++ = *s++; *d = 0; }
static void g_cat_int(char *d, int v) { char b[8]; int i = 0, n = 0; if (v < 0) { g_cat(d, "-"); v = -v; } do { b[i++] = (char)('0' + v % 10); v /= 10; } while (v); while (d[n]) n++; while (i) d[n++] = b[--i]; d[n] = 0; }
/* what to do (big) and a detail (small) */
static void g_todo(char *big, char *detail)
{
    big[0] = detail[0] = 0;
    switch (G.step) {
    case GS_PLAY: g_cat(big, "Press PLAY"); break;
    case GS_K1: case GS_K2: case GS_K4:
        { int k = G.step == GS_K1 ? 0 : G.step == GS_K2 ? 1 : 3; g_cat(big, "Turn K"); g_cat_int(big, k + 1); g_cat(big, " up"); g_cat(detail, "To 6 or more. Now "); g_cat_int(detail, G.vol[k]); g_cat(detail, "."); } break;
    case GS_K3OFF: g_cat(big, "Turn K3 down to 0"); g_cat(detail, "Now "); g_cat_int(detail, G.vol[2]); g_cat(detail, "."); break;
    case GS_K3ON: g_cat(big, "Turn K3 up again"); g_cat(detail, "Now "); g_cat_int(detail, G.vol[2]); g_cat(detail, "."); break;
    case GS_SOLO: g_cat(big, G.playing ? "Press KICK, SNARE, HAT" : "Press PLAY"); g_cat(detail, "The three lit keys, side by side. "); g_cat_int(detail, g_bits(G.solo_seen)); g_cat(detail, "/3."); break;
    case GS_PARTS: g_cat(big, "Turn PRESETS"); g_cat(detail, "Try the 3 drum parts: "); g_cat_int(detail, g_bits(G.parts_seen)); g_cat(detail, "/3. Now "); g_cat(detail, G_PART[G.part].name); g_cat(detail, "."); break;
    case GS_STYLES: g_cat(big, "Turn PRESETS"); g_cat(detail, "Try 3 styles: "); g_cat_int(detail, g_bits(G.styles_seen)); g_cat(detail, "/3. Now "); g_cat(detail, G_STYLE[G.style].name); g_cat(detail, "."); break;
    case GS_TEMPO: g_cat(big, "Turn ALGORITHM"); g_cat(detail, "Slower, faster. Now "); g_cat_int(detail, G.bpm); g_cat(detail, " BPM."); break;
    case GS_KEY: g_cat(big, !G.playing ? "Press PLAY" : !G.home_hit ? "Press a bright key" : "Now the dim keys");
        g_cat(detail, !G.home_hit ? "HOME, the A: it sounds settled." : "The family: "); if (G.home_hit) { g_cat_int(detail, G.dim_hits > 3 ? 3 : G.dim_hits); g_cat(detail, "/3."); } break;
    case GS_OCT: g_cat(big, !G.key_moved ? "Press OCT+ or OCT-" : G.key ? "Back to A" : "Back in A"); if (G.key_moved && G.key) g_cat(detail, "Press OCT until the screen says A MIN."); break;
    case GS_MELODY: g_cat(big, G.playing ? "Play the lit keys" : "Press PLAY"); g_cat(detail, "Short notes, some long, some silence. "); g_cat_int(detail, G.lit_notes > 12 ? 12 : G.lit_notes); g_cat(detail, "/12."); break;
    default: g_cat(big, "Lesson done"); g_cat(detail, "Play on as you like. Lesson 2 comes next."); break;
    }
}

/* ---- the keys ---- */
static int g_drum_key(int n) { return n == 60 ? 'k' : n == 62 ? 's' : n == 64 ? 'h' : 0; }
static void g_key_down(int k, int vel)
{
    int n = KIT_DRUM_LO + k, d = g_drum_key(n);
    if (G.step == GS_SOLO && d) {               /* one drum on its own */
        int pad = d == 'k' ? G_KICK : d == 's' ? g_drums()->snare_pad : G_HAT;
        kit_track_note_on(KIT_TRACK_DRUMS, KIT_DRUM_LO + pad, 110); kit_track_note_off(KIT_TRACK_DRUMS, KIT_DRUM_LO + pad);
        G.solo = d; G.solo_seen |= (uint8_t)(d == 'k' ? 1 : d == 's' ? 2 : 4); g_build(); return;
    }
    if (G.step < GS_KEY) return;
    kit_track_note_on(KIT_TRACK_MELODY, n, vel < 60 ? 90 : vel);
    G.held[k] = 1;
    if (!g_scale_has(n)) { G.out_note = n; G.out_at = kit_ms(); return; }   /* outside the key: say why it rubs */
    if (G.step == GS_KEY) { if (g_is_home(n)) G.home_hit = 1; else if (G.home_hit) G.dim_hits++; }
    if (G.step == GS_MELODY) G.lit_notes++;
}
static void g_key_up(int k) { if (G.held[k]) { kit_track_note_off(KIT_TRACK_MELODY, KIT_DRUM_LO + k); G.held[k] = 0; } }

/* ---- enter, leave ---- */
static void g_enter(void)
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++) { G.saved_e[t] = kit_track_engine(t); G.saved_p[t] = kit_track_preset(t); G.saved_lv[t] = kit_track_level_of(t); }
    G.saved_rv = kit_reverb_of();                 /* the song goes on until PLAY here plays the lesson's (R-102) */
    kit_track_sound(KIT_TRACK_CHORDS, KIT_ENGINE_BASIC, KIT_INST_PIANO);   /* the FM-1's own sounds for now (#98, question 2) */
    kit_track_sound(KIT_TRACK_BASS, 1, 2);      /* Analog: SQR BASS */
    kit_track_sound(KIT_TRACK_MELODY, 2, 4);    /* Phase: BELL */
    kit_reverb(25);
    G.on = 1; G.playing = 0; G.list = 0; G.key = 0; G.bpm = 88; G.part = 0; G.style = 0; G.styles_on = 0;
    G.step = set.g_step < G_STEPS ? set.g_step : 0;   /* where it was, over power-off (#98, question 1) */
    for (t = 0; t < 27; t++) G.held[t] = 0;
    g_mix(0, 0, 8, 0);
    g_enter_step();
}
static void g_leave(void)
{
    int t;
    if (G.playing) kit_seq_stop();
    for (t = 0; t < KIT_NTRACKS; t++) { kit_track_notes_off(t); kit_track_level(t, G.saved_lv[t]); kit_track_sound(t, G.saved_e[t], G.saved_p[t]); }
    kit_reverb(G.saved_rv);
    kit_lights_off();
    G.on = 0;
}
static void g_go(int d)                         /* SELECT: the next or the previous step */
{
    int s = G.step + d;
    if (s < 0 || s >= G_STEPS) return;
    G.step = s; g_enter_step();
}
static void g_note(const char *s) { G.note = s; G.note_at = kit_ms(); }

static int guided_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_KEY_DOWN) { if (!G.list) g_key_down(e->id, e->vel); return 1; }
    if (e->type == KIT_EV_KEY_UP) { g_key_up(e->id); return 1; }
    if (e->type == KIT_EV_ENC) {
        int d = e->delta > 0 ? 1 : -1, i;
        if (G.list) { if (e->id == KIT_ENC_SELECT) { G.list_sel += d; G.list_sel = G.list_sel < 0 ? 0 : G.list_sel >= G_LESSONS ? G_LESSONS - 1 : G.list_sel; } return 1; }
        if (e->id == KIT_ENC_SELECT) { g_go(d); return 1; }
        if (e->id == KIT_ENC_ALGORITHM) { G.bpm += d * 2; G.bpm = G.bpm < 60 ? 60 : G.bpm > 180 ? 180 : G.bpm; kit_song_set_tempo(&g_song, G.bpm); G.bpm_moved = 1; return 1; }
        if (e->id == KIT_ENC_PRESETS) {
            if (G.step == GS_PARTS) { G.part = (G.part + d + G_PARTS) % G_PARTS; G.parts_seen |= (uint8_t)(1 << G.part); }
            else if (G.step >= GS_STYLES) { G.styles_on = 1; G.style = (G.style + d + G_NSTYLES) % G_NSTYLES; G.styles_seen |= (uint8_t)(1 << G.style); G.bpm = G_STYLE[G.style].bpm; }
            else return 1;
            g_build(); return 1;
        }
        for (i = 0; i < 4; i++)
            if (e->id == KIT_ENC_KNOB1 + i) { G.vol[i] += d; G.vol[i] = G.vol[i] < 0 ? 0 : G.vol[i] > 10 ? 10 : G.vol[i]; g_levels(); }
        return 1;
    }
    if (e->type == KIT_EV_BTN_DOWN) {
        if (e->id == KIT_BTN_PLAY) { G.playing = !G.playing; if (G.playing) kit_seq_play(&g_song); else kit_seq_stop(); return 1; }   /* the lesson's song */
        if (e->id == KIT_BTN_REC) { g_note("REC comes in lesson 2"); return 1; }
        if ((e->id == KIT_BTN_OCT_DOWN || e->id == KIT_BTN_OCT_UP) && G.step >= GS_OCT) {
            G.key += e->id == KIT_BTN_OCT_UP ? 1 : -1; G.key = G.key < -6 ? -6 : G.key > 6 ? 6 : G.key; G.key_moved = 1; g_build(); return 1; }
    }
    return 1;                                   /* the rest does nothing here */
}
static void guided_home_tap(void)               /* HOME tapped: the lessons; on the list, opens the one chosen */
{
    if (!G.list) { G.list = 1; G.list_sel = 0; return; }
    if (G.list_sel == 0) { G.list = 0; G.step = 0; g_enter_step(); }
    else { G.list = 0; g_note("That lesson comes soon"); }
}

/* ---- the screen: layout E ---- */
static int g_wrap(int x, int y, int w, const char *s, int font, kit_color_t c, int lh, int max_lines)   /* returns the y below */
{
    char line[96], word[48];
    int n = 0, lines = 0;
    line[0] = 0;
    while (*s && lines < max_lines) {
        int k = 0;
        while (*s == ' ') s++;
        while (*s && *s != ' ' && k < 47) word[k++] = *s++;
        word[k] = 0;
        if (!k) break;
        {
            char t[96]; int i = 0, j = 0;
            while (line[i] && i < 90) { t[i] = line[i]; i++; }
            if (i) t[i++] = ' ';
            while (word[j] && i < 95) t[i++] = word[j++];
            t[i] = 0;
            if (kit_text_width(t, font) > w && line[0]) { kit_text(x, y + lines * lh, line, font, KIT_ALIGN_LEFT, c); lines++; i = 0; while (word[i]) { line[i] = word[i]; i++; } line[i] = 0; }
            else { i = 0; while (t[i]) { line[i] = t[i]; i++; } line[i] = 0; }
        }
        n++;
    }
    if (line[0] && lines < max_lines) { kit_text(x, y + lines * lh, line, font, KIT_ALIGN_LEFT, c); lines++; }
    return y + lines * lh;
}
static void g_tick_mark(int x, int y, kit_color_t c)   /* a tick, drawn (the fonts are ASCII) */
{
    int i;
    for (i = 0; i < 3; i++) { kit_line(x, y + 9 + i, x + 6, y + 15 + i, c); kit_line(x + 6, y + 15 + i, x + 17, y + 2 + i, c); }
}
static void g_draw_seq(int y0)
{
    static const char *const ROWS[6] = { "KICK", "SNARE", "HATS", "BASS", "CHORD", "MELODY" };
    int tick = G.playing ? kit_seq_tick() : -1, bar = tick >= 0 ? (tick / KIT_TICKS_PER_BAR) % 4 : 0, cur = tick >= 0 ? (tick % KIT_TICKS_PER_BAR) / G_SIXT : -1;
    int r, s, i, gx = 44, cw = 12, rh = 12;
    int focus = G.step == GS_K1 || G.step == GS_SOLO || G.step == GS_PARTS || G.step == GS_STYLES ? 1 : G.step == GS_K2 ? 2 : G.step == GS_K3OFF || G.step == GS_K3ON || G.step == GS_KEY ? 4
              : G.step == GS_K4 || G.step == GS_MELODY ? 8 : 15;
    for (r = 0; r < 6; r++) {
        int y = y0 + r * rh, trk = r < 3 ? KIT_TRACK_DRUMS : r == 3 ? KIT_TRACK_BASS : r == 4 ? KIT_TRACK_CHORDS : KIT_TRACK_MELODY;
        int lit = (focus >> (r < 3 ? 0 : r - 2)) & 1;
        kit_color_t col = lit ? G_TRK_COL[r < 3 ? 0 : r - 2] : G_OFF;
        kit_text(4, y + 1, ROWS[r], KIT_FONT_TINY, KIT_ALIGN_LEFT, col);
        for (s = 0; s < 16; s++) kit_rect(gx + s * cw, y, cw - 1, rh - 2, s == cur ? KIT_RGB(45, 53, 80) : s % 4 == 0 ? KIT_RGB(27, 34, 54) : KIT_RGB(20, 26, 42));
        for (i = 0; i < g_song.count[trk]; i++) {
            const kit_song_note_t *nt = &g_song.notes[trk][i];
            int b = nt->start / KIT_TICKS_PER_BAR, st = (nt->start % KIT_TICKS_PER_BAR) / G_SIXT, len = (nt->len + G_SIXT - 1) / G_SIXT, w;
            if (b != bar) continue;
            if (r < 3) {
                int pad = nt->note - KIT_DRUM_LO, row = pad <= 3 ? 0 : pad <= 11 ? 1 : 2, h = 2 + (rh - 5) * nt->vel / 127;
                if (row != r) continue;
                kit_rect(gx + st * cw + 1, y + rh - 3 - h, cw - 3, h, col);
                continue;
            }
            w = len * cw - 3; if (st * cw + w > 16 * cw) w = 16 * cw - st * cw - 2;
            kit_rect(gx + st * cw + 1, y + (r == 4 ? 1 : 3), w, r == 4 ? rh - 4 : 4, col);
        }
        if (r == 4 && lit) kit_text(gx + 4, y, bar % 2 ? "F" : "Am", KIT_FONT_TINY, KIT_ALIGN_LEFT, KIT_RGB(11, 14, 24));
        if (r == 5 && g_melody_yours()) kit_text(236, y + 1, "YOU", KIT_FONT_TINY, KIT_ALIGN_RIGHT, G_TRK_COL[3]);
    }
    if (cur >= 0) kit_rect(gx + cur * cw + cw / 2, y0 - 2, 1, 6 * rh, G_INK);
    for (i = 0; i < 4; i++) kit_rect(gx + i * 48, y0 + 6 * rh + 1, 46, 2, i == bar ? G_TA : KIT_RGB(42, 49, 80));
}
static void g_draw_strip(int y)
{
    static const char *const NAMES[4] = { "DRUMS", "BASS", "CHORDS", "MELODY" };
    int i, want = G.step == GS_K1 ? 0 : G.step == GS_K2 ? 1 : G.step == GS_K3OFF || G.step == GS_K3ON ? 2 : G.step == GS_K4 ? 3 : -1;
    kit_rect(0, y, KIT_W, KIT_H - y, KIT_RGB(21, 26, 43));
    for (i = 0; i < 4; i++) {
        int x = 4 + i * 59, v = G.vol[i], on = i == want;
        char k[4] = { 'K', (char)('1' + i), 0, 0 };
        if (i == 3 && g_melody_yours()) v = 10;
        if (on) kit_rect(x, y + 29, 55, 2, g_done() ? G_TG : G_TA);
        kit_text(x + 2, y + 2, k, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, on ? G_INK : G_DIMT);   /* K1 and its level; the track under */
        kit_rect(x + 22, y + 6, 30, 5, KIT_RGB(10, 13, 22)); kit_rect(x + 22, y + 6, 3 * v, 5, v ? G_TRK_COL[i] : G_OFF);
        kit_text(x + 2, y + 16, i == 3 && g_melody_yours() ? "YOU" : NAMES[i], KIT_FONT_TINY, KIT_ALIGN_LEFT, v ? G_TRK_COL[i] : G_OFF);
    }
}
static void g_draw_list(void)
{
    int i;
    kit_clear(KIT_RGB(11, 14, 24));
    kit_rect(0, 0, KIT_W, 2, MODE_COL);
    kit_text(120, 8, "LESSONS", KIT_FONT_SMALL, KIT_ALIGN_CENTER, G_INK);
    for (i = 0; i < 6; i++) {
        int li = G.list_sel < 6 ? i : G.list_sel - 5 + i, y = 32 + i * 32, sel = li == G.list_sel;
        char num[3] = { (char)('1' + li), 0, 0 };
        kit_rect(8, y, 224, 28, sel ? KIT_RGB(29, 35, 53) : KIT_RGB(18, 23, 37));
        if (sel) kit_rect(8, y, 3, 28, G_TA);
        kit_text(24, y + 6, num, KIT_FONT_SMALL, KIT_ALIGN_CENTER, sel ? G_TA : G_DIMT);
        kit_text(40, y + 2, G_TITLES[li], KIT_FONT_XSMALL, KIT_ALIGN_LEFT, li == 0 ? G_INK : G_DIMT);
        kit_text(40, y + 16, li == 0 ? (set.g_stars & 1 ? "14 steps   DONE" : "14 steps") : "coming soon", KIT_FONT_TINY, KIT_ALIGN_LEFT, G_DIMT);
    }
    kit_rect(0, 228, KIT_W, 12, KIT_RGB(21, 26, 43));
    kit_text(120, 229, "SELECT choose   HOME open", KIT_FONT_TINY, KIT_ALIGN_CENTER, G_DIMT);
}
static void g_draw(uint32_t now)
{
    char big[48], detail[96], top[40], st[8];
    int done = g_done(), y, warn = G.out_at && now - G.out_at < 2000, note = G.note && now - G.note_at < 2500;
    kit_color_t col;
    if (G.list) { g_draw_list(); return; }
    kit_clear(KIT_RGB(11, 14, 24));
    kit_rect(0, 0, KIT_W, 2, MODE_COL);
    top[0] = 0; g_cat(top, "1  "); g_cat(top, G_TITLES[0]);
    st[0] = 0; g_cat_int(st, G.step + 1); g_cat(st, " / "); g_cat_int(st, G_STEPS);
    kit_text(8, 5, top, KIT_FONT_TINY, KIT_ALIGN_LEFT, G_DIMT);
    kit_text(232, 5, st, KIT_FONT_TINY, KIT_ALIGN_RIGHT, G_DIMT);
    g_todo(big, detail);
    if (warn) { big[0] = detail[0] = 0; g_cat(big, kit_note_name(G.out_note, 0)); g_cat(big, ": not in the key"); g_cat(detail, "It rubs: stay on the lit keys."); col = G_TR; }
    else if (note) { big[0] = detail[0] = 0; g_cat(big, G.note); col = G_TR; }
    else if (done && G.step != GS_FREE) { big[0] = 0; g_cat(big, "Done!"); detail[0] = 0; g_cat(detail, "SELECT > the next step."); col = G_TG; }
    else col = G.step == GS_FREE ? G_TG : G_TA;
    /* what to do: 24 px when it fits, else 16 px on two lines (R-233) */
    {
        int bx = done && !warn && !note && G.step != GS_FREE ? 36 : 12;   /* room for the tick */
        if (kit_text_width(big, KIT_FONT_MEDIUM) <= 234 - bx) { kit_text(bx, 20, big, KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, col); y = 50; }
        else y = g_wrap(bx, 22, 234 - bx, big, KIT_FONT_SMALL, col, 18, 2) + 4;
    }
    kit_rect(0, 20, 4, y - 24, col);
    if (done && !warn && !note && G.step != GS_FREE) g_tick_mark(12, 25, G_TG);
    if (detail[0]) y = g_wrap(12, y, 220, detail, KIT_FONT_XSMALL, G_INK, 15, 2);
    g_wrap(12, y, 220, G_WORDS[G.step], KIT_FONT_XSMALL, G_DIMT, 15, (112 - y) / 15 > 0 ? (112 - y) / 15 : 1);
    g_draw_seq(120);
    {
        char info[40]; info[0] = 0;
        g_cat_int(info, G.bpm); g_cat(info, " BPM   "); g_cat(info, kit_note_name(57 + G.key, 0)); g_cat(info, " MIN   "); g_cat(info, g_drums()->name);
        kit_text(8, 196, info, KIT_FONT_TINY, KIT_ALIGN_LEFT, G_DIMT);
    }
    g_draw_strip(210);
}
static void g_lights(void)
{
    int k;
    for (k = 0; k < 27; k++) {
        int n = KIT_DRUM_LO + k, lv = KIT_LIGHT_OFF;
        if (G.list) lv = KIT_LIGHT_OFF;
        else if (G.step == GS_SOLO) lv = g_drum_key(n) ? KIT_LIGHT_ON : KIT_LIGHT_OFF;
        else if (G.step == GS_KEY || G.step == GS_OCT) lv = g_is_home(n) ? KIT_LIGHT_ON : g_scale_has(n) ? KIT_LIGHT_DIM : KIT_LIGHT_OFF;
        else if (G.step >= GS_MELODY) lv = g_scale_has(n) ? KIT_LIGHT_ON : KIT_LIGHT_OFF;
        if (G.held[k]) lv = KIT_LIGHT_ON;
        kit_key_light(k, lv);
    }
    kit_btn_light(KIT_BTN_PLAY, G.playing ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
}
static void guided_frame(uint32_t now)
{
    int done;
    if (!G.on) g_enter();
    if (G.playing && !kit_seq_playing()) G.playing = 0;
    done = g_done();
    if (done && !G.was_done && G.step != GS_FREE) { kit_click(1, 10); G.chime_at = now + 110; }   /* done: a chime, two notes (R-220) */
    if (G.chime_at && (int32_t)(now - G.chime_at) >= 0) { kit_click(1, 10); G.chime_at = 0; }
    G.was_done = done;
    if (G.step == GS_FREE && !(set.g_stars & 1)) { set.g_stars |= 1; settings_save(); }
    g_draw(now);
    g_lights();
}
