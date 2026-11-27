/* SPDX-License-Identifier: GPL-3.0-only */
/* companion: the FM-1 Music Companion (concept v0.4), Producer mode. One song of four tracks; ALGORITHM picks the
 * track, PLAY plays the song, REC records the track as played (D-031). The chords track has the chord pad (D-034) and
 * manual chords (D-036), as docs/MANUAL.md describes; the other tracks are plain keyboards until their stories.
 *
 *   CHORDS track, PLAY       the keys play notes (and MIDI channel 1): the FM-1 names the chord they hold (Am, Am9), its
 *                            numeral in the key, the notes and the closest chord when it has no name (D-042); K1 HELP
 *                            key lights or none; the FM-1 starts here
 *   CHORDS track, AUTO (SCL) white keys: the key's chords I..vii twice; F5 NEXT, G5 AGAIN; black keys held: a colour
 *                            (sus2 sus4 7 maj7 9 11 13 add9 6, D#5 the 3rd in the bass); F#5 AUTO; K1 style,
 *                            K2 progression, K3 voicing, K4 block / strum / arpeggio
 *   CHORDS track, IDEAS      the keys play notes; K1 root, K2 type, K3 colour, K4 voicing build a chord that the keys
 *                            and the screen's piano show; SELECT walks the ideas; EDIT lights off / on; ARP arpeggiates
 *                            the held notes
 *   BASS (SCL: PLAY, IDEAS, AUTO)  PLAY: the bass an octave down with HELP and SCALE; IDEAS: a style's line falls
 *                            onto its keys; AUTO: a degree pad plays a pattern on its root (D-040)
 *   MELODY (SCL: PLAY, IDEAS, AUTO)  PLAY: the keys with HELP (free, key, the chord now, white keys), SCALE (the key's
 *                            pentatonic first), OUTSIDE and PLUS (a 3rd, a 6th, the octave, an echo); IDEAS:
 *                            Sketchpad's figures fall onto the keys, in versions, slower if you like; AUTO: a degree pad
 *                            plays a style's phrase, call and answer (D-050)
 *   DRUMS                    the keys play notes on that track
 *   GLO held                 help for the screen; PRESETS the track's sound; OCT- / OCT+ an octave, two each way
 *   MIDI in                  each track's channel (MIDI page; 1 the chords, 2 the bass, 3 the melody, 10 the drums at
 *                            first); the keys go out on
 *                            the track's channel (the drums on 10); a keyboard's pitch bend, mod wheel (CC1), sustain
 *                            pedal (CC64) and all notes off (CC120, CC123; CC121 resets) play on the track (D-044)
 *   MIDI clock               Start, Continue and Stop in play and stop the song, which then follows the clock in; PLAY
 *                            sends Start, the clock and Stop over USB (D-044)
 *   GLO (tap)                SONG: the song's key, tempo, length; SCL steps the sub-pages CLICK (click, level,
 *                            count-in), MIDI (each track's channel, what comes in, MIDI wakes the screen) and SYSTEM
 *                            (screen off, TUNE), kept with kit_store (D-045) */
#include "fm1kit.h"
#include "app_assets.h"   /* generated: the user kit from kits/user/ (D-056), when there is one */
#include "pad.c"
#include "manual.c"
#include "bass.c"
#include "melody.c"
#include "drumstyle.c"
#include "feel.c"

enum { PAGE_PLAY = 0, PAGE_IDEAS, PAGE_AUTO, NPAGES };   /* the chords track's pages, on SCL */
#define OCT_RANGE 2
#define STRUM_MS 30             /* between the notes of a strum, lowest first */
#define SONG_BPM 90
#define SONG_BARS 4
#define KEY_NEXT 24             /* F5 */
#define KEY_AUTO 25             /* F#5 */
#define KEY_AGAIN 26            /* G5 */
#define KEY_SLASH 22            /* D#5: the 3rd in the bass */

/* the pads: white keys F3..E4 and F4..E5 are degrees 0..6, twice */
static const int8_t PAD_DEG[KIT_NKEYS] = { 0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6, 0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6,
                                           -1, -1, -1 };
/* the black keys' colours, F#3..C#5 */
static const int8_t BLACK_COL[KIT_NKEYS] = { -1, KIT_COL_SUS2, -1, KIT_COL_SUS4, -1, KIT_COL_7, -1, -1, KIT_COL_MAJ7, -1,
                                             KIT_COL_9, -1, -1, KIT_COL_11, -1, KIT_COL_13, -1, KIT_COL_ADD9, -1, -1,
                                             KIT_COL_6, -1, -1, -1, -1, -1, -1 };
static const char *const TRACK_NAMES[KIT_NTRACKS] = { "CHORDS", "BASS", "MELODY", "DRUMS" };   /* the templates' names */
enum { ROLE_CHORDS = 0, ROLE_BASS, ROLE_MELODY, ROLE_DRUMS, NROLES };
static const char *const VOICING_NAMES[KIT_NVOICINGS] = { "Root", "Inv 1", "Inv 2", "Open", "Wide" };
static const char *const FEEL_NAMES[PAD_NFEELS] = { "Block", "Strum", "Arp" };

static pad_t pads[KIT_NTRACKS];           /* each track's chord pad, manual page and bass (D-046) */
#define KIT_NSEQS 16                    /* the sequencer's sequences (ST-85, R-274) */
static kit_song_t seqs[KIT_NSEQS] KIT_BIG;  /* each a section of the four tracks, in the RAM pool (API 29) */
static int cur_seq;                     /* the one the track pages show and REC records into: `song` */
#define song (seqs[cur_seq])
static int track = KIT_TRACK_CHORDS, octave, armed, help, dirty = 1;
static int pages[KIT_NTRACKS];             /* each track's page on SCL (D-046: per track) */
/* the modes (HOME) and the song page (GLO), D-038 */
enum { MODE_PRODUCER = 0, MODE_GUIDED, MODE_GAME, NMODES };
static int mode = MODE_PRODUCER, picker, pick, confirm, song_page;
static int home_long, glo_long;
static uint32_t home_at, glo_at;
#define HOLD_MS 500                             /* a press this long is a hold */
static int snd[KIT_NTRACKS];        /* each track's sound: an index over every engine's presets */
static int last_note = -1, last_channel;   /* the last note played, and its MIDI channel (0: from the keys) */
static uint32_t black_at[KIT_NKEYS], presses;   /* when each black key went down, for "the last one held" */
static int chord_key = -1, chord_row, chord_vel;   /* the key sounding the pad's chord */
static uint8_t on_notes[KIT_CHORD_MAX];         /* the chord's notes that are sounding */
static int n_on;
static int strum_i, arp_i = -1;                 /* the next note to strum; the arpeggio's note (-1: none) */
static uint32_t due_ms;
static int rec_degree = -1, bar = -1;
static man_t mans[KIT_NTRACKS];
static int man_arps[KIT_NTRACKS], lights_offs[KIT_NTRACKS];   /* IDEAS: ARP on; EDIT: the lights off */
static int marp_note = -1, marp_i;              /* the arpeggiator's sounding note and its place among the held */
static uint32_t marp_due;
static uint32_t rec_sig;                        /* which recorded notes sound now (to redraw when they change) */
/* the bass track (D-040) */
static bass_t basses[KIT_NTRACKS];
static int8_t bass_key_note[KIT_NKEYS];         /* the note each held key sounds on the bass (-1: none) */
static int8_t bass_key_plus[KIT_NKEYS];         /* its PLUS note, the octave or the fifth above (-1: none; ST-84) */
static int bass_root_pad = -1, bass_auto_started; /* AUTO: the held pad's root (-1: none); the pad started the song */
static int bass_prev_tick = -1, bass_loops;     /* the song's place last frame, and how many loops it went round */
typedef struct { int16_t off; uint8_t note, rec, trk; } bass_off_t;
static bass_off_t bass_offs[8];                 /* notes to release: their tick, counted from the loop's start */
static int bass_noffs, bass_next_idea = -1, bass_now_idea = -1;
static uint32_t bass_pop_until;                 /* K2 SCALE: the pop-up list shows until then */
/* the melody track (D-050) */
static mel_t mels[KIT_NTRACKS];
static int8_t mel_key_note[KIT_NKEYS], mel_key_plus[KIT_NKEYS];   /* each held key's note and its PLUS voice (-1: none) */
static int mel_root_pad = -1, mel_auto_started, mel_now_idea = -1, mel_next_idea = -1;
static uint32_t mel_pop_until;
static uint32_t free_note_until;                /* SCALE or OUTSIDE turned with HELP Free: say why it does nothing (ST-44) */
typedef struct { uint32_t on, off; uint8_t note, vel, sounding; } mel_echo_t;
static mel_echo_t mel_echo[8];                  /* PLUS echo: the repeats to come */
static int practice, practice_bpm;              /* IDEAS' SPEED below 100 %: the song's own tempo meanwhile */
/* the settings (D-045): the song page's sub-pages, kept over power-off */
#define SETTINGS_VERSION 6                      /* 6: the guided mode's place (ST-57); 5: the train's stars (ST-55); 4: the unicorn's taught songs (#84); 3: explain
                                                 * (ST-43); older records are taken over (explanations on, no songs, no stars) */
#define MY_SONGS 4                              /* the game's MY SONG 1..4, taught (#84, D-061) */
#define MY_SONG_MAX 120                         /* notes each */
typedef struct {
    uint8_t version;
    uint8_t click;          /* 0 off, 1 while recording, 2 always */
    uint8_t click_lvl;      /* 1..10 */
    uint8_t count_in;       /* bars: 0 off, 1, 2 */
    uint8_t ch[KIT_NTRACKS];/* each track's MIDI channel, 1..16 */
    uint8_t midi_in;        /* 0 by channel, 1 every channel to the track on screen, 2 by channel and the screen follows */
    uint8_t wake;           /* 0 the panel wakes the screen, 1 a MIDI note too */
    uint8_t scr_off;        /* 0 never, then SCR_MIN[] minutes */
    int8_t tune;            /* cents, -50..50 */
    uint8_t role[KIT_NTRACKS];  /* each track's template (D-046): ROLE_* */
    uint8_t explain;        /* 1: a knob explains its value (ST-43, D-052); off unless chosen (D-072) */
    uint8_t my_len[MY_SONGS];               /* the taught songs: how many notes (0: not taught) */
    uint8_t my_song[MY_SONGS][MY_SONG_MAX]; /* and the notes, F3..G5 */
    uint8_t train_stars;    /* the stars the train has won at Star Station (ST-55) */
    uint8_t g_lesson, g_step, g_stars;   /* the guided mode: the lesson and step it was on, a bit a lesson done (ST-57) */
} settings_t;
static settings_t set = { SETTINGS_VERSION, 0, 6, 0, { 1, 2, 3, 10 }, 0, 1, 0, 0, { ROLE_CHORDS, ROLE_BASS, ROLE_MELODY, ROLE_DRUMS }, 0, { 0 }, { { 0 } }, 0, 0, 0, 0 };
#define SETTINGS_V5 ((int)sizeof(settings_t) - 3)  /* a version 5 record: no guided place */
#define SETTINGS_V4 (SETTINGS_V5 - 1)              /* a version 4 record: no stars */
#define SETTINGS_V3 (SETTINGS_V4 - MY_SONGS - MY_SONGS * MY_SONG_MAX)   /* a version 3 record: no songs */
static const uint8_t SCR_MIN[6] = { 0, 1, 5, 15, 30, 60 };
enum { SUB_SONG = 0, SUB_TRACKS, SUB_CLICK, SUB_MIDI, SUB_SYSTEM, SUB_USB, NSUBS };
/* GLO USB AUDIO (ST-98, R-400..R-404, D-130): OUT, MONITOR, IN LEVEL; kept with the song (songsave.c) */
static struct { uint8_t mon; int8_t lvl; } usb_set = { KIT_USB_MON_SAMPLING, 0 };
static const char *const USB_MON[3] = { "off", "sampling", "on" };
static int usb_shown = -1;                      /* the connection drawn */
static int pend_track = -1, pend_role;
static uint32_t steal_seen, steal_until;        /* D-047: the last count of voices taken, and the dot's end */
static int voices_shown = -1;                   /* TRACKS: the voices and load drawn */          /* TRACKS: a template change waiting for SAVE (the track has notes) */
static int song_sub, set_track;                 /* the song page's sub-page; MIDI: the track K2 sets */
static int asleep, eat;                         /* the screen dark; the panel's events swallowed until all let go */
static uint32_t last_input;
static int last_mtrack = -1;                    /* the track the last MIDI note went to */

/* CHORDS PLAY (D-042) */
static uint32_t midi_held[4];                   /* the notes MIDI channel 1 holds, a bit each */
enum { PH_FREE = 0, PH_KEY, PH_SHAPES, PH_PADS, PH_N };   /* PLAY's K1 HELP (ST-79, R-246) */
static const char *const PH_NAMES[PH_N] = { "Free", "Key", "Shapes", "Pads" };
static int play_helps[KIT_NTRACKS] = { PH_KEY, PH_KEY, PH_KEY, PH_KEY };

/* the colours (D-037): dark, quiet, one accent; the mode is the top line's colour */
#define BG KIT_RGB(13, 15, 21)
#define FG KIT_RGB(234, 238, 245)
#define MUTED KIT_RGB(120, 128, 146)
#define LINE KIT_RGB(34, 39, 54)
#define CARD KIT_RGB(24, 27, 37)
#define CARD2 KIT_RGB(31, 35, 48)
#define ACC KIT_RGB(96, 160, 255)
#define ACC_M KIT_RGB(52, 92, 160)
#define ACC_D KIT_RGB(34, 56, 96)
#define REC KIT_RGB(235, 80, 80)
#define OK KIT_RGB(110, 210, 140)
#define WARN KIT_RGB(242, 163, 58)
#define KEY_W KIT_RGB(205, 211, 222)
#define KEY_B KIT_RGB(34, 38, 50)
static const kit_color_t MODE_COLS[NMODES] = { KIT_RGB(96, 160, 255), KIT_RGB(46, 196, 182), KIT_RGB(167, 139, 250) };
static const char *const MODE_NAMES[NMODES] = { "Producer", "Guided", "Game" };
static const char *const MODE_DESC[NMODES] = { "Your own song, in any order", "Build a song step by step",
                                               "A unicorn, three dancers, a robin" };
#define MODE_COL MODE_COLS[mode]

static char *cat(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; return d; }
static char *cat_int(char *d, int v)
{
    char t[8];
    int k = 0;
    if (v < 0) { *d++ = '-'; v = -v; }
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (k) *d++ = t[--k];
    *d = 0;
    return d;
}

static int sound_count(void)
{
    int e, n = 0;
    for (e = 0; e < kit_engine_count(); e++) n += kit_preset_count(e);
    return n;
}

static void sound_of(int i, int *engine, int *preset)
{
    int e;
    for (e = 0; e < kit_engine_count(); e++) {
        if (i < kit_preset_count(e)) { *engine = e; *preset = i; return; }
        i -= kit_preset_count(e);
    }
    *engine = 0; *preset = 0;
}

static int sound_index(int engine, int preset)
{
    int e, n = 0;
    for (e = 0; e < engine; e++) n += kit_preset_count(e);
    return n + preset;
}

static void ar_load_preset(int t, int engine, int preset);
static void sound_mod_reset(int t);

static void pick_sound(int t)
{
    int e, p;
    sound_of(snd[t], &e, &p);
    ar_load_preset(t, e, p);
    kit_track_sound(t, e, p);
    sound_mod_reset(t);
}

static int channel_of(int t) { return set.ch[t]; }

/* the track a channel plays (D-045): MIDI IN "this track" sends every channel to the track on screen; else the track
 * set to that channel (the first, if two share it), or none (-1) */
static int midi_track(int channel)
{
    int t;
    if (set.midi_in == 1) return track;
    for (t = 0; t < KIT_NTRACKS; t++) if (set.ch[t] == channel) return t;
    return -1;
}

static void settings_save(void) { kit_store_save(&set, sizeof set); }

/* the key the pads use: the song's (locked, or AUTO's when it is sure); looked at when the song is not playing, so
 * the pads never move under the hands */
static void update_key(void)
{
    int t, m, i;
    if (kit_song_key(&song, &t, &m) || !song.key_auto)
        for (i = 0; i < KIT_NTRACKS; i++) { pads[i].tonic = t; pads[i].minor = m; }
}

static int held_colour(void)        /* the colour of the black key pressed last of those held, or -1 */
{
    int k, best = -1;
    uint32_t at = 0;
    for (k = 0; k < KIT_NKEYS; k++)
        if (BLACK_COL[k] >= 0 && kit_key_held(k) && black_at[k] >= at) { at = black_at[k]; best = BLACK_COL[k]; }
    return best;
}

static void note_on(int note, int vel)
{
    kit_seq_note_on(track, note, vel);
    kit_midi_note_on(channel_of(track), note, vel);
    on_notes[n_on++] = (uint8_t)note;
}

static void notes_off(void)
{
    int i;
    for (i = 0; i < n_on; i++) {
        kit_seq_note_off(track, on_notes[i]);
        kit_midi_note_off(channel_of(track), on_notes[i]);
    }
    n_on = 0;
}

static void chord_release(void)
{
    notes_off();
    chord_key = -1;
    strum_i = pads[track].now.n;
    arp_i = -1;
}

/* play degree (as step of the progression, or -1) from key, the way K4 says */
static void chord_sound(int key, int vel);
static void chord_start(int key, int degree, int step, int row, int vel)
{
    chord_release();
    pad_play(&pads[track], degree, step, row, held_colour(), kit_key_held(KEY_SLASH));
    chord_row = row;
    chord_sound(key, vel);
}

/* sound pads[track].now from key, the way K4 FEEL says (the pads and PLAY's Pads, ST-80) */
static void chord_sound(int key, int vel)
{
    uint32_t now = kit_ms();
    chord_key = key;
    chord_vel = vel;
    strum_i = pads[track].now.n;
    if (pads[track].feel == PAD_BLOCK) {
        int i;
        for (i = 0; i < pads[track].now.n; i++) note_on(pads[track].now.notes[i], vel);
    } else if (pads[track].feel == PAD_STRUM) {
        note_on(pads[track].now.notes[0], vel);
        strum_i = 1;
        due_ms = now + STRUM_MS;
    } else {
        note_on(pads[track].now.notes[0], vel);
        arp_i = 0;
        due_ms = now + kit_beat_ms(song.bpm) / 2;   /* eighth notes at the song's tempo */
    }
    dirty = 1;
}

static void chord_poll(uint32_t now)    /* the strum's next notes and the arpeggio's steps, on time */
{
    while (chord_key >= 0 && strum_i < pads[track].now.n && (int32_t)(now - due_ms) >= 0) {
        note_on(pads[track].now.notes[strum_i++], chord_vel);
        due_ms += STRUM_MS;
    }
    if (chord_key >= 0 && arp_i >= 0 && pads[track].now.n && (int32_t)(now - due_ms) >= 0) {
        notes_off();
        arp_i = (arp_i + 1) % pads[track].now.n;
        note_on(pads[track].now.notes[arp_i], chord_vel);
        due_ms += kit_beat_ms(song.bpm) / 2;
    }
}

static int key_note(int key) { return KIT_KEY0_NOTE + key + 12 * octave; }
static int kb_low(void) { return KIT_KEY0_NOTE + 12 * octave; }
static int is_chords(void) { return set.role[track] == ROLE_CHORDS; }
static int is_manual(void) { return 0; }           /* the manual chord builder: retired by ST-82 (#108) */
static int is_ideas(void) { return is_chords() && pages[track] == PAGE_IDEAS; }   /* ST-82: the chord ideas */
static int is_play(void) { return is_chords() && pages[track] == PAGE_PLAY; }
static int is_pad(void) { return 0; }              /* the chord pad on AUTO: retired by ST-83 (#108) */
static int is_auto(void) { return is_chords() && pages[track] == PAGE_AUTO; }   /* ST-83: the song parts */
static int is_play_pads(void) { return is_play() && play_helps[track] == PH_PADS; }   /* ST-80 */
static int play_shift(void);
static void play_knob(int id, int d);
static void ph_pad_down(int key, int vel);
static void ph_pad_up(int key);
static void ci_knob(int id, int d);
static void sq_new_song(void);
static int sq_play_song(void);
static int sq_event(const kit_event_t *e);
static int sq_view;
static int rl_open, rl_ask;                     /* reclayer.c's: the REC layer open, asking (SEQ is theirs then) */
static void au_knob(int id, int d);
static void au_key_down(int key);
static void au_release(void);
static void ci_leave(void);

/* the notes held on PLAY, low to high: the keys and MIDI channel 1 */
static int play_held(uint8_t *out, int max)
{
    uint32_t m[4];
    int k, n = 0;
    for (k = 0; k < 4; k++) m[k] = midi_held[k];
    for (k = 0; k < KIT_NKEYS; k++)
        if (kit_key_held(k)) { int x = key_note(k) + play_shift(); m[x >> 5] |= 1u << (x & 31); }
    for (k = 0; k < 128 && n < max; k++) if (m[k >> 5] >> (k & 31) & 1u) out[n++] = (uint8_t)k;
    return n;
}

/* the key a note lights: its own, or the nearest of its name on the keys */
static int key_of(int note)
{
    int low = kb_low();
    while (note < low) note += 12;
    while (note > low + KIT_NKEYS - 1) note -= 12;
    return note - low;
}

static int has_key(const uint8_t *notes, int n, int key)
{
    int i;
    for (i = 0; i < n; i++) if (key_of(notes[i]) == key) return 1;
    return 0;
}

static void marp_off(void)
{
    if (marp_note < 0) return;
    kit_seq_note_off(track, marp_note);
    kit_midi_note_off(channel_of(track), marp_note);
    marp_note = -1;
}

/* ARP on MANUAL: the held notes one at a time, upwards, an eighth note each at the song's tempo, recorded as played */
static void man_arp_poll(uint32_t now)
{
    uint8_t h[KIT_NKEYS];
    int n = 0, k;
    if (!(man_arps[track] && is_manual())) { marp_off(); return; }
    for (k = 0; k < KIT_NKEYS; k++) if (kit_key_held(k)) h[n++] = (uint8_t)key_note(k);
    if (!n) { marp_off(); return; }
    if (marp_note >= 0 && (int32_t)(now - marp_due) < 0) return;
    if (marp_note < 0) { marp_i = 0; marp_due = now; }
    else marp_i = (marp_i + 1) % n;
    marp_off();
    if (marp_i >= n) marp_i = 0;
    marp_note = h[marp_i];
    kit_seq_note_on(track, marp_note, 100);
    kit_midi_note_on(channel_of(track), marp_note, 100);
    marp_due += kit_beat_ms(song.bpm) / 2;
    dirty = 1;
}

static void wrap(int *v, int d, int n);
static int is_bass(void) { return set.role[track] == ROLE_BASS; }
static int is_melody(void) { return set.role[track] == ROLE_MELODY; }
static void mel_step(int step, int at);
static int bass_low(void) { return KIT_KEY0_NOTE - 12 + 12 * octave; }   /* the note key 0 sounds on the bass */
static int bass_chord_now(int *third)
{
    return kit_seq_playing() ? bass_chord_at(&song, kit_seq_tick(), pads[track].tonic, pads[track].minor, third) : -1;
}

static void bass_release_all(void)
{
    int i;
    for (i = 0; i < bass_noffs; i++) {
        if (bass_offs[i].rec) kit_seq_note_off(bass_offs[i].trk, bass_offs[i].note);
        else kit_track_note_off(bass_offs[i].trk, bass_offs[i].note);
    }
    bass_noffs = 0;
    for (i = 0; i < KIT_NKEYS; i++)
        if (bass_key_note[i] >= 0) { kit_seq_note_off(track, bass_key_note[i]); bass_key_note[i] = -1; }
        if (bass_key_plus[i] >= 0) { kit_seq_note_off(track, bass_key_plus[i]); bass_key_plus[i] = -1; }
    bass_root_pad = -1;
}

static void bass_key_down(int key, int vel)
{
    int note = KIT_KEY0_NOTE + key - 12 + 12 * octave, third;
    if (basses[track].page == BASS_AUTO) {               /* a degree pad: its root; the song starts if it was stopped */
        kit_chord_t tri[7];
        if (PAD_DEG[key] < 0 || !kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri)) return;
        bass_root_pad = tri[PAD_DEG[key]].root;
        if (!kit_seq_playing()) { kit_seq_play(&song); bass_auto_started = 1; }
        dirty = 1;
        return;
    }
    if (basses[track].page == BASS_PLAY && basses[track].help == HELP_WHITES) {
        int w, k;
        if (!kit_is_white(KIT_KEY0_NOTE + key)) return;  /* the black keys rest */
        for (w = 0, k = 0; k < key; k++) w += kit_is_white(KIT_KEY0_NOTE + k);
        note = bass_white_note(&basses[track], pads[track].tonic, pads[track].minor, w, bass_low());
    } else if (basses[track].page == BASS_PLAY && basses[track].help != HELP_FREE
               && bass_role(&basses[track], pads[track].tonic, pads[track].minor, note, bass_chord_now(&third)) == ROLE_OUT) {
        if (basses[track].outside == OUT_MUTE) return;
        if (basses[track].outside == OUT_SOFT) vel = vel * 40 / 100 + 1;
    }
    last_note = note;
    last_channel = 0;
    bass_key_note[key] = (int8_t)note;
    kit_seq_note_on(track, note, vel);
    kit_midi_note_on(channel_of(track), note, vel);
    if (basses[track].page == BASS_PLAY && basses[track].plus != BPLUS_OFF) {   /* PLUS: the octave or the fifth with it (R-272) */
        int p = note + (basses[track].plus == BPLUS_OCT ? 12 : 7);
        if (p < 128) { bass_key_plus[key] = (int8_t)p; kit_seq_note_on(track, p, vel * 3 / 4 + 1); kit_midi_note_on(channel_of(track), p, vel * 3 / 4 + 1); }
    }
}

static void bass_key_up(int key)
{
    if (basses[track].page == BASS_AUTO) {
        int k, any = 0;
        for (k = 0; k < KIT_NKEYS; k++) if (k != key && PAD_DEG[k] >= 0 && kit_key_held(k)) any = 1;
        if (!any && PAD_DEG[key] >= 0) {
            bass_root_pad = -1;
            if (bass_auto_started && !armed) { kit_seq_stop(); bar = -1; update_key(); }
            bass_auto_started = 0;
        }
        dirty = 1;
        return;
    }
    if (bass_key_plus[key] >= 0) { kit_seq_note_off(track, bass_key_plus[key]); kit_midi_note_off(channel_of(track), bass_key_plus[key]); bass_key_plus[key] = -1; }
    if (bass_key_note[key] < 0) return;
    kit_seq_note_off(track, bass_key_note[key]);
    kit_midi_note_off(channel_of(track), bass_key_note[key]);
    bass_key_note[key] = -1;
}

/* IDEAS (heard) and AUTO: start the notes of each sixteenth as the song passes it, and release them on time */
static void bass_start(int note, int len_ticks, int at, int rec)
{
    if (bass_noffs >= 8) return;
    if (rec) kit_seq_note_on(track, note, 100);
    else kit_track_note_on(track, note, 100);
    bass_offs[bass_noffs].off = (int16_t)(at + len_ticks);
    bass_offs[bass_noffs].note = (uint8_t)note;
    bass_offs[bass_noffs].rec = (uint8_t)rec;
    bass_offs[bass_noffs].trk = (uint8_t)track;
    bass_noffs++;
}

static void bass_step(int step, int at)           /* step: the sixteenth of the loop; at: its tick */
{
    const int len = song.bars * KIT_TICKS_PER_BAR, s16 = step % 16;
    int third, i, root, next_third, next;
    if (basses[track].page == BASS_IDEAS && is_bass()) {
        const bass_line_t *ln = bass_ideas_line(&basses[track]);
        int hear = basses[track].hear == HEAR_ON || (basses[track].hear == HEAR_LISTEN && bass_loops % 2 == 0);
        root = bass_chord_at(&song, at, pads[track].tonic, pads[track].minor, &third);
        next = bass_chord_at(&song, (at / KIT_TICKS_PER_BAR + 1) * KIT_TICKS_PER_BAR % len, pads[track].tonic, pads[track].minor, &next_third);
        if (root < 0) return;
        for (i = 0; i < ln->n; i++)
            if (ln->note[i].step == s16 && hear)
                bass_start(bass_line_note(&basses[track], pads[track].tonic, pads[track].minor, &ln->note[i], root, third, next < 0 ? root : next, bass_low()),
                           ln->note[i].len * 6 - 1, at, 0);
    } else if (basses[track].page == BASS_AUTO && is_bass()) {
        const bass_line_t *ln = bass_auto_line(&basses[track]);
        root = bass_root_pad >= 0 ? bass_root_pad : basses[track].follow ? bass_chord_at(&song, at, pads[track].tonic, pads[track].minor, &third) : -1;
        if (root < 0) return;
        third = 4;
        { kit_chord_t tri[7]; int d; if (kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri)) for (d = 0; d < 7; d++) if (tri[d].root == root) third = tri[d].quality == KIT_Q_MINOR || tri[d].quality == KIT_Q_DIM ? 3 : 4; }
        next = bass_chord_at(&song, (at / KIT_TICKS_PER_BAR + 1) * KIT_TICKS_PER_BAR % len, pads[track].tonic, pads[track].minor, &next_third);
        for (i = 0; i < ln->n; i++)
            if (ln->note[i].step == s16)
                bass_start(bass_line_note(&basses[track], pads[track].tonic, pads[track].minor, &ln->note[i], root, third, next < 0 ? root : next, bass_low()),
                           ln->note[i].len * 6 - 1, at, 1);
    }
}

/* the idea's notes around now, for the lights and the screen */
static void bass_ideas_around(int tick)
{
    const bass_line_t *ln = bass_ideas_line(&basses[track]);
    int i, k, best = 9999, third, nt, len = song.bars * KIT_TICKS_PER_BAR;
    bass_now_idea = bass_next_idea = -1;
    if (tick < 0 || !is_bass() || basses[track].page != BASS_IDEAS) return;
    for (k = 0; k < 2; k++)
        for (i = 0; i < ln->n; i++) {
            int bar0 = (tick / KIT_TICKS_PER_BAR + k), at = (bar0 * KIT_TICKS_PER_BAR + ln->note[i].step * 6), d = at - tick;
            int root = bass_chord_at(&song, at % len, pads[track].tonic, pads[track].minor, &third), next;
            if (root < 0) continue;
            next = bass_chord_at(&song, ((bar0 + 1) * KIT_TICKS_PER_BAR) % len, pads[track].tonic, pads[track].minor, &nt);
            if (d <= 0 && d + ln->note[i].len * 6 > 0)
                bass_now_idea = bass_line_note(&basses[track], pads[track].tonic, pads[track].minor, &ln->note[i], root, third, next < 0 ? root : next, bass_low());
            else if (d > 0 && d < best) {
                best = d;
                bass_next_idea = bass_line_note(&basses[track], pads[track].tonic, pads[track].minor, &ln->note[i], root, third, next < 0 ? root : next, bass_low());
            }
        }
}

static void bass_poll(void)
{
    int tick = kit_seq_playing() ? kit_seq_tick() : -1, i;
    if (tick < 0) {
        if (bass_prev_tick >= 0) bass_release_all();
        bass_prev_tick = -1;
        bass_loops = 0;
        return;
    }
    if (bass_prev_tick < 0) bass_prev_tick = tick - 1;
    if (tick < bass_prev_tick) {                /* round the loop */
        int len = song.bars * KIT_TICKS_PER_BAR;
        for (i = 0; i < bass_noffs; i++) bass_offs[i].off = (int16_t)(bass_offs[i].off - len);
        for (i = bass_prev_tick + 1; i < len; i++) if (i % 6 == 0) { if (is_melody()) mel_step(i / 6, i); else bass_step(i / 6, i); }
        bass_prev_tick = -1;
        bass_loops++;
    }
    for (i = bass_prev_tick + 1; i <= tick; i++) {
        int sw = ((is_bass() && basses[track].page == BASS_AUTO && basses[track].swing) || (is_melody() && mels[track].page == MEL_AUTO && mels[track].swing))
                 && (i / 6) % 2 == 1 ? 2 : 0;    /* swing: the off-sixteenths later */
        if ((i - sw) % 6 == 0 && i - sw >= 0) { if (is_melody()) mel_step((i - sw) / 6, i); else bass_step((i - sw) / 6, i); }
    }
    for (i = 0; i < bass_noffs; ) {
        if (bass_offs[i].off <= tick) {
            if (bass_offs[i].rec) kit_seq_note_off(bass_offs[i].trk, bass_offs[i].note);
            else kit_track_note_off(bass_offs[i].trk, bass_offs[i].note);
            bass_offs[i] = bass_offs[--bass_noffs];
        } else i++;
    }
    if (tick / 6 != bass_prev_tick / 6) dirty = 1;
    bass_prev_tick = tick;
}

static void bass_knob(int id, int d)
{
    if (basses[track].page == BASS_PLAY) {
        if (basses[track].help == HELP_FREE && (id == KIT_ENC_KNOB2 || id == KIT_ENC_KNOB3)) { free_note_until = kit_ms() + 1500; return; }
        if (id == KIT_ENC_KNOB1) wrap(&basses[track].help, d, BASS_NHELPS);
        if (id == KIT_ENC_KNOB2) { bass_scale_step(&basses[track], &song, pads[track].tonic, pads[track].minor, d); bass_pop_until = kit_ms() + 1500; }
        if (id == KIT_ENC_KNOB3) wrap(&basses[track].outside, d, BASS_NOUTS);
        if (id == KIT_ENC_KNOB4) wrap(&basses[track].plus, d, BASS_NPLUS);
    } else if (basses[track].page == BASS_IDEAS) {
        if (id == KIT_ENC_KNOB1) { wrap(&basses[track].style, d, BASS_NSTYLES); basses[track].line = 0; }
        if (id == KIT_ENC_KNOB2 || id == KIT_ENC_SELECT) wrap(&basses[track].line, d > 0 ? 1 : -1, BASS_STYLES[basses[track].style].n);
        if (id == KIT_ENC_KNOB3) { basses[track].speed += d; if (basses[track].speed < 0) basses[track].speed = 0; if (basses[track].speed >= MEL_NSPEEDS) basses[track].speed = MEL_NSPEEDS - 1; }   /* SPEED (R-271) */
        if (id == KIT_ENC_KNOB4) wrap(&basses[track].hear, d, BASS_NHEARS);
    } else {
        if (id == KIT_ENC_KNOB1) { wrap(&basses[track].astyle, d, BASS_NSTYLES); basses[track].aline = 0; }
        if (id == KIT_ENC_KNOB2 || id == KIT_ENC_SELECT) wrap(&basses[track].aline, d > 0 ? 1 : -1, BASS_STYLES[basses[track].astyle].n);
        if (id == KIT_ENC_KNOB3) wrap(&basses[track].swing, d, 2);
        if (id == KIT_ENC_KNOB4) wrap(&basses[track].follow, d, 2);
    }
}

/* ---- the melody (D-050) ---- */
static int mel_low(void) { return KIT_KEY0_NOTE + 12 * octave; }
static int mel_white_note(int key)               /* HELP Whites: the white keys play the scale from its tonic */
{
    const kit_scale_t *sc = mel_scale(&mels[track], pads[track].minor);
    int w = 0, k, low = mel_low(), t0;
    if (!kit_is_white(KIT_KEY0_NOTE + key)) return -1;
    for (k = 0; k < key; k++) w += kit_is_white(KIT_KEY0_NOTE + k);
    t0 = low + pc12(pads[track].tonic - low);
    return t0 + sc->iv[w % sc->n] + 12 * (w / sc->n);
}
static uint16_t mel_chord_now(void)              /* the pitch classes of the chord sounding now (none: 0) */
{
    uint8_t nt[16];
    uint16_t m = 0;
    int i, n = song_notes_at(&song, kit_seq_playing() ? kit_seq_tick() : 0, nt, 16);
    for (i = 0; i < n; i++) m |= (uint16_t)(1u << (nt[i] % 12));
    return m;
}
static void mel_release_all(void)
{
    int k;
    for (k = 0; k < KIT_NKEYS; k++) {
        if (mel_key_note[k] >= 0) { kit_seq_note_off(track, mel_key_note[k]); kit_midi_note_off(channel_of(track), mel_key_note[k]); }
        if (mel_key_plus[k] >= 0) kit_seq_note_off(track, mel_key_plus[k]);
        mel_key_note[k] = mel_key_plus[k] = -1;
    }
    for (k = 0; k < 8; k++) { if (mel_echo[k].sounding) kit_track_note_off(track, mel_echo[k].note); mel_echo[k].on = mel_echo[k].off = 0; mel_echo[k].sounding = 0; }
    mel_root_pad = -1;
}
static void mel_echo_add(int note, int vel, uint32_t on, uint32_t len)
{
    int i;
    for (i = 0; i < 8; i++) if (!mel_echo[i].on && !mel_echo[i].sounding) {
        mel_echo[i].on = on; mel_echo[i].off = on + len; mel_echo[i].note = (uint8_t)note; mel_echo[i].vel = (uint8_t)vel; return;
    }
}
static void mel_echo_poll(uint32_t now)
{
    int i;
    for (i = 0; i < 8; i++) {
        mel_echo_t *e = &mel_echo[i];
        if (e->on && (int32_t)(now - e->on) >= 0) { kit_track_note_on(track, e->note, e->vel); e->sounding = 1; e->on = 0; }
        if (e->sounding && (int32_t)(now - e->off) >= 0) { kit_track_note_off(track, e->note); e->sounding = 0; }
    }
}
static void mel_key_down(int key, int vel)
{
    mel_t *m = &mels[track];
    int note = key_note(key), plus;
    if (m->page == MEL_AUTO) {                  /* a degree pad: its chord; the song starts if it was stopped */
        kit_chord_t tri[7];
        if (PAD_DEG[key] < 0) return;
        kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
        mel_root_pad = tri[PAD_DEG[key]].root;
        if (!kit_seq_playing()) { kit_seq_play(&song); mel_auto_started = 1; }
        return;
    }
    if (m->page == MEL_PLAY && m->help == MHELP_WHITES) {
        note = mel_white_note(key);
        if (note < 0) return;                   /* the black keys rest */
    } else if (m->page == MEL_PLAY && m->help != MHELP_FREE && m->outside == MOUT_MUTE
               && !(scale_mask(mel_scale(m, pads[track].minor), pads[track].tonic) >> pc12(note) & 1u)) {
        return;                                 /* OUTSIDE mute */
    }
    kit_seq_note_on(track, note, vel);
    kit_midi_note_on(channel_of(track), note, vel);
    mel_key_note[key] = (int8_t)note;
    last_note = note;
    last_channel = 0;
    plus = m->page == MEL_PLAY ? mel_plus_note(m, pads[track].tonic, pads[track].minor, note) : -1;
    if (plus >= 0 && plus < 128) { kit_seq_note_on(track, plus, vel * 3 / 4 + 1); mel_key_plus[key] = (int8_t)plus; }
    if (m->page == MEL_PLAY && m->plus == MPLUS_ECHO) {   /* two softer repeats, a dotted eighth apart */
        uint32_t de = kit_beat_ms(song.bpm) * 3 / 4, now = kit_ms();
        mel_echo_add(note, vel * 55 / 100 + 1, now + de, de * 2 / 3);
        mel_echo_add(note, vel * 30 / 100 + 1, now + 2 * de, de * 2 / 3);
    }
}
static void mel_key_up(int key)
{
    if (mels[track].page == MEL_AUTO) {
        int k, any = 0;
        for (k = 0; k < KIT_NKEYS; k++) if (k != key && kit_key_held(k) && PAD_DEG[k] >= 0) any = 1;
        if (!any) {
            mel_root_pad = -1;
            if (mel_auto_started && !armed) { kit_seq_stop(); bar = -1; update_key(); }
            mel_auto_started = 0;
        }
        return;
    }
    if (mel_key_note[key] >= 0) { kit_seq_note_off(track, mel_key_note[key]); kit_midi_note_off(channel_of(track), mel_key_note[key]); }
    if (mel_key_plus[key] >= 0) kit_seq_note_off(track, mel_key_plus[key]);
    mel_key_note[key] = mel_key_plus[key] = -1;
}
static void mel_knob(int id, int d)
{
    mel_t *m = &mels[track];
    if (m->page == MEL_PLAY) {
        if (m->help == MHELP_FREE && (id == KIT_ENC_KNOB2 || id == KIT_ENC_KNOB3)) { free_note_until = kit_ms() + 1500; return; }
        if (id == KIT_ENC_KNOB1) wrap(&m->help, d, MEL_NHELPS);
        if (id == KIT_ENC_KNOB2) { mel_scale_step(m, &song, pads[track].tonic, pads[track].minor, d); mel_pop_until = kit_ms() + 1500; }
        if (id == KIT_ENC_KNOB3) wrap(&m->outside, d, MEL_NOUTS);
        if (id == KIT_ENC_KNOB4) wrap(&m->plus, d, MEL_NPLUS);
    } else if (m->page == MEL_IDEAS) {
        int f[MEL_NFIGS], n;
        if (id == KIT_ENC_KNOB1) { wrap(&m->style, d, 6); m->fig = 0; }
        n = mel_style_figs(m->style, f);
        if (id == KIT_ENC_KNOB2) wrap(&m->fig, d > 0 ? 1 : -1, n ? n : 1);
        if (id == KIT_ENC_KNOB3) { m->speed += d > 0 ? 1 : -1; m->speed = m->speed < 0 ? 0 : m->speed >= MEL_NSPEEDS ? MEL_NSPEEDS - 1 : m->speed; }
        if (id == KIT_ENC_KNOB4) wrap(&m->hear, d, BASS_NHEARS);
        if (id == KIT_ENC_SELECT) { m->version += d > 0 ? 1 : -1; m->version = m->version < 1 ? 1 : m->version > 9 ? 9 : m->version; }
    } else {
        if (id == KIT_ENC_KNOB1) { wrap(&m->astyle, d, MEL_NPSTYLES); m->aphrase = 0; }
        if (id == KIT_ENC_KNOB2 || id == KIT_ENC_SELECT) wrap(&m->aphrase, d > 0 ? 1 : -1, MEL_PHRASES[m->astyle].n);
        if (id == KIT_ENC_KNOB3) wrap(&m->swing, d, 2);
        if (id == KIT_ENC_KNOB4) wrap(&m->follow, d, 2);
    }
}
/* IDEAS: the figure over the bar's chord (Sketchpad renderFigure; the version is the seed); how many notes */
static int mel_bar_figure(int bar, kit_fig_note_t *out)
{
    const mel_fig_t *f = mel_idea(&mels[track]);
    uint8_t iv[12], niv2[12];
    int len = song.bars * KIT_TICKS_PER_BAR, root, nroot = -1, niv, nn;
    niv = mel_chord_at(&song, (bar * KIT_TICKS_PER_BAR) % len, pads[track].tonic, pads[track].minor, &root, iv);
    if (!niv) return 0;
    nn = mel_chord_at(&song, ((bar + 1) * KIT_TICKS_PER_BAR) % len, pads[track].tonic, pads[track].minor, &nroot, niv2);
    return kit_render_figure(f->st, f->n, root, iv, niv, nn ? nroot : root, scale_mask(mel_scale(&mels[track], pads[track].minor), pads[track].tonic),
                             mels[track].version * 1000 + (bar % song.bars) * 7919, 67 + 12 * octave, out);
}
static void mel_step(int step, int at)          /* step: the sixteenth of the loop; at: its tick */
{
    mel_t *m = &mels[track];
    int i, bar = at / KIT_TICKS_PER_BAR, s16 = step % 16;
    if (m->page == MEL_IDEAS) {
        kit_fig_note_t fn[16];
        int n = mel_bar_figure(bar, fn), hear = m->hear == HEAR_ON || (m->hear == HEAR_LISTEN && bass_loops % 2 == 0);
        if (!hear) return;
        for (i = 0; i < n; i++) if (fn[i].pos == s16) bass_start(fn[i].note, fn[i].dur * 6 * 9 / 10, at, 0);
    } else if (m->page == MEL_AUTO) {
        const mel_phrase_t *ph = mel_phrase(m);
        int third, root = mel_root_pad >= 0 ? mel_root_pad : m->follow ? bass_chord_at(&song, at, pads[track].tonic, pads[track].minor, &third) : -1;
        int b = (bass_loops * song.bars + bar) % 2, nn = b ? ph->nb : ph->na;
        const mel_pnote_t *pn = b ? ph->b : ph->a;
        if (root < 0) return;
        for (i = 0; i < nn; i++)
            if (pn[i].step == s16) {
                int note = mel_phrase_note(m, pads[track].tonic, pads[track].minor, pn[i].deg, root) + 12 * octave;
                bass_start(note, pn[i].len * 6 * 9 / 10, at, armed);
                last_note = note;
            }
    }
}
/* IDEAS: the figure's note sounding now and the next one, for the lights */
static void mel_ideas_around(int tick)
{
    kit_fig_note_t fn[16];
    int n, i, bar, best = 1 << 30;
    mel_now_idea = mel_next_idea = -1;
    if (tick < 0 || !is_melody() || mels[track].page != MEL_IDEAS) return;
    bar = tick / KIT_TICKS_PER_BAR;
    n = mel_bar_figure(bar, fn);
    for (i = 0; i < n; i++) {
        int a2 = bar * KIT_TICKS_PER_BAR + fn[i].pos * 6, e = a2 + fn[i].dur * 6;
        if (tick >= a2 && tick < e) mel_now_idea = fn[i].note;
        else if (a2 > tick && a2 - tick < best) { best = a2 - tick; mel_next_idea = fn[i].note; }
    }
    if (mel_next_idea < 0) { n = mel_bar_figure(bar + 1, fn); if (n) mel_next_idea = fn[0].note; }
}

static void pad_key_down(int key, int vel)
{
    int step, degree;
    if (PAD_DEG[key] >= 0) {
        degree = PAD_DEG[key];
        chord_start(key, degree, pad_step_of(&pads[track], degree), key >= 12, vel);
    } else if (key == KEY_NEXT) {
        degree = pad_next_degree(&pads[track], &step);
        chord_start(key, degree, step, 0, vel);
    } else if (key == KEY_AGAIN && pads[track].now_degree >= 0) {
        chord_start(key, pads[track].now_degree, pads[track].now_step, chord_row, vel);
    } else if (key == KEY_AUTO) {       /* the style's colours on / off; with a colour held: lock it, again: unlock */
        int h = held_colour();
        if (h >= 0) pads[track].locked = pads[track].locked == h ? -1 : h;
        else if (pads[track].locked >= 0) pads[track].locked = -1;
        else pads[track].auto_on = !pads[track].auto_on;
    } else if (BLACK_COL[key] >= 0) {
        black_at[key] = ++presses;
    }
    dirty = 1;
}

static void wrap(int *v, int d, int n) { *v = ((*v + d) % n + n) % n; }

/* ---- the templates (D-046) ---- */
static int role_sound(int r)                    /* a template's first sound */
{
    return r == ROLE_CHORDS ? sound_index(KIT_ENGINE_BASIC, KIT_INST_PIANO) : r == ROLE_BASS ? sound_index(1, 2)   /* Analog: SQR BASS */
         : r == ROLE_MELODY ? sound_index(2, 4) : sound_index(KIT_ENGINE_DRUMS, 0);                                 /* Phase: BELL */
}

/* a track's name: its template, numbered when two tracks share it ("MELODY 2") */
static const char *track_name(int t)
{
    static char nm[KIT_NTRACKS][12];
    int i, n = 0, k = 0;
    for (i = 0; i < KIT_NTRACKS; i++) if (set.role[i] == set.role[t]) { n++; if (i <= t) k++; }
    cat(nm[t], TRACK_NAMES[set.role[t]]);
    if (n > 1) { char *d = nm[t]; while (*d) d++; *d++ = ' '; cat_int(d, k); }
    return nm[t];
}

/* the song knows which tracks are drums (the key and transposing leave them out) */
static void roles_sync(void)
{
    int t;
    song.unpitched = 0;
    for (t = 0; t < KIT_NTRACKS; t++) if (set.role[t] == ROLE_DRUMS) song.unpitched |= (uint8_t)(1u << t);
}

/* the harmony the help reads (D-046): a chords track reads its own recording; the others read every other pitched
 * track, whatever its template, so changing a template changes no other track's help */
static void harm_sync(void)
{
    int t;
    if (is_chords()) { harm_mask = (uint8_t)(1u << track); return; }
    harm_mask = 0;
    for (t = 0; t < KIT_NTRACKS; t++) if (t != track && set.role[t] != ROLE_DRUMS) harm_mask |= (uint8_t)(1u << t);
}

static void rl_apply(int t);
static int u_event(const kit_event_t *e);
static int game_event(const kit_event_t *e);
static int dr_event(const kit_event_t *e);
static int dr_ed;                               /* the drums' EDIT page (drumpage.c), -1: closed */
static int dr_fxp;                              /* the drums' FX page (drumpage.c), -1: closed */
static int is_drums(void);
static void dr_init(void);
static void song_restore(void);
static void sb_init(void);
static int sb_event(const kit_event_t *e);
static void sb_frame(uint32_t now);
static void sb_draw(void);
static int sb_open;
static int ar_event(const kit_event_t *e);
static void ar_frame(uint32_t now);
static int ar_page(void);
static void ar_draw(void);
static void ar_lights(void);
static int sound_fx_event(const kit_event_t *e);
static void sound_fx_close(void);
static void sound_fx_bend(int t, int value);
static void sound_fx_mod(int t, int value);
static void sound_mod_apply(int t, int value);
static void sound_edit_press(void);
static void sound_edit_release(void);
static int sound_edit_event(const kit_event_t *e);
static void sound_edit_close(void);
static int dr_src_kit(int t, int p);
static int rl_open;                             /* (reclayer.c) */
static void rl_close(void);                     /* (reclayer.c) the layer and its question closed (R-241) */
static uint8_t rl_feel[KIT_NTRACKS], rl_grid[KIT_NTRACKS];
static int rl_nfeels(void);
static int rl_ngrids(void);
static const char *rl_grid_name(int t);
static const char *rl_feel_name(int t);
static int rl_grid_ticks(int t);
static const char *rl_feel_word(int t);
static int rl_feel_fine(int t, int g, int tick);
static const char *dr_kit_title(int t);
static void exp_knob(int id);
static int rl_event(const kit_event_t *e);
static void rec_snap_begin(void);
static void role_set(int t, int r)              /* a track takes a template: its notes stay; drums <-> pitched: the template's sound */
{
    int old = set.role[t];
    if (old == r) return;
    kit_track_notes_off(t);
    set.role[t] = (uint8_t)r;
    if ((old == ROLE_DRUMS) != (r == ROLE_DRUMS)) { snd[t] = role_sound(r); pick_sound(t); }
    pages[t] = 0;
    basses[t].page = BASS_PLAY;
    mels[t].page = MEL_PLAY;                    /* every template opens on PLAY (ST-50) */
    rl_apply(t);                                /* a drums track's feel goes by its drums */
    roles_sync();
    update_key();
    settings_save();
}

void app_init(void)
{
    kit_volume_limit(85);
    kit_usb_audio(usb_set.mon, usb_set.lvl);    /* USB audio (ST-98): monitor when sampling, 0 dB, until a song says otherwise */
    kit_volume(80);
    int t;
    {                                           /* the settings, as kept (D-045), with each track's template (D-046) */
        settings_t k;
        int n = kit_store_load(&k, sizeof k);
        int j;
        if (n == (int)sizeof k && k.version == SETTINGS_VERSION) set = k;
        else if (n == SETTINGS_V5 && k.version == 5) { k.g_lesson = k.g_step = k.g_stars = 0; k.version = SETTINGS_VERSION; set = k; }   /* before ST-57 */
        else if (n == SETTINGS_V4 && k.version == 4) { k.train_stars = 0; k.g_lesson = k.g_step = k.g_stars = 0; k.version = SETTINGS_VERSION; set = k; }   /* before ST-55 */
        else if ((n == SETTINGS_V3 && k.version == 3) || (n == SETTINGS_V3 - 1 && k.version == 2)) {   /* before #84 (and ST-43) */
            if (k.version == 2) k.explain = 1;
            for (j = 0; j < MY_SONGS; j++) k.my_len[j] = 0;
            k.train_stars = 0; k.g_lesson = k.g_step = k.g_stars = 0;
            k.version = SETTINGS_VERSION; set = k;
        }
    }
    for (t = 0; t < KIT_NTRACKS; t++) { snd[t] = role_sound(set.role[t]); pick_sound(t); }
    { int i; for (i = 0; i < KIT_NSEQS; i++) kit_song_init(&seqs[i], SONG_BPM, SONG_BARS); }
    sq_new_song();                              /* R-282 */
#ifdef USER_KIT_N
    kit_drum_user_kit(USER_KIT, USER_KIT_N);  /* kits/user/: the fifth kit, USER (ST-35) */
#endif
    dr_init();
    for (t = 0; t < KIT_NTRACKS; t++) { pad_init(&pads[t]); man_init(&mans[t]); bass_init(&basses[t]); mel_init(&mels[t]); }
    { int k; for (k = 0; k < KIT_NKEYS; k++) mel_key_note[k] = mel_key_plus[k] = -1; }
    roles_sync();
    { int k; for (k = 0; k < KIT_NKEYS; k++) bass_key_note[k] = bass_key_plus[k] = -1; }
    update_key();
    for (t = 0; t < KIT_NTRACKS; t++) man_set_root(&mans[t], 0, pads[t].tonic, pads[t].minor);
    kit_seq_clock_out(1);                       /* the song's clock goes out over USB (D-044) */
    kit_tune(set.tune);
    sb_init();                                  /* the Sound Browser's sounds (ST-68) */
    song_restore();                             /* the song kept at the last power-off (ST-89, R-297) */
    last_input = kit_ms();
    kit_log("companion: up");
}

static void guided_home_tap(void);
static int guided_event(const kit_event_t *e);
static void home_tap(void)
{
    sound_fx_close();
    sound_edit_close();
    dr_ed = -1;                                 /* the drums' EDIT pages close too (ST-96) */
    dr_fxp = -1;                                /* and its FX pages (ST-97) */
    if (confirm) {                              /* confirmed: switch, the song kept */
        chord_release();
        marp_off();
        kit_track_notes_off(track);
        mode = pick;
        picker = confirm = 0;
        song_page = 0;
    } else if (picker) {
        if (pick == mode) picker = 0;
        else confirm = 1;
    } else if (mode == MODE_GUIDED) {
        guided_home_tap();                      /* the guided mode: the lessons (ST-57) */
    } else {
        song_page = 0;                          /* the mode's home: the track page, with nothing open over it (R-241) */
        sq_view = -1;                           /* and out of the sequencer's pages (ST-85) */
        pend_track = -1;
        rl_close();
    }
    dirty = 1;
}

static void clampi(int *v, int d, int lo, int hi) { *v += d; if (*v < lo) *v = lo; if (*v > hi) *v = hi; }

static void sub_knob(int id, int d)             /* TRACKS (D-046), CLICK, MIDI, SYSTEM (D-045) */
{
    int v;
    if (song_sub == SUB_TRACKS) {
        int t = id - KIT_ENC_KNOB1, r = pend_track == t ? pend_role : set.role[t];
        wrap(&r, d, NROLES);
        if (r == set.role[t]) pend_track = -1;
        else if (song.count[t]) { pend_track = t; pend_role = r; }   /* it has notes: SAVE to change it */
        else { pend_track = -1; role_set(t, r); }
        return;
    }
    if (song_sub == SUB_CLICK) {
        if (id == KIT_ENC_KNOB1) { v = set.click; clampi(&v, d, 0, 2); set.click = (uint8_t)v; }
        if (id == KIT_ENC_KNOB2) { v = set.click_lvl; clampi(&v, d, 1, 10); set.click_lvl = (uint8_t)v; kit_click(1, v); }
        if (id == KIT_ENC_KNOB3) { v = set.count_in; clampi(&v, d, 0, 2); set.count_in = (uint8_t)v; }
    } else if (song_sub == SUB_MIDI) {
        if (id == KIT_ENC_KNOB1) wrap(&set_track, d, KIT_NTRACKS);
        if (id == KIT_ENC_KNOB2) { v = set.ch[set_track]; clampi(&v, d, 1, 16); set.ch[set_track] = (uint8_t)v; }
        if (id == KIT_ENC_KNOB3) { v = set.midi_in; clampi(&v, d, 0, 2); set.midi_in = (uint8_t)v; }
        if (id == KIT_ENC_KNOB4) { v = set.wake; clampi(&v, d, 0, 1); set.wake = (uint8_t)v; }
    } else if (song_sub == SUB_SYSTEM) {
        if (id == KIT_ENC_KNOB1) { v = set.scr_off; clampi(&v, d, 0, 5); set.scr_off = (uint8_t)v; }
        if (id == KIT_ENC_KNOB2) { v = set.tune; clampi(&v, d, -50, 50); set.tune = (int8_t)v; kit_tune(v); }
        if (id == KIT_ENC_KNOB3) { v = set.explain; clampi(&v, d, 0, 1); set.explain = (uint8_t)v; }
    } else if (song_sub == SUB_USB) {          /* K1 OUT: the mix (4 tracks later); K2 MONITOR; K3 IN LEVEL (R-404) */
        if (id == KIT_ENC_KNOB2) { v = usb_set.mon; clampi(&v, d, 0, 2); usb_set.mon = (uint8_t)v; }
        if (id == KIT_ENC_KNOB3) { v = usb_set.lvl; clampi(&v, d, -12, 24); usb_set.lvl = (int8_t)v; }
        kit_usb_audio(usb_set.mon, usb_set.lvl);
        return;
    }
    settings_save();
}

static void song_knob(int id, int d)            /* the song page: K1 key, K2 tempo, K3 length, K4 4/4 */
{
    if (song_sub != SUB_SONG) { sub_knob(id, d); return; }
    if (id == KIT_ENC_KNOB1) {
        int i = song.key_auto ? 0 : 1 + song.key_tonic * 2 + song.key_minor;
        wrap(&i, d, 25);
        if (i == 0) kit_song_auto_key(&song);
        else kit_song_lock_key(&song, (i - 1) / 2, (i - 1) % 2);
        update_key();
    }
    if (id == KIT_ENC_KNOB2) kit_song_set_tempo(&song, kit_tempo_clamp(song.bpm + d * KIT_TEMPO_STEP));
    if (id == KIT_ENC_KNOB3) {
        int b = song.bars + d;
        kit_song_set_bars(&song, b < 1 ? 1 : b > KIT_SONG_MAX_BARS ? KIT_SONG_MAX_BARS : b);
    }
}

/* HOME, GLO, the mode picker and the song page come first; returns 1 when the event is theirs */
static int mode_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_HOME) { home_at = kit_ms(); home_long = 0; return 1; }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_HOME) { if (!home_long) home_tap(); return 1; }
    if (picker) {
        if ((e->type == KIT_EV_KEY_UP && mode == MODE_PRODUCER) || e->type == KIT_EV_MIDI_NOTE_OFF)
            return 0;                           /* a key let go still stops its note (R-244, HAZ-02) */
        if (e->type == KIT_EV_ENC && e->id == KIT_ENC_SELECT && !confirm) wrap(&pick, e->delta > 0 ? 1 : -1, NMODES);
        else if (e->type == KIT_EV_BTN_DOWN) { if (confirm) confirm = 0; else picker = 0; }
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_GLO) { glo_at = kit_ms(); glo_long = 0; return 1; }
    if (e->type == KIT_EV_BTN_UP && e->id == KIT_BTN_GLO) {
        if (help) help = 0;
        else if (mode == MODE_PRODUCER) { song_page = !song_page; song_sub = SUB_SONG; }
        dirty = 1;
        return 1;
    }
    if (mode == MODE_GAME && !picker && !help) return game_event(e);   /* the games: their keys and knobs (ST-51, D-062) */
    if (mode == MODE_GUIDED && !picker && !help) return guided_event(e);   /* the guided mode (ST-57, D-068) */
    if (mode != MODE_PRODUCER) return e->type != KIT_EV_MIDI_NOTE_ON && e->type != KIT_EV_MIDI_NOTE_OFF;
    if (song_page && pend_track >= 0 && (e->type == KIT_EV_ENC || e->type == KIT_EV_KEY_DOWN))
        return 1;                               /* a question takes the knobs and new keys until it is answered (R-242) */
    if (song_page && pend_track >= 0 && e->type == KIT_EV_BTN_DOWN) {   /* TRACKS: SAVE changes it, any other button keeps it */
        if (e->id == KIT_BTN_SAVE) role_set(pend_track, pend_role);
        pend_track = -1;
        dirty = 1;
        return 1;
    }
    if (song_page && ((e->type == KIT_EV_ENC && (e->id == KIT_ENC_ALGORITHM || e->id == KIT_ENC_PRESETS)) ||
                      (e->type == KIT_EV_BTN_DOWN && (e->id == KIT_BTN_OCT_DOWN || e->id == KIT_BTN_OCT_UP)))) {
        song_page = 0;                          /* they change the track: the page closes so you see what they did (R-240) */
        dirty = 1;
        return 0;
    }
    if (song_page && e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_SCL) {   /* the sub-pages (D-045) */
        wrap(&song_sub, 1, NSUBS);
        dirty = 1;
        return 1;
    }
    if (song_page && e->type == KIT_EV_ENC && e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB4) {
        song_knob(e->id, e->delta);
        dirty = 1;
        return 1;
    }
    return 0;
}

static int panel_event(const kit_event_t *e)
{
    return e->type == KIT_EV_KEY_DOWN || e->type == KIT_EV_KEY_UP || e->type == KIT_EV_BTN_DOWN || e->type == KIT_EV_BTN_UP
        || e->type == KIT_EV_ENC;
}

static int panel_held(void)
{
    int i;
    for (i = 0; i < KIT_NKEYS; i++) if (kit_key_held(i)) return 1;
    for (i = 0; i < KIT_NBTNS; i++) if (kit_btn_held(i)) return 1;
    return 0;
}

void app_event(const kit_event_t *e)
{
    int panel = panel_event(e);
    harm_sync();
    if (panel || (e->type == KIT_EV_MIDI_NOTE_ON && set.wake)) {
        last_input = kit_ms();
        if (asleep) {                           /* it wakes: a press, key or turn does nothing else */
            asleep = 0;
            dirty = 1;
            if (panel) { eat = 1; return; }
        }
    }
    if (eat && panel) return;
    if (e->type == KIT_EV_ENC) exp_knob(e->id);  /* explain the card once the page shows its new value (ST-43) */
    if (sb_event(e))                            /* the Sound Browser: PRESETS, and K1 and HOME while it shows (ST-68) */
        return;
    if (mode_event(e))
        return;
    if (sound_fx_event(e))
        return;
    if (sound_edit_event(e))
        return;
    if (sq_event(e))                            /* the sequencer's pages (ST-85) */
        return;
    if (rl_event(e))                            /* the REC layer, EDIT + key, SAVE (ST-40, ST-41) */
        return;
    if (ar_event(e))                            /* ARP: the button, and the keys and knobs while it is on (ST-77, ST-78) */
        return;
    if (dr_event(e))                            /* the drums' keys, kit and pads (ST-30, ST-38, ST-39) */
        return;
    switch (e->type) {
    case KIT_EV_KEY_DOWN:
        if (is_auto()) {
            au_key_down(e->id);                 /* a part, or a move held (ST-83) */
        } else if (is_play_pads()) {
            ph_pad_down(e->id, e->vel);
        } else if (is_pad()) {
            pad_key_down(e->id, e->vel);
        } else if (is_bass()) {
            bass_key_down(e->id, e->vel);
        } else if (is_melody()) {
            mel_key_down(e->id, e->vel);
        } else {
            last_note = key_note(e->id) + play_shift();   /* TRANSPOSE whites on PLAY (R-251) */
            last_channel = 0;
            if (!(man_arps[track] && is_manual())) {        /* with ARP on, the arpeggiator plays the held notes */
                kit_seq_note_on(track, last_note, e->vel);
                kit_midi_note_on(channel_of(track), last_note, e->vel);
            }
        }
        dirty = 1;
        break;
    case KIT_EV_KEY_UP:
        if (is_auto()) {
            ;                                   /* the moves last while held */
        } else if (is_play_pads()) {
            ph_pad_up(e->id);
        } else if (is_pad()) {
            if (e->id == chord_key) chord_release();
        } else if (is_bass()) {
            bass_key_up(e->id);
        } else if (is_melody()) {
            mel_key_up(e->id);
        } else if (!(man_arps[track] && is_manual())) {
            kit_seq_note_off(track, key_note(e->id) + play_shift());
            kit_midi_note_off(channel_of(track), key_note(e->id) + play_shift());
        }
        dirty = 1;
        break;
    case KIT_EV_MIDI_NOTE_ON: {
        int t = midi_track(e->delta);
        if (t < 0) break;                       /* a channel no track listens to */
        if (set.midi_in == 2 && t != track) {   /* the screen follows the track played */
            chord_release();
            kit_track_notes_off(track);
            if (is_bass()) bass_release_all();
            track = t;
            if (armed) { kit_seq_record(track); rec_snap_begin(); }
        }
        if (t == track && is_chords() && e->id < 128) midi_held[e->id >> 5] |= 1u << (e->id & 31);
        last_note = e->id;
        last_channel = e->delta;
        last_mtrack = t;
        kit_track_note_on(t, e->id, e->vel);
        dirty = 1;
        break;
    }
    case KIT_EV_MIDI_NOTE_OFF: {
        int t = midi_track(e->delta);
        if (t < 0) break;
        if (t == track && is_chords() && e->id < 128) midi_held[e->id >> 5] &= ~(1u << (e->id & 31));
        kit_track_note_off(t, e->id);
        dirty = 1;
        break;
    }
    case KIT_EV_MIDI_CC: {                      /* a keyboard's controls, on the channel's track (D-044) */
        int t = midi_track(e->delta);
        if (t < 0) break;
        if (e->id == 64) kit_track_sustain(t, e->vel >= 64);
        else if (e->id == 1) sound_fx_mod(t, e->vel);
        else if (e->id == 120 || e->id == 123) {
            kit_track_sustain(t, 0);
            kit_track_notes_off(t);
            if (t == track) { midi_held[0] = midi_held[1] = midi_held[2] = midi_held[3] = 0; dirty = 1; }
        } else if (e->id == 121) {
            sound_fx_bend(t, 0);
            sound_fx_mod(t, 0);
            kit_track_sustain(t, 0);
        }
        break;
    }
    case KIT_EV_MIDI_BEND: {
        int t = midi_track(e->delta);
        if (t >= 0) sound_fx_bend(t, (e->vel << 7 | e->id) - 8192);
        break;
    }
    case KIT_EV_MIDI_CLOCK:                     /* the other gear leads: the song follows its clock (D-044) */
        if (e->id == 0xFC) {
            kit_seq_stop();
            kit_seq_follow(0);
            bar = -1;
            update_key();
        } else {
            kit_seq_follow(1);
            if (e->id == 0xFA) kit_seq_play(&song);
            else kit_seq_resume(&song);
        }
        dirty = 1;
        break;
    case KIT_EV_SEQ_BAR:
        bar = e->id;
        dirty = 1;
        break;
    case KIT_EV_BTN_DOWN:
        switch (e->id) {
        case KIT_BTN_PLAY:
            kit_seq_follow(0);                  /* PLAY on the FM-1: it leads */
            if (kit_seq_counting()) {           /* PLAY during the count-in: stop */
                kit_seq_stop();
            } else if (!kit_seq_playing() && armed && set.count_in) {   /* REC armed: count in first (D-045) */
                sq_play_song();
                kit_seq_count_in(&song, set.count_in, set.click_lvl);
            } else if (kit_seq_playing()) {
                kit_seq_stop();
                bar = -1;
                update_key();
            } else {
                sq_play_song();                 /* on SONG: the song's order from the cursor's block */
                kit_seq_play(&song);
            }
            break;
        case KIT_BTN_SCL:
            if (is_chords()) {
                chord_release();
                if (is_ideas()) ci_leave();       /* SPEED back to full (R-264) */
                if (is_auto()) au_release();
                kit_track_notes_off(track);
                marp_off();
                wrap(&pages[track], 1, NPAGES);
            } else if (is_bass()) {
                bass_release_all();
                kit_track_notes_off(track);
                wrap(&basses[track].page, 1, BASS_NPAGES);
            } else if (is_melody()) {
                mel_release_all();
                bass_release_all();
                kit_track_notes_off(track);
                wrap(&mels[track].page, 1, MEL_NPAGES);
            }
            break;
        case KIT_BTN_OCT_DOWN:
        case KIT_BTN_OCT_UP:
            chord_release();
            kit_track_notes_off(track);
            octave += e->id == KIT_BTN_OCT_UP ? 1 : -1;
            octave = octave < -OCT_RANGE ? -OCT_RANGE : octave > OCT_RANGE ? OCT_RANGE : octave;
            pads[track].octave = octave;
            break;
        }
        dirty = 1;
        break;
    case KIT_EV_BTN_UP:
        dirty = 1;
        break;
    case KIT_EV_ENC:
        if (e->id == KIT_ENC_ALGORITHM) {
            if (is_ideas()) ci_leave();           /* the IDEAS page closes (R-264) */
            if (is_auto()) au_release();
            chord_release();
            kit_track_notes_off(track);
            if (is_bass() || is_melody()) bass_release_all();
            if (is_melody()) mel_release_all();
            wrap(&track, e->delta, KIT_NTRACKS);
            if (armed) { kit_seq_record(track); rec_snap_begin(); }
        } else if (e->id == KIT_ENC_PRESETS) {
            wrap(&snd[track], e->delta, sound_count());
            pick_sound(track);
        } else if (is_chords() && pages[track] == PAGE_PLAY) {
            play_knob(e->id, e->delta);
        } else if (is_pad()) {
            if (e->id == KIT_ENC_KNOB1) { wrap(&pads[track].style, e->delta, PAD_NSTYLES); pads[track].prog = 0; pads[track].pos = 0; }
            if (e->id == KIT_ENC_KNOB2) { wrap(&pads[track].prog, e->delta, PAD_STYLES[pads[track].style].n); pads[track].pos = 0; }
            if (e->id == KIT_ENC_KNOB3) wrap(&pads[track].voicing, e->delta, KIT_NVOICINGS);
            if (e->id == KIT_ENC_KNOB4) wrap(&pads[track].feel, e->delta, PAD_NFEELS);
        } else if (is_bass()) {
            bass_knob(e->id, e->delta);
        } else if (is_melody()) {
            mel_knob(e->id, e->delta);
        } else if (is_auto()) {
            au_knob(e->id, e->delta);
        } else if (is_ideas()) {
            ci_knob(e->id, e->delta);
        } else if (is_manual()) {
            if (e->id == KIT_ENC_KNOB1) man_set_root(&mans[track], mans[track].root_i + e->delta, pads[track].tonic, pads[track].minor);
            if (e->id == KIT_ENC_KNOB2) wrap(&mans[track].type, e->delta, MAN_NTYPES);
            if (e->id == KIT_ENC_KNOB3) wrap(&mans[track].colour, e->delta, MAN_NCOLS);
            if (e->id == KIT_ENC_KNOB4) wrap(&mans[track].voicing, e->delta, KIT_NVOICINGS);
            if (e->id == KIT_ENC_SELECT) man_walk(&mans[track], &pads[track], e->delta > 0 ? 1 : -1, pads[track].tonic, pads[track].minor);
        }
        dirty = 1;
        break;
    }
}

/* ------------------------------------------------------------------------------------------------- screen --- */
/* D-037: four blocks, top to bottom: the song (track, page, key, tempo, octave, and the loop as played), the knobs (four
 * soft cards above the four knobs), the chord (big, with what it is and what comes next), and the keys (the pads with
 * their colours on PAD, a piano on MANUAL and the other tracks). One accent colour; the mode is the top line's colour. */
#define TL_Y 24                                 /* the loop's line */
#define TL_X0 8
#define TL_X1 232
static int tl_px = -1;                          /* where the playhead was drawn */

/* text centred on (x, cy) */
static int ctext(int x, int cy, const char *s, int font, kit_color_t c)
{
    return kit_text(x, cy - kit_font_height(font) / 2, s, font, KIT_ALIGN_CENTER, c);
}

static void key_name(char *d)       /* "C maj", "A min" */
{
    d = cat(d, kit_note_name_in_key(pads[track].tonic, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
    cat(d, pads[track].minor ? " min" : " maj");
}

/* the loop as played: the track's notes where they were played, the playhead while the song plays */
static void draw_timeline(void)
{
    int i, len = song.bars * KIT_TICKS_PER_BAR, w = TL_X1 - TL_X0;
    kit_rect(0, TL_Y - 4, KIT_W, 9, BG);
    kit_rect(TL_X0, TL_Y, w, 1, LINE);
    for (i = 0; i < song.count[track]; i++) {
        const kit_song_note_t *n = &song.notes[track][i];
        int a, b;
        if (n->start >= len) continue;
        a = TL_X0 + n->start * w / len;
        b = TL_X0 + (n->start + n->len > len ? len : n->start + n->len) * w / len;
        kit_rect(a, TL_Y - 1, b - a < 1 ? 1 : b - a, 3, ACC_M);
    }
    tl_px = -1;
    if (kit_seq_playing() && kit_seq_tick() >= 0) {
        tl_px = TL_X0 + kit_seq_tick() * w / len;
        kit_rect(tl_px - 1, TL_Y - 3, 2, 7, FG);
    }
}

static const char *top_title;                   /* instead of the track's name (the song page) */
static void top(const char *page_name)
{
    char s[40], *d;
    int w;
    kit_rect(0, 0, KIT_W, 2, MODE_COL);         /* the mode: the top line's colour */
    w = kit_text(8, 5, top_title ? top_title : track_name(track), KIT_FONT_XSMALL, KIT_ALIGN_LEFT, FG);
    if (page_name) kit_text(13 + w, 5, page_name, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    if (is_drums() && !top_title) cat(s, dr_kit_title(track));   /* the drums: the kit, not a key */
    else key_name(s);
    d = s + 0;
    while (*d) d++;
    d = cat_int(cat(d, "   "), song.bpm);
    if (practice) {
        cat_int(cat(d, " of "), practice_bpm);  /* practising slower: of the song's own tempo (no room for the octave) */
    } else if (is_drums() && !top_title) {
        ;                                       /* the drums: no octave (the keys are the sounds) */
    } else {
        d = cat(d, "   Oct ");
        if (octave > 0) d = cat(d, "+");
        cat_int(d, octave);
    }
    w = kit_text(232, 5, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, MUTED);
    if (armed) kit_circle(232 - w - 9, 12, 3, REC);
    if (steal_until) kit_circle(232 - w - (armed ? 19 : 9), 12, 3, WARN);   /* a voice was taken: the FM-1 is full */
    draw_timeline();
}

#include "explain.c"

static void draw_count(void)                   /* the count-in: the beats left, big and red over the knobs' row (last, over any page) */
{
    char c[4];
    if (!kit_seq_counting()) return;
    cat_int(c, kit_seq_counting());
    kit_round_rect(176, 30, 60, 34, 8, REC);
    ctext(206, 47, c, KIT_FONT_MEDIUM, FG);
}

static void card(int i, const char *label, const char *value, int hi)
{
    int x = 4 + i * 58;
    exp_card(i, label, value, 0);
    kit_round_rect(x, 32, 56, 30, 6, CARD);
    ctext(x + 28, 40, label, KIT_FONT_TINY, MUTED);
    ctext(x + 28, 53, value, KIT_FONT_XSMALL, hi ? ACC : FG);
}

static void chord_name(const kit_vchord_t *c, char *out) { kit_chord_label(c, pads[track].tonic, pads[track].minor, out, 20); }

/* the big chord with what it is (right) and a quiet line under it */
static void hero(const char *name, const char *what, const char *sub, kit_color_t sub_col)
{
    int w = kit_text(8, 80, name, KIT_FONT_LARGE, KIT_ALIGN_LEFT, FG);
    if (what && 16 + w + kit_text_width(what, KIT_FONT_SMALL) <= 232)
        kit_text(232, 86, what, KIT_FONT_SMALL, KIT_ALIGN_RIGHT, ACC);
    if (sub) kit_text(232, 107, sub, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, sub_col);
}

static void tick_mark(int x, int y, kit_color_t c)    /* a tick: the notes held are the chord */
{
    int i;
    for (i = 0; i < 2; i++) {
        kit_line(x, y + 5 + i, x + 4, y + 9 + i, c);
        kit_line(x + 4, y + 9 + i, x + 11, y + i, c);
    }
}

/* the pads: the black keys' colours above, the white keys' chords below (root, degree), NEXT and AGAIN */
static void draw_pads(int y)
{
    static const char *const LAB[KIT_NKEYS] = { 0, "s2", 0, "s4", 0, "7", 0, 0, "M7", 0, "9", 0, 0, "11", 0, "13", 0, "a9",
                                                0, 0, "6", 0, "/3", 0, 0, "AU", 0 };
    kit_chord_t tri[7];
    int k, wi = 0, step, nd = pad_next_degree(&pads[track], &step), h = kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k;
        if (!kit_is_white(n)) {
            int x = wi * 15 - 7, on = (BLACK_COL[k] >= 0 && (kit_key_held(k) || BLACK_COL[k] == pads[track].locked))
                                      || (k == KEY_SLASH && kit_key_held(k)) || (k == KEY_AUTO && pads[track].auto_on && pads[track].locked < 0);
            if (k == KEY_AUTO && pads[track].locked >= 0) on = 2;
            kit_round_rect(x, y, 14, 16, 3, on == 1 ? ACC : on == 2 ? ACC_M : CARD2);
            ctext(x + 7, y + 8, LAB[k], KIT_FONT_TINY, on ? BG : MUTED);
            continue;
        }
        {
            int x = wi * 15 + 1, yy = y + 20;
            if (PAD_DEG[k] >= 0 && h) {
                int d = PAD_DEG[k], now = d == pads[track].now_degree || d == rec_degree, nx = !now && d == nd;
                char num[2] = { (char)('1' + d), 0 };
                kit_round_rect(x, yy, 13, 52, 3, now ? ACC : nx ? ACC_D : CARD);
                ctext(x + 7, yy + 12, kit_note_name_in_key(tri[d].root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS),
                      KIT_FONT_TINY, now ? BG : FG);
                ctext(x + 7, yy + 30, num, KIT_FONT_TINY, now ? BG : nx ? ACC : MUTED);
                if (d == 0) kit_rect(x + 3, yy + 45, 7, 2, now ? BG : MUTED);   /* I: home */
            } else {
                kit_round_rect(x, yy, 13, 52, 3, CARD2);
                if (k == KEY_NEXT) {                                        /* a triangle */
                    int i;
                    for (i = 0; i < 6; i++) kit_rect(x + 4 + i, yy + 20 + i, 1, 12 - 2 * i, FG);
                } else {
                    ctext(x + 7, yy + 26, "re", KIT_FONT_TINY, MUTED);
                }
            }
        }
        wi++;
    }
}

/* the keys F3..G5 (moved by OCT), letters at the bottom: the chord built (accent), the chord recorded at this moment
 * (deep accent), held keys (light) */
static void piano(int y, int h, const kit_vchord_t *c, const uint8_t *rec, int nr)
{
    int k, wi = 0, low = kb_low();
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = low + k, on;
        kit_color_t col;
        if (!kit_is_white(n)) continue;
        on = c && has_key(c->notes, c->n, k);
        col = on ? ACC : has_key(rec, nr, k) ? ACC_M : kit_key_held(k) ? KIT_RGB(150, 190, 255) : KEY_W;
        kit_rect(wi * 15, y, 14, h, col);
        kit_text(wi * 15 + 7, y + h - kit_font_height(KIT_FONT_TINY) - 2,
                 kit_note_name_in_key(n, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), KIT_FONT_TINY, KIT_ALIGN_CENTER,
                 on ? BG : has_key(rec, nr, k) ? FG : KIT_RGB(96, 102, 118));
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = low + k, on, x, bh = h * 58 / 100;
        kit_color_t col;
        if (kit_is_white(n)) { wi++; continue; }
        x = wi * 15 - 5;
        on = c && has_key(c->notes, c->n, k);
        col = on ? ACC : has_key(rec, nr, k) ? ACC_M : kit_key_held(k) ? KIT_RGB(150, 190, 255) : KEY_B;
        kit_rect(x, y, 9, bh, col);
        if (on) {
            char l[2] = { kit_note_name_in_key(n, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)[0], 0 };
            kit_text(x + 5, y + bh - kit_font_height(KIT_FONT_TINY) - 1, l, KIT_FONT_TINY, KIT_ALIGN_CENTER, BG);
        }
    }
}

static void draw_pad(void)
{
    const pad_prog_t *pr = pad_prog(&pads[track]);
    char s[40], v[12], *d;
    kit_chord_t tri[7];
    int i, step, nd = pad_next_degree(&pads[track], &step);
    kit_vchord_t c;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    kit_clear(BG);
    top("AUTO");
    card(0, "STYLE", PAD_STYLES[pads[track].style].name, 0);
    if (pr) { d = cat_int(v, pads[track].prog + 1); d = cat(d, " of "); cat_int(d, PAD_STYLES[pads[track].style].n); }
    else cat(v, "-");
    card(1, "PROG", v, 0);
    card(2, "VOICE", VOICING_NAMES[pads[track].voicing], pads[track].voicing != KIT_VOICE_ROOT);
    card(3, "PLAY", FEEL_NAMES[pads[track].feel], pads[track].feel != PAD_BLOCK);
    d = cat(s, PAD_STYLES[pads[track].style].name);   /* the progression's name */
    if (pr) { d = cat(d, "  /  "); cat(d, pr->name); }
    kit_text(8, 66, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    pad_chord(&pads[track], nd, 0, pad_colour(&pads[track], step, -1), 0, &c);
    chord_name(&c, cat(v, ""));
    {
        char sub[32];
        int h = held_colour();
        d = sub;
        if (h >= 0) { d = cat(d, kit_colour_name(h)); cat(d, " held"); }
        else if (pads[track].locked >= 0) { d = cat(d, kit_colour_name(pads[track].locked)); cat(d, " locked"); }
        else { d = cat(d, "next  "); chord_name(&c, d); }
        if (pads[track].now_degree >= 0) {
            chord_name(&pads[track].now, s);
            hero(s, tri[pads[track].now_degree].roman, sub, MUTED);
        } else {
            kit_text(8, 86, "Play a pad", KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, MUTED);
            kit_text(232, 107, sub, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, MUTED);
        }
    }
    if (pr) {                           /* the progression: the chord now, the next one in a deeper blue */
        int w = 224 / pr->n;
        for (i = 0; i < pr->n; i++) {
            int x = 8 + i * w, now = i == pads[track].now_step, nx = !now && i == pads[track].pos % pr->n;
            pad_chord(&pads[track], pr->step[i].degree, 0, pad_colour(&pads[track], i, -1), 0, &c);
            chord_name(&c, s);
            kit_round_rect(x, 132, w - 4, 22, 5, now ? ACC : nx ? ACC_D : CARD);
            ctext(x + (w - 4) / 2, 143, s, KIT_FONT_XSMALL, now ? BG : nx ? FG : MUTED);
        }
    } else {
        kit_text(8, 137, "Free: the likely next chord breathes", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    }
    draw_pads(164);
}

static void draw_manual(void)
{
    char s[40], v[24], *d;
    kit_vchord_t c, nx;
    kit_chord_t tri[7];
    uint8_t held[KIT_NKEYS], rec[16];
    int k, w, nh = 0, nr = song_notes_at(&song, kit_seq_tick(), rec, 16), low = kb_low();
    int pc = man_root_pc(&mans[track], pads[track].tonic, pads[track].minor);
    man_chord(&mans[track], pads[track].tonic, pads[track].minor, low, &c);
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    kit_clear(BG);
    top("IDEAS");
    card(0, "ROOT", kit_note_name_in_key(pc, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), 0);
    card(1, "TYPE", MAN_TYPE_NAMES[mans[track].type], 0);
    card(2, "COLOR", mans[track].colour ? kit_colour_name(MAN_COLS[mans[track].colour]) : "-", 0);
    card(3, "VOICE", VOICING_NAMES[mans[track].voicing], 0);
    man_next_idea(&mans[track], &pads[track], pads[track].tonic, pads[track].minor, low, &nx);
    chord_name(&nx, cat(s, "SELECT >  "));
    kit_text(8, 66, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    if (man_arps[track]) kit_text(232, 66, "ARP", KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, ACC);
    chord_name(&c, v);
    s[0] = 0;
    if (nr) kit_identify_chord(rec, nr, pads[track].tonic, pads[track].minor, cat(s, "now  "), 24, 0);
    hero(v, man_outside(&mans[track]) ? "outside" : tri[mans[track].root_i].roman, nr ? s : (lights_offs[track] ? "lights off" : 0), nr ? ACC : MUTED);
    for (k = 0; k < KIT_NKEYS; k++) if (kit_key_held(k)) held[nh++] = (uint8_t)key_note(k);
    if (last_note >= 0 && last_channel) {       /* a note from a MIDI keyboard or a computer (until a key is played) */
        d = cat_int(cat(s, "MIDI channel "), last_channel);
        d = cat(cat(d, "  "), (last_mtrack >= 0 ? track_name(last_mtrack) : "-"));
        d = cat(d, "  ");
        cat_int(cat(d, kit_note_name_in_key(last_note, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)), kit_octave(last_note));
        kit_round_rect(4, 128, 232, 22, 6, ACC_D);
        ctext(120, 139, s, KIT_FONT_XSMALL, FG);
    } else if (nh) {                            /* what you hold, named; a tick when it is the chord */
        uint16_t a = 0, b = 0;
        int i;
        d = cat(s, "you  ");
        if (!kit_identify_chord(held, nh, pads[track].tonic, pads[track].minor, d, 24, 0))
            cat(d, kit_note_name_in_key(held[0], pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
        w = kit_text(8, 132, s, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FG);
        for (i = 0; i < nh; i++) a |= (uint16_t)(1u << (held[i] % 12));
        for (i = 0; i < c.n; i++) b |= (uint16_t)(1u << (c.notes[i] % 12));
        if (a == b) tick_mark(16 + w, 135, OK);
    }
    piano(158, 82, &c, rec, nr);
}

#include "chordplay.c"
#include "chideas.c"
#include "chauto.c"
#include "seqsong.c"
#include "chdraw.c"

/* CHORDS PLAY: what you hold, named big (Am, Am9), with its numeral in the key or "outside"; two notes "C + G"; a
 * chord with no name shows its notes, and under them the closest chord (the fewest notes missing); the piano lights
 * what you hold (D-042) */
static void draw_play(void) { ch_draw_play(); }   /* the lab's page (chdraw.c, D-084) */

static void draw_keyboard(void)
{
    char s[24];
    kit_clear(BG);
    top(0);
    if (last_note >= 0) {
        kit_text(8, 40, kit_note_name_in_key(last_note, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), KIT_FONT_HUGE,
                 KIT_ALIGN_LEFT, FG);
        cat_int(cat(s, "Oct "), kit_octave(last_note));
        kit_text(232, 46, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, MUTED);
        if (last_channel) {             /* a note from a MIDI keyboard or computer: which channel, so which track */
            char *d = cat_int(cat(s, "MIDI channel "), last_channel);
            cat(cat(d, "  "), (last_mtrack >= 0 ? track_name(last_mtrack) : "-"));
            kit_round_rect(4, 128, 232, 22, 6, ACC_D);
            ctext(120, 139, s, KIT_FONT_XSMALL, FG);
        }
    } else {
        kit_text(8, 80, "Play the keys", KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, MUTED);
    }
    if (last_note < 0 || !last_channel)
        kit_text(8, 136, "ALGORITHM: the track", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    piano(158, 82, 0, 0, 0);
}

static const char *const CLICK_NAMES[3] = { "Off", "Rec", "On" };
static const char *const IN_NAMES[3] = { "Channels", "This track", "Follow" };

static void row(int y, const char *label, const char *value, int hi)
{
    kit_text(8, y, label, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    kit_text(232, y, value, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, hi ? ACC : FG);
}

static void sub_tabs(void)                      /* SONG CLICK MIDI SYSTEM, the one on screen lit (SCL steps them) */
{
    static const char *const N[NSUBS] = { "SONG", "TRACKS", "CLICK", "MIDI", "SYSTEM", "USB" };
    int i;
    for (i = 0; i < NSUBS; i++) {
        kit_round_rect(4 + i * 39, 214, 37, 20, 5, i == song_sub ? ACC_D : CARD);
        ctext(22 + i * 39, 224, N[i], KIT_FONT_TINY, i == song_sub ? FG : MUTED);
    }
}

static void draw_sub(void)                      /* CLICK, MIDI, SYSTEM (D-045) */
{
    char s[40], v[16], *d;
    int i;
    kit_clear(BG);
    top_title = song_sub == SUB_TRACKS ? "TRACKS" : song_sub == SUB_CLICK ? "CLICK" : song_sub == SUB_MIDI ? "MIDI" : song_sub == SUB_USB ? "USB AUDIO" : "SYSTEM";
    top(0);
    top_title = 0;
    if (song_sub == SUB_TRACKS) {               /* each track's template (D-046) */
        for (i = 0; i < KIT_NTRACKS; i++) {
            const char *nm = TRACK_NAMES[pend_track == i ? pend_role : set.role[i]];
            cat_int(cat(v, "TRACK "), i + 1);
            card(i, v, nm, pend_track == i);
        }
        for (i = 0; i < KIT_NTRACKS; i++) {
            int y = 70 + i * 22, e2, p2;
            if (i == track) kit_round_rect(4, y - 3, 232, 20, 5, ACC_D);
            cat_int(s, i + 1);
            kit_text(12, y, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
            kit_text(28, y, track_name(i), KIT_FONT_XSMALL, KIT_ALIGN_LEFT, i == track ? FG : MUTED);
            sound_of(snd[i], &e2, &p2);
            d = cat_int(s, song.count[i]);
            d = cat(d, song.count[i] == 1 ? " note  " : " notes  ");
            cat(d, kit_preset_name(e2, p2));
            kit_text(228, y, s, KIT_FONT_TINY, KIT_ALIGN_RIGHT, MUTED);
        }
        if (pend_track >= 0) {
            d = cat(s, track_name(pend_track));
            d = cat(d, " has notes: SAVE makes it ");
            cat(d, TRACK_NAMES[pend_role]);
            kit_round_rect(4, 160, 232, 22, 6, ACC_D);
            ctext(120, 171, s, KIT_FONT_TINY, FG);
            kit_text(8, 188, "The notes stay. Any other button: no", KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
        } else {
            kit_text(8, 162, "K1-K4: each track's template", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
            kit_text(8, 180, "The help hears every track's notes", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        }
        {                                       /* the voices and the CPU they take (D-047) */
            int vu = kit_voices_used(), ld = kit_voices_load();
            d = cat_int(cat(s, "Voices "), vu);
            d = cat_int(cat(d, " of "), KIT_NVOICES);
            cat(cat_int(cat(d, "   CPU "), ld), " %");
            kit_text(8, 198, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, steal_until ? WARN : MUTED);
            voices_shown = vu * 1000 + ld;
        }
    } else if (song_sub == SUB_CLICK) {
        card(0, "CLICK", CLICK_NAMES[set.click], set.click != 0);
        cat_int(v, set.click_lvl);
        card(1, "LEVEL", v, 0);
        if (set.count_in) cat(cat_int(v, set.count_in), set.count_in == 1 ? " bar" : " bars"); else cat(v, "Off");
        card(2, "COUNT-IN", v, set.count_in != 0);
        kit_text(8, 80, set.click == 2 ? "Click on every beat" : set.click == 1 ? "Click while recording" : "No click",
                 KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, FG);
        kit_text(8, 112, set.count_in ? "REC, PLAY: a count-in, then it records" : "REC, PLAY: it records at once",
                 KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        kit_text(8, 130, "The click is never recorded", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    } else if (song_sub == SUB_MIDI) {
        card(0, "TRACK", track_name(set_track), 0);
        cat_int(v, set.ch[set_track]);
        card(1, "CHANNEL", v, 0);
        card(2, "IN", IN_NAMES[set.midi_in], set.midi_in != 0);
        card(3, "WAKE", set.wake ? "+MIDI" : "Panel", 0);
        for (i = 0; i < KIT_NTRACKS; i++) {     /* each track's channel, in and out */
            int y = 70 + i * 22;
            if (i == set_track) kit_round_rect(4, y - 3, 232, 20, 5, ACC_D);
            kit_text(12, y, track_name(i), KIT_FONT_XSMALL, KIT_ALIGN_LEFT, i == set_track ? FG : MUTED);
            d = cat(s, "channel ");
            cat_int(d, set.ch[i]);
            kit_text(228, y, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, i == set_track ? FG : MUTED);
        }
        kit_text(8, 162, set.midi_in == 1 ? "In: every channel plays the track on screen"
                         : set.midi_in == 2 ? "In: by channel, the screen follows" : "In: each channel plays its track",
                 KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        kit_text(8, 180, "Out (USB): on these channels, the clock", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    } else if (song_sub == SUB_USB) {           /* as the lab draws it (D-084): what each setting does, the cable */
        int c = kit_usb_audio_connected();
        card(0, "OUT", "mix", 0);
        card(1, "MONITOR", USB_MON[usb_set.mon], usb_set.mon != KIT_USB_MON_SAMPLING);
        d = v;
        if (usb_set.lvl > 0) d = cat(d, "+");
        cat(cat_int(d, usb_set.lvl), " dB");
        card(2, "IN LEVEL", v, usb_set.lvl != 0);
        row(76, "To USB", "what you hear", 0);
        row(98, "Input heard", usb_set.mon == 2 ? "always" : usb_set.mon == 1 ? "on TAKE and CHOP" : "never", 0);
        row(120, "Computer", c ? "connected" : "not connected", c);
        usb_shown = c;
    } else {
        int hz10 = 4400 + set.tune * 254 / 100;         /* A4, near enough: 440 Hz x 2^(cents / 1200) */
        if (set.scr_off) cat(cat_int(v, SCR_MIN[set.scr_off]), " min"); else cat(v, "Never");
        card(0, "SCREEN OFF", v, set.scr_off != 0);
        d = v;
        if (set.tune > 0) d = cat(d, "+");
        cat(cat_int(d, set.tune), " ct");
        card(1, "TUNE", v, set.tune != 0);
        card(2, "EXPLAIN", set.explain ? "On" : "Off", set.explain);
        d = cat(s, "A4 = ");
        d = cat_int(d, hz10 / 10);
        d = cat(d, ".");
        cat(cat_int(d, hz10 % 10), " Hz");
        row(76, "Tuning", s, set.tune != 0);
        row(98, "Screen", set.scr_off ? "dark when nothing is played" : "always on", 0);
        row(120, "Wakes it", set.wake ? "a key, a knob, a MIDI note" : "a key or a knob", 0);
    }
    sub_tabs();
}

/* GLO: the song page. The four knobs, the key big (AUTO's, or the locked one with what the notes suggest), and the
 * four tracks as they were played, with the playhead */
static void draw_song(void)
{
    char s[40], v[12], *d;
    int t, m, sure = kit_song_key(&song, &t, &m), i, len = song.bars * KIT_TICKS_PER_BAR;
    uint32_t ms;
    kit_clear(BG);
    top_title = "SONG";
    top(0);
    top_title = 0;
    d = v;
    if (song.key_auto) cat(v, "Auto");
    else { d = cat(d, kit_note_name_in_key(song.key_tonic, song.key_tonic, song.key_minor, KIT_NAMES_LETTERS)); cat(d, song.key_minor ? "m" : ""); }
    card(0, "KEY", v, !song.key_auto);
    cat_int(v, song.bpm);
    card(1, "TEMPO", v, 0);
    cat(cat_int(v, song.bars), song.bars == 1 ? " bar" : " bars");
    card(2, "LENGTH", v, 0);
    card(3, "TIME", "4/4", 0);
    d = cat(s, kit_note_name_in_key(t, t, m, KIT_NAMES_LETTERS));
    cat(d, m ? " minor" : " major");
    if (song.key_auto) {
        hero(s, "AUTO", sure ? "heard in the notes" : "not sure yet", MUTED);
    } else {
        static kit_song_t probe;                /* what the notes suggest, beside the locked key (static: 3 KB) */
        int pt, pm;
        probe = song;
        probe.key_auto = 1;
        if (kit_song_key(&probe, &pt, &pm) && (pt != t || pm != m)) {
            d = cat(cat(s + 20, "notes say "), kit_note_name_in_key(pt, pt, pm, KIT_NAMES_LETTERS));
            cat(d, pm ? "m" : "");
            hero(s, "locked", s + 20, ACC);
        } else {
            hero(s, "locked", 0, MUTED);
        }
    }
    ms = (uint32_t)song.bars * KIT_BEATS_PER_BAR * kit_beat_ms(song.bpm);
    d = cat_int(s, song.bars);
    d = cat(d, song.bars == 1 ? " bar at " : " bars at ");
    d = cat_int(d, song.bpm);
    d = cat(d, " BPM: ");
    d = cat_int(d, (int)(ms / 1000));
    d = cat(d, ".");
    d = cat_int(d, (int)(ms % 1000 / 100));
    cat(d, " s");
    kit_text(8, 130, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    for (i = 0; i < KIT_NTRACKS; i++) {         /* the four tracks, as played */
        int y = 150 + i * 15, k;
        kit_text(8, y, track_name(i), KIT_FONT_TINY, KIT_ALIGN_LEFT, i == track ? FG : MUTED);
        kit_rect(60, y + 5, 172, 1, LINE);
        for (k = 0; k < song.count[i]; k++) {
            const kit_song_note_t *n = &song.notes[i][k];
            int a, b;
            if (n->start >= len) continue;
            a = 60 + n->start * 172 / len;
            b = 60 + (n->start + n->len > len ? len : n->start + n->len) * 172 / len;
            kit_rect(a, y + 2, b - a < 2 ? 2 : b - a, 7, i == track ? ACC : ACC_M);
        }
        if (kit_seq_playing() && kit_seq_tick() >= 0) kit_rect(60 + kit_seq_tick() * 172 / len, y, 1, 11, FG);
    }
    sub_tabs();
}

/* HOME held: the three modes; SELECT chooses, HOME picks, HOME again confirms */
static void draw_picker(void)
{
    int i;
    kit_clear(BG);
    kit_rect(0, 0, KIT_W, 2, MODE_COLS[pick]);
    kit_text(8, 10, "Mode", KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, FG);
    kit_text(232, 18, "HOME", KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, MODE_COLS[pick]);
    for (i = 0; i < NMODES; i++) {
        int y = 46 + i * 52;
        kit_round_rect(8, y, 224, 46, 8, i == pick ? CARD2 : CARD);
        if (i == pick) kit_rect_outline(8, y, 224, 46, 2, MODE_COLS[i]);
        kit_circle(24, y + 23, 5, MODE_COLS[i]);
        kit_text(38, y + 6, MODE_NAMES[i], KIT_FONT_SMALL, KIT_ALIGN_LEFT, FG);
        kit_text(38, y + 26, MODE_DESC[i], KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        if (i == mode) kit_text(222, y + 8, "now", KIT_FONT_TINY, KIT_ALIGN_RIGHT, MODE_COLS[i]);
    }
    kit_text(8, 210, "SELECT choose     HOME pick", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
    if (confirm) {
        char s[32];
        kit_round_rect(16, 64, 208, 112, 10, CARD2);
        kit_rect_outline(16, 64, 208, 112, 2, MODE_COLS[pick]);
        cat(cat(cat(s, "Switch to "), MODE_NAMES[pick]), "?");
        ctext(120, 88, s, KIT_FONT_SMALL, FG);
        ctext(120, 114, "Your song is kept.", KIT_FONT_XSMALL, MUTED);
        ctext(120, 150, "HOME yes     other: no", KIT_FONT_XSMALL, MODE_COLS[pick]);
    }
}

static void draw_placeholder(void)              /* Guided and Game, for now */
{
    kit_clear(BG);
    kit_rect(0, 0, KIT_W, 2, MODE_COL);
    ctext(120, 96, MODE_NAMES[mode], KIT_FONT_LARGE, FG);
    ctext(120, 132, "coming soon", KIT_FONT_SMALL, MODE_COL);
    ctext(120, 214, "Hold HOME: modes", KIT_FONT_XSMALL, MUTED);
}

/* ------------------------------------------------------------------------------------------------ the bass --- */
static void card_off(int i, const char *label, const char *value);
static void free_note(void)                     /* why a greyed card does nothing (ST-44) */
{
    kit_round_rect(6, 92, 228, 50, 8, CARD2);
    kit_text(120, 100, "HELP is Free", KIT_FONT_SMALL, KIT_ALIGN_CENTER, FG);
    kit_text(120, 122, "so the scale does nothing now", KIT_FONT_XSMALL, KIT_ALIGN_CENTER, MUTED);
}
static const char *const BASS_PAGE[BASS_NPAGES] = { "PLAY", "IDEAS", "AUTO" };
static const char *const BASS_HELP[BASS_NHELPS] = { "Free", "Key", "Key+root", "Whites" };
static const char *const BASS_OUT[BASS_NOUTS] = { "play", "soft", "mute" };
static const char *const BASS_HEAR[BASS_NHEARS] = { "listen/play", "on", "off" };
static const char *const BASS_PLUS[BASS_NPLUS] = { "off", "octave", "fifth" };
static uint8_t bass_lv[KIT_NKEYS];               /* each key's light on the bass pages (also drawn on the screen) */

static int bass_key_of(int note)                 /* the key a bass note sits on (the nearest of its name) */
{
    int k = note + 12 - 12 * octave - KIT_KEY0_NOTE;
    while (k < 0) k += 12;
    while (k >= KIT_NKEYS) k -= 12;
    return k;
}

static int bass_white_at(int key)                /* Whites: the note a white key plays, -1 for a black one */
{
    int w = 0, k;
    if (!kit_is_white(KIT_KEY0_NOTE + key)) return -1;
    for (k = 0; k < key; k++) w += kit_is_white(KIT_KEY0_NOTE + k);
    return bass_white_note(&basses[track], pads[track].tonic, pads[track].minor, w, bass_low());
}

static void bass_levels(void)
{
    int k, third, root = bass_chord_now(&third);
    const kit_scale_t *sc = bass_scale(&basses[track], pads[track].minor);
    for (k = 0; k < KIT_NKEYS; k++) {
        int lv = KIT_LIGHT_OFF, n = KIT_KEY0_NOTE + k;
        if (basses[track].page == BASS_PLAY) {
            if (basses[track].help == HELP_WHITES) {
                int wn = bass_white_at(k);
                if (wn >= 0) lv = root >= 0 && wn % 12 == root ? KIT_LIGHT_ON : wn % 12 == pads[track].tonic ? KIT_LIGHT_BREATH : KIT_LIGHT_DIM;
            } else if (basses[track].help != HELP_FREE) {
                int r = bass_role(&basses[track], pads[track].tonic, pads[track].minor, n, root);
                lv = r == ROLE_ROOT ? KIT_LIGHT_ON : r == ROLE_TONIC ? KIT_LIGHT_BREATH : r == ROLE_SCALE ? KIT_LIGHT_DIM : KIT_LIGHT_OFF;
            }
            if (bass_key_note[k] >= 0) lv = KIT_LIGHT_ON;
        } else if (basses[track].page == BASS_IDEAS) {
            lv = kit_scale_has(sc, pads[track].tonic, n) ? KIT_LIGHT_DIM : KIT_LIGHT_OFF;
        } else {
            int r2 = bass_root_pad >= 0 ? bass_root_pad : basses[track].follow ? root : -1;
            kit_chord_t tri[7];
            if (PAD_DEG[k] >= 0 && kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri))
                lv = tri[PAD_DEG[k]].root == r2 ? KIT_LIGHT_ON : KIT_LIGHT_DIM;
        }
        bass_lv[k] = (uint8_t)lv;
    }
    if (basses[track].page == BASS_IDEAS) {
        if (bass_next_idea >= 0) bass_lv[bass_key_of(bass_next_idea)] = KIT_LIGHT_BREATH;
        if (bass_now_idea >= 0) bass_lv[bass_key_of(bass_now_idea)] = KIT_LIGHT_ON;
    }
}

static void bass_lights(void)
{
    int k;
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, bass_lv[k]);
}

static int key_x(int key)                        /* the screen piano's centre for a key */
{
    int wi = 0, k;
    for (k = 0; k < key; k++) wi += kit_is_white(KIT_KEY0_NOTE + k);
    return kit_is_white(KIT_KEY0_NOTE + key) ? wi * 15 + 7 : wi * 15 - 1;
}

static void bass_piano(int y, int h)              /* the keys as the lights show them, with their notes' letters */
{
    int k, wi = 0, bh = h * 58 / 100;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k, lv = bass_lv[k], nn;
        kit_color_t col;
        if (!kit_is_white(n)) continue;
        col = lv == KIT_LIGHT_ON ? ACC : lv == KIT_LIGHT_BREATH ? ACC_M : lv == KIT_LIGHT_DIM ? KIT_RGB(168, 186, 216) : KEY_W;
        kit_rect(wi * 15, y, 14, h, col);
        nn = basses[track].page == BASS_PLAY && basses[track].help == HELP_WHITES ? bass_white_at(k) : n;
        kit_text(wi * 15 + 7, y + h - kit_font_height(KIT_FONT_TINY) - 2, kit_note_name_in_key(nn, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS),
                 KIT_FONT_TINY, KIT_ALIGN_CENTER, lv == KIT_LIGHT_ON ? BG : KIT_RGB(80, 88, 106));
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k, lv = bass_lv[k];
        if (kit_is_white(n)) { wi++; continue; }
        kit_rect(wi * 15 - 5, y, 9, bh, lv == KIT_LIGHT_ON ? ACC : lv == KIT_LIGHT_BREATH ? ACC_M : lv == KIT_LIGHT_DIM ? KIT_RGB(60, 82, 128) : KEY_B);
    }
}

static void scale_label(char *out)               /* "A minor pentatonic" */
{
    char *d = cat(out, kit_note_name_in_key(pads[track].tonic, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
    d = cat(d, " ");
    cat(d, bass_scale(&basses[track], pads[track].minor)->name);
    for (d = out; *d; d++) if (d >= out + 2 && *d >= 'A' && *d <= 'Z' && d[-1] == ' ') *d = (char)(*d - 'A' + 'a');
}

static void draw_bass(void)
{
    char s[48], v[16], *d;
    int third, root = bass_chord_now(&third), tick = kit_seq_playing() ? kit_seq_tick() : -1;
    kit_clear(BG);
    top(BASS_PAGE[basses[track].page]);
    scale_label(s);
    if (basses[track].page == BASS_PLAY) {
        static const char *const SHORT_MIN[] = { "key", "penta", "blues", "dorian", "phryg", "harm", "melod", "hira", "locr" };
        static const char *const SHORT_MAJ[] = { "key", "penta", "blues", "mixo", "lydian", "lyd dom", "phr dom", "whole" };
        int i = basses[track].scale < bass_nscales(pads[track].minor) ? basses[track].scale : bass_nscales(pads[track].minor) - 1;
        static const char *const SHORT_ID[17] = { "major", "minor", "M penta", "m penta", "dorian", "harm", "phryg", "blues", "mixo",
                                                   "lydian", "melod", "hira", "locr", "M blues", "lyd dom", "phr dom", "whole" };
        int sid = (int)(bass_scale(&basses[track], pads[track].minor) - KIT_SCALES);
        card(0, "HELP", BASS_HELP[basses[track].help], basses[track].help != HELP_FREE);
        if (basses[track].help == HELP_FREE) {   /* HELP Free: the scale does nothing, so it is greyed (ST-44) */
            card_off(1, "SCALE", basses[track].scale_id >= 0 ? SHORT_ID[sid] : pads[track].minor ? SHORT_MIN[i] : SHORT_MAJ[i]);
            card_off(2, "OUTSIDE", BASS_OUT[basses[track].outside]);
        } else {
            card(1, "SCALE", basses[track].scale_id >= 0 ? SHORT_ID[sid] : pads[track].minor ? SHORT_MIN[i] : SHORT_MAJ[i], sid != (pads[track].minor ? 1 : 0));
            card(2, "OUTSIDE", BASS_OUT[basses[track].outside], 0);
        }
        card(3, "PLUS", BASS_PLUS[basses[track].plus], basses[track].plus != BPLUS_OFF);
        kit_text(8, 66, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        if (last_note >= 0 && !last_channel) {
            const kit_scale_t *sc = bass_scale(&basses[track], pads[track].minor);
            int deg = kit_scale_degree(sc, pads[track].tonic, last_note);
            d = cat(v, "sounds ");
            cat_int(cat(d, kit_note_name_in_key(last_note, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)), kit_octave(last_note));
            hero(kit_note_name_in_key(last_note, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), deg ? 0 : "outside", v, MUTED);
            if (deg) { char dg[4] = { (char)('0' + deg), 0 }; kit_text(232, 86, dg, KIT_FONT_SMALL, KIT_ALIGN_RIGHT, ACC); }
        } else {
            kit_text(8, 86, "Play the bass", KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, MUTED);
        }
        if (basses[track].help == HELP_ROOT && root >= 0) {
            d = cat(s, "chord now root ");
            cat(d, kit_note_name_in_key(root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
            kit_text(8, 134, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, FG);
        } else if (basses[track].help == HELP_WHITES) {
            kit_text(8, 134, "white keys: the scale   black keys rest", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, FG);
        }
        bass_piano(158, 82);
        if (kit_ms() < free_note_until) free_note();
        if (kit_ms() < bass_pop_until) {          /* K2 turned: the list, the chosen one in the middle, with its fit */
            uint8_t ids[17], held[17];
            int nu, n = bass_scale_list(&song, pads[track].tonic, pads[track].minor, ids, held, &nu), cur = (int)(bass_scale(&basses[track], pads[track].minor) - KIT_SCALES);
            int at = 0, r, j;
            for (j = 0; j < n; j++) if (ids[j] == cur) at = j;
            kit_round_rect(6, 64, 228, 108, 8, CARD2);
            kit_text(14, 68, nu ? "Scales for your chords, best first" : "Scales for the key", KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
            for (r = -2; r <= 2; r++) {
                int k2 = at + r, y = 84 + (r + 2) * 17;
                char row[40], *e;
                if (k2 < 0 || k2 >= n) continue;
                e = cat(row, kit_note_name_in_key(pads[track].tonic, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
                e = cat(e, " ");
                cat(e, KIT_SCALES[ids[k2]].name);
                for (e = row + 2; *e; e++) if (*e >= 'A' && *e <= 'Z') *e = (char)(*e - 'A' + 'a');
                if (r == 0) kit_round_rect(10, y - 1, 220, 17, 4, ACC_D);
                kit_text(16, y, row, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, r == 0 ? FG : MUTED);
                if (nu) {                         /* fits, or the chords' notes it lacks (ST-49) */
                    uint16_t lack = mel_scale_lacks(&song, ids[k2], pads[track].tonic);
                    char lk[24];
                    if (!lack) cat(lk, "fits");
                    else { int p, c2 = 0; e = cat(lk, "lacks"); for (p = 0; p < 12 && c2 < 2; p++) if (lack >> p & 1u) { e = cat(cat(e, " "), kit_note_name_in_key(p, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)); c2++; } }
                    kit_text(224, y, lk, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, lack ? MUTED : ACC);
                }
            }
        }
    } else if (basses[track].page == BASS_IDEAS) {
        const bass_style_t *st = &BASS_STYLES[basses[track].style];
        const bass_line_t *ln = bass_ideas_line(&basses[track]);
        const char *phase = tick < 0 ? "PLAY to start" : basses[track].hear == HEAR_LISTEN ? (bass_loops % 2 == 0 ? "LISTEN" : "YOUR TURN")
                          : basses[track].hear == HEAR_ON ? "PLAY ALONG" : "ON YOUR OWN";
        const int TOP = 84, HIT = 154, LA = 36, len = song.bars * KIT_TICKS_PER_BAR;
        int k, i, x;
        card(0, "STYLE", st->name, 0);
        d = cat_int(v, basses[track].line % st->n + 1);
        cat_int(cat(d, " of "), st->n);
        card(1, "LINE", v, 0);
        cat(cat_int(v, MEL_SPEEDS[basses[track].speed]), " %");
        card(2, "SPEED", v, basses[track].speed < MEL_NSPEEDS - 1);
        card(3, "HEAR", BASS_HEAR[basses[track].hear], basses[track].hear == HEAR_LISTEN);
        d = cat(cat(v, ""), "");
        kit_text(8, 66, ln->name, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        kit_text(232, 66, phase, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, ACC);
        for (x = 8; x < 232; x += 6) kit_rect(x, HIT, 3, 1, MUTED);
        if (tick >= 0)                            /* the line falls onto its keys, LA ticks ahead */
            for (k = 0; k < 2; k++)
                for (i = 0; i < ln->n; i++) {
                    int bar0 = tick / KIT_TICKS_PER_BAR + k, at = bar0 * KIT_TICKS_PER_BAR + ln->note[i].step * 6, dd = at - tick, nt;
                    int r = bass_chord_at(&song, at % len, pads[track].tonic, pads[track].minor, &third), nx, note, key, w, bottom, top2, sounding;
                    if (r < 0 || dd > LA || dd + ln->note[i].len * 6 <= 0) continue;
                    nx = bass_chord_at(&song, ((bar0 + 1) * KIT_TICKS_PER_BAR) % len, pads[track].tonic, pads[track].minor, &nt);
                    note = bass_line_note(&basses[track], pads[track].tonic, pads[track].minor, &ln->note[i], r, third, nx < 0 ? r : nx, bass_low());
                    key = bass_key_of(note);
                    w = kit_is_white(KIT_KEY0_NOTE + key) ? 12 : 8;
                    bottom = HIT - dd * (HIT - TOP) / LA;
                    top2 = bottom - ln->note[i].len * 6 * (HIT - TOP) / LA;
                    if (top2 < TOP) top2 = TOP;
                    if (bottom > HIT) bottom = HIT;
                    sounding = dd <= 0;
                    if (bottom - top2 >= 2) kit_round_rect(key_x(key) - w / 2, top2, w, bottom - top2, 2, sounding ? ACC : ACC_M);
                }
        bass_piano(158, 82);
    } else {
        const bass_style_t *st = &BASS_STYLES[basses[track].astyle];
        const bass_line_t *ln = bass_auto_line(&basses[track]);
        int r2 = bass_root_pad >= 0 ? bass_root_pad : basses[track].follow ? root : -1, i, s16 = tick >= 0 ? (tick / 6) % 16 : -1, k, wi = 0;
        kit_chord_t tri[7];
        card(0, "STYLE", st->name, 0);
        card(1, "PATTERN", ln->name, 0);
        card(2, "FEEL", basses[track].swing ? "swing" : "straight", basses[track].swing);
        card(3, "FOLLOW", basses[track].follow ? "chords" : "off", basses[track].follow);
        d = cat(v, "");
        cat(cat(cat(s + 0, ""), ""), "");
        {
            char m[48];
            d = cat(m, basses[track].follow ? "following the chords   in " : "hold a pad   in ");
            scale_label(d);
            kit_text(8, 66, m, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        }
        if (r2 >= 0) kit_text(8, 80, kit_note_name_in_key(r2, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), KIT_FONT_LARGE, KIT_ALIGN_LEFT, FG);
        else kit_text(8, 86, "Hold a pad", KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, MUTED);
        for (i = 0; i < 16; i++) {                /* the pattern over a bar of sixteenths */
            int on = 0, j;
            for (j = 0; j < ln->n; j++) if (ln->note[j].step == i) on = 1;
            kit_round_rect(8 + i * 14, 132, 12, 8, 2, on ? (i == s16 ? FG : ACC) : CARD);
        }
        kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
        for (k = 0; k < KIT_NKEYS; k++) {         /* the degree pads */
            int n = KIT_KEY0_NOTE + k, x = wi * 15 + 1;
            if (!kit_is_white(n)) continue;
            if (PAD_DEG[k] >= 0) {
                int dg = PAD_DEG[k], now = tri[dg].root == r2;
                char num[2] = { (char)('1' + dg), 0 };
                kit_round_rect(x, 158, 13, 80, 3, now ? ACC : CARD);
                ctext(x + 7, 180, kit_note_name_in_key(tri[dg].root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), KIT_FONT_TINY, now ? BG : FG);
                ctext(x + 7, 202, num, KIT_FONT_TINY, now ? BG : MUTED);
            } else {
                kit_round_rect(x, 158, 13, 80, 3, CARD2);
            }
            wi++;
        }
    }
}

/* ---- the melody's lights and screens (D-050) ---- */
static const char *const MEL_PAGE[MEL_NPAGES] = { "PLAY", "IDEAS", "AUTO" };
static const char *const MEL_HELP[MEL_NHELPS] = { "Free", "Key", "Chord", "Whites" };
static const char *const MEL_PLUS[MEL_NPLUS] = { "off", "3rd", "6th", "octave", "echo" };
static const char *const SCALE_SHORT[17] = { "major", "minor", "M penta", "m penta", "dorian", "harm", "phryg", "blues", "mixo",
                                             "lydian", "melod", "hira", "locr", "M blues", "lyd dom", "phr dom", "whole" };
static int mel_key_of(int note)                  /* the key a note sits on (the nearest of its name) */
{
    int k = note - mel_low();
    while (k < 0) k += 12;
    while (k >= KIT_NKEYS) k -= 12;
    return k;
}
static void mel_levels(void)                     /* into bass_lv: each key's light */
{
    mel_t *m = &mels[track];
    const kit_scale_t *sc = mel_scale(m, pads[track].minor);
    uint16_t smask = scale_mask(sc, pads[track].tonic), cm = (m->help == MHELP_CHORD || m->help == MHELP_WHITES) ? mel_chord_now() : 0;
    int k, third, r2 = mel_root_pad >= 0 ? mel_root_pad : m->follow && kit_seq_playing() ? bass_chord_at(&song, kit_seq_tick(), pads[track].tonic, pads[track].minor, &third) : -1;
    kit_chord_t tri[7];
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    for (k = 0; k < KIT_NKEYS; k++) {
        int lv = KIT_LIGHT_OFF, n = key_note(k), p = pc12(n);
        if (m->page == MEL_PLAY) {
            if (m->help == MHELP_WHITES) {
                int wn = mel_white_note(k);
                if (wn >= 0) lv = cm >> pc12(wn) & 1u ? KIT_LIGHT_ON : pc12(wn) == pads[track].tonic ? KIT_LIGHT_BREATH : KIT_LIGHT_DIM;
            } else if (m->help == MHELP_CHORD && cm) {   /* the chord now full, the rest of the scale dim (Sketchpad's guide) */
                int r = kit_melody_role(p, cm, smask);
                lv = r == KIT_MEL_STABLE ? KIT_LIGHT_ON : r == KIT_MEL_MOVES ? KIT_LIGHT_DIM : KIT_LIGHT_OFF;
            } else if (m->help != MHELP_FREE) {
                lv = smask >> p & 1u ? (p == pads[track].tonic ? KIT_LIGHT_BREATH : KIT_LIGHT_DIM) : KIT_LIGHT_OFF;
            }
            if (mel_key_note[k] >= 0) lv = KIT_LIGHT_ON;       /* Free: only what is played */
        } else if (m->page == MEL_IDEAS) {
            lv = smask >> p & 1u ? KIT_LIGHT_DIM : KIT_LIGHT_OFF;
        } else if (PAD_DEG[k] >= 0) {
            lv = tri[PAD_DEG[k]].root == r2 ? KIT_LIGHT_ON : KIT_LIGHT_DIM;
        }
        bass_lv[k] = (uint8_t)lv;
    }
    if (m->page == MEL_IDEAS) {
        if (mel_next_idea >= 0) bass_lv[mel_key_of(mel_next_idea)] = KIT_LIGHT_BREATH;
        if (mel_now_idea >= 0) bass_lv[mel_key_of(mel_now_idea)] = KIT_LIGHT_ON;
    }
}
static void mel_piano(int y, int h)
{
    int k, wi = 0, bh = h * 58 / 100;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k, lv = bass_lv[k], nn;
        if (!kit_is_white(n)) continue;
        kit_rect(wi * 15, y, 14, h, lv == KIT_LIGHT_ON ? ACC : lv == KIT_LIGHT_BREATH ? ACC_M : lv == KIT_LIGHT_DIM ? KIT_RGB(168, 186, 216) : KEY_W);
        nn = mels[track].page == MEL_PLAY && mels[track].help == MHELP_WHITES ? mel_white_note(k) : key_note(k);
        kit_text(wi * 15 + 7, y + h - kit_font_height(KIT_FONT_TINY) - 2, kit_note_name_in_key(nn, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS),
                 KIT_FONT_TINY, KIT_ALIGN_CENTER, lv == KIT_LIGHT_ON ? BG : KIT_RGB(80, 88, 106));
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        int lv = bass_lv[k];
        if (kit_is_white(KIT_KEY0_NOTE + k)) { wi++; continue; }
        kit_rect(wi * 15 - 5, y, 9, bh, lv == KIT_LIGHT_ON ? ACC : lv == KIT_LIGHT_BREATH ? ACC_M : lv == KIT_LIGHT_DIM ? KIT_RGB(60, 82, 128) : KEY_B);
    }
}
/* a card greyed: it does nothing in this setting (ST-44) */
static void card_off(int i, const char *label, const char *value)
{
    int x = 4 + i * 58;
    exp_card(i, label, value, 1);
    kit_round_rect(x, 32, 56, 30, 6, BG);
    kit_rect(x + 4, 32, 48, 1, LINE);
    kit_rect(x + 4, 61, 48, 1, LINE);
    ctext(x + 28, 40, label, KIT_FONT_TINY, LINE);
    ctext(x + 28, 53, value, KIT_FONT_XSMALL, KIT_RGB(60, 66, 82));
}
static void draw_melody(void)
{
    mel_t *m = &mels[track];
    char v[16], *d;
    int sid = (int)(mel_scale(m, pads[track].minor) - KIT_SCALES), tick = kit_seq_playing() ? kit_seq_tick() : -1;
    kit_clear(BG);
    top(MEL_PAGE[m->page]);
    if (m->page == MEL_PLAY) {
        uint8_t held[KIT_NKEYS];
        int nh = 0, k, i, x;
        uint16_t cm = m->help == MHELP_CHORD ? mel_chord_now() : 0, smask = scale_mask(mel_scale(m, pads[track].minor), pads[track].tonic);
        card(0, "HELP", MEL_HELP[m->help], m->help != MHELP_FREE);
        if (m->help == MHELP_FREE) { card_off(1, "SCALE", SCALE_SHORT[sid]); card_off(2, "OUTSIDE", m->outside ? "mute" : "play"); }
        else { card(1, "SCALE", SCALE_SHORT[sid], 1); card(2, "OUTSIDE", m->outside ? "mute" : "play", m->outside); }
        card(3, "PLUS", MEL_PLUS[m->plus], m->plus != MPLUS_OFF);
        for (k = 0; k < KIT_NKEYS; k++) if (mel_key_note[k] >= 0) held[nh++] = (uint8_t)mel_key_note[k];
        for (i = 1; i < nh; i++) { uint8_t t = held[i]; int j = i - 1; while (j >= 0 && held[j] > t) { held[j + 1] = held[j]; j--; } held[j + 1] = t; }
        if (!nh && last_note >= 0 && !last_channel) held[nh++] = (uint8_t)last_note;
        if (!nh) {
            kit_text(8, 80, "-", KIT_FONT_LARGE, KIT_ALIGN_LEFT, MUTED);
        } else {
            char nm[24];
            int named = nh >= 3 && kit_identify_chord(held, nh, pads[track].tonic, pads[track].minor, nm, sizeof nm, 0);
            if (named) kit_text(8, 78, nm, KIT_FONT_LARGE, KIT_ALIGN_LEFT, FG);
            for (i = 0, x = 8; i < nh && i < 6; i++) {   /* the notes, coloured by what they do on HELP Chord */
                int r = cm ? kit_melody_role(held[i], cm, smask) : -1;
                kit_color_t col = r == KIT_MEL_STABLE ? OK : r == KIT_MEL_MOVES ? ACC : r == KIT_MEL_PULLS ? WARN : FG;
                x += kit_text(x, named ? 120 : 80, kit_note_name_in_key(held[i], pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS),
                              named ? KIT_FONT_SMALL : nh > 1 ? KIT_FONT_MEDIUM : KIT_FONT_LARGE, KIT_ALIGN_LEFT, col) + 8;
            }
            k = -1;
            for (i = 0; i < KIT_NKEYS; i++) if (mel_key_plus[i] >= 0) k = mel_key_plus[i];
            if (k >= 0) { d = cat(v, "+"); cat(d, kit_note_name_in_key(k, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)); kit_text(232, 84, v, KIT_FONT_SMALL, KIT_ALIGN_RIGHT, ACC); }
        }
        mel_piano(158, 82);
        if (kit_ms() < free_note_until) free_note();
        if (kit_ms() < mel_pop_until) {          /* K2: the scales around the choice: fits, or what of the chords it lacks */
            uint8_t ids[KIT_NSCALES], hl[KIT_NSCALES];
            int nu, n = mel_scale_list(&song, pads[track].tonic, pads[track].minor, ids, hl, &nu), at = 0, r, j;
            for (j = 0; j < n; j++) if (ids[j] == sid) at = j;
            kit_round_rect(6, 64, 228, 108, 8, CARD2);
            kit_text(14, 68, nu ? "Scales for your chords, best first" : "Scales for the key", KIT_FONT_TINY, KIT_ALIGN_LEFT, MUTED);
            for (r = -2; r <= 2; r++) {
                int k2 = at + r, y = 84 + (r + 2) * 17;
                char row[40], *e;
                if (k2 < 0 || k2 >= n) continue;
                e = cat(row, kit_note_name_in_key(pads[track].tonic, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
                cat(cat(e, " "), KIT_SCALES[ids[k2]].name);
                for (e = row + 2; *e; e++) if (*e >= 'A' && *e <= 'Z') *e = (char)(*e - 'A' + 'a');
                if (r == 0) kit_round_rect(10, y - 1, 220, 17, 4, ACC_D);
                kit_text(16, y, row, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, r == 0 ? FG : MUTED);
                if (nu) {
                    uint16_t lack = mel_scale_lacks(&song, ids[k2], pads[track].tonic);
                    char lk[24];
                    if (!lack) cat(lk, "fits");
                    else { int p, c2 = 0; e = cat(lk, "lacks"); for (p = 0; p < 12 && c2 < 2; p++) if (lack >> p & 1u) { e = cat(cat(e, " "), kit_note_name_in_key(p, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)); c2++; } }
                    kit_text(224, y, lk, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, lack ? MUTED : ACC);
                }
            }
        }
    } else if (m->page == MEL_IDEAS) {
        const mel_fig_t *f = mel_idea(m);
        const char *phase = tick < 0 ? "PLAY to start" : m->hear == HEAR_LISTEN ? (bass_loops % 2 == 0 ? "LISTEN" : "YOUR TURN")
                          : m->hear == HEAR_ON ? "PLAY ALONG" : "ON YOUR OWN";
        const int TOP = 84, HIT = 154, LA = 36;
        int i, x, k, ff[MEL_NFIGS], nf = mel_style_figs(m->style, ff);
        card(0, "STYLE", MEL_STYLE_NAMES[m->style], 0);
        d = cat_int(v, m->fig % (nf ? nf : 1) + 1);
        cat_int(cat(d, "."), m->version);
        card(1, "IDEA", v, 0);
        cat(cat_int(v, MEL_SPEEDS[m->speed]), " %");
        card(2, "SPEED", v, m->speed < MEL_NSPEEDS - 1);
        card(3, "HEAR", BASS_HEAR[m->hear], m->hear == HEAR_LISTEN);
        kit_text(8, 66, f->name, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        kit_text(232, 66, phase, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, ACC);
        for (x = 8; x < 232; x += 6) kit_rect(x, HIT, 3, 1, MUTED);
        if (tick >= 0)                            /* the figure falls onto its keys, LA ticks ahead */
            for (k = 0; k < 2; k++) {
                kit_fig_note_t fn[16];
                int bar0 = tick / KIT_TICKS_PER_BAR + k, n = mel_bar_figure(bar0, fn);
                for (i = 0; i < n; i++) {
                    int at = bar0 * KIT_TICKS_PER_BAR + fn[i].pos * 6, dd = at - tick, key, w, bottom, top2;
                    if (dd > LA || dd + fn[i].dur * 6 <= 0) continue;
                    key = mel_key_of(fn[i].note);
                    w = kit_is_white(KIT_KEY0_NOTE + key) ? 12 : 8;
                    bottom = HIT - dd * (HIT - TOP) / LA;
                    top2 = bottom - fn[i].dur * 6 * (HIT - TOP) / LA;
                    if (top2 < TOP) top2 = TOP;
                    if (bottom > HIT) bottom = HIT;
                    if (bottom - top2 >= 2) kit_round_rect(key_x(key) - w / 2, top2, w, bottom - top2, 2, dd <= 0 ? ACC : ACC_M);
                }
            }
        mel_piano(158, 82);
    } else {
        const mel_phrase_t *ph = mel_phrase(m);
        int third, r2 = mel_root_pad >= 0 ? mel_root_pad : m->follow && tick >= 0 ? bass_chord_at(&song, tick, pads[track].tonic, pads[track].minor, &third) : -1;
        int i, s16 = tick >= 0 ? (tick / 6) % 16 : -1, b = tick >= 0 ? (bass_loops * song.bars + tick / KIT_TICKS_PER_BAR) % 2 : 0, k, wi = 0;
        const mel_pnote_t *pn = b ? ph->b : ph->a;
        int nn = b ? ph->nb : ph->na;
        kit_chord_t tri[7];
        card(0, "STYLE", MEL_PHRASES[m->astyle].name, 0);
        card(1, "PHRASE", ph->name, 0);
        card(2, "FEEL", m->swing ? "swing" : "straight", m->swing);
        card(3, "FOLLOW", m->follow ? "chords" : "off", m->follow);
        kit_text(8, 66, m->follow ? "following the chords" : "hold a pad", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        if (r2 >= 0) {
            kit_text(8, 80, kit_note_name_in_key(r2, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), KIT_FONT_LARGE, KIT_ALIGN_LEFT, FG);
            kit_text(232, 106, b ? "answer" : "call", KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, MUTED);
        } else kit_text(8, 86, "Hold a pad", KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, MUTED);
        for (i = 0; i < 16; i++) {                /* the phrase's bar of sixteenths */
            int on = 0, j;
            for (j = 0; j < nn; j++) if (pn[j].step == i) on = 1;
            kit_round_rect(8 + i * 14, 132, 12, 8, 2, on ? (i == s16 ? FG : ACC) : CARD);
        }
        kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
        for (k = 0; k < KIT_NKEYS; k++) {         /* the degree pads */
            int n = KIT_KEY0_NOTE + k, x = wi * 15 + 1;
            if (!kit_is_white(n)) continue;
            if (PAD_DEG[k] >= 0) {
                int dg = PAD_DEG[k], now = tri[dg].root == r2;
                char num[2] = { (char)('1' + dg), 0 };
                kit_round_rect(x, 158, 13, 80, 3, now ? ACC : CARD);
                ctext(x + 7, 180, kit_note_name_in_key(tri[dg].root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS), KIT_FONT_TINY, now ? BG : FG);
                ctext(x + 7, 202, num, KIT_FONT_TINY, now ? BG : MUTED);
            } else {
                kit_round_rect(x, 158, 13, 80, 3, CARD2);
            }
            wi++;
        }
    }
}

static const char *const HELP_MEL[MEL_NPAGES][7] = {
    { "The keys: the melody", "K1 HELP: free, key, the chord now,", "  white keys = the scale",
      "K2 SCALE: the pentatonic first", "K3 OUTSIDE: play, mute", "K4 PLUS: 3rd, 6th, octave, echo", "SCL: IDEAS, AUTO" },
    { "An idea falls onto its keys:", "  play each note as it lands", "K1 style   K2 idea   SELECT version",
      "K3 SPEED: the whole song slower", "K4 HEAR: listen/play, on, off", "PLAY to start   SCL: AUTO", 0 },
    { "Hold a pad: a phrase on its chord", "  a call, then an answer", "K1 style   K2 phrase",
      "K3 feel: straight or swing", "K4 FOLLOW: plays on the chords", "Notes stay in the scale", "SCL: PLAY" },
};

static const char *const HELP_BASS[BASS_NPAGES][7] = {
    { "The keys: the bass, an octave down", "K1 HELP: free, key, key + root,", "  white keys = the scale",
      "K2 SCALE: on the song's key", "K3 OUTSIDE: play, soft, mute", "K4 PLUS: octave, fifth   SCL: IDEAS", 0 },
    { "A style's line falls onto its keys:", "  play each note as it lands", "K1 style   K2 line (or SELECT)",
      "K3 SPEED: the whole song slower", "K4 HEAR: listen/play, on, off", "PLAY to start   SCL: AUTO", 0 },
    { "Hold a pad: a pattern on its root", "  (the song starts; let go: it stops)", "K1 style   K2 pattern",
      "K3 feel: straight or swing", "K4 FOLLOW: plays on the chords", "Notes stay in the scale", "SCL: PLAY" },
};

static const char *const HELP_PLAY[] = {
    "Play notes: the FM-1 names the chord", "  on the keys or MIDI channel 1", "Am, add a note: Am9",
    "K1 HELP: Free, Key, Shapes, Pads", "K2 TRANSPOSE: whites play the key", "Key: hold a note, its chord lights",
    "  K3 COLOUR   SELECT: another chord", "Shapes: full play, half next", "Pads: C = I, D = ii ... B = vii",
    "  black held: a colour; F5 NEXT", "  F#5 LOCK, G5 AGAIN   K4 FEEL", "SCL: IDEAS, AUTO",
};
static const char *const HELP_AUTO[] = {   /* ST-83 */
    "White keys: the song's parts", "  intro verse pre chorus bridge", "  break outro: low calm, high energy",
    "Another part: on the next bar", "Hold black: fill build half 8va stop", "K1 STYLE   K2 DENSITY", "K3 SWING   K4 FOLLOW",
    "REC records what it plays   SCL: PLAY",
};
static const char *const HELP_IDEAS[] = {   /* ST-82 */
    "An idea: a progression with a rhythm", "PLAY: it plays over the song", "Full: play now   Half: next",
    "SELECT: walk its chords", "K1 STYLE   K2 IDEA", "K3 SPEED: the song slower", "K4 HEAR: once, on, off", "SCL: AUTO",
};
static const char *const HELP_MANUAL[] = {
    "K1 root: the key's chords first", "K2 type   K3 colour   K4 voicing", "The keys and the piano show it",
    "SELECT: the ideas, from the pad", "  (or I-V-vi-IV)", "EDIT: lights off / on", "ARP: arpeggiate held notes",
    "REC, PLAY: record as played", "SCL: AUTO",
};
static const char *const HELP_PAD[] = {
    "White keys: the key's chords, twice", "F5 NEXT chord     G5 AGAIN", "Hold a black key: a colour",
    "  sus2 sus4 7 maj7 9 11 13 add9 6", "  D#5: the 3rd in the bass", "F#5 AUTO: style colours on / off",
    "  hold a colour + AUTO: lock it", "K1 style   K2 progression", "K3 voicing   K4 how it plays",
    "REC, PLAY: record as played   SCL: PLAY",
};
static const char *const HELP_SONG[] = {
    "K1 key: AUTO hears it, or lock one", "K2 tempo   K3 length (1-8 bars)", "K4 time: 4/4",
    "Nothing you played changes", "The tracks as you played them", "SCL: TRACKS, CLICK, MIDI, SYSTEM",
    "GLO: back to the track",
};
static const char *const HELP_KEYS[] = {
    "The keys play notes on the track", "ALGORITHM: the track", "SCL on CHORDS: play, ideas, auto",
    "PRESETS: the track's sound", "OCT- OCT+: an octave", "REC, PLAY: record as played",
};

static void draw_help(void)
{
    const char *const *lines = HELP_KEYS;
    const char *title = "Help";
    int i, n = (int)(sizeof HELP_KEYS / sizeof HELP_KEYS[0]);
    if (is_melody() && !song_page) {
        for (n = 0; n < 7 && HELP_MEL[mels[track].page][n]; n++) ;
        lines = HELP_MEL[mels[track].page]; title = "Melody";
    } else if (is_bass() && !song_page) {
        for (n = 0; n < 7 && HELP_BASS[basses[track].page][n]; n++) ;
        lines = HELP_BASS[basses[track].page]; title = "Bass";
    } else if (song_page) {
        lines = HELP_SONG; n = (int)(sizeof HELP_SONG / sizeof HELP_SONG[0]); title = "Song";
    } else if (is_pad()) {
        lines = HELP_PAD; n = (int)(sizeof HELP_PAD / sizeof HELP_PAD[0]); title = "Chord auto";
    } else if (is_auto()) {
        lines = HELP_AUTO; n = (int)(sizeof HELP_AUTO / sizeof HELP_AUTO[0]); title = "Chord auto";
    } else if (is_ideas()) {
        lines = HELP_IDEAS; n = (int)(sizeof HELP_IDEAS / sizeof HELP_IDEAS[0]); title = "Chord ideas";
    } else if (is_play()) {
        lines = HELP_PLAY; n = (int)(sizeof HELP_PLAY / sizeof HELP_PLAY[0]); title = "Chord play";
    }
    kit_clear(BG);
    kit_rect(0, 0, KIT_W, 2, MODE_COL);
    kit_text(8, 10, title, KIT_FONT_MEDIUM, KIT_ALIGN_LEFT, FG);
    kit_text(232, 18, "GLO", KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, ACC);
    kit_rect(8, 42, 224, 1, LINE);
    for (i = 0; i < n; i++)
        kit_text(8, 52 + i * 18, lines[i], KIT_FONT_XSMALL, KIT_ALIGN_LEFT, i % 1 ? MUTED : FG);
}

/* ------------------------------------------------------------------------------------------------- lights --- */
static void pad_lights(void)
{
    int k, step, nd = pad_next_degree(&pads[track], &step);
    for (k = 0; k < KIT_NKEYS; k++) {
        int level = KIT_LIGHT_OFF;
        if (PAD_DEG[k] >= 0) {
            int d = PAD_DEG[k];
            level = KIT_LIGHT_DIM;
            if (d == nd) level = KIT_LIGHT_BREATH;
            if (d == pads[track].now_degree || d == rec_degree) level = KIT_LIGHT_ON;
        } else if (k == KEY_NEXT || k == KEY_AGAIN) {
            level = KIT_LIGHT_DIM;
        } else if (k == KEY_AUTO) {
            level = pads[track].locked >= 0 ? KIT_LIGHT_BREATH : pads[track].auto_on ? KIT_LIGHT_ON : KIT_LIGHT_OFF;
        } else if (BLACK_COL[k] >= 0) {
            level = kit_key_held(k) || BLACK_COL[k] == pads[track].locked ? KIT_LIGHT_ON : KIT_LIGHT_OFF;
        } else if (k == KEY_SLASH) {
            level = kit_key_held(k) ? KIT_LIGHT_ON : KIT_LIGHT_OFF;
        }
        kit_key_light(k, level);
    }
}

/* MANUAL: the chord built full, the chord recorded at this moment breathing, the key's notes dim; EDIT: all off */
static void manual_lights(void)
{
    const kit_scale_t *sc = &KIT_SCALES[pads[track].minor ? 1 : 0];
    kit_vchord_t c;
    uint8_t rec[16];
    int k, nr = song_notes_at(&song, kit_seq_tick(), rec, 16);
    man_chord(&mans[track], pads[track].tonic, pads[track].minor, kb_low(), &c);
    for (k = 0; k < KIT_NKEYS; k++)
        kit_key_light(k, lights_offs[track] ? KIT_LIGHT_OFF : has_key(c.notes, c.n, k) ? KIT_LIGHT_ON
                         : has_key(rec, nr, k) ? KIT_LIGHT_BREATH
                         : kit_scale_has(sc, pads[track].tonic, key_note(k)) ? KIT_LIGHT_DIM : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_ARP, man_arps[track] ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_EDIT, lights_offs[track] ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
}

static void keyboard_lights(void)
{
    const kit_scale_t *s = &KIT_SCALES[pads[track].minor ? 1 : 0];
    int k;
    for (k = 0; k < KIT_NKEYS; k++)
        kit_key_light(k, kit_key_held(k) ? KIT_LIGHT_ON
                         : set.role[track] != ROLE_DRUMS && kit_scale_has(s, pads[track].tonic, key_note(k)) ? KIT_LIGHT_DIM : KIT_LIGHT_OFF);
}

#include "drumpage.c"
#include "reclayer.c"
#include "unicorn.c"
#include "damatxo.c"
#include "aldapa.c"
#include "train.c"
#include "games.c"
#include "guided.c"
#include "arp.c"
#include "songsave.c"
#include "sbrowse.c"
#include "soundfx.c"
#include "soundedit.c"

void app_frame(uint32_t now_ms)
{
    harm_sync();
    if (!picker && !home_long && kit_btn_held(KIT_BTN_HOME) && now_ms - home_at >= HOLD_MS) {
        home_long = 1;                          /* HOME held: the mode picker */
        picker = 1;
        pick = mode;
        confirm = 0;
        help = 0;
        dirty = 1;
    }
    if (!picker && mode == MODE_PRODUCER && !glo_long && kit_btn_held(KIT_BTN_GLO) && now_ms - glo_at >= HOLD_MS) {
        glo_long = 1;                           /* GLO held: help */
        help = 1;
        dirty = 1;
    }
    if (game_on() && (mode != MODE_GAME || picker)) game_leave();   /* the game closes: the tracks' sounds back */
    sound_fx_frame(now_ms);
    sound_edit_frame(now_ms);
    if (mode == MODE_GAME && !picker && !help) { game_frame(now_ms); return; }   /* the game draws every frame */
    if (G.on && (mode != MODE_GUIDED || picker)) g_leave();   /* the guided mode closes: the tracks' sounds and levels back */
    if (mode == MODE_GUIDED && !picker && !help) { guided_frame(now_ms); return; }   /* the guided mode draws every frame */
    if (mode == MODE_PRODUCER && !G.on && !game_on()) song_keep(now_ms);   /* the song kept over power-off (ST-89) */
    if (picker || mode != MODE_PRODUCER) {
        if (!dirty) return;
        dirty = 0;
        if (picker) draw_picker();
        else if (help) draw_help();
        else draw_placeholder();
        kit_lights_off();
        return;
    }
    int pd = is_pad();
    int rd = kit_seq_playing() && pd ? pad_recorded_degree(&song, kit_seq_tick(), pads[track].tonic, pads[track].minor) : -1;
    {                                           /* the click: always, or while recording (D-045) */
        static int met = -1, counting = -1;
        int want = set.click == 2 || (set.click == 1 && armed) ? set.click_lvl : 0;
        if (want != met) { kit_seq_metronome(want); met = want; }
        if (kit_seq_counting() != counting) { counting = kit_seq_counting(); dirty = 1; }
    }
    if (eat && !panel_held()) eat = 0;
    if (kit_voices_stolen() != steal_seen) {     /* a voice was taken: the dot for two seconds (D-047) */
        steal_seen = kit_voices_stolen();
        steal_until = now_ms + 2000;
        if (!steal_until) steal_until = 1;
        dirty = 1;
    } else if (steal_until && (int32_t)(now_ms - steal_until) >= 0) {
        steal_until = 0;
        dirty = 1;
    }
    if (song_page && song_sub == SUB_TRACKS && kit_voices_used() * 1000 + kit_voices_load() != voices_shown) dirty = 1;
    if (song_page && song_sub == SUB_USB && kit_usb_audio_connected() != usb_shown) dirty = 1;   /* the cable (ST-98) */          /* everything let go: the panel plays again */
    if (!asleep && set.scr_off && !kit_seq_counting() && now_ms - last_input >= (uint32_t)SCR_MIN[set.scr_off] * 60000u) {
        asleep = 1;                             /* nothing played for a while: the screen goes dark, the lights stay */
        kit_clear(KIT_BLACK);
        dirty = 1;
    }
    if (kit_seq_following()) {                  /* the clock in's tempo, on the top line */
        int b = kit_seq_clock_bpm();
        if (b && kit_tempo_clamp(b) != song.bpm) { kit_song_set_tempo(&song, b); dirty = 1; }
    }
    sb_frame(now_ms);
    ar_frame(now_ms);
    rl_frame(now_ms);
    dr_frame(now_ms);
    dr_poll();
    exp_frame(now_ms);
    chord_poll(now_ms);
    man_arp_poll(now_ms);
    ci_poll(now_ms);                            /* the chord ideas (ST-82) */
    sq_poll(now_ms);                            /* the song's chain (ST-85) */
    au_poll(now_ms);                            /* the song parts (ST-83) */
    bass_poll();
    ph_feel_poll(now_ms);                       /* FEEL's later notes (ST-81) */
    if (bass_pop_until && now_ms >= bass_pop_until) { bass_pop_until = 0; dirty = 1; }
    if (ph_menu_until && now_ms >= ph_menu_until) { ph_menu_until = 0; dirty = 1; }   /* the progression menu closes */
    if (is_bass()) bass_ideas_around(kit_seq_playing() ? kit_seq_tick() : -1);
    if (is_melody()) {                          /* the melody: its idea's notes for the lights, the echo, the pop-up */
        int was = mel_now_idea * 131 + mel_next_idea;
        mel_ideas_around(kit_seq_playing() ? kit_seq_tick() : -1);
        if (mel_now_idea * 131 + mel_next_idea != was) dirty = 1;
        if (mel_pop_until && now_ms >= mel_pop_until) { mel_pop_until = 0; dirty = 1; }
        if (free_note_until && now_ms >= free_note_until) { free_note_until = 0; dirty = 1; }
    }
    mel_echo_poll(now_ms);
    {                                           /* IDEAS' SPEED: the whole song slower while practising (ST-42) */
        int pct = is_melody() && mels[track].page == MEL_IDEAS && !song_page ? MEL_SPEEDS[mels[track].speed]
                : is_drums() && drt[track].page == DR_IDEAS && !song_page ? MEL_SPEEDS[drt[track].speed]
                : is_bass() && basses[track].page == BASS_IDEAS && !song_page ? MEL_SPEEDS[basses[track].speed]
                : is_ideas() && !song_page ? MEL_SPEEDS[ci_speed[track]] : 100;
        if (pct < 100 && !practice) { practice = 1; practice_bpm = song.bpm; }
        if (practice) {
            int want = pct < 100 ? practice_bpm * pct / 100 : practice_bpm;
            if (song.bpm != want) { kit_song_set_tempo(&song, want); dirty = 1; }
            if (pct == 100) practice = 0;
        }
    }
    if (rd != rec_degree) { rec_degree = rd; dirty = 1; }
    if (is_manual()) {                          /* the recorded notes sounding now: redraw when they change */
        uint8_t rec[16];
        uint32_t sig = 0;
        int i, nr = song_notes_at(&song, kit_seq_tick(), rec, 16);
        for (i = 0; i < nr; i++) sig = sig * 131u + rec[i] + 1u;
        if (sig != rec_sig) { rec_sig = sig; dirty = 1; }
    }
    if (song_page && kit_seq_playing()) {        /* the song page's playhead: redraw when it moves */
        int px = TL_X0 + kit_seq_tick() * (TL_X1 - TL_X0) / (song.bars * KIT_TICKS_PER_BAR);
        if (px != tl_px) { tl_px = px; dirty = 1; }
    }
    if (sq_view >= 0 && mode == MODE_PRODUCER && kit_seq_playing()) {   /* the sequencer's pages draw their own playhead (D-084) */
        int px = 6 + kit_seq_tick() * 228 / (song.bars * KIT_TICKS_PER_BAR);
        if (px != tl_px) { tl_px = px; dirty = 1; }
    }
    if (!dirty && !help && !song_page && !asleep && !sound_fx_open() && !sound_edit_open() && kit_seq_playing() && !(sq_view >= 0 && mode == MODE_PRODUCER)
        && !(mode == MODE_PRODUCER && !pd && (is_play() || is_ideas() || is_auto()))) {   /* only the playhead moved: redraw the loop's line (the chords' pages have none, as the lab, D-084) */
        int px = TL_X0 + kit_seq_tick() * (TL_X1 - TL_X0) / (song.bars * KIT_TICKS_PER_BAR);
        if (px != tl_px) draw_timeline();
    }
    if (!dirty)
        return;
    dirty = 0;
    if (is_bass()) bass_levels();
    if (is_melody()) mel_levels();
    if (asleep) ;                               /* dark: nothing drawn, the lights go on */
    else if (help) draw_help();
    else if (song_page && song_sub != SUB_SONG) draw_sub();
    else if (song_page) draw_song();
    else if (sq_view == SQV_SONG && mode == MODE_PRODUCER) sq_draw();
    else if (sq_view == SQV_SEQ && mode == MODE_PRODUCER) sq_seq_draw();
    else if (sq_view == SQV_STEPS && mode == MODE_PRODUCER) st_draw();
    else if (sound_fx_open()) sound_fx_draw();
    else if (sound_edit_open()) sound_edit_draw();
    else if (ar_page()) ar_draw();              /* ARP on: its page (ST-77) */
    else if (pd) draw_pad();
    else if (is_ideas()) ci_draw();
    else if (is_auto()) au_draw();
    else if (is_play_pads()) ph_draw_pads_page();
    else if (is_play()) draw_play();
    else if (is_bass()) draw_bass();
    else if (is_melody()) draw_melody();
    else if (is_drums() && dr_ed >= 0) draw_drums_edit();
    else if (is_drums() && dr_fxp >= 0) draw_drums_fx();
    else if (is_drums()) { if (drt[track].page == DR_IDEAS) draw_drums_ideas(); else if (drt[track].page == DR_AUTO) draw_drums_auto(); else draw_drums(); }
    else draw_keyboard();
    if (!asleep && !help && !song_page && !ar_page() && !sound_fx_open() && !sound_edit_open()) draw_rec_layer();
    if (!asleep && !help) draw_count();
    if (!asleep && !help && !sound_fx_open() && !sound_edit_open()) exp_draw(now_ms);
    if (sb_open && !asleep && !help && !song_page) sb_draw();   /* the Sound Browser over the page (ST-68) */
    if (sq_view == SQV_SONG && !help && !song_page) sq_lights();
    else if (sq_view == SQV_STEPS && !help && !song_page) st_lights();
    else if (ar_page()) ar_lights();
    else if (pd) { pad_lights(); kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF); kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF); }
    else if (is_ideas() && !help && !song_page) ci_lights();
    else if (is_auto() && !song_page) au_lights();   /* under GLO's help too, as the pads were */
    else if (is_play() && !help && !song_page) { play_lights(); kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF); kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF); }
    else if (is_melody() && !help && !song_page) { bass_lights(); kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF); kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF); }
    else if (is_bass() && !help && !song_page) { bass_lights(); kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF); kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF); }
    else if (is_drums() && !help && !song_page) { drums_lights(); kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF); kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF); }
    else { keyboard_lights(); kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF); kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF); }
    kit_btn_light(KIT_BTN_SCL, pd || is_ideas() || is_auto() || (is_bass() && basses[track].page != BASS_PLAY) || (is_melody() && mels[track].page != MEL_PLAY) || (is_drums() && drt[track].page != DR_PLAY) ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_SEQ, sq_view >= 0 ? KIT_LIGHT_ON : KIT_LIGHT_OFF);   /* in the sequencer's pages */
    kit_btn_light(KIT_BTN_FX, sound_fx_open() ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    if (sound_edit_open()) kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_ON);
    kit_btn_light(KIT_BTN_PLAY, kit_seq_playing() ? KIT_LIGHT_ON : kit_seq_counting() ? KIT_LIGHT_BREATH : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_REC, armed ? (kit_seq_playing() ? KIT_LIGHT_ON : KIT_LIGHT_BREATH) : KIT_LIGHT_OFF);
}
