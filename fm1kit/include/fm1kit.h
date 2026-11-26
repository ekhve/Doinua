/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit: the fixed platform API for FM-1 apps.
 *
 * An app is three functions (app_init, app_event, app_frame) and only ever talks to the
 * hardware through this header. The same API has two implementations:
 *   - device (fm1kit/src/device.c): the real FM-1, on felucca-bsp
 *   - host   (fm1kit/src/host.c):   a PC build that renders to PNG/WAV for fast tests
 * Anything not in this header is not part of the platform. Changing it is a versioned decision:
 * bump FM1KIT_API and record why in docs/PLATFORM.md.
 *
 * Conventions: coordinates in pixels, (0,0) top left; notes are MIDI numbers (60 = C4);
 * piano keys are numbered 0 (the leftmost, F3 with no octave shift) to 26 (G5). */
#ifndef FM1KIT_H
#define FM1KIT_H
#include <stdint.h>

#define FM1KIT_API 45

/* ------------------------------------------------------------------ app entry points --- */
/* The app defines these three. The kit calls app_init once after the hardware is up, then
 * loops: input events (app_event, one per change), then app_frame, then flushes the screen. */
typedef struct kit_event kit_event_t;
void app_init(void);
void app_event(const kit_event_t *e);
void app_frame(uint32_t now_ms);

/* ------------------------------------------------------------------------- input --- */
#define KIT_NKEYS 27            /* piano keys: 0 = F3 .. 26 = G5 (before octave shift) */
#define KIT_KEY0_NOTE 53        /* MIDI note of key 0 */
#define KIT_NENCS 7

enum {                          /* the 14 buttons */
    KIT_BTN_OCT_DOWN = 0, KIT_BTN_OCT_UP, KIT_BTN_FX, KIT_BTN_SCL, KIT_BTN_ENV, KIT_BTN_LFO,
    KIT_BTN_EDIT, KIT_BTN_GLO, KIT_BTN_HOME, KIT_BTN_SAVE, KIT_BTN_ARP, KIT_BTN_SEQ,
    KIT_BTN_PLAY, KIT_BTN_REC, KIT_NBTNS
};
enum {                          /* the 7 encoders */
    KIT_ENC_SELECT = 0, KIT_ENC_ALGORITHM, KIT_ENC_KNOB1, KIT_ENC_KNOB2, KIT_ENC_KNOB3,
    KIT_ENC_KNOB4, KIT_ENC_PRESETS
};
enum {
    KIT_EV_KEY_DOWN = 1,        /* id = piano key 0..26, vel = 1..127 */
    KIT_EV_KEY_UP,              /* id = piano key */
    KIT_EV_BTN_DOWN,            /* id = KIT_BTN_* */
    KIT_EV_BTN_UP,
    KIT_EV_ENC,                 /* id = KIT_ENC_*, delta = clicks (+ = clockwise) */
    KIT_EV_MIDI_NOTE_ON,        /* MIDI in (USB or the TRS jack): id = note, vel = velocity 1..127, delta = channel 1..16 */
    KIT_EV_MIDI_NOTE_OFF,       /* id = note, vel = release velocity, delta = channel (note on at 0 arrives as this) */
    KIT_EV_MIDI_CC,             /* id = controller, vel = value 0..127, delta = channel */
    KIT_EV_PLAY_NOTE,           /* the player started a note: id = note, vel = velocity, delta = its index (0..) */
    KIT_EV_PLAY_END,            /* the player finished a sequence that does not loop */
    KIT_EV_SEQ_BAR,             /* the song player reached a bar: id = bar (0..), delta = 1 at the loop's start */
    KIT_EV_MIDI_CLOCK,          /* API 17: MIDI Start / Continue / Stop in: id = 0xFA, 0xFB, 0xFC (clock bytes F8 go to the
                                 * song player, kit_seq_follow) */
    KIT_EV_MIDI_BEND,           /* API 17: pitch bend in: id = its low 7 bits, vel = its high 7 bits (64 0: the middle),
                                 * delta = channel */
};
struct kit_event {
    uint8_t type, id, vel;
    int8_t delta;
};
int kit_key_held(int key);      /* 1 while piano key is down */
int kit_btn_held(int btn);
int kit_pot(void);              /* MASTER pot, 0..1023 */

/* ------------------------------------------------------------------------ screen --- */
#define KIT_W 240
#define KIT_H 240
typedef uint16_t kit_color_t;   /* RGB565 */
#define KIT_RGB(r, g, b) ((kit_color_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define KIT_BLACK KIT_RGB(0, 0, 0)
#define KIT_WHITE KIT_RGB(255, 255, 255)

enum { KIT_FONT_SMALL = 0, KIT_FONT_MEDIUM, KIT_FONT_LARGE, KIT_FONT_HUGE, KIT_FONT_TINY, KIT_FONT_XSMALL,
       KIT_NFONTS };   /* ~16, 24, 40, 96 px; TINY ~10 and XSMALL ~13 (API 14, D-037) */
enum { KIT_ALIGN_LEFT = 0, KIT_ALIGN_CENTER, KIT_ALIGN_RIGHT };

void kit_clear(kit_color_t c);
void kit_pixel(int x, int y, kit_color_t c);
void kit_rect(int x, int y, int w, int h, kit_color_t c);              /* filled */
void kit_rect_outline(int x, int y, int w, int h, int thick, kit_color_t c);
void kit_round_rect(int x, int y, int w, int h, int r, kit_color_t c); /* filled, rounded corners */
void kit_circle(int cx, int cy, int r, kit_color_t c);                 /* filled */
void kit_line(int x0, int y0, int x1, int y1, kit_color_t c);

/* images (API 7, D-028): an app's PNGs in apps/<app>/assets/ become IMG_<name> in "app_assets.h" at build time.
 * Palette-indexed and compressed in flash; `clear` is the palette index left transparent (-1: none).
 * API 33 (D-089): `lz` 1 = the indices LZ-coded (KIT_IMAGE_LZ: about half the run-length size), 0 = run-length pairs. */
typedef struct {
    uint16_t w, h;
    const kit_color_t *pal;
    const uint8_t *rle;         /* lz 0: (count 1..255, palette index) pairs, row after row; lz 1: KIT_IMAGE_LZ */
    int16_t clear;
    uint8_t lz;
} kit_image_t;
/* KIT_IMAGE_LZ: bytes b < 0x80: b + 1 indices follow as they are; b >= 0x80: (b & 0x7F) + 3 indices repeat those
 * `off` back (two bytes, little-endian, 1..4096), row after row. tools/gen_images.py writes it. */
void kit_image(int x, int y, const kit_image_t *im, int scale);        /* top-left at x, y; each pixel scale x scale (1..8) */
/* Text: ASCII 32..126. Returns the advance width. x is the left edge, centre or right edge
 * depending on align; y is the top of the line. */
int kit_text(int x, int y, const char *s, int font, int align, kit_color_t fg);
int kit_text_width(const char *s, int font);
int kit_font_height(int font);

/* ------------------------------------------------------------------------ lights --- */
enum { KIT_LIGHT_OFF = 0, KIT_LIGHT_DIM, KIT_LIGHT_ON, KIT_LIGHT_BREATH,
       KIT_LIGHT_HALF };   /* API 28 (D-075): a steady level between DIM and ON (felucca-bsp fm1_led_mid, ~15 %) */
void kit_key_light(int key, int level);   /* the LED of piano key 0..26 */
void kit_btn_light(int btn, int level);   /* the LED of a button */
void kit_lights_off(void);

/* ------------------------------------------------------------------------- sound --- */
/* A sound is an engine and one of its presets. Engine 0, "Basic", is the kit's own: clean tones for
 * learning (its presets are the KIT_INST_* below). Engines 1.. are Felucca-Engines (MIT): Analog, Phase,
 * Lo-fi, Trio, Organ wheel, Noise, each with its own presets. The last, "Drums", has four kits (its presets, API 23,
 * D-053): Dusty, 808, Live, Latin, of 27 sounds each, one a key from F3 (KIT_DRUM_LO), named as sound files are
 * (kick1, snare2, conga1). Notes outside the keys: General MIDI's drums (36 kick, 38 snare, 42 hat ...), else by
 * pitch class. A drums track's pads can each play any kit's sound, tuned and with its own decay (My kit).
 *
 * Tracks (API 8, D-029): four tracks sound at once, each with its own sound. Together they share up to KIT_NVOICES
 * voices (API 20, D-047), within a CPU budget: each voice counts its engine's measured cost, and a note starts only
 * while the estimate stays under KIT_CPU_LIMIT %. When there is no room, a new note takes (1) a releasing voice,
 * (2) the oldest voice of another track that has two or more, the song's tracks before the ones played live, (3) the
 * oldest of its own track; a track never loses its last note (else the new note is dropped). Every voice taken or
 * note dropped counts in kit_voices_stolen. The functions without a track (kit_sound, kit_note_on, ...) are track 0's. */
enum {
    KIT_INST_SINE = 0,          /* pure tone: ear training, tuning */
    KIT_INST_PIANO,             /* bright attack, decays */
    KIT_INST_ORGAN,             /* steady while held */
    KIT_INST_MUSICBOX,          /* short, bell-like */
    KIT_NINSTS
};
#define KIT_ENGINE_BASIC 0
#define KIT_ENGINE_DRUMS 7
#define KIT_ENGINE_FMINI 8              /* API 32 (D-086): fmini-1's Minimoog engine, the owner's 66 picks */
#define KIT_ENGINE_OBERVAVE 11          /* API 38: OberVave (ST-72) */
#define KIT_ENGINE_CF_VA 12             /* API 40: CoralFlower VA (ST-73) */
#define KIT_ENGINE_BS2 10               /* API 37: Bass Something 2 (ST-92) */
#define KIT_ENGINE_JUNO 9               /* API 34 (D-090): FM-106's Juno-106 engine, the owner's 28 picks */
#define KIT_DRUM_KITS 4
#define KIT_DRUM_PADS 27
#define KIT_DRUM_LO 53                          /* F3: the first pad */
/* a track's FX (API 23, D-055): its Basic and drum voices through the drum lab's Punch (a compressor), MPC (a hard
 * compressor and a soft clip), Lofi (fewer bits, a band, a soft compressor, vinyl crackle), Tape (saturation, a
 * gentle low-pass and compressor). Felucca-Engines parts are not put through it. */
enum { KIT_FX_OFF = 0, KIT_FX_PUNCH, KIT_FX_MPC, KIT_FX_LOFI, KIT_FX_TAPE, KIT_NFX };
void kit_track_fx(int track, int fx);
int kit_track_fx_of(int track);
/* the user kit (API 23, D-056): sounds an app brings (tools/gen_images.py makes USER_KIT from kits/user/NN-name.wav,
 * mono, 22 050 Hz): kit_drum_user_kit makes them the fifth kit, "USER" (KIT_DRUM_USER), each on its pad; a pad
 * without one plays Dusty's. */
typedef struct { uint8_t pad; const char *name; const int16_t *pcm; uint32_t len; } kit_sample_t;
#define KIT_DRUM_USER 4
void kit_drum_user_kit(const kit_sample_t *sounds, int n);
const char *kit_drum_kit_name(int kit);
const char *kit_drum_name(int kit, int pad);    /* "kick1" */
int kit_drum_group(int kit, int pad);           /* 0 kick, 1 snare, claps and rims, 2 hats, shakers, cymbals, 3 the rest */
int kit_drum_note(int kit, int pad, int tune);  /* the note a pitched sound sounds, tuned tune semitones; -1: unpitched */
int kit_drum_pad_of(int note);                  /* the pad a note plays */
void kit_drum_pad_set(int track, int pad, int kit, int src, int tune, int decay);   /* pad plays kit's (-1: the track's
                                                 * kit's) sound src, tune -12..12 semitones, decay 16..255 x/64 */
#define KIT_NVOICES 12
enum { KIT_TRACK_CHORDS = 0, KIT_TRACK_BASS, KIT_TRACK_MELODY, KIT_TRACK_DRUMS, KIT_NTRACKS };
#define KIT_CPU_LIMIT 75        /* % of the CPU the voices may take (API 35, D-091: the cpu gate's limit is 80) */
void kit_track_sound(int track, int engine, int preset);   /* stops what the track sounds, then switches */
/* API 44 (D-119): select a base sound and its track sends while preserving the song's shared effects. */
void kit_track_sound_local(int track, int engine, int preset);
/* Factory arpeggio metadata. Ticks are at 24 per quarter. Gate is a fraction of 128.
 * Swing is 0..100; probability is 0..127 (127 always plays). */
enum {
    KIT_PRESET_ARP_UP, KIT_PRESET_ARP_DOWN, KIT_PRESET_ARP_UP_DOWN,
    KIT_PRESET_ARP_AS_PLAYED, KIT_PRESET_ARP_RANDOM, KIT_PRESET_ARP_REPEAT,
    KIT_PRESET_ARP_UP_OCTAVE, KIT_PRESET_ARP_DIVERGE
};
typedef struct { uint8_t mode, ticks, octaves, gate, swing, probability; } kit_preset_arp_t;
int kit_preset_arp(int engine, int preset, kit_preset_arp_t *arp); /* 0 if no factory arp */

/* API 26 (D-067): a track's level, 0..10 (10 at start), a mixer's fader: each note the track starts plays at its
 * velocity x level / 10; at 0 the track stops what it sounds and starts nothing */
void kit_track_level(int track, int level);
int kit_track_level_of(int track);
/* API 27 (ST-46, D-073): how a synth track plays its notes. mode: KIT_VMODE_POLY (chords), KIT_VMODE_MONO (one note, the
 * last key; letting it go returns to a key still held) or KIT_VMODE_LEGATO (one note; a key pressed while another is held
 * slides there without a new attack). glide_ms 0..2000: the time a note takes to slide from the last one (0: no glide); a
 * chord's notes each slide from the nearest note of the chord before. slide: KIT_SLIDE_ALWAYS (portamento: even after the
 * keys were let go) or KIT_SLIDE_LEGATO (only while a key is held). Drums are never mono and never glide. Every track
 * starts POLY with no glide. */
enum { KIT_VMODE_POLY = 0, KIT_VMODE_MONO, KIT_VMODE_LEGATO };
enum { KIT_SLIDE_ALWAYS = 0, KIT_SLIDE_LEGATO };
void kit_track_voice(int track, int mode, int glide_ms, int slide);
int kit_track_voice_mode(int track);
int kit_track_glide_ms(int track);
int kit_track_slide(int track);
int kit_voices_used(void);      /* API 20: voices sounding (or releasing) now */
int kit_voices_load(void);      /* API 20: the estimated CPU the voices take, % */
uint32_t kit_voices_stolen(void);   /* API 20: how many voices were taken or notes dropped since start */
int kit_track_engine(int track);
int kit_track_preset(int track);
/* Native filter controls of the active sound, 0..127. Return 0 for unsupported tracks. */
typedef struct { uint8_t cutoff, resonance, amount, keytrack; } kit_sound_filter_t;
int kit_track_filter_of(int track, kit_sound_filter_t *filter);
int kit_track_filter(int track, const kit_sound_filter_t *filter);
/* Selected FMini or Juno-106 track's spectrum before the shared effects. A negative track disables capture. */
#define KIT_SPECTRUM_BANDS 24
void kit_track_spectrum(int track);
int kit_track_spectrum_read(uint8_t bands[KIT_SPECTRUM_BANDS]);
int kit_track_spectrum_position(int track, int cutoff);   /* nominal cutoff on the spectrum axis, 0..100 */
/* First two oscillators of an editable sound. Detune is signed cents, mix is oscillator 2's share (0..127). */
enum { KIT_OSC_TRI, KIT_OSC_SHARK, KIT_OSC_SAW, KIT_OSC_SQR, KIT_OSC_WIDE, KIT_OSC_NARROW, KIT_OSC_SINE, KIT_OSC_NWAVES };
enum { KIT_OSC_WAVE1, KIT_OSC_WAVE2, KIT_OSC_DETUNE, KIT_OSC_MIX };
typedef struct { uint8_t wave1, wave2, mix; int16_t detune; } kit_sound_osc_t;
int kit_track_osc_of(int track, kit_sound_osc_t *osc);
int kit_track_osc(int track, int control, int value);
/* API 45 (D-120): exact active levels of oscillators 1 and 2, without balance rounding.
 * Zero means the oscillator is off. Both calls return 0 on unsupported tracks. */
int kit_track_osc_levels_of(int track, uint8_t levels[2]);
int kit_track_osc_levels(int track, const uint8_t levels[2]);
/* Native envelopes of an editable sound. Controls are 0..127; times use the engine's dial scale. */
enum { KIT_ENV_FILTER, KIT_ENV_AMP };
enum { KIT_ENV_ATTACK, KIT_ENV_DECAY, KIT_ENV_SUSTAIN, KIT_ENV_RELEASE };
int kit_track_envelope_of(int track, int kind, uint8_t values[4]);
int kit_track_envelope(int track, int kind, int control, int value);
int kit_track_envelope_ms(int track, int time_dial);      /* FMini's common time dial */
int kit_track_envelope_stage_ms(int track, int control, int time_dial);
/* FMini LFO rate and depth use 0..127. The editable route has zero depth in factory presets. */
enum { KIT_LFO_SINE, KIT_LFO_TRI, KIT_LFO_SAW, KIT_LFO_SQUARE, KIT_LFO_NWAVES };
enum { KIT_LFO_PITCH, KIT_LFO_CUTOFF, KIT_LFO_VOLUME, KIT_LFO_NTARGETS };
typedef struct { uint8_t rate, depth, target, wave; } kit_sound_lfo_t;
int kit_track_lfo_of(int track, kit_sound_lfo_t *lfo);
int kit_track_lfo(int track, const kit_sound_lfo_t *lfo);
int kit_track_lfo_centihz(int track, int dial);
/* Real-time FMini graph markers for its newest sounding voice. Stage 1/2/3 is attack/decay or sustain/release. */
typedef struct { uint8_t amp_stage, amp_level, filter_stage, filter_level, lfo_phase; } kit_sound_motion_t;
int kit_track_motion_of(int track, kit_sound_motion_t *motion);   /* levels 0..127, phase 0..255 */
/* Temporary modulation of native FMini controls and track effects, in control steps (-127..127). */
typedef struct { int16_t cutoff, resonance, rate, drive, delay, reverb; } kit_sound_mod_t;
void kit_track_mod_shape(int track, const kit_sound_mod_t *mod);
void kit_track_note_on(int track, int note, int vel);
void kit_track_note_off(int track, int note);
void kit_track_notes_off(int track);
/* A MIDI keyboard's controls on a track (API 17, D-044). Bend: -8192..8191, two semitones each way by default. Mod: 0..127, a
 * vibrato (5.5 Hz) up to half a semitone each way. Sustain: while on, the track's note-offs wait, then all go at once
 * when it goes off (kit_track_notes_off clears them). The drums neither bend nor wobble. */
void kit_track_bend(int track, int value);
void kit_track_bend_range(int track, int semitones);       /* 1..12, retained for the track */
int kit_track_bend_range_of(int track);
void kit_track_mod(int track, int depth);
void kit_track_sustain(int track, int on);
/* API 18 (D-045): a click (a short blip on its own voice, never on a track: 1 kHz, 2 kHz accented), level 1..10; and
 * the FM-1's tuning, -50..+50 cents on every track but the drums. */
void kit_click(int accent, int level);
void kit_tune(int cents);
/* API 24 (D-060): a reverb on the whole sound (a small Freeverb, 0 off .. 100 the most), and sound effects on a
 * voice of their own, never on a track (as the click): PFFT a low fluttering buzz (half a second), PLOP a drop, NEIGH
 * a horse's whinny (a second), GULL a seagull's cry (two falling squawks), BAA a sheep's bleat, CHIRP a small bird far off.
 * API 25 (D-064): WHISTLE a steam whistle's two blasts (choo choo), COUGH an engine's cough, SNEEZE "ah... ah... CHOO",
 * WOOF a dog's bark, MOO a cow, HOOT an owl (two hoots), HOWL a coyote. */
void kit_reverb(int level);
int kit_reverb_of(void);
/* Shared room shape: SIZE 0..127, DAMP 0..100 percent, PRE-DELAY 0..120 ms. */
typedef struct { uint8_t size, damp, pre_ms; } kit_reverb_shape_t;
void kit_reverb_shape(const kit_reverb_shape_t *shape);
void kit_reverb_shape_of(kit_reverb_shape_t *shape);
int kit_reverb_tail_ms_of(void);            /* send tail processing time before pre-delay */
/* API 36 (D-093): the effects bus (ST-69), after Felucca's and fmini-1's: each track's DRIVE, then its sends into
 * three shared buses, CHORUS, DELAY and REVERB (the reverb above, fed by the sends as well as by kit_reverb's
 * level). A track's sends are 0..127; kit_track_sound sets them from the sound's preset (its FX(DIST, CHORUS, DELAY,
 * REVERB); every engine's sound, D-094) and an FMini patch also sets the bus (its twelve FX settings), as fmini-1 does; the app may change them
 * after. The bus: CHORUS TYPE 0 STEREO 1 JUNO 2 MULTI, DEPTH 0..127 (60 typical), RATE 0..127; DELAY TYPE 0 ANALOG
 * 1 DIGITAL 2 TAPE 3 SLAP, TIME 0..127 (10 ms .. 0.74 s), FEEDBACK 0..120, MIX 0..127. */
typedef struct { uint8_t cho_type, cho_depth, cho_rate, dly_type, dly_time, dly_fb, dly_mix; } kit_fx_t;
enum { KIT_SEND_DRIVE = 0, KIT_SEND_CHORUS, KIT_SEND_DELAY, KIT_SEND_REVERB };
void kit_track_sends(int track, int drive, int chorus, int delay, int reverb);
int kit_track_send_of(int track, int which);
void kit_fx_bus(const kit_fx_t *fx);
void kit_fx_bus_of(kit_fx_t *fx);
/* Shared delay tone: 0 is darkest, 126 brightest, 127 uses the delay type's original tone. */
void kit_delay_tone(int tone);
int kit_delay_tone_of(void);
/* Rounded delay time of the selected type and setting, within the available delay line. */
int kit_delay_ms_of(void);
/* Track DIRT controls, 0..127. DRIVE is the existing drive send. */
typedef struct { uint8_t crush, tone, mix; } kit_dirt_t;
void kit_track_dirt(int track, const kit_dirt_t *dirt);
void kit_track_dirt_of(int track, kit_dirt_t *dirt);
int kit_dirt_bits(int crush);
int32_t kit_track_dirt_curve(int track, int32_t sample);   /* static Q15 transfer: drive, crush, mix */
/* One live performance pair. X and Y are 0..127. A negative track clears the overlay. */
enum { KIT_PERFORM_FILTER, KIT_PERFORM_WAH, KIT_PERFORM_DUB, KIT_PERFORM_SPACE, KIT_PERFORM_DIRT,
       KIT_PERFORM_WOBBLE, KIT_NPERFORM_FX };
void kit_perform_fx(int track, int pair, int x, int y);
enum { KIT_SFX_PFFT = 0, KIT_SFX_PLOP, KIT_SFX_NEIGH, KIT_SFX_GULL, KIT_SFX_BAA, KIT_SFX_CHIRP,
       KIT_SFX_WHISTLE, KIT_SFX_COUGH, KIT_SFX_SNEEZE, KIT_SFX_WOOF, KIT_SFX_MOO, KIT_SFX_HOOT, KIT_SFX_HOWL, KIT_NSFX };
void kit_sfx(int id, int level);          /* level 1..10 */
void kit_note_on(int note, int vel);      /* vel 1..127 */
void kit_note_off(int note);
void kit_all_notes_off(void);
int kit_engine_count(void);
const char *kit_engine_name(int engine);
int kit_preset_count(int engine);
const char *kit_preset_name(int engine, int preset);
void kit_sound(int engine, int preset);   /* stops what sounds, then switches */
int kit_sound_engine(void);
int kit_sound_preset(void);
void kit_instrument(int inst);            /* = kit_sound(KIT_ENGINE_BASIC, inst) */
const char *kit_instrument_name(int inst);
void kit_volume(int pct);                 /* 0..100, before the MASTER pot */
void kit_volume_limit(int pct);           /* hard ceiling (kid-safe); kit_volume is clamped to it */

/* -------------------------------------------------------------------------- song --- */
/* A song (API 10, D-031): a key (AUTO or locked), a tempo and up to 8 bars of 4/4, with four tracks of notes.
 * Times are in ticks, KIT_TICKS_PER_BEAT a beat. What is recorded is kept as it was played: nothing quantises it or
 * changes it (GRID and FEEL, API 22, only move where it plays). kit_seq_* play a song, looping, on the tracks' sounds, and record into one track while it plays. */
#define KIT_TICKS_PER_BEAT 24
#define KIT_BEATS_PER_BAR 4
#define KIT_TICKS_PER_BAR (KIT_TICKS_PER_BEAT * KIT_BEATS_PER_BAR)
#define KIT_SONG_MAX_BARS 8
#define KIT_SONG_MAX_NOTES 128                  /* per track */
typedef struct { uint16_t start, len; uint8_t note, vel; } kit_song_note_t;
typedef struct {
    uint8_t key_auto;                           /* 1: the key is worked out from the notes (kit_keys_for_notes) */
    uint8_t key_tonic, key_minor;               /* the locked key (or the last one chosen) */
    uint8_t bpm, bars;                          /* KIT_TEMPO_MIN..MAX, 1..KIT_SONG_MAX_BARS */
    uint8_t unpitched;                          /* API 19 (D-046): the tracks that are drums, a bit each (the key and
                                                 * transposing leave them out); kit_song_init: KIT_TRACK_DRUMS */
    uint8_t count[KIT_NTRACKS];
    kit_song_note_t notes[KIT_NTRACKS][KIT_SONG_MAX_NOTES];   /* each track's notes, by start */
} kit_song_t;
void kit_song_init(kit_song_t *s, int bpm, int bars);        /* empty, key AUTO */
int kit_song_length(const kit_song_t *s);                    /* ticks */
int kit_song_add(kit_song_t *s, int track, int start, int len, int note, int vel);   /* 0, or -1 when full */
int kit_song_add_chord(kit_song_t *s, int track, int bar, const uint8_t *notes, int n, int vel);   /* one bar long */
void kit_song_clear(kit_song_t *s, int track);
void kit_seq_play(kit_song_t *s);                            /* from the top, looping; s must stay valid */
/* API 29 (D-081): the sequencer's chain. kit_seq_next: on the next bar line (the end of the loop at the latest) go on
 * with s from its top (its own length) instead of carrying on; NULL cancels; play, resume and stop cancel it.
 * kit_seq_song: the song playing (NULL: none). */
void kit_seq_next(kit_song_t *s);
kit_song_t *kit_seq_song(void);
/* API 29: an app's large data (the sequencer's sequences) in the RAM pool, zeroed at boot: `static T x[n] KIT_BIG;` */
#define KIT_BIG __attribute__((section(".pool")))
void kit_seq_stop(void);
/* MIDI clock (API 17, D-044). A MIDI clock is 24 a beat, as the song's ticks: while following, the song player moves a
 * tick for each clock that comes in (USB or the TRS jack) instead of by time, so it keeps exactly in time with the
 * other gear; with no clock it waits. Resume plays on from where kit_seq_stop stopped it (MIDI Continue). Clock out:
 * while on and not following, PLAY sends Start, then a clock a tick, and stopping sends Stop, over USB. */
void kit_seq_follow(int on);
int kit_seq_following(void);
void kit_seq_resume(kit_song_t *s);
int kit_seq_clock_bpm(void);                                 /* the clock in's tempo, rounded; 0 when none for a second */
void kit_seq_clock_out(int on);
/* The metronome and the count-in (API 18, D-045). Metronome: while the song plays, a click on every beat at level
 * 1..10 (0: none), accented on the bar's first. Count-in: bars (1 or 2) of clicks at the song's tempo, then the song
 * plays from the top on the beat after the last; kit_seq_counting says how many beats are left (0: not counting);
 * kit_seq_stop cancels it. */
void kit_seq_metronome(int level);
void kit_seq_count_in(kit_song_t *s, int bars, int level);
int kit_seq_counting(void);
int kit_seq_playing(void);
int kit_seq_tick_fine(void);                                 /* sixteenths of a song tick, -1 when stopped */
int kit_seq_tick(void);                                      /* where the song is (ticks), -1 when stopped */
void kit_seq_record(int track);                              /* record into track while playing; -1: no */
void kit_seq_note_on(int track, int note, int vel);          /* play a note, and record it when armed */
void kit_seq_note_off(int track, int note);
/* GRID and FEEL (API 22, D-051): how a track's notes are placed as the song plays; what is recorded is not changed, so
 * GRID 0 plays it as played again. GRID: a grid in ticks (6 a sixteenth, 12 an eighth, 4 and 8 their triplets), each
 * note played at the nearest grid place. FEEL, by pitch class (a drums track's groups are its pitch classes), with a
 * GRID only: swing 50..75 % (an off-sixteenth played late: 50 straight, 66 the MPC's), shift -40..40 ms (late or
 * early), loose 0..30 ms (a fixed small variation per step, the same every loop). NULL: straight. kit_seq_place: where
 * note i of track plays, in sixteenths of a tick from the loop's start (the screen draws it there). */
typedef struct { uint8_t swing; int8_t shift_ms; uint8_t loose_ms; } kit_feel_t;
void kit_seq_grid(int track, int ticks);
/* API 30 (D-082): the sequencer's QUANTIZE. The grid in sixteenths of a tick (KIT_TICKS_PER_BEAT * 16 a beat: a 1/64 note
 * is 24, a 1/16 96; 0 as played) and STRENGTH, how far a note moves toward it (0..100 %); only the playback changes.
 * kit_seq_grid(t, ticks) is kit_seq_quantize(t, ticks * 16, 100). A note recorded less than half a 16th before the loop's
 * end goes on the next loop's first step. */
void kit_seq_quantize(int track, int fine, int strength_pct);
void kit_seq_feel(int track, const kit_feel_t *by_pitch_class);
void kit_seq_feel_note(int track, int note, kit_feel_t f);   /* API 23: one note's (a drums pad's) own feel */
int kit_seq_place(const kit_song_t *s, int track, int i);
/* the song's settings (API 11, D-032: concept v0.2's GLO page underneath): none of them changes a note that was played,
 * except kit_song_transpose, which is the explicit way to move the song */
int kit_song_key(const kit_song_t *s, int *tonic, int *minor);  /* the key: locked, or AUTO's best from the bass, melody
                                                                   and chords; returns 1 when sure, 0 when ambiguous */
void kit_song_lock_key(kit_song_t *s, int tonic, int minor);    /* the notes stay as they are */
void kit_song_auto_key(kit_song_t *s);
void kit_song_transpose(kit_song_t *s, int semitones);          /* every note but the drums', and a locked key */
void kit_song_set_tempo(kit_song_t *s, int bpm);                /* while playing: on from the same tick */
void kit_song_set_bars(kit_song_t *s, int bars);                /* notes beyond the end are kept, not played */

/* ------------------------------------------------------------------------- music --- */
/* Ported from Sketchpad core/notes and core/scales; factory/oracle checks it gives the same
 * answers for every key. Names are ASCII: "C#", "Bb". */
typedef struct {
    const char *id, *name;
    uint8_t n;                  /* notes in the scale */
    uint8_t iv[8];              /* semitones above the tonic */
    uint8_t minor;              /* 1 = a minor-mode scale (spelling follows the minor key) */
} kit_scale_t;
extern const kit_scale_t KIT_SCALES[];
extern const int KIT_NSCALES;

enum { KIT_NAMES_LETTERS = 0, KIT_NAMES_SOLFEGE };
int kit_pc(int note);                                    /* pitch class 0..11 */
int kit_octave(int note);                                /* 60 -> 4 */
int kit_is_white(int note);
const char *kit_note_name(int note, int names);         /* sharps: "C#" / "Do#" */
/* The note spelled as the key of (tonic, minor) writes it: Eb in C minor, F# in G. */
const char *kit_note_name_in_key(int note, int tonic, int minor, int names);
int kit_scale_has(const kit_scale_t *s, int tonic, int note);
int kit_scale_degree(const kit_scale_t *s, int tonic, int note);   /* 1..n, or 0 if not in scale */
/* Which scales fit (API 15, D-041): Sketchpad core/scales fitScales. The scales on tonic, ranked by the share of the
 * pitch classes in `used` (bit p: pitch class p) that they hold, best first, fewer notes first on a tie; out[i] is an
 * index into KIT_SCALES and held[i] how many of the used pitch classes it holds (of *nused). Up to limit (17: all). */
int kit_fit_scales(uint16_t used, int tonic, uint8_t *out, uint8_t *held, int limit, int *nused);

/* Run orders (Sketchpad core/arpeggio runOrder): the notes, unique, in the order a run plays them. Up; down; up and
 * back down without repeating the top or the bottom (so it loops); or every note once in an order fixed by `seed`,
 * keeping the top note for last when end_on is 1 (a scale's top). `out` holds up to 2 n notes; returns how many. */
enum { KIT_RUN_UP = 0, KIT_RUN_DOWN, KIT_RUN_UPDOWN, KIT_RUN_RANDOM, KIT_NRUNS };
int kit_run_order(const uint8_t *notes, int n, int pattern, uint32_t seed, int end_on, uint8_t *out);
/* Tempo (Sketchpad core/transport, D-117 there; D-048 here): 40..240 BPM, steps of 5; a beat in ms */
#define KIT_TEMPO_MIN 40
#define KIT_TEMPO_MAX 240
#define KIT_TEMPO_START 90
#define KIT_TEMPO_STEP 5
int kit_tempo_clamp(int bpm);
uint32_t kit_beat_ms(int bpm);

/* chords and keys (API 9, D-030): Sketchpad core/harmony and core/chords, checked by the oracle */
enum { KIT_Q_MAJOR = 0, KIT_Q_MINOR, KIT_Q_DIM, KIT_Q_AUG, KIT_Q_OTHER,
       KIT_Q_SUS2, KIT_Q_SUS4 };   /* chord types for kit_colour_chord (API 13); kit_triads never gives them */
typedef struct {
    uint8_t root;               /* pitch class */
    uint8_t quality;            /* KIT_Q_* */
    uint8_t n;                  /* notes: 3 for a triad */
    uint8_t notes[4];           /* MIDI, ascending from the root at C3 (Sketchpad's voicing) */
    char roman[8];              /* "I", "ii", "viio", "bVII" ... (ASCII: b flat, # sharp, o diminished, + augmented) */
} kit_chord_t;
int kit_triads(int tonic, int scale, kit_chord_t out[7]);   /* a seven-note scale's triads; 0 for other scales */
const char *kit_quality_sym(int quality);                   /* "", "m", "dim", "aug" */
int kit_inversion(const kit_chord_t *c, int inv, uint8_t *out);   /* 0 root position, 1, 2: the notes, ascending */
typedef struct {
    int fits;                   /* chords whose notes are all in the key */
    uint32_t outside;           /* bit i: chord i is not */
    int ok;                     /* every chord fits */
    int suggest_tonic;          /* -1, or a key that holds more of the chords */
    int suggest_minor, suggest_fits;
} kit_keyfit_t;
void kit_key_fit(const kit_chord_t *prog, int n, int tonic, int minor, kit_keyfit_t *out);   /* n <= 32 */
typedef struct { uint8_t note; uint16_t len; } kit_note_ev_t;     /* a played note and its length (any unit) */
typedef struct {
    uint8_t tonic, minor;
    uint16_t outside;           /* bit p: pitch class p was played but is not in the key */
    uint8_t evidence;           /* 0..4: the first, last, lowest and most-played notes that are the tonic */
    uint32_t held, total;       /* the playing the key holds, of all of it (fit = held / total) */
} kit_key_cand_t;
/* the keys played notes could be in, best first (up to limit, of 24); *ambiguous when they can't tell */
int kit_keys_for_notes(const kit_note_ev_t *ev, int n, kit_key_cand_t *out, int limit, int *ambiguous, uint16_t *heard);

/* colour chords (API 12, D-033): a triad with a colour, in one of five voicings. The chords are Sketchpad's (core/chords
 * DICTIONARY: intervals and symbols), the voicings core/voicing voicingsFor's and core/chords inversions; the next chord
 * is core/harmony suggestNextChords' first choice. Checked by the oracle. */
enum { KIT_COL_NONE = 0, KIT_COL_SUS2, KIT_COL_SUS4, KIT_COL_7, KIT_COL_MAJ7, KIT_COL_9, KIT_COL_11, KIT_COL_13,
       KIT_COL_ADD9, KIT_COL_6, KIT_NCOLS };
enum { KIT_VOICE_ROOT = 0, KIT_VOICE_INV1, KIT_VOICE_INV2, KIT_VOICE_OPEN, KIT_VOICE_WIDE, KIT_NVOICINGS };
#define KIT_CHORD_MAX 8
typedef struct {
    uint8_t root;               /* pitch class */
    uint8_t bass;               /* pitch class of a bass note under the chord (a slash chord), or 0xFF */
    uint8_t n;
    uint8_t notes[KIT_CHORD_MAX];   /* MIDI, ascending */
    const char *sym;            /* Sketchpad's symbol in ASCII: "m9", "m7b5", "m(add9)" */
} kit_vchord_t;
const char *kit_colour_name(int colour);                    /* "", "sus2", "sus4", "7", "maj7", "9", "11", "13", "add9", "6" */
const char *kit_colour_sym(int quality, int colour);        /* the chord a triad becomes with a colour: (minor, 9) -> "m9" */
/* The chord on root (pitch class) with that triad quality and colour, voiced from base (48: Sketchpad's C3); slash3 puts the
 * triad's 3rd in the bass, below the chord. Returns the number of notes (0 for an unknown colour or voicing). */
int kit_colour_chord(int root, int quality, int colour, int slash3, int voicing, int base, kit_vchord_t *out);
int kit_chord_label(const kit_vchord_t *c, int tonic, int minor, char *out, int len);   /* "Dm9", "C/E", spelled in the key */
int kit_next_degree(int tonic, int minor, int degree);      /* the likely next chord (0..6) after the key's chord `degree` */
/* What played notes are (API 13, D-035): Sketchpad core/chords identifyChord, exact (every tone of the chord, nothing
 * else), its best reading, labelled in the key: "Cmaj7", "C/E", "C + G" for two notes. Returns 0 when the notes are no
 * chord it knows (and out is ""), else 1; *root the chord's root (pitch class). */
int kit_identify_chord(const uint8_t *notes, int n, int tonic, int minor, char *out, int len, int *root);
/* The closest chord (API 16, D-042): Sketchpad identifyChord with missing: the dictionary chord the notes are part of,
 * the fewest of its tones missing (and the root in the bass) first: "Cmaj9/E". Returns how many tones are missing,
 * or -1 when the notes (three or more different) belong to no chord in the dictionary (out is then ""). */
int kit_closest_chord(const uint8_t *notes, int n, int tonic, int minor, char *out, int len);
/* The melody guide (API 21, D-049): Sketchpad core/melody melodyRole. Over a chord (its pitch classes, a bit each) and a
 * scale (the same): a note of the chord is STABLE (lands well), else one of the scale MOVES, else it PULLS. */
enum { KIT_MEL_STABLE = 0, KIT_MEL_MOVES, KIT_MEL_PULLS };
int kit_melody_role(int pc, uint16_t chord_pcs, uint16_t scale_pcs);
/* Figures (API 21, D-049): Sketchpad core/figures renderFigure. A step: its place and length in sixteenths and a role;
 * the chord as its root's pitch class and its notes as intervals from its lowest (in Sketchpad's order); the scale as
 * pitch classes. Seeded (Sketchpad's rng): the same seed, the same notes. Returns the notes written to out (at most n). */
enum { KIT_FIG_ROOT = 0, KIT_FIG_OCTAVE, KIT_FIG_FIFTH, KIT_FIG_THIRD, KIT_FIG_TOP, KIT_FIG_CHORDTONE, KIT_FIG_STEP,
       KIT_FIG_STEPUP, KIT_FIG_STEPDOWN, KIT_FIG_APPROACH };
typedef struct { uint8_t pos, dur, role; } kit_fig_step_t;
typedef struct { uint8_t note, pos, dur, role; } kit_fig_note_t;
int kit_render_figure(const kit_fig_step_t *steps, int n, int root_pc, const uint8_t *iv, int niv, int next_root_pc,
                      uint16_t scale_pcs, int32_t seed, int center, kit_fig_note_t *out);

/* ------------------------------------------------------------------------ player --- */
/* Plays a sequence of notes by itself, one every step_ms, each sounding for 92 % of a step (Sketchpad's breath before
 * the next), on the current sound. Each note it starts arrives as KIT_EV_PLAY_NOTE, so the app can light and name
 * it; a sequence that does not loop ends with KIT_EV_PLAY_END. Up to KIT_PLAY_MAX notes. */
#define KIT_PLAY_MAX 64
void kit_play_start(const uint8_t *notes, int n, uint32_t step_ms, int vel, int loop);
void kit_play_stop(void);
int kit_playing(void);

/* -------------------------------------------------------------------------- midi --- */
/* In: KIT_EV_MIDI_* events above, from the USB-MIDI port and the TRS MIDI IN jack (the app decides whether a note
 * plays). Out: over USB-MIDI to a computer or tablet; dropped when nothing is connected. Channels 1..16. */
void kit_midi_note_on(int channel, int note, int vel);
void kit_midi_note_off(int channel, int note);
void kit_midi_cc(int channel, int controller, int value);
int kit_midi_connected(void);   /* 1 while a USB host has the MIDI port open */
void kit_midi_realtime(int byte);   /* API 17: one real-time byte out over USB: 0xF8 clock, 0xFA Start, 0xFB Continue, 0xFC Stop */

/* ------------------------------------------------------------------------- store --- */
/* One record that survives power-off: settings, progress. kit_store_save queues the record and returns at once;
 * the kit writes it to flash when it has stayed the same for 2 s, at least 60 s after the last write, while no
 * key is held and nothing sounds, and only if it differs from what is stored. So an app may save on every change
 * without wearing the flash; a power-off loses at most the last minute's changes. A power cut during a write keeps
 * the previous record. The store never erases another firmware's data: if its flash is all taken by others, the
 * write fails and kit_store_status() says KIT_STORE_ERR_FULL. */
#define KIT_STORE_MAX 1024
enum { KIT_STORE_ERR_SIZE = -1, KIT_STORE_ERR_FULL = -2, KIT_STORE_ERR_FLASH = -3 };
int kit_store_load(void *buf, int len);         /* bytes loaded into buf (at most len); 0 = nothing saved */
int kit_store_save(const void *buf, int len);   /* 0 = queued; KIT_STORE_ERR_SIZE = too big (nothing queued) */
int kit_store_pending(void);                    /* 1 while a queued record waits to be written */
int kit_store_status(void);                     /* the last write: 0 or KIT_STORE_ERR_* (the previous record stands) */
#define KIT_STORE_SETTLE_MS 2000u
#define KIT_STORE_GAP_MS 60000u

/* ------------------------------------------------------------------------------ bank --- */
/* API 31 (D-085): one big record that survives power-off, for the song. As the store, but up to KIT_BANK_MAX bytes,
 * written sooner (when it has stayed the same for 3 s, at least 10 s after the last write) and only while nothing
 * plays, no key is held and the song is stopped. A power cut during a write keeps the previous record; the bank never
 * erases another firmware's data (KIT_BANK_ERR_FULL). */
#define KIT_BANK_MAX 16320
enum { KIT_BANK_ERR_SIZE = -1, KIT_BANK_ERR_FULL = -2, KIT_BANK_ERR_FLASH = -3 };
int kit_bank_load(void *buf, int len);          /* bytes loaded into buf (at most len); 0 = nothing saved */
int kit_bank_save(const void *buf, int len);    /* 0 = queued; KIT_BANK_ERR_SIZE = too big (nothing queued) */
int kit_bank_pending(void);                     /* 1 while a queued record waits to be written */
int kit_bank_status(void);                      /* the last write: 0 or KIT_BANK_ERR_* (the previous record stands) */
#define KIT_BANK_SETTLE_MS 3000u
#define KIT_BANK_GAP_MS 10000u

/* -------------------------------------------------------------------------- misc --- */
uint32_t kit_ms(void);
uint32_t kit_random(void);
void kit_log(const char *s);    /* USB serial on the device, stdout on the host */

#endif
