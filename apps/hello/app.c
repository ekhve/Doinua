/* SPDX-License-Identifier: GPL-3.0-only */
/* hello: the platform's smoke test and first scale toy.
 *   - the keys of the chosen scale glow dim; the tonic breathes; a held key lights fully
 *   - pressing a key plays it and shows its name huge, with the octave and scale degree
 *   - SELECT turns the tonic, ALGORITHM the scale, PRESETS the sound (every engine's presets in turn)
 *   - SCL switches letters / Do-Re-Mi; OCT-/OCT+ shift an octave, up to two each way
 *   - MIDI in plays the four tracks (D-029): channel 1 the chosen sound, 2 the bass (Analog SQR BASS), 3 the melody
 *     (Phase BELL), 4 and 10 the drums
 *   - SEQ plays a two-bar demo song on the four tracks, looping, and stops it (D-031): in bar 1 each track alone
 *     (the bass, the melody, a chord, the drums), in bar 2 all together
 *   - HOME shows two static concept screens (D-027): a kids' game screen and a study screen, then back
 *   - PLAY plays the scale by itself over an octave, looping, and stops it; KNOB1 picks the pattern (up, down, up and
 *     down, random), KNOB2 the tempo (40..240 BPM); the playing note lights and is named like a pressed key
 *   - all of that is remembered across power-off (saved on every change; the kit paces the flash writes)
 *   - a MIDI keyboard (USB or the TRS jack) plays and names notes the same way; the keys also go out over USB-MIDI */
#include "fm1kit.h"
#include "app_assets.h"   /* generated from apps/hello/assets/*.png (D-028) */

static int tonic = 0;           /* pitch class: C */
static int scale = 0;           /* KIT_SCALES[0]: Major */
static int names = KIT_NAMES_LETTERS;
static int sound = KIT_INST_PIANO; /* an index over every engine's presets, Basic's first */
static int octave = 0;          /* shift, in octaves */
static int last_note = -1;
static int dirty = 1;
static int pattern = KIT_RUN_UP;
static int tempo = KIT_TEMPO_START;
static kit_song_t demo;          /* the demo song (SEQ) */
static int page;                /* 0: hello, 1: the game screen, 2: the study screen (HOME) */
static int auto_note = -1;      /* the note the player is sounding, or -1 */
static uint32_t auto_seed = 1;
#define OCT_RANGE 2

static const char *const PATTERN_NAMES[KIT_NRUNS] = { "Up", "Down", "Up & down", "Random" };

typedef struct { uint8_t version, tonic, scale, names, sound; int8_t octave; uint8_t pattern, tempo; } settings_t;
#define SETTINGS_VERSION 2

static void load_settings(void)
{
    settings_t st;
    if (kit_store_load(&st, sizeof st) != (int)sizeof st || st.version != SETTINGS_VERSION)
        return;                 /* nothing saved, or another layout: the defaults */
    tonic = st.tonic % 12;
    scale = st.scale % KIT_NSCALES;
    names = st.names ? KIT_NAMES_SOLFEGE : KIT_NAMES_LETTERS;
    sound = st.sound;
    octave = st.octave < -OCT_RANGE ? -OCT_RANGE : st.octave > OCT_RANGE ? OCT_RANGE : st.octave;
    pattern = st.pattern % KIT_NRUNS;
    tempo = kit_tempo_clamp(st.tempo);
}

static void save_settings(void)
{
    settings_t st = { SETTINGS_VERSION, (uint8_t)tonic, (uint8_t)scale, (uint8_t)names, (uint8_t)sound, (int8_t)octave,
                      (uint8_t)pattern, (uint8_t)tempo };
    kit_store_save(&st, sizeof st);
}

static void changed(void)
{
    dirty = 1;
    save_settings();            /* queued; the kit writes it when it is due (fm1kit.h, store) */
}

#define BG KIT_RGB(16, 18, 28)
#define FG KIT_WHITE
#define MUTED KIT_RGB(140, 150, 175)
#define ACCENT KIT_RGB(255, 196, 64)

static int sound_count(void)
{
    int e, n = 0;
    for (e = 0; e < kit_engine_count(); e++)
        n += kit_preset_count(e);
    return n;
}

static void sound_of(int i, int *engine, int *preset)
{
    int e;
    for (e = 0; e < kit_engine_count(); e++) {
        if (i < kit_preset_count(e)) {
            *engine = e;
            *preset = i;
            return;
        }
        i -= kit_preset_count(e);
    }
    *engine = 0;
    *preset = 0;
}

static void pick_sound(void)
{
    int e, p;
    sound_of(sound, &e, &p);
    kit_sound(e, p);
}

static int key_note(int key) { return KIT_KEY0_NOTE + key + 12 * octave; }

static int midi_track(int channel)      /* 1: the chosen sound (track 0), 2: bass, 3: melody, 4 and 10: drums */
{
    return channel == 2 ? KIT_TRACK_BASS : channel == 3 ? KIT_TRACK_MELODY
         : channel == 4 || channel == 10 ? KIT_TRACK_DRUMS : KIT_TRACK_CHORDS;
}

/* the scale over one octave from its tonic, placed so it fits the 27 keys (F3..G5, shifted by the octave) */
static void play_scale(void)
{
    const kit_scale_t *s = &KIT_SCALES[scale];
    uint8_t notes[16], order[32];
    int base = (tonic <= 7 ? 60 : 48) + tonic + 12 * octave, i, n;
    for (i = 0; i < s->n; i++)
        notes[i] = (uint8_t)(base + s->iv[i]);
    notes[s->n] = (uint8_t)(base + 12);
    n = kit_run_order(notes, s->n + 1, pattern, auto_seed++, 1, order);
    kit_play_start(order, n, kit_beat_ms(tempo), 100, 1);
}

static void show_lights(void)
{
    const kit_scale_t *s = &KIT_SCALES[scale];
    int k;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = key_note(k), level = KIT_LIGHT_OFF;
        if (kit_scale_has(s, tonic, n))
            level = kit_pc(n) == tonic ? KIT_LIGHT_BREATH : KIT_LIGHT_DIM;
        if (kit_key_held(k) || n == auto_note)
            level = KIT_LIGHT_ON;
        kit_key_light(k, level);
    }
    kit_btn_light(KIT_BTN_SCL, names == KIT_NAMES_SOLFEGE ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
}

static void draw(void)
{
    const kit_scale_t *s = &KIT_SCALES[scale];
    char line[40];
    const char *t = kit_note_name_in_key(tonic, tonic, s->minor, names);
    int i = 0, j;
    kit_clear(BG);
    /* top: the key */
    for (j = 0; t[j]; j++) line[i++] = t[j];
    line[i++] = ' ';
    for (j = 0; s->name[j] && i < 38; j++) line[i++] = s->name[j];
    line[i] = 0;
    kit_text(120, 8, line, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, ACCENT);
    {
        const char *en = kit_engine_name(kit_sound_engine()), *pn = kit_preset_name(kit_sound_engine(), kit_sound_preset());
        char snd[48];
        int k = 0;
        for (j = 0; en[j] && k < 20; j++) snd[k++] = en[j];
        snd[k++] = ':'; snd[k++] = ' ';
        for (j = 0; pn[j] && k < 46; j++) snd[k++] = pn[j];
        snd[k] = 0;
        kit_text(120, 40, snd, KIT_FONT_SMALL, KIT_ALIGN_CENTER, MUTED);
    }
    /* middle: the last note, huge */
    if (last_note >= 0) {
        const char *n = kit_note_name_in_key(last_note, tonic, s->minor, names);
        int deg = kit_scale_degree(s, tonic, last_note);
        kit_round_rect(30, 66, 180, 120, 18, deg ? KIT_RGB(40, 60, 110) : KIT_RGB(70, 40, 50));
        kit_text(120, 64, n, KIT_FONT_HUGE, KIT_ALIGN_CENTER, FG);
        line[0] = 'O'; line[1] = 'c'; line[2] = 't'; line[3] = ' ';
        line[4] = (char)('0' + kit_octave(last_note));
        line[5] = 0;
        if (deg) {
            line[5] = ' '; line[6] = ' '; line[7] = 'D'; line[8] = 'e'; line[9] = 'g'; line[10] = ' ';
            line[11] = (char)('0' + deg); line[12] = 0;
        }
        kit_text(120, 192, line, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, deg ? FG : MUTED);
    } else {
        kit_text(120, 100, "Play a key", KIT_FONT_LARGE, KIT_ALIGN_CENTER, FG);
    }
    {   /* the player: "PLAY: Up, 90 BPM" or "Playing: Up, 90 BPM" */
        char f[40];
        const char *a = kit_playing() ? "Playing: " : "PLAY: ", *pn = PATTERN_NAMES[pattern];
        int k = 0;
        for (j = 0; a[j]; j++) f[k++] = a[j];
        for (j = 0; pn[j]; j++) f[k++] = pn[j];
        f[k++] = ','; f[k++] = ' ';
        if (tempo >= 100) f[k++] = (char)('0' + tempo / 100);
        f[k++] = (char)('0' + tempo / 10 % 10);
        f[k++] = (char)('0' + tempo % 10);
        f[k++] = ' '; f[k++] = 'B'; f[k++] = 'P'; f[k++] = 'M';
        f[k] = 0;
        kit_text(120, 222, f, KIT_FONT_SMALL, KIT_ALIGN_CENTER, kit_playing() ? ACCENT : MUTED);
    }
}

static void make_demo(void)
{
    static const uint8_t ceg[3] = { 60, 64, 67 }, fac[3] = { 53, 57, 60 };
    int i;
    kit_song_init(&demo, 120, 2);
    kit_song_add(&demo, KIT_TRACK_BASS, 0, 24, 45, 100);            /* bar 1: each alone, a beat each */
    kit_song_add(&demo, KIT_TRACK_MELODY, 24, 24, 76, 100);
    for (i = 0; i < 3; i++)
        kit_song_add(&demo, KIT_TRACK_CHORDS, 48, 24, ceg[i], 90);
    kit_song_add(&demo, KIT_TRACK_DRUMS, 72, 6, 36, 110);
    kit_song_add(&demo, KIT_TRACK_DRUMS, 84, 6, 42, 90);
    kit_song_add_chord(&demo, KIT_TRACK_CHORDS, 1, fac, 3, 80);     /* bar 2: all together */
    kit_song_add(&demo, KIT_TRACK_BASS, 96, 44, 41, 100);
    kit_song_add(&demo, KIT_TRACK_BASS, 144, 44, 45, 100);
    kit_song_add(&demo, KIT_TRACK_MELODY, 120, 48, 72, 100);
    for (i = 0; i < 8; i++)
        kit_song_add(&demo, KIT_TRACK_DRUMS, 96 + i * 12, 6, i % 4 == 0 ? 36 : i % 4 == 2 ? 38 : 42, 100);
}

void app_init(void)
{
    load_settings();
    if (sound < 0 || sound >= sound_count())
        sound = KIT_INST_PIANO;
    kit_volume_limit(85);
    kit_volume(80);
    pick_sound();
    kit_track_sound(KIT_TRACK_BASS, 1, 2);          /* Analog: SQR BASS */
    kit_track_sound(KIT_TRACK_MELODY, 2, 4);        /* Phase: BELL (the drums track starts on the kit) */
    make_demo();
    dirty = 1;
    kit_log("hello: up");
}

void app_event(const kit_event_t *e)
{
    switch (e->type) {
    case KIT_EV_KEY_DOWN:
        last_note = key_note(e->id);
        kit_note_on(last_note, e->vel);
        kit_midi_note_on(1, last_note, e->vel);
        dirty = 1;
        break;
    case KIT_EV_KEY_UP:
        kit_note_off(key_note(e->id));
        kit_midi_note_off(1, key_note(e->id));
        dirty = 1;
        break;
    case KIT_EV_MIDI_NOTE_ON:
        last_note = e->id;
        kit_track_note_on(midi_track(e->delta), e->id, e->vel);
        dirty = 1;
        break;
    case KIT_EV_MIDI_NOTE_OFF:
        kit_track_note_off(midi_track(e->delta), e->id);
        dirty = 1;
        break;
    case KIT_EV_PLAY_NOTE:
        auto_note = e->id;
        last_note = e->id;
        dirty = 1;
        break;
    case KIT_EV_PLAY_END:
        auto_note = -1;
        dirty = 1;
        break;
    case KIT_EV_BTN_DOWN:
        if (e->id == KIT_BTN_SEQ) {
            if (kit_seq_playing())
                kit_seq_stop();
            else
                kit_seq_play(&demo);
            break;
        }
        if (e->id == KIT_BTN_HOME) {
            page = (page + 1) % 3;
            dirty = 1;
            break;
        }
        if (e->id == KIT_BTN_PLAY) {
            if (kit_playing()) {
                kit_play_stop();
                auto_note = -1;
            } else {
                play_scale();
            }
            dirty = 1;
            break;
        }
        if (e->id == KIT_BTN_SCL) names = !names;
        if (e->id == KIT_BTN_OCT_DOWN && octave > -OCT_RANGE) { kit_all_notes_off(); octave--; }
        if (e->id == KIT_BTN_OCT_UP && octave < OCT_RANGE) { kit_all_notes_off(); octave++; }
        if ((e->id == KIT_BTN_OCT_DOWN || e->id == KIT_BTN_OCT_UP) && kit_playing())
            play_scale();       /* the playback follows the octave */
        changed();
        break;
    case KIT_EV_ENC:
        if (e->id == KIT_ENC_SELECT) tonic = kit_pc(tonic + e->delta);
        if (e->id == KIT_ENC_ALGORITHM) scale = ((scale + e->delta) % KIT_NSCALES + KIT_NSCALES) % KIT_NSCALES;
        if (e->id == KIT_ENC_PRESETS) {
            int n = sound_count();
            sound = ((sound + e->delta) % n + n) % n;
            pick_sound();
        }
        if (e->id == KIT_ENC_KNOB1) pattern = ((pattern + e->delta) % KIT_NRUNS + KIT_NRUNS) % KIT_NRUNS;
        if (e->id == KIT_ENC_KNOB2) tempo = kit_tempo_clamp(tempo + e->delta * KIT_TEMPO_STEP);
        if (kit_playing() && e->id != KIT_ENC_PRESETS)
            play_scale();       /* the key, scale, pattern or tempo changed: play the new one */
        changed();
        break;
    }
}

/* ---- two static concept screens (D-027, docs/concepts/v0.2): proof that the firmware can draw them ---- */
static void small_piano(int x, int y, int w, int h, int lit)    /* F3..G5, the FM-1's 16 white keys */
{
    static const int8_t white[16] = { 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79 };
    int i, kw = w / 16;
    for (i = 0; i < 16; i++)
        kit_rect(x + i * kw, y, kw - 1, h, white[i] == lit ? ACCENT : KIT_RGB(220, 224, 232));
    for (i = 0; i < 15; i++) {
        int n = white[i] % 12;
        if (n == 4 || n == 11)
            continue;
        kit_rect(x + i * kw + kw * 2 / 3, y, kw * 2 / 3, h * 3 / 5, white[i] + 1 == lit ? ACCENT : KIT_RGB(40, 44, 58));
    }
}

static void draw_game(void)
{
    static const char *const names[3] = { "E", "D", "C" };
    int i;
    kit_clear(KIT_RGB(120, 190, 240));                              /* sky */
    kit_circle(198, 52, 17, KIT_RGB(255, 225, 110));                /* sun */
    kit_image(20, 40, &IMG_CLOUD, 2);
    kit_image(110, 30, &IMG_CLOUD, 2);
    kit_rect(0, 152, 240, 88, KIT_RGB(90, 170, 80));                /* grass */
    kit_rect(0, 152, 240, 3, KIT_RGB(70, 140, 60));
    kit_image(12, 96, &IMG_HORSE, 3);
    for (i = 0; i < 3; i++) {                                       /* fences: the next notes (the last one clipped) */
        int x = 124 + i * 48;
        kit_image(x, 120, &IMG_FENCE, 3);
        kit_circle(x + 18, 104, 12, i == 0 ? ACCENT : KIT_WHITE);
        kit_text(x + 18, 94, names[i], KIT_FONT_SMALL, KIT_ALIGN_CENTER, BG);
    }
    kit_rect(0, 0, 240, 24, KIT_RGB(60, 120, 170));
    kit_text(6, 3, "Twinkle Twinkle", KIT_FONT_SMALL, KIT_ALIGN_LEFT, KIT_WHITE);
    for (i = 0; i < 3; i++)
        kit_image(176 + i * 20, 3, &IMG_STAR, 2);
    kit_text(120, 160, "Play E to jump!", KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, KIT_WHITE);
    small_piano(8, 196, 224, 36, 64);
}

static void draw_study(void)
{
    static const struct { int8_t note; uint8_t x, w; } bass[] = {
        { 57, 64, 16 }, { 57, 84, 8 }, { 60, 98, 8 }, { 62, 112, 16 }, { 64, 134, 8 },
        { 62, 148, 8 }, { 57, 162, 24 }, { 55, 192, 8 }, { 57, 206, 16 },
    };
    const int lo = 53, hi = 72, top = 30, rh = 8;
    int i, m;
    kit_clear(BG);
    kit_rect(0, 0, 240, 24, KIT_RGB(30, 34, 50));
    kit_text(6, 3, "Funk bass etude", KIT_FONT_SMALL, KIT_ALIGN_LEFT, KIT_WHITE);
    kit_text(234, 3, "75%", KIT_FONT_SMALL, KIT_ALIGN_RIGHT, MUTED);
    for (m = hi - 1; m >= lo; m--) {                                /* the keyboard down the left, one row a note */
        int y = top + (hi - 1 - m) * rh, black = (0x54A >> (m % 12)) & 1;
        kit_rect(4, y, black ? 22 : 32, rh - 1, m == 57 ? ACCENT : black ? KIT_RGB(40, 44, 58) : KIT_RGB(200, 204, 215));
        kit_rect(40, y + rh - 1, 196, 1, KIT_RGB(30, 34, 50));
    }
    for (i = 0; i < 4; i++)                                         /* the beats */
        kit_rect(64 + i * 44, top, 1, (hi - lo) * rh, KIT_RGB(44, 50, 70));
    for (i = 0; i < (int)(sizeof bass / sizeof bass[0]); i++)
        kit_round_rect(bass[i].x, top + (hi - 1 - bass[i].note) * rh, bass[i].w, rh - 1, 2, KIT_RGB(170, 120, 255));
    kit_rect(62, top, 2, (hi - lo) * rh, ACCENT);                   /* now */
    kit_text(120, 186, "next: A  C  D  E", KIT_FONT_SMALL, KIT_ALIGN_CENTER, KIT_WHITE);
    kit_round_rect(60, 208, 120, 8, 3, KIT_RGB(30, 34, 50));
    kit_round_rect(60, 208, 86, 8, 3, KIT_RGB(90, 200, 120));
    kit_text(120, 220, "in time 72%   (concept)", KIT_FONT_SMALL, KIT_ALIGN_CENTER, MUTED);
}

void app_frame(uint32_t now_ms)
{
    if (!dirty)
        return;
    dirty = 0;
    if (page == 1)
        draw_game();
    else if (page == 2)
        draw_study();
    else
        draw();
    show_lights();
}
