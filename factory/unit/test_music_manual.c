/* SPDX-License-Identifier: GPL-3.0-only */
/* Manual chords' music (apps/companion/manual.c, D-036) on the real theory (music.c, checked by the oracle): K1's order
 * (the key's chords with their types, then the roots outside the key), where the chord sits on the keys, SELECT's ideas
 * (the pad's progression, or I-V-vi-IV), and what the chords track has at a tick and in each bar. */
#include <stdio.h>
#include <string.h>
#include "../../apps/companion/pad.c"
#include "../../apps/companion/manual.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static const char *label(const man_t *m, int tonic, int minor)
{
    static char s[24];
    kit_vchord_t c;
    man_chord(m, tonic, minor, 53, &c);
    kit_chord_label(&c, tonic, minor, s, sizeof s);
    return s;
}

static kit_song_t song;
static void add(int start, int len, int note)
{
    kit_song_note_t *x = &song.notes[KIT_TRACK_CHORDS][song.count[KIT_TRACK_CHORDS]++];
    x->start = (uint16_t)start; x->len = (uint16_t)len; x->note = (uint8_t)note; x->vel = 100;
}

int main(void)
{
    man_t m;
    pad_t p;
    kit_vchord_t c;
    uint8_t nt[16];
    int i, n;
    static const char *const c_major[12] = { "C", "Dm", "Em", "F", "G", "Am", "Bdim", "Dbdim", "Ebdim", "F#dim", "Abdim", "Bbdim" };
    static const int outside_am[5] = { 10, 1, 3, 6, 8 };

    /* MC-R2: K1 steps through the key's seven chords, with their types, then the five roots outside it (type kept) */
    man_init(&m);
    for (i = 0; i < 12; i++) {
        man_set_root(&m, i, 0, 0);
        CHECK(!strcmp(label(&m, 0, 0), c_major[i]), "C major, K1 %d: %s, not %s", i, c_major[i], label(&m, 0, 0));
        CHECK(man_outside(&m) == (i >= 7), "K1 %d outside the key: %d", i, man_outside(&m));
    }
    man_set_root(&m, 12, 0, 0);
    CHECK(m.root_i == 0 && !strcmp(label(&m, 0, 0), "C"), "K1 goes round to I");
    for (i = 0; i < 5; i++) {
        man_set_root(&m, 7 + i, 9, 1);
        CHECK(man_root_pc(&m, 9, 1) == outside_am[i], "A minor, outside root %d: pitch class %d, not %d", i, outside_am[i], man_root_pc(&m, 9, 1));
    }
    man_set_root(&m, 0, 9, 1);
    CHECK(!strcmp(label(&m, 9, 1), "Am"), "A minor, K1 0: Am (%s)", label(&m, 9, 1));

    /* K2, K3: the type and the colour; MC-R1: the chord on the keys, its root the first of its name from F3 */
    man_init(&m);
    man_set_root(&m, 0, 0, 0);
    m.colour = 2;                                               /* maj7 */
    man_chord(&m, 0, 0, 53, &c);
    CHECK(c.n == 4 && c.notes[0] == 60 && c.notes[3] == 71 && !strcmp(label(&m, 0, 0), "Cmaj7"), "Cmaj7 on C4 E4 G4 B4 (%s, %d..%d)", label(&m, 0, 0), c.notes[0], c.notes[3]);
    man_set_root(&m, 3, 0, 0);
    man_chord(&m, 0, 0, 53, &c);
    CHECK(c.notes[0] == 53, "F's root is F3, the lowest key (%d)", c.notes[0]);
    man_chord(&m, 0, 0, 65, &c);
    CHECK(c.notes[0] == 65, "an octave up (OCT+): F4 (%d)", c.notes[0]);
    m.type = 3; m.colour = 1;                                   /* sus4, 7 */
    CHECK(!strcmp(label(&m, 0, 0), "F7sus4"), "F, sus4, 7: F7sus4 (%s)", label(&m, 0, 0));
    m.type = 2; m.colour = 0;
    CHECK(!strcmp(label(&m, 0, 0), "Fsus2"), "F sus2 (%s)", label(&m, 0, 0));

    /* MC-R8: SELECT walks the pad's progression (Neo soul ii-V-I), with its colours, both ways and round */
    pad_init(&p);
    man_init(&m);
    {
        static const char *const want[5] = { "Dm9", "G13", "Cmaj7", "Dm9" };
        for (i = 0; i < 4; i++) {
            man_walk(&m, &p, 1, 0, 0);
            CHECK(!strcmp(label(&m, 0, 0), want[i]), "SELECT > %d: %s, not %s", i + 1, want[i], label(&m, 0, 0));
        }
        man_walk(&m, &p, -1, 0, 0);
        CHECK(!strcmp(label(&m, 0, 0), "Cmaj7"), "SELECT <: back to Cmaj7 (%s)", label(&m, 0, 0));
    }
    man_next_idea(&m, &p, 0, 0, 53, &c);
    CHECK(c.root == 2, "the next idea after Cmaj7 is Dm9 (root %d)", c.root);
    /* no progression on the pad (Free): I-V-vi-IV in the key */
    p.style = 0;
    man_init(&m);
    {
        static const char *const want[4] = { "G", "D", "Em", "C" };
        for (i = 0; i < 4; i++) {
            man_walk(&m, &p, 1, 7, 0);
            CHECK(!strcmp(label(&m, 7, 0), want[i]), "Free, G major, SELECT > %d: %s, not %s", i + 1, want[i], label(&m, 7, 0));
        }
    }
    /* a progression's sus colour becomes the type */
    p.style = 6; p.prog = 0;                                     /* Electronic vi-IV-I-V: Am(add9) F(add9) Csus2 Gsus4 */
    man_init(&m);
    for (i = 0; i < 3; i++) man_walk(&m, &p, 1, 0, 0);
    CHECK(!strcmp(label(&m, 0, 0), "Csus2") && m.colour == 0, "Electronic step 3: Csus2 (%s)", label(&m, 0, 0));
    /* another progression chosen on the pad: SELECT starts at its first chord */
    p.prog = 1;                                                  /* Electronic I-V-vi-IV: Cadd9 first */
    man_walk(&m, &p, 1, 0, 0);
    CHECK(!strcmp(label(&m, 0, 0), "Cadd9"), "a new progression starts at its first chord: Cadd9 (%s)", label(&m, 0, 0));

    /* MC-R7, MC-R9: what the chords track has at a tick, and the first chord of each bar */
    memset(&song, 0, sizeof song);
    song.bars = 4;
    add(4, 90, 57); add(5, 90, 60); add(6, 90, 64);            /* Am, played a little late */
    add(130, 40, 53); add(131, 40, 57); add(132, 40, 60);      /* F in bar 2, then G later in the same bar */
    add(170, 20, 55); add(171, 20, 59); add(172, 20, 62);
    add(370, 40, 48);                                          /* a C held across the loop's end */
    n = song_notes_at(&song, 50, nt, 16);
    CHECK(n == 3 && nt[0] == 57 && nt[2] == 64, "tick 50: A C E (%d notes)", n);
    n = song_notes_at(&song, 10, nt, 16);
    CHECK(n == 4, "tick 10: A C E and the C from the loop's end (%d notes)", n);
    CHECK(song_notes_at(&song, 100, nt, 16) == 0, "tick 100: nothing");
    n = song_bar_chord(&song, 1, nt, 16);
    CHECK(n == 3 && nt[0] == 53, "bar 2: its first chord, F (%d notes, %d)", n, nt[0]);
    CHECK(song_bar_chord(&song, 2, nt, 16) == 0, "bar 3: empty");

    (void)pad_recorded_degree; (void)pad_next_degree; (void)pad_play; (void)pad_step_of;   /* the pad's, tested elsewhere */
    printf("%s: manual chords, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
