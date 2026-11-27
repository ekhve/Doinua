/* SPDX-License-Identifier: GPL-3.0-only */
/* GRID and FEEL (API 22, D-051), on a simulated clock: notes played early and late are kept as played; GRID 1/16
 * plays them on the sixteenths, GRID 0 as played again; a swing of 66 % plays an off-sixteenth a third of a sixteenth
 * late (exactly, below a tick); a drums track's feel moves each pitch class by its own shift; the loose part is the
 * same every loop; a note pushed before the loop's start plays at the end of the loop before. */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

static uint32_t now;
uint32_t kit_ms(void) { return now; }
int kit_keys_for_notes(const kit_note_ev_t *e, int n, kit_key_cand_t *o, int l, int *a, uint16_t *h) { (void)e; (void)n; (void)o; (void)l; *a = 1; *h = 0; return 0; }
int kit_tempo_clamp(int b) { return b < KIT_TEMPO_MIN ? KIT_TEMPO_MIN : b > KIT_TEMPO_MAX ? KIT_TEMPO_MAX : b; }
static struct { uint32_t t; int note; } on[4000];
static int non;
void kit_track_note_on(int t, int n, int v) { (void)t; (void)v; if (non < 4000) { on[non].t = now; on[non++].note = n; } }
void kit_track_note_off(int t, int n) { (void)t; (void)n; }
void kit_click(int a, int l) { (void)a; (void)l; }
int kit__bg;
void kit__midi_send_rt(uint8_t b) { (void)b; }
void app_event(const kit_event_t *e) { (void)e; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static void run(uint32_t ms) { uint32_t end = now + ms; while (now < end) { now += 1; kit__seq_poll(now); } }
static long when(int note, int k) { int i; for (i = 0; i < non; i++) if (on[i].note == note && k-- == 0) return (long)on[i].t; return -1; }
static int near(long a, long b, long tol) { return a >= 0 && a - b <= tol && b - a <= tol; }
static kit_song_t song;

int main(void)
{
    kit_feel_t f[12];
    int i;
    kit_song_init(&song, 120, 1);                       /* 500 ms a beat, a sixteenth 125 ms, a tick 20.83 ms, a loop 2 s */
    kit_song_add(&song, KIT_TRACK_BASS, 7, 3, 40, 100);     /* a tick late of the 2nd sixteenth (6) */
    kit_song_add(&song, KIT_TRACK_BASS, 22, 3, 41, 100);    /* two ticks early of the beat (24) */

    now = 1000; non = 0;
    kit_seq_play(&song); run(1990); kit_seq_stop();
    CHECK(near(when(40, 0), 1000 + 146, 1) && near(when(41, 0), 1000 + 458, 1), "as played (%ld %ld)", when(40, 0), when(41, 0));

    kit_seq_grid(KIT_TRACK_BASS, 6);
    now = 5000; non = 0;
    kit_seq_play(&song); run(1990); kit_seq_stop();
    CHECK(near(when(40, 0), 5000 + 125, 1) && near(when(41, 0), 5000 + 500, 1), "GRID 1/16: on the sixteenths (%ld %ld)", when(40, 0), when(41, 0));
    CHECK(song.notes[KIT_TRACK_BASS][0].start == 7 && song.notes[KIT_TRACK_BASS][1].start == 22, "what was recorded is kept");

    for (i = 0; i < 12; i++) { f[i].swing = 66; f[i].shift_ms = 0; f[i].loose_ms = 0; }
    kit_seq_feel(KIT_TRACK_BASS, f);
    now = 9000; non = 0;
    kit_seq_play(&song); run(1990); kit_seq_stop();
    /* the off-sixteenth (6 ticks) at 66 %: 0.32 of a beat's half... (66*2-100)/100 of a sixteenth late: 40 ms */
    CHECK(near(when(40, 0), 9000 + 125 + 40, 1) && near(when(41, 0), 9000 + 500, 1), "swing 66: the off-sixteenth 40 ms late, the beat on time (%ld %ld)", when(40, 0), when(41, 0));

    kit_seq_grid(KIT_TRACK_BASS, 0);
    now = 13000; non = 0;
    kit_seq_play(&song); run(1990); kit_seq_stop();
    CHECK(near(when(40, 0), 13000 + 146, 1) && near(when(41, 0), 13000 + 458, 1), "GRID as played: as played again, the feel needs a grid (%ld %ld)", when(40, 0), when(41, 0));

    /* a drums track: each pitch class its own feel (Dilla's snare 32 ms late, the kick 4 ms early) */
    kit_song_init(&song, 120, 1);
    kit_song_add(&song, KIT_TRACK_DRUMS, 0, 1, 36, 100);    /* kick C */
    kit_song_add(&song, KIT_TRACK_DRUMS, 24, 1, 38, 100);   /* snare D */
    kit_song_add(&song, KIT_TRACK_DRUMS, 48, 1, 36, 100);
    memset(f, 0, sizeof f);
    for (i = 0; i < 12; i++) f[i].swing = 50;
    f[0].shift_ms = -4; f[2].shift_ms = 32; f[2].loose_ms = 6;
    kit_seq_grid(KIT_TRACK_DRUMS, 6);
    kit_seq_feel(KIT_TRACK_DRUMS, f);
    now = 20000; non = 0;
    kit_seq_play(&song); run(4000); kit_seq_stop();
    CHECK(near(when(38, 0), 20000 + 500 + 32, 7) && when(38, 0) - 20000 - 500 == when(38, 1) - 22000 - 500, "the snare late by its shift, loose the same every loop (%ld %ld)", when(38, 0), when(38, 1));
    CHECK(near(when(36, 0), 20000 + 1000 - 4, 1), "the kick 4 ms early (%ld)", when(36, 0));
    CHECK(near(when(36, 1), 20000 + 2000 - 4, 1), "the kick on the loop's start pushed early plays at the end of the loop before (%ld)", when(36, 1));
    CHECK(kit_seq_place(&song, KIT_TRACK_DRUMS, 1) / 16 == 24 + 1, "kit_seq_place: the snare drawn a tick and a half late (%d)", kit_seq_place(&song, KIT_TRACK_DRUMS, 1));
    /* API 46 (R-371): with a record grid, a note lands on the nearest grid place as it is recorded; without, as played */
    kit_song_clear(&song, KIT_TRACK_MELODY);
    kit_seq_rec_grid(KIT_TRACK_MELODY, 6);
    kit_seq_record(KIT_TRACK_MELODY);
    now = 30000; non = 0;
    kit_seq_play(&song); run(146);              /* 7 ticks in: a tick after the 2nd sixteenth */
    kit_seq_note_on(KIT_TRACK_MELODY, 60, 100); run(100); kit_seq_note_off(KIT_TRACK_MELODY, 60);
    kit_seq_rec_grid(KIT_TRACK_MELODY, 0);
    run(250);                                   /* 23 ticks in (496 ms): one before the beat */
    kit_seq_note_on(KIT_TRACK_MELODY, 62, 100); run(50); kit_seq_note_off(KIT_TRACK_MELODY, 62);
    kit_seq_stop(); kit_seq_record(-1);
    CHECK(song.count[KIT_TRACK_MELODY] == 2 && song.notes[KIT_TRACK_MELODY][0].start == 6 && song.notes[KIT_TRACK_MELODY][1].start == 23,
          "a record grid places the note as it is recorded, none keeps it as played (%d: %d %d)", song.count[KIT_TRACK_MELODY],
          song.notes[KIT_TRACK_MELODY][0].start, song.notes[KIT_TRACK_MELODY][1].start);
    printf("%s: seq grid and feel, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
