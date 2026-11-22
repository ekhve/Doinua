/* SPDX-License-Identifier: GPL-3.0-only */
/* seq.c's song settings (D-032), with the real key analysis (music.c, which the oracle checks against Sketchpad):
 *   - AUTO: the key from the bass, melody and chords (not the drums), and whether it is ambiguous
 *   - locking and releasing the key leave every note as it was; transposing moves the notes (not the drums') and the key
 *   - a new tempo while playing goes on from the same tick; a shorter song keeps the notes beyond its end, unplayed */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"
#include "../../fm1kit/src/music.c"     /* the real kit_keys_for_notes and kit_tempo_clamp */

static uint32_t now;
uint32_t kit_ms(void) { return now; }
static int last_on = -1;
static uint32_t last_on_t;
void kit_track_note_on(int t, int n, int v) { (void)t; (void)v; last_on = n; last_on_t = now; }
void kit_track_note_off(int t, int n) { (void)t; (void)n; }
void kit_click(int a, int l) { (void)a; (void)l; }
int kit__bg;
void kit__midi_send_rt(uint8_t b) { (void)b; }
void app_event(const kit_event_t *e) { (void)e; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static kit_song_t s, copy;

int main(void)
{
    int t, m, sure;
    kit_song_init(&s, 100, 2);
    sure = kit_song_key(&s, &t, &m);
    CHECK(!sure, "an empty song's AUTO key is not sure");
    kit_song_add(&s, KIT_TRACK_BASS, 0, 24, 48, 100);              /* C F C: C major or C minor */
    kit_song_add(&s, KIT_TRACK_BASS, 24, 24, 53, 100);
    kit_song_add(&s, KIT_TRACK_BASS, 48, 24, 48, 100);
    kit_song_add(&s, KIT_TRACK_DRUMS, 0, 4, 37, 100);               /* a drum on C#: not a note of the key */
    sure = kit_song_key(&s, &t, &m);
    CHECK(!sure && t == 0 && m == 0, "C F C on the bass: C major first, but ambiguous (%d %d %d)", sure, t, m);
    kit_song_add(&s, KIT_TRACK_MELODY, 72, 12, 64, 100);            /* E and B: major */
    kit_song_add(&s, KIT_TRACK_MELODY, 84, 12, 71, 100);
    kit_song_add(&s, KIT_TRACK_BASS, 96, 48, 48, 100);
    sure = kit_song_key(&s, &t, &m);
    CHECK(sure && t == 0 && m == 0, "with E and B from the melody: C major, sure (%d %d %d)", sure, t, m);

    memcpy(&copy, &s, sizeof s);
    kit_song_lock_key(&s, 9, 1);
    sure = kit_song_key(&s, &t, &m);
    CHECK(sure && t == 9 && m == 1 && !s.key_auto, "locked to A minor, it says A minor");
    CHECK(!memcmp(s.notes, copy.notes, sizeof s.notes) && !memcmp(s.count, copy.count, sizeof s.count), "locking changed no note");
    kit_song_auto_key(&s);
    CHECK(!memcmp(s.notes, copy.notes, sizeof s.notes) && s.key_auto, "releasing it changed no note either");

    kit_song_lock_key(&s, 0, 0);
    kit_song_transpose(&s, 2);
    CHECK(s.notes[KIT_TRACK_BASS][0].note == 50 && s.notes[KIT_TRACK_MELODY][0].note == 66 && s.key_tonic == 2,
          "transposing moves the notes and the locked key up a tone");
    CHECK(s.notes[KIT_TRACK_DRUMS][0].note == 37, "but not the drums (a drum's pitch class is which drum)");
    kit_song_transpose(&s, -2);

    kit_song_set_bars(&s, 1);
    CHECK(s.count[KIT_TRACK_BASS] == 4 && kit_song_length(&s) == 96, "one bar: the bass note in bar 2 is kept");
    kit_song_auto_key(&s);
    sure = kit_song_key(&s, &t, &m);
    now = 1000;
    last_on = -1;
    kit_seq_play(&s);
    for (; now < 1000 + 3 * 2400; now++) kit__seq_poll(now);
    CHECK(last_on != 48 || last_on_t % 2400 < 600, "the note beyond the end is not played");
    kit_seq_stop();
    kit_song_set_bars(&s, 2);
    CHECK(s.notes[KIT_TRACK_BASS][3].start == 96, "two bars again: it is back");

    kit_song_init(&s, 60, 2);                                       /* a beat a second */
    kit_song_add(&s, KIT_TRACK_BASS, 48, 4, 40, 100);              /* beat 3 */
    now = 10000;
    kit_seq_play(&s);
    for (; now < 11000; now++) kit__seq_poll(now);                  /* tick 24: beat 2 */
    kit_song_set_tempo(&s, 120);                                    /* from here a beat is 500 ms */
    last_on = -1;
    for (; now < 13000; now++) kit__seq_poll(now);
    CHECK(last_on == 40 && last_on_t >= 11495 && last_on_t <= 11505, "after a tempo change it goes on from the same tick (beat 3 at %u, want 11500)", last_on_t);
    kit_seq_stop();

    printf("%s: song settings, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
