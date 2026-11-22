/* SPDX-License-Identifier: GPL-3.0-only */
/* seq.c (D-031), on a simulated clock:
 *   - a song's notes start and stop at their ticks on their tracks, looping; bar events at each bar
 *   - no drift: at 97 BPM (a loop of 4948.45 ms), polled every 7 ms, loop 1000 starts within one poll of its time
 *   - recording keeps a note's tick and length as played (no quantising), also when held over the loop's end,
 *     and the next loop plays it back at the same time
 *   - stopping silences what sounds; songs keep notes by start, refuse beyond KIT_SONG_MAX_NOTES
 *   - MIDI clock (D-044): out, Start then a clock a tick (24 a beat) and Stop, none while following; in, following
 *     moves a tick a clock whatever the time, waits without clocks, and Continue resumes where it stopped
 *   - the metronome (D-045): a click a beat, the bar's first accented; the count-in: a bar of clicks, then the song
 *     from the top on the next beat, and STOP cancels it */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

static uint32_t now;
uint32_t kit_ms(void) { return now; }
int kit_keys_for_notes(const kit_note_ev_t *e, int n, kit_key_cand_t *o, int l, int *a, uint16_t *h) { (void)e; (void)n; (void)o; (void)l; *a = 1; *h = 0; return 0; }
int kit_tempo_clamp(int b) { return b < KIT_TEMPO_MIN ? KIT_TEMPO_MIN : b > KIT_TEMPO_MAX ? KIT_TEMPO_MAX : b; }
static struct { uint32_t t; int on, track, note; } lg[20000];
static int nlg, sounding;
void kit_track_note_on(int t, int n, int v) { (void)v; if (nlg < 20000) { lg[nlg].t = now; lg[nlg].on = 1; lg[nlg].track = t; lg[nlg++].note = n; } sounding++; }
void kit_track_note_off(int t, int n) { if (nlg < 20000) { lg[nlg].t = now; lg[nlg].on = 0; lg[nlg].track = t; lg[nlg++].note = n; } sounding--; }
static int nfa, nfb, nfc, nf8;
static int nclick, naccent;
void kit_click(int a, int l) { (void)l; nclick++; naccent += a != 0; }
int kit__bg;
void kit__midi_send_rt(uint8_t b) { if (b == 0xFA) nfa++; else if (b == 0xFB) nfb++; else if (b == 0xFC) nfc++; else if (b == 0xF8) nf8++; }
static int bars[64], nbars;
void app_event(const kit_event_t *e) { if (e->type == KIT_EV_SEQ_BAR && nbars < 64) bars[nbars++] = e->id + (e->delta ? 100 : 0); }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static void run(uint32_t ms, uint32_t every) { uint32_t end = now + ms; while (now < end) { now += every; kit__seq_poll(now); } }
static long find(int on, int note, int k)        /* the time of the k-th (0..) on/off of note, or -1 */
{
    int i;
    for (i = 0; i < nlg; i++) if (lg[i].on == on && lg[i].note == note && k-- == 0) return (long)lg[i].t;
    return -1;
}
static int near(long a, long b, long tol) { return a >= 0 && (a - b <= tol && b - a <= tol); }
static kit_song_t song;

int main(void)
{
    static const uint8_t c[3] = { 60, 64, 67 };
    int i, t0;
    kit_song_init(&song, 120, 2);                       /* 500 ms a beat, 4000 ms a loop */
    CHECK(kit_song_length(&song) == 192 && song.key_auto == 1, "two bars of 4/4 are 192 ticks, the key AUTO");
    kit_song_add(&song, KIT_TRACK_BASS, 0, 24, 45, 100);
    kit_song_add(&song, KIT_TRACK_MELODY, 24, 12, 76, 100);
    kit_song_add_chord(&song, KIT_TRACK_CHORDS, 1, c, 3, 90);
    kit_song_add(&song, KIT_TRACK_DRUMS, 120, 1, 36, 100);
    kit_song_add(&song, KIT_TRACK_DRUMS, 48, 1, 42, 100);
    CHECK(song.notes[KIT_TRACK_DRUMS][0].start == 48 && song.notes[KIT_TRACK_DRUMS][1].start == 120, "notes are kept by start");

    now = 1000;
    kit_seq_play(&song);
    run(12000, 1);
    CHECK(near(find(1, 45, 0), 1000, 1) && near(find(1, 45, 1), 5000, 1) && near(find(1, 45, 2), 9000, 1), "the bass starts each loop (%ld %ld %ld)", find(1, 45, 0), find(1, 45, 1), find(1, 45, 2));
    CHECK(near(find(0, 45, 0), 1500, 1), "and stops after its beat (%ld)", find(0, 45, 0));
    CHECK(near(find(1, 76, 0), 1500, 1) && near(find(0, 76, 0), 1750, 1), "the melody note on its tick, half a beat long (%ld %ld)", find(1, 76, 0), find(0, 76, 0));
    CHECK(near(find(1, 64, 0), 3000, 1) && near(find(0, 64, 0), 5000, 1), "the step-entered chord fills bar 2 (%ld %ld)", find(1, 64, 0), find(0, 64, 0));
    CHECK(near(find(1, 42, 0), 2000, 1) && near(find(1, 36, 0), 3500, 1), "the drums on their beats");
    CHECK(nbars >= 4 && bars[0] == 100 && bars[1] == 1 && bars[2] == 100 && bars[3] == 1, "bar events: 0 (the loop's start), 1, 0, 1");
    kit_seq_stop();
    CHECK(sounding == 0 && !kit_seq_playing(), "stopping silences what sounds (%d left)", sounding);

    nlg = 0;                                            /* no drift */
    kit_song_init(&song, 97, 2);
    kit_song_add(&song, KIT_TRACK_BASS, 0, 6, 40, 100);
    now = 10000;
    kit_seq_play(&song);
    run(1001u * 4949u, 7);
    {
        long want = 10000 + (long)(1000.0 * 8 * 60000 / 97), got = find(1, 40, 1000);
        CHECK(near(got, want, 8), "loop 1000 at 97 BPM starts within one poll of its time (%ld, want %ld)", got, want);
    }
    kit_seq_stop();

    nlg = 0;                                            /* recording */
    kit_song_init(&song, 120, 2);
    now = 20000;
    kit_seq_play(&song);
    kit_seq_record(KIT_TRACK_BASS);
    run(625, 1);                                        /* tick 30 */
    kit_seq_note_on(KIT_TRACK_BASS, 50, 99);
    run(500, 1);                                        /* tick 54 */
    kit_seq_note_off(KIT_TRACK_BASS, 50);
    CHECK(song.count[KIT_TRACK_BASS] == 1 && song.notes[1][0].start == 30 && song.notes[1][0].len == 24 && song.notes[1][0].vel == 99,
          "recorded as played: tick 30, 24 ticks (%d, %d)", song.notes[1][0].start, song.notes[1][0].len);
    run(4000 - 1125 + 625 + 10, 1);
    CHECK(near(find(1, 50, 1), 24000 + 625, 1), "played back on the next loop at the same time (%ld)", find(1, 50, 1));
    t0 = (int)(now - 20000) % 4000;
    run(4000 - t0 + 3750, 1);                           /* tick 180 of a loop */
    kit_seq_note_on(KIT_TRACK_BASS, 52, 80);
    run(500, 1);                                        /* tick 12 of the next */
    kit_seq_note_off(KIT_TRACK_BASS, 52);
    for (i = 0; i < song.count[1] && song.notes[1][i].note != 52; i++) ;
    CHECK(i < song.count[1] && song.notes[1][i].start == 180 && song.notes[1][i].len == 24, "held over the loop's end: tick 180, 24 ticks (%d, %d)",
          i < song.count[1] ? song.notes[1][i].start : -1, i < song.count[1] ? song.notes[1][i].len : -1);
    kit_seq_record(-1);
    kit_seq_note_on(KIT_TRACK_BASS, 55, 80);
    kit_seq_note_off(KIT_TRACK_BASS, 55);
    CHECK(song.count[1] == 2, "nothing is recorded when not armed");
    kit_seq_stop();

    kit_song_init(&song, 100, 8);
    for (i = 0; i < KIT_SONG_MAX_NOTES; i++) kit_song_add(&song, 0, (i * 5) % kit_song_length(&song), 4, 60, 100);
    CHECK(kit_song_add(&song, 0, 0, 4, 61, 100) == -1 && song.count[0] == KIT_SONG_MAX_NOTES, "a full track refuses more");
    for (i = 1; i < song.count[0]; i++) if (song.notes[0][i - 1].start > song.notes[0][i].start) break;
    CHECK(i == song.count[0], "and keeps them by start");
    /* MIDI clock out: Start, a clock a tick for 2 loops (2 x 192), Stop */
    kit_song_init(&song, 120, 2);
    kit_song_add(&song, 0, 0, 24, 60, 100);
    kit_seq_clock_out(1);
    nfa = nfb = nfc = nf8 = 0;
    kit_seq_play(&song);
    run(8000 - 7, 7);                                   /* two loops of 4000 ms, polled every 7 ms */
    CHECK(nfa == 1 && nf8 >= 383 && nf8 <= 385, "clock out: Start and 384 clocks in two loops (%d %d)", nfa, nf8);
    kit_seq_stop();
    CHECK(nfc == 1, "clock out: Stop when stopped (%d)", nfc);
    /* MIDI clock in: following, a tick a clock; time alone moves nothing */
    nlg = 0; nf8 = 0;
    kit_seq_follow(1);
    kit_seq_play(&song);
    CHECK(find(1, 60, 0) >= 0, "following: Start plays tick 0");
    run(5000, 7);
    CHECK(find(0, 60, 0) < 0 && kit_seq_tick() == 0, "following: with no clock the song waits (tick %d)", kit_seq_tick());
    for (i = 0; i < 24; i++) { now += 3; kit__seq_clock(now); }
    CHECK(kit_seq_tick() == 24 && find(0, 60, 0) >= 0, "following: 24 clocks are 24 ticks, a beat, at any pace (tick %d)", kit_seq_tick());
    for (i = 0; i < 168; i++) { now += 3; kit__seq_clock(now); }
    CHECK(find(1, 60, 1) >= 0 && kit_seq_tick() == 0, "following: 192 clocks go round the loop, its first note again");
    CHECK(nf8 == 0, "following: no clock goes out (%d)", nf8);
    for (i = 0; i < 30; i++) { now += 25; kit__seq_clock(now); }
    kit_seq_stop();
    kit_seq_resume(&song);
    CHECK(kit_seq_tick() == 30, "Continue resumes where it stopped (tick %d)", kit_seq_tick());
    CHECK(kit_seq_clock_bpm() == 100, "the clock in's tempo: 25 ms a clock is 100 BPM (%d)", kit_seq_clock_bpm());
    kit_seq_stop();
    kit_seq_follow(0);
    kit_seq_clock_out(0);
    /* the metronome: two loops of two bars at 120 BPM, 16 beats, 4 bars */
    kit_song_init(&song, 120, 2);
    nclick = naccent = 0;
    kit_seq_metronome(5);
    kit_seq_play(&song);
    run(8000 - 7, 7);
    CHECK(nclick == 16 && naccent == 4, "metronome: 16 clicks, 4 accented, in 4 bars (%d %d)", nclick, naccent);
    kit_seq_stop();
    kit_seq_metronome(0);
    /* the count-in: a bar of clicks, then the song on the next beat */
    kit_song_add(&song, 0, 0, 24, 62, 100);
    nclick = naccent = 0; nlg = 0;
    t0 = (int)now;
    kit_seq_count_in(&song, 1, 5);
    CHECK(kit_seq_counting() == 3 && !kit_seq_playing() && nclick == 1, "count-in: the first click at once, three to come (%d)", kit_seq_counting());
    run(1990, 1);
    CHECK(nclick == 4 && naccent == 1 && !kit_seq_playing() && find(1, 62, 0) < 0, "count-in: four clicks, the first accented, nothing played yet");
    run(20, 1);
    CHECK(kit_seq_playing() && near(find(1, 62, 0), t0 + 2000, 2), "count-in: the song starts on the fifth beat (%ld)", find(1, 62, 0) - t0);
    kit_seq_stop();
    kit_seq_count_in(&song, 2, 5);
    kit_seq_stop();
    CHECK(kit_seq_counting() == 0, "STOP cancels a count-in");
    run(5000, 7);
    CHECK(!kit_seq_playing(), "and the song does not start");
    {                                           /* the chain (API 29): one loop of a, then b from its top, b loops */
        static kit_song_t a, b;
        kit_song_init(&a, 120, 1);                  /* 2000 ms a loop */
        kit_song_init(&b, 120, 1);
        kit_song_add(&a, KIT_TRACK_MELODY, 0, 12, 61, 100);
        kit_song_add(&b, KIT_TRACK_MELODY, 0, 12, 66, 100);
        nlg = 0;
        t0 = (int)now;
        kit_seq_play(&a);
        kit_seq_next(&b);
        CHECK(kit_seq_song() == &a, "next: a plays first");
        run(5000, 7);
        CHECK(near(find(1, 61, 0), t0, 7) && find(1, 61, 1) < 0, "next: a once (%ld, %ld)", find(1, 61, 0) - t0, find(1, 61, 1));
        CHECK(near(find(1, 66, 0), t0 + 2000, 7) && near(find(1, 66, 1), t0 + 4000, 7), "next: b on a's end, then b loops (%ld %ld)", find(1, 66, 0) - t0, find(1, 66, 1) - t0);
        CHECK(kit_seq_song() == &b, "next: b is the song playing");
        kit_seq_next(&a);
        kit_seq_stop();
        kit_seq_play(&b);
        nlg = 0;
        run(4100, 7);
        CHECK(find(1, 61, 0) < 0, "stop cancels a next");
        kit_seq_stop();
        CHECK(kit_seq_song() == 0, "stopped: no song playing");
        kit_song_init(&a, 120, 2);              /* a of 2 bars: b comes in on the next bar line, not a's end */
        kit_song_add(&a, KIT_TRACK_MELODY, 96, 12, 62, 100);
        nlg = 0;
        t0 = (int)now;
        kit_seq_play(&a);
        run(500, 7);
        kit_seq_next(&b);
        run(2600, 7);
        CHECK(near(find(1, 66, 0), t0 + 2000, 7) && find(1, 62, 0) < 0, "next: b on the next bar line (%ld), a's second bar never played", find(1, 66, 0) - t0);
        kit_seq_stop();
    }
    {                                           /* QUANTIZE with STRENGTH, 1/64, and the beat-1 wrap (API 30) */
        static kit_song_t q;
        kit_song_init(&q, 120, 1);
        kit_song_add(&q, KIT_TRACK_MELODY, 4, 6, 70, 100);
        kit_song_add(&q, KIT_TRACK_MELODY, 1, 1, 72, 100);
        kit_seq_quantize(KIT_TRACK_MELODY, 6 * 16, 50);
        CHECK(kit_seq_place(&q, KIT_TRACK_MELODY, 1) == 80, "strength 50: tick 4 halfway to the 1/16 at 6 (%d fine)", kit_seq_place(&q, KIT_TRACK_MELODY, 1));
        kit_seq_quantize(KIT_TRACK_MELODY, 6 * 16, 0);
        CHECK(kit_seq_place(&q, KIT_TRACK_MELODY, 1) == 64, "strength 0: as played (%d)", kit_seq_place(&q, KIT_TRACK_MELODY, 1));
        kit_seq_quantize(KIT_TRACK_MELODY, 24, 100);
        CHECK(kit_seq_place(&q, KIT_TRACK_MELODY, 0) == 24, "1/64 (1.5 ticks): tick 1 to 24 fine (%d)", kit_seq_place(&q, KIT_TRACK_MELODY, 0));
        kit_seq_quantize(KIT_TRACK_MELODY, 0, 100);
        CHECK(kit_seq_place(&q, KIT_TRACK_MELODY, 1) == 64, "off: as played");
        kit_song_init(&q, 120, 1);              /* 2000 ms a loop, 96 ticks: tick 94 is 1958 ms in */
        t0 = (int)now;
        kit_seq_play(&q);
        kit_seq_record(KIT_TRACK_MELODY);
        run(1960, 1);
        kit_seq_note_on(KIT_TRACK_MELODY, 74, 100);
        run(200, 1);
        kit_seq_note_off(KIT_TRACK_MELODY, 74);
        CHECK(q.count[KIT_TRACK_MELODY] == 1 && q.notes[KIT_TRACK_MELODY][0].start == 0, "a note just before the loop's end goes on its first step (%d)", q.notes[KIT_TRACK_MELODY][0].start);
        CHECK(q.notes[KIT_TRACK_MELODY][0].len >= 6 && q.notes[KIT_TRACK_MELODY][0].len <= 8, "with the length played after the loop began, 160 ms (%d)", q.notes[KIT_TRACK_MELODY][0].len);
        kit_seq_record(-1);
        kit_seq_stop();
    }
    printf("%s: seq, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
