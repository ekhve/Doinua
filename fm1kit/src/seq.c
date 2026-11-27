/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit song, recorder and song player (D-031). Portable; the backends call kit__seq_poll from their main loop,
 * never from an interrupt. Ticks are worked out from the loop's start each time, and the loop's start advances by
 * the loop's exact length (an integer part and a remainder), so the song never drifts however late a poll is or how
 * many times it loops. Recording keeps the tick a note was played at and how long it was held, as played: nothing
 * quantises it. factory/unit/test_seq.c checks it. */
#include "kit_internal.h"

void kit_song_init(kit_song_t *s, int bpm, int bars)
{
    int t;
    s->key_auto = 1;
    s->key_tonic = 0;
    s->key_minor = 0;
    s->bpm = (uint8_t)kit_tempo_clamp(bpm);
    s->bars = (uint8_t)(bars < 1 ? 1 : bars > KIT_SONG_MAX_BARS ? KIT_SONG_MAX_BARS : bars);
    for (t = 0; t < KIT_NTRACKS; t++)
        s->count[t] = 0;
    s->unpitched = 1u << KIT_TRACK_DRUMS;
}

int kit_song_length(const kit_song_t *s) { return s->bars * KIT_TICKS_PER_BAR; }

int kit_song_add(kit_song_t *s, int track, int start, int len, int note, int vel)
{
    kit_song_note_t *v;
    int n, i;
    if (track < 0 || track >= KIT_NTRACKS || start < 0 || start >= kit_song_length(s) || note < 0 || note > 127)
        return -1;
    n = s->count[track];
    if (n >= KIT_SONG_MAX_NOTES)
        return -1;
    if (len < 1) len = 1;
    if (len > kit_song_length(s)) len = kit_song_length(s);
    v = s->notes[track];
    for (i = n; i > 0 && v[i - 1].start > start; i--)          /* keep them by start; equal starts in the order added */
        v[i] = v[i - 1];
    v[i].start = (uint16_t)start;
    v[i].len = (uint16_t)len;
    v[i].note = (uint8_t)note;
    v[i].vel = (uint8_t)(vel < 1 ? 1 : vel > 127 ? 127 : vel);
    s->count[track] = (uint8_t)(n + 1);
    return 0;
}

int kit_song_add_chord(kit_song_t *s, int track, int bar, const uint8_t *notes, int n, int vel)
{
    int i;
    if (bar < 0 || bar >= s->bars)
        return -1;
    for (i = 0; i < n; i++)
        if (kit_song_add(s, track, bar * KIT_TICKS_PER_BAR, KIT_TICKS_PER_BAR, notes[i], vel))
            return -1;
    return 0;
}

void kit_song_clear(kit_song_t *s, int track)
{
    if (track >= 0 && track < KIT_NTRACKS)
        s->count[track] = 0;
}

/* ---- the song player ---- */
#define MAXOFF 32
static kit_song_t *song, *next_song;          /* the song playing; the one after this loop (API 29) */
static int playing, rec_track = -1, last_tick, last_bar;
static uint32_t loop_start, loop_rem;
static struct { uint8_t track, note; int16_t at; } offs[MAXOFF];       /* notes sounding: when they stop (ticks) */
static int noffs;
static struct { uint8_t note, vel; int16_t at; uint32_t loop; } held[16];   /* notes being recorded */
static int nheld;
static uint32_t loops;
static int cur_bpm;                     /* the tempo the loop's start was worked out for */
/* MIDI clock (API 17, D-044) */
static int follow, clk_out, ext_pos, clk_sent, stop_tick;   /* ext_pos: clocks into the loop; clk_sent: clocks sent */
static uint32_t clk_at[25];             /* when the last 25 clocks came in (a ring), for the tempo */
static int clk_n;
/* the metronome and the count-in (API 18, D-045) */
static int met_level, cnt_beats, cnt_done, cnt_level;
static uint32_t cnt_start;
static kit_song_t *cnt_song;
/* GRID and FEEL (API 22, D-051): applied as the song plays, never to what is recorded. Places are worked out in
 * sixteenths of a tick ("fine"), so a swing of 55 % (0.6 of a tick at a sixteenth note) is placed exactly. */
#define FINE 16
static uint16_t grid[KIT_NTRACKS];              /* sixteenths of a tick (API 30); 0: as played */
static uint8_t strength[KIT_NTRACKS] = { 100, 100, 100, 100 };   /* how far a note moves to the grid, % (API 30) */
static kit_feel_t feel[KIT_NTRACKS][128];       /* by note (API 23: a drums track's pads each their own) */
static int last_fine;                           /* the fine place reached in this loop; -1 before its start */

static int tick_at(uint32_t now)                /* ticks since the loop's start (now is never before it) */
{
    uint32_t el = now - loop_start;
    if (follow)
        return ext_pos;
    return (int)(el * song->bpm * KIT_TICKS_PER_BEAT / 60000u);
}

static int fine_at(uint32_t now)                /* fine places since the loop's start: el * bpm * 24 * 16 / 60000 */
{
    if (follow)
        return ext_pos * FINE;
    return (int)((now - loop_start) * song->bpm * 8u / 1250u);   /* 8 bars at 40 BPM: 48 s x 40 x 8, within 32 bits */
}

static uint32_t feel_hash(int a, int b)         /* a number in 0..2^16 for a step and a pitch class (the looseness) */
{
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (h ^ (h >> 16)) & 0xFFFFu;
}

int kit_seq_place(const kit_song_t *s, int track, int i)
{
    const kit_song_note_t *v = &s->notes[track][i];
    int len = kit_song_length(s), g = grid[track], st = v->start, pos, at = v->start * FINE, to;
    const kit_feel_t *f;
    if (!g)
        return st * FINE;                       /* as played */
    to = (at + g / 2) / g * g;                  /* the nearest grid place (fine) */
    pos = at + (to - at) * strength[track] / 100;   /* STRENGTH: that far toward it (API 30) */
    st = to / FINE;                             /* the grid place's tick, for the feel */
    if (st >= len)
        st -= len;
    f = &feel[track][v->note & 127];
    if (st % 6 == 0 && (st / 6) % 2 == 1 && f->swing > 50)   /* an off-sixteenth: swung late */
        pos += (f->swing * 2 - 100) * 6 * FINE / 100;
    if (f->shift_ms)
        pos += f->shift_ms * s->bpm * 8 / 1250;
    if (f->loose_ms)
        pos += ((int)feel_hash(st / 6, v->note % 12) - 32768) * f->loose_ms / 32768 * s->bpm * 8 / 1250;
    pos %= len * FINE;
    if (pos < 0)
        pos += len * FINE;
    return pos;
}

void kit_seq_quantize(int track, int fine, int strength_pct)
{
    if (track < 0 || track >= KIT_NTRACKS)
        return;
    grid[track] = (uint16_t)(fine < 0 ? 0 : fine > KIT_TICKS_PER_BAR * FINE ? KIT_TICKS_PER_BAR * FINE : fine);
    strength[track] = (uint8_t)(strength_pct < 0 ? 0 : strength_pct > 100 ? 100 : strength_pct);
}

void kit_seq_grid(int track, int ticks) { kit_seq_quantize(track, ticks * FINE, 100); }

static kit_feel_t feel_clamp(kit_feel_t f)
{
    if (f.swing < 50) f.swing = 50;
    if (f.swing > 75) f.swing = 75;
    if (f.shift_ms > 40) f.shift_ms = 40;
    if (f.shift_ms < -40) f.shift_ms = -40;
    if (f.loose_ms > 30) f.loose_ms = 30;
    return f;
}

void kit_seq_feel(int track, const kit_feel_t *by_pc)
{
    int n;
    kit_feel_t straight = { 50, 0, 0 };
    if (track < 0 || track >= KIT_NTRACKS)
        return;
    for (n = 0; n < 128; n++)
        feel[track][n] = feel_clamp(by_pc ? by_pc[n % 12] : straight);
}

void kit_seq_feel_note(int track, int note, kit_feel_t f)
{
    if (track >= 0 && track < KIT_NTRACKS && note >= 0 && note < 128)
        feel[track][note] = feel_clamp(f);
}

static void sound_off_all(void)
{
    int i;
    for (i = 0; i < noffs; i++)
        kit_track_note_off(offs[i].track, offs[i].note);
    noffs = 0;
}

static void start_at(kit_song_t *s, int tick)   /* play from tick of the loop */
{
    uint32_t now = kit_ms();
    kit_seq_stop();
    song = s;
    next_song = 0;
    if (tick < 0 || tick >= kit_song_length(s))
        tick = 0;
    playing = 1;
    loop_start = now - (uint32_t)tick * 60000u / ((uint32_t)s->bpm * KIT_TICKS_PER_BEAT);
    loop_rem = 0;
    loops = 0;
    cur_bpm = s->bpm;
    ext_pos = tick;
    clk_sent = tick;
    last_tick = tick - 1;
    last_fine = tick * FINE - 1;
    last_bar = -1;
    if (clk_out && !follow)
        kit__midi_send_rt(tick ? 0xFBu : 0xFAu);   /* Start, or Continue */
    kit__seq_poll(now);
}

void kit_seq_play(kit_song_t *s) { start_at(s, 0); }
void kit_seq_next(kit_song_t *s) { if (playing) next_song = s; }
kit_song_t *kit_seq_song(void) { return playing ? song : 0; }
void kit_seq_metronome(int level) { met_level = level < 0 ? 0 : level > 10 ? 10 : level; }
int kit_seq_counting(void) { return cnt_beats ? cnt_beats - cnt_done : 0; }

void kit_seq_count_in(kit_song_t *s, int bars, int level)
{
    kit_seq_stop();
    cnt_song = s;
    cnt_beats = (bars < 1 ? 1 : bars > 2 ? 2 : bars) * KIT_BEATS_PER_BAR;
    cnt_done = 0;
    cnt_level = level < 1 ? 1 : level > 10 ? 10 : level;
    cnt_start = kit_ms();
    kit__seq_poll(cnt_start);
}

static void metro(int from, int to)             /* a click on each beat in ticks (from, to] */
{
    int t;
    if (!met_level)
        return;
    for (t = from + 1; t <= to; t++)
        if (t % KIT_TICKS_PER_BEAT == 0)
            kit_click(t % KIT_TICKS_PER_BAR == 0, met_level);
}
void kit_seq_resume(kit_song_t *s) { start_at(s, stop_tick); }
void kit_seq_follow(int on) { follow = on != 0; }
int kit_seq_following(void) { return follow; }
void kit_seq_clock_out(int on) { clk_out = on != 0; }

void kit__seq_clock(uint32_t now)
{
    clk_at[clk_n % 25] = now;
    clk_n++;
    if (playing && follow) {
        ext_pos++;
        kit__seq_poll(now);
    }
}

int kit_seq_clock_bpm(void)
{
    uint32_t last, beat;
    if (clk_n < 25)
        return 0;
    last = clk_at[(clk_n - 1) % 25];
    if (kit_ms() - last > 1000u)
        return 0;
    beat = last - clk_at[clk_n % 25];          /* 24 clocks: a beat */
    return beat ? (int)((60000u + beat / 2u) / beat) : 0;
}

void kit_seq_stop(void)
{
    int i;
    cnt_beats = 0;                              /* a count-in stops too */
    next_song = 0;
    if (!playing)
        return;
    stop_tick = last_tick < 0 ? 0 : last_tick;
    if (clk_out && !follow)
        kit__midi_send_rt(0xFCu);
    sound_off_all();
    for (i = 0; i < nheld; i++)
        kit_track_note_off(rec_track, held[i].note);
    nheld = 0;
    playing = 0;
}

int kit_seq_playing(void) { return playing; }
int kit_seq_tick_fine(void) { return playing ? (last_fine < 0 ? 0 : last_fine) : -1; }
int kit_seq_tick(void) { return playing ? (last_tick < 0 ? 0 : last_tick) : -1; }
void kit_seq_record(int track) { rec_track = track >= 0 && track < KIT_NTRACKS ? track : -1; }

static void start_notes(int from, int to)       /* the notes placed in fine places (from, to] of this loop */
{
    int t, i, len = kit_song_length(song);
    for (t = 0; t < KIT_NTRACKS; t++)
        for (i = 0; i < song->count[t]; i++) {
            const kit_song_note_t *v = &song->notes[t][i];
            int pos;
            if (v->start >= len)
                continue;                       /* beyond the end: kept, not played */
            pos = kit_seq_place(song, t, i);
            if (pos <= from || pos > to)
                continue;
            kit__bg = 1;                        /* the song's notes: live playing keeps its voices first (D-047) */
            kit_track_note_on(t, v->note, v->vel);
            kit__bg = 0;
            if (noffs < MAXOFF) {
                int end = pos / FINE + v->len;
                offs[noffs].track = (uint8_t)t;
                offs[noffs].note = v->note;
                offs[noffs].at = (int16_t)(end > len ? len : end);
                noffs++;
            }
        }
}

static void stop_notes(int upto)                /* the notes due to stop at or before tick upto */
{
    int i = 0;
    while (i < noffs)
        if (offs[i].at <= upto) {
            kit_track_note_off(offs[i].track, offs[i].note);
            offs[i] = offs[--noffs];
        } else {
            i++;
        }
}

static void bar_event(int tick)
{
    int bar = tick / KIT_TICKS_PER_BAR;
    if (bar != last_bar) {
        kit_event_t e;
        e.type = KIT_EV_SEQ_BAR;
        e.id = (uint8_t)bar;
        e.vel = 0;
        e.delta = bar == 0;
        last_bar = bar;
        app_event(&e);
    }
}

static void send_clocks(int upto)               /* clock out: a clock for each tick reached */
{
    if (!clk_out || follow)
        return;
    while (clk_sent <= upto) {
        kit__midi_send_rt(0xF8u);
        clk_sent++;
    }
}

void kit__seq_poll(uint32_t now)
{
    int len, tick;
    if (cnt_beats) {                            /* counting in: a click a beat, then the song from the top */
        uint32_t beat = 60000u / cnt_song->bpm;
        while (cnt_done < cnt_beats && now - cnt_start >= (uint32_t)cnt_done * beat) {
            kit_click(cnt_done % KIT_BEATS_PER_BAR == 0, cnt_level);
            cnt_done++;
        }
        if (now - cnt_start < (uint32_t)cnt_beats * beat)
            return;
        cnt_beats = 0;
        start_at(cnt_song, 0);
        loop_start = cnt_start + (uint32_t)cnt_done * beat;   /* on the beat, not on the poll */
        return;
    }
    if (playing && follow) {                    /* following a MIDI clock: a tick a clock, never by time */
        len = kit_song_length(song);
        while (ext_pos >= len) {
            metro(last_tick, len - 1);
            start_notes(last_fine, len * FINE - 1);
            stop_notes(len);
            sound_off_all();
            loops++;
            last_tick = -1;
            last_fine = -1;
            last_bar = -1;
            ext_pos -= len;
        }
        if (ext_pos > last_tick) {
            metro(last_tick, ext_pos);
            stop_notes(ext_pos);
            start_notes(last_fine, ext_pos * FINE);
            last_tick = ext_pos;
            last_fine = ext_pos * FINE;
            bar_event(ext_pos);
        }
        return;
    }
    if (!playing || (int32_t)(now - loop_start) < 0)
        return;
    if (song->bpm != cur_bpm) {             /* a new tempo: the same place in the bar now, at the new pace from here */
        uint32_t el = now - loop_start;
        loop_start = now - el * (uint32_t)cur_bpm / song->bpm;
        loop_rem = 0;
        cur_bpm = song->bpm;
    }
    len = kit_song_length(song);
    tick = tick_at(now);
    for (;;) {                                  /* the end of the loop (or, with a next song, of the bar): finish it */
        int end = len;
        uint32_t total;
        if (next_song) {                        /* the chain (API 29): the next song on the next bar line */
            end = ((last_tick < 0 ? 0 : last_tick) / KIT_TICKS_PER_BAR + 1) * KIT_TICKS_PER_BAR;
            if (end > len) end = len;
        }
        if (tick < end) break;
        total = (uint32_t)(end / KIT_TICKS_PER_BAR) * KIT_BEATS_PER_BAR * 60000u;
        send_clocks(end - 1);
        clk_sent = 0;
        metro(last_tick, end - 1);
        start_notes(last_fine, end * FINE - 1);
        stop_notes(end);
        sound_off_all();
        loop_rem += total;                      /* the loop's exact length: no drift */
        loop_start += loop_rem / song->bpm;
        loop_rem %= song->bpm;
        loops++;
        last_tick = -1;
        last_fine = -1;
        last_bar = -1;
        if (next_song) {                        /* the chain (API 29): the next song from its top */
            song = next_song;
            next_song = 0;
            len = kit_song_length(song);
            if (song->bpm != cur_bpm) { loop_rem = 0; cur_bpm = song->bpm; }
        }
        tick = tick_at(now);
    }
    send_clocks(tick);
    {
        int fine = fine_at(now);
        if (fine >= len * FINE)
            fine = len * FINE - 1;
        if (tick > last_tick || last_tick < 0) {
            metro(last_tick, tick);
            stop_notes(tick);
            last_tick = tick;
            bar_event(tick);
        }
        if (fine > last_fine) {
            start_notes(last_fine, fine);       /* (after a loop's start last_fine is -1: place 0 is included) */
            last_fine = fine;
        }
    }
}

/* ---- recording ---- */
void kit_seq_note_on(int track, int note, int vel)
{
    kit_track_note_on(track, note, vel);
    if (playing && track == rec_track && nheld < 16) {
        int i;
        for (i = 0; i < nheld; i++)
            if (held[i].note == note)
                return;
        held[nheld].note = (uint8_t)note;
        held[nheld].vel = (uint8_t)vel;
        held[nheld].at = (int16_t)tick_at(kit_ms());
        held[nheld].loop = loops;
        if (kit_song_length(song) - held[nheld].at < KIT_TICKS_PER_BEAT / 8) {   /* under half a 16th before the end: */
            held[nheld].at = 0;                 /* the next loop's first step (R-288, API 30) */
            held[nheld].loop = loops + 1;
        }
        nheld++;
    }
}

void kit_seq_note_off(int track, int note)
{
    int i;
    kit_track_note_off(track, note);
    if (track != rec_track)
        return;
    for (i = 0; i < nheld; i++)
        if (held[i].note == note) {
            int len = kit_song_length(song);
            int now = tick_at(kit_ms()), start = held[i].at;
            int dur = (int)(loops - held[i].loop) * len + now - start;   /* held over the loop's end: still its length */
            if (dur < 1) dur = 1;                 /* let go before the loop it was moved to began */
            if (start >= len) start = len - 1;
            kit_song_add(song, track, start, dur, note, held[i].vel);
            held[i] = held[--nheld];
            return;
        }
}

/* ---- the song's settings (D-032) ---- */
int kit_song_key(const kit_song_t *s, int *tonic, int *minor)
{
    uint8_t tracks[KIT_NTRACKS];                /* the pitched tracks (D-046), as before: the bass and melody first */
    static const uint8_t ORDER[KIT_NTRACKS] = { KIT_TRACK_BASS, KIT_TRACK_MELODY, KIT_TRACK_CHORDS, KIT_TRACK_DRUMS };
    kit_note_ev_t ev[64];
    kit_key_cand_t c[1];
    int idx[KIT_NTRACKS] = { 0 }, n = 0, amb = 1, len = kit_song_length(s), nt = 0, k;
    for (k = 0; k < KIT_NTRACKS; k++)
        if (!(s->unpitched >> ORDER[k] & 1u)) tracks[nt++] = ORDER[k];
    uint16_t heard;
    if (!s->key_auto) {
        *tonic = s->key_tonic;
        *minor = s->key_minor;
        return 1;
    }
    while (n < 64) {                        /* the notes in the order they are played: by start, across the tracks */
        int best = -1;
        for (k = 0; k < nt; k++) {
            int t = tracks[k];
            while (idx[k] < s->count[t] && s->notes[t][idx[k]].start >= len) idx[k]++;   /* beyond the end: not played */
            if (idx[k] < s->count[t] && (best < 0 || s->notes[t][idx[k]].start < s->notes[tracks[best]][idx[best]].start))
                best = k;
        }
        if (best < 0)
            break;
        ev[n].note = s->notes[tracks[best]][idx[best]].note;
        ev[n].len = s->notes[tracks[best]][idx[best]].len;
        n++;
        idx[best]++;
    }
    if (kit_keys_for_notes(ev, n, c, 1, &amb, &heard) == 0) {
        *tonic = s->key_tonic;
        *minor = s->key_minor;
        return 0;
    }
    *tonic = c[0].tonic;
    *minor = c[0].minor;
    return !amb;
}

void kit_song_lock_key(kit_song_t *s, int tonic, int minor)
{
    s->key_auto = 0;
    s->key_tonic = (uint8_t)(((tonic % 12) + 12) % 12);
    s->key_minor = (uint8_t)(minor != 0);
}

void kit_song_auto_key(kit_song_t *s) { s->key_auto = 1; }

void kit_song_transpose(kit_song_t *s, int semis)
{
    int t, i;
    for (t = 0; t < KIT_NTRACKS; t++) {
        if (s->unpitched >> t & 1u)
            continue;
        for (i = 0; i < s->count[t]; i++) {
            int n = s->notes[t][i].note + semis;
            s->notes[t][i].note = (uint8_t)(n < 0 ? 0 : n > 127 ? 127 : n);
        }
    }
    s->key_tonic = (uint8_t)((((s->key_tonic + semis) % 12) + 12) % 12);
}

void kit_song_set_tempo(kit_song_t *s, int bpm) { s->bpm = (uint8_t)kit_tempo_clamp(bpm); }

void kit_song_set_bars(kit_song_t *s, int bars)
{
    s->bars = (uint8_t)(bars < 1 ? 1 : bars > KIT_SONG_MAX_BARS ? KIT_SONG_MAX_BARS : bars);
}
