/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit internals: what the portable parts (gfx.c, synth.c, music.c, core.c) and a backend
 * (device.c on the FM-1, host.c on a PC) share. Not for apps. */
#ifndef KIT_INTERNAL_H
#define KIT_INTERNAL_H
#include "fm1kit.h"

#define KIT_FS 44100            /* audio sample rate */
#define KIT_BLOCK 32            /* synth control block, samples */

/* screen: native RGB565, row-major; the backend flushes rows [dirty_y0, dirty_y1] */
extern uint16_t *kit__fb;
extern int kit__dirty_y0, kit__dirty_y1;
void kit__dirty(int y0, int y1);

/* sound: render n mono samples (n a multiple of KIT_BLOCK), Q15 */
void kit__synth_init(void);
void kit__synth_render(int16_t *out, int n);

/* engines.c: Felucca-Engines, engine numbers 0.. here (1.. in the API) */
int kit__eng_count(void);
const char *kit__eng_name(int i);
int kit__eng_npresets(int i);
const char *kit__eng_preset_name(int i, int p);
void kit__eng_select(int part, int i, int p);     /* i < 0: none */
void kit__fx_mod(int track, int drive, int delay, int reverb);    /* caller holds the kit lock */
void kit__eng_note_on(int part, int note, int vel);
/* API 27: from16 the pitch (1/16 semitones) the note slides from (-1: none) over glide_ms; mono: the part's one voice takes
 * the note (legato: without a new attack). Returns 1 when it played. */
int kit__eng_note_glide(int part, int note, int vel, int from16, int glide_ms, int mono, int legato);
int kit__eng_retarget(int part, int from_note, int note, int glide_ms);   /* mono: the voice sounding from_note slides to note, no new attack */
void kit__eng_note_off(int part, int note);
void kit__eng_all_off(int part);
void kit__eng_render(int32_t *mix, int n);      /* every part */
int kit__eng_active(void);
int kit__eng_nactive(int part);
int kit__eng_has(int part, int note);
int kit__eng_steal(int part);

/* lights: the backend shows them */
void kit__light_set(int led_id, int level);   /* led_id: 0..13 buttons, 14..40 piano keys */

/* input: the backend reports raw state; core.c turns it into events */
void kit__core_poll(uint32_t notes, uint32_t buttons, const int8_t enc_steps[KIT_NENCS], const uint8_t vel[KIT_NKEYS]);
uint32_t kit__core_buttons(void);

/* store: KIT_STORE_SECTORS candidate 4 KiB sectors (0..) the backend keeps; 0 = done */
#define KIT_STORE_SECTORS 4
int kit__store_write(const void *src, int len);   /* now (tests, and kit__store_poll) */
void kit__store_poll(uint32_t now_ms);            /* the main loop, every pass */
void kit__player_poll(uint32_t now_ms);
void kit__seq_poll(uint32_t now_ms);              /* the song player: the main loop, every pass */           /* the main loop, every pass */
int kit__core_busy(void);                         /* a key or button is held */
int kit__synth_active(void);                      /* a voice sounds */
int kit__track_voices(int track);                 /* voices sounding or releasing on a track */
int kit__track_sounding(int track, int note);     /* (Basic and drum voices) the note sounds or releases there */
int kit__flash_read(int sector, int off, uint8_t *dst, int n);
int kit__flash_erase(int sector);
int kit__flash_write(int sector, int off, const uint8_t *src, int n);
/* bank (API 31): KIT_BANK_SECTORS 4 KiB sectors (0..), two slots of half of them; 0 = done */
#define KIT_BANK_SECTORS 8
int kit__bank_write(const void *src, int len);    /* now (tests, and kit__bank_poll) */
void kit__bank_poll(uint32_t now_ms);             /* the main loop, every pass */
int kit__bank_read(int sector, int off, uint8_t *dst, int n);
int kit__bank_erase(int sector);
int kit__bank_write_at(int sector, int off, const uint8_t *src, int n);

/* midi: the backend feeds bytes in from the main loop, and sends packets out */
#define KIT_MIDI_SOURCES 2      /* 0 USB, 1 TRS */
void kit__midi_byte(int source, uint8_t b);
void kit__midi_send(uint8_t status, uint8_t d1, uint8_t d2);
void kit__midi_send_rt(uint8_t b);                 /* one real-time byte out (USB) */
void kit__seq_clock(uint32_t now_ms);              /* a MIDI clock (F8) came in */
int kit__track_pitch(int track);
extern int kit__bg;                                /* 1 while the song player starts notes (D-047: live notes keep theirs) */
int kit__eng_preset_mono(int eng, int preset);
/* juno.c (D-090): FM-106's Juno-106, one a track (engines.c routes a track that plays it here) */
int kit__juno_npresets(void);
const char *kit__juno_preset_name(int p);
void kit__juno_select(int t, int p);
int kit__juno_on(int t);
int kit__juno_envelope_of(int t, uint8_t values[4]);
int kit__juno_envelope(int t, int control, int value);
int kit__juno_envelope_ms(int control, int dial);
int kit__juno_motion_of(int t, kit_sound_motion_t *motion);
int kit__juno_filter_of(int t, kit_sound_filter_t *filter);
int kit__juno_filter(int t, const kit_sound_filter_t *filter);
unsigned kit__juno_cutoff_bin(int cutoff);
int kit__spectrum_wants(int pt);
void kit__spectrum_capture(const int32_t *out, int count);
void kit__juno_note_on(int t, int note, int vel);
void kit__juno_note_off(int t, int note);
void kit__juno_all_off(int t);
int kit__juno_nactive(int t);
int kit__juno_nreleasing(int t);
int kit__juno_has(int t, int note);
int kit__juno_steal(int t);
void kit__juno_render(int t, int32_t *mix, int n);    /* engines.c: the preset is MONO (legato) */
int kit__eng_nreleasing(int part);                 /* engines.c: voices releasing on a part */                   /* the track's bend, vibrato and tune now, 1/16 semitones */

/* backend services */
uint32_t kit__lock(void);
void kit__unlock(uint32_t k);
int kit__pot(void);
uint32_t kit__usb_in_gain(void);                /* API 48: the computer's audio's gain now, Q12 (0: not heard) */

/* the effects bus (fxbus.c, D-093) */
int kit__fx_sends(int track);                   /* the track sends to the bus (or drives) */
void kit__fx_begin(void);
void kit__fx_track(int track, int32_t *x, int32_t *mix);
void kit__fx_buses(int32_t *mix);
int32_t *kit__fx_rev_in(void);
void kit__rv_room(int size);                    /* synth.c: the reverb's DECAY 0..127 */
void kit__eng_render_part(int part, int32_t *buf, int n);
int kit__eng_fx(int part, uint8_t sends[4], uint8_t bus12[12]);   /* the preset's sends; 1: bus12 holds an FMini patch's FX */

#endif
