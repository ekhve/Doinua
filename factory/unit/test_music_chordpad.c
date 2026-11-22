/* SPDX-License-Identifier: GPL-3.0-only */
/* The chord pad's music (apps/companion/pad.c, D-034) on the real theory (music.c, which the oracle checks against
 * Sketchpad): the chords of the pads in a key, a progression's colours, NEXT, a held colour, a locked colour, the
 * voicings, and which pad a recorded chord lights. */
#include <stdio.h>
#include <string.h>
#include "../../apps/companion/pad.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static const char *label(const pad_t *p)
{
    static char s[24];
    kit_chord_label(&p->now, p->tonic, p->minor, s, sizeof s);
    return s;
}

static int notes_are(const kit_vchord_t *c, int n, const int *want)
{
    int i;
    if (c->n != n) return 0;
    for (i = 0; i < n; i++) if (c->notes[i] != want[i]) return 0;
    return 1;
}

static kit_song_t song;

static void add_chord(int start, int len, const int *notes, int n)   /* in start order, as the recorder keeps them */
{
    int i;
    for (i = 0; i < n; i++) {
        kit_song_note_t *x = &song.notes[KIT_TRACK_CHORDS][song.count[KIT_TRACK_CHORDS]++];
        x->start = (uint16_t)start; x->len = (uint16_t)len; x->note = (uint8_t)notes[i]; x->vel = 100;
    }
}

int main(void)
{
    pad_t p;
    int step, d, i;
    static const int dm[3] = { 50, 53, 57 }, g_up[3] = { 67, 71, 74 };

    /* CP-R1: a pad sounds its degree's chord in the key */
    pad_init(&p);
    p.auto_on = 0;
    pad_play(&p, 1, -1, 0, -1, 0);
    CHECK(notes_are(&p.now, 3, dm) && !strcmp(label(&p), "Dm"), "C major, pad ii: Dm D3 F3 A3 (%s)", label(&p));
    p.tonic = 7;
    pad_play(&p, 0, -1, 1, -1, 0);
    CHECK(notes_are(&p.now, 3, g_up) && !strcmp(label(&p), "G"), "G major, upper pad I: G4 B4 D5 (%s)", label(&p));
    p.tonic = 9; p.minor = 1;
    pad_play(&p, 0, -1, 0, -1, 0);
    CHECK(!strcmp(label(&p), "Am"), "A minor, pad i: Am (%s)", label(&p));
    pad_play(&p, 6, -1, 0, -1, 0);
    CHECK(!strcmp(label(&p), "G"), "A minor, pad VII: G (%s)", label(&p));

    /* CP-R2 and CP-R3: Neo soul ii-V-I gives each chord its colour; NEXT walks it, round the loop */
    pad_init(&p);
    {
        static const char *const want[4] = { "Dm9", "G13", "Cmaj7", "Dm9" };
        for (i = 0; i < 4; i++) {
            d = pad_next_degree(&p, &step);
            pad_play(&p, d, step, 0, -1, 0);
            CHECK(!strcmp(label(&p), want[i]), "NEXT %d: %s, not %s", i + 1, want[i], label(&p));
        }
    }
    d = pad_next_degree(&p, &step);
    CHECK(d == 4 && step == 1, "after Dm9 the next is G13 (degree %d step %d)", d, step);
    /* pressing a pad of the progression picks it up from there */
    pad_play(&p, 0, pad_step_of(&p, 0), 0, -1, 0);
    CHECK(!strcmp(label(&p), "Cmaj7") && pad_next_degree(&p, &step) == 1, "the I pad is Cmaj7, then ii is next (%s)", label(&p));
    /* a pad outside the progression: plain, and the progression waits */
    pad_play(&p, 5, pad_step_of(&p, 5), 0, -1, 0);
    CHECK(!strcmp(label(&p), "Am") && pad_next_degree(&p, &step) == 1, "vi outside ii-V-I: Am, ii still next (%s)", label(&p));
    /* AUTO off: the style's colours off */
    p.auto_on = 0;
    pad_play(&p, 4, pad_step_of(&p, 4), 0, -1, 0);
    CHECK(!strcmp(label(&p), "G"), "AUTO off: V is G (%s)", label(&p));

    /* Free: NEXT is Sketchpad's likely next chord */
    pad_init(&p);
    p.style = 0;
    d = pad_next_degree(&p, &step);
    CHECK(d == 0 && step == -1, "Free with nothing played: I");
    pad_play(&p, 0, -1, 0, -1, 0);
    CHECK(pad_next_degree(&p, &step) == 3, "Free after I: IV");
    pad_play(&p, 1, -1, 0, -1, 0);
    CHECK(pad_next_degree(&p, &step) == 4, "Free after ii: V");

    /* CP-R5: a held colour; CP-R6: a locked colour, until unlocked; a held one still wins */
    pad_init(&p);
    pad_play(&p, 2, pad_step_of(&p, 2), 0, KIT_COL_13, 0);
    CHECK(!strcmp(label(&p), "Em13"), "13 held on iii: Em13 (%s)", label(&p));
    pad_play(&p, 1, pad_step_of(&p, 1), 0, KIT_COL_7, 0);
    CHECK(!strcmp(label(&p), "Dm7"), "7 held on ii, over the progression's 9: Dm7 (%s)", label(&p));
    p.locked = KIT_COL_9;
    pad_play(&p, 0, pad_step_of(&p, 0), 0, -1, 0);
    CHECK(!strcmp(label(&p), "C9"), "9 locked: I is C9 (%s)", label(&p));
    pad_play(&p, 5, pad_step_of(&p, 5), 0, -1, 0);
    CHECK(!strcmp(label(&p), "Am9"), "9 locked: vi is Am9 (%s)", label(&p));
    pad_play(&p, 5, -1, 0, KIT_COL_SUS4, 0);
    CHECK(!strcmp(label(&p), "Asus4"), "sus4 held over the lock: Asus4 (%s)", label(&p));
    p.locked = -1;
    pad_play(&p, 3, -1, 0, -1, 1);
    CHECK(!strcmp(label(&p), "F/A") && p.now.notes[0] % 12 == 9 && p.now.notes[0] < 53, "the 3rd in the bass: F/A, A below (%s)", label(&p));
    pad_play(&p, 1, -1, 0, KIT_COL_11, 0);
    CHECK(!strcmp(label(&p), "Dm11") && p.now.n == 6, "every note of a big chord: Dm11 has six (%d)", p.now.n);

    /* K3: the voicings keep the chord's notes */
    pad_init(&p);
    p.auto_on = 0;
    for (p.voicing = 0; p.voicing < KIT_NVOICINGS; p.voicing++) {
        uint16_t pcs = 0;
        pad_play(&p, 0, -1, 0, KIT_COL_MAJ7, 0);
        for (i = 0; i < p.now.n; i++) pcs |= (uint16_t)(1u << (p.now.notes[i] % 12));
        CHECK(p.now.n == 4 && pcs == ((1u << 0) | (1u << 4) | (1u << 7) | (1u << 11)), "voicing %d: Cmaj7's four notes", p.voicing);
    }

    /* CP-R11: which pad a recorded chord lights, at its time */
    memset(&song, 0, sizeof song);
    song.bars = 4;
    {
        static const int dm9[5] = { 50, 53, 57, 60, 64 }, g13[6] = { 55, 59, 62, 65, 69, 76 }, cmaj7_e[4] = { 52, 55, 59, 60 },
                         am_add9[4] = { 57, 60, 64, 71 };
        add_chord(10, 80, dm9, 5);                /* played a little late, held almost a bar */
        add_chord(96, 96, g13, 6);
        add_chord(192, 90, cmaj7_e, 4);           /* E in the bass */
        add_chord(300, 120, am_add9, 4);          /* from bar 4 across the loop's end (384) */
    }
    CHECK(pad_recorded_degree(&song, 5, 0, 0) == 5, "tick 5: the Am(add9) held across the loop's end (%d)", pad_recorded_degree(&song, 5, 0, 0));
    CHECK(pad_recorded_degree(&song, 40, 0, 0) == 1, "tick 40: Dm9 lights ii, not IV (%d)", pad_recorded_degree(&song, 40, 0, 0));
    CHECK(pad_recorded_degree(&song, 95, 0, 0) == -1, "tick 95: nothing sounds (%d)", pad_recorded_degree(&song, 95, 0, 0));
    CHECK(pad_recorded_degree(&song, 150, 0, 0) == 4, "tick 150: G13 lights V (%d)", pad_recorded_degree(&song, 150, 0, 0));
    CHECK(pad_recorded_degree(&song, 200, 0, 0) == 0, "tick 200: Cmaj7 over E lights I (%d)", pad_recorded_degree(&song, 200, 0, 0));
    CHECK(pad_recorded_degree(&song, -1, 0, 0) == -1, "stopped: nothing");

    printf("%s: chord pad, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
