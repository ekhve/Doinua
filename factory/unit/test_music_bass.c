/* SPDX-License-Identifier: GPL-3.0-only */
/* The bass track's music (apps/companion/bass.c, D-040) on the real theory (music.c, checked by the oracle): the scales
 * SCALE offers by the key's mode, what each note is (scale, root, outside), the white keys as the scale, a line's notes
 * kept inside the scale, and the chord the bass follows. */
#include <stdio.h>
#include <string.h>
#include "../../apps/companion/pad.c"
#include "../../apps/companion/manual.c"
#include "../../apps/companion/bass.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static kit_song_t song;
static void add(int start, int len, int note)
{
    kit_song_note_t *x = &song.notes[KIT_TRACK_CHORDS][song.count[KIT_TRACK_CHORDS]++];
    x->start = (uint16_t)start; x->len = (uint16_t)len; x->note = (uint8_t)note; x->vel = 100;
}

int main(void)
{
    bass_t b;
    int i, third, r;
    bass_note_t x;

    /* SCALE: the key's own first, then the rest for that mode */
    bass_init(&b);
    CHECK(!strcmp(bass_scale(&b, 1)->id, "natural-minor") && !strcmp(bass_scale(&b, 0)->id, "major"), "the key's own scale first");
    CHECK(bass_nscales(1) == 9 && bass_nscales(0) == 8, "nine scales for a minor key, eight for a major one");
    b.scale = 7;
    CHECK(!strcmp(bass_scale(&b, 1)->id, "hirajoshi"), "minor scale 8 is hirajoshi (%s)", bass_scale(&b, 1)->id);
    CHECK(!strcmp(bass_scale(&b, 0)->id, "whole-tone"), "major scale 8 is whole tone (%s)", bass_scale(&b, 0)->id);
    b.scale = 20;
    CHECK(!strcmp(bass_scale(&b, 0)->id, "whole-tone"), "an index past the end stays on the last");

    /* HELP: Key lights the scale and its tonic; Key+root the chord now's root over the scale */
    bass_init(&b);
    CHECK(bass_role(&b, 9, 1, 57, -1) == ROLE_TONIC, "A minor, Key: A is the tonic");
    CHECK(bass_role(&b, 9, 1, 60, -1) == ROLE_SCALE, "A minor, Key: C is in the scale");
    CHECK(bass_role(&b, 9, 1, 61, -1) == ROLE_OUT, "A minor, Key: C# is outside");
    b.help = HELP_ROOT;
    CHECK(bass_role(&b, 9, 1, 50, 2) == ROLE_ROOT, "Key+root over Dm: D is the root");
    CHECK(bass_role(&b, 9, 1, 57, 2) == ROLE_SCALE, "Key+root over Dm: A is only in the scale (no tonic mark)");
    CHECK(bass_role(&b, 9, 1, 66, 2) == ROLE_OUT, "Key+root over Dm: F# is outside A minor");
    b.scale = 1;                                              /* minor pentatonic */
    CHECK(bass_role(&b, 9, 1, 59, 2) == ROLE_OUT, "pentatonic: B is outside");

    /* Whites: the white keys play the scale from its tonic, from the lowest bass note up */
    bass_init(&b);
    CHECK(bass_white_note(&b, 9, 1, 0, 41) == 45 && bass_white_note(&b, 9, 1, 6, 41) == 55 && bass_white_note(&b, 9, 1, 7, 41) == 57,
          "A minor whites: A2, ... G3, A3 (%d %d %d)", bass_white_note(&b, 9, 1, 0, 41), bass_white_note(&b, 9, 1, 6, 41), bass_white_note(&b, 9, 1, 7, 41));
    b.scale = 1;
    CHECK(bass_white_note(&b, 9, 1, 5, 41) == 57, "pentatonic whites: the sixth white is the octave (%d)", bass_white_note(&b, 9, 1, 5, 41));
    CHECK(bass_white_note(&b, 0, 0, 0, 41) == 48, "C major whites start on C3 (%d)", bass_white_note(&b, 0, 0, 0, 41));

    /* lines keep inside the scale: a flat 7 over C in C major becomes A; over Am in A minor it is G */
    bass_init(&b);
    x.step = 0; x.iv = 10; x.len = 2;
    CHECK(pc12(bass_line_note(&b, 0, 0, &x, 0, 4, 0, 41)) == 9, "b7 over C in C major moves to A (%d)", bass_line_note(&b, 0, 0, &x, 0, 4, 0, 41));
    CHECK(pc12(bass_line_note(&b, 9, 1, &x, 9, 3, 9, 41)) == 7, "b7 over Am in A minor is G");
    x.iv = BASS_APPROACH;
    CHECK(pc12(bass_line_note(&b, 0, 0, &x, 0, 4, 7, 41)) == 5, "the approach to G from C, kept in C major: F");
    x.iv = BASS_THIRD;
    CHECK(pc12(bass_line_note(&b, 9, 1, &x, 2, 3, 9, 41)) == 5, "the 3rd of Dm: F");
    x.iv = 0;
    r = bass_line_note(&b, 9, 1, &x, 9, 3, 9, 41);
    CHECK(r >= 41 && r < 53 && pc12(r) == 9, "the root sits in the lowest octave (%d)", r);

    /* the chord the bass follows: the one recorded at that moment, or I-V-vi-IV when there are none */
    memset(&song, 0, sizeof song);
    song.bars = 4;
    CHECK(bass_chord_at(&song, 120, 0, 0, &third) == 7 && third == 4, "no chords, bar 2: V (G), major");
    CHECK(bass_chord_at(&song, 200, 0, 0, &third) == 9 && third == 3, "no chords, bar 3: vi (A), minor");
    add(10, 80, 50); add(10, 80, 53); add(10, 80, 57); add(10, 80, 60); add(10, 80, 64);
    CHECK(bass_chord_at(&song, 40, 0, 0, &third) == 2 && third == 3, "Dm9 recorded: D, minor (%d %d)", bass_chord_at(&song, 40, 0, 0, &third), third);
    CHECK(bass_chord_at(&song, 150, 0, 0, &third) == -1, "chords recorded, none at this moment: no root");

    /* SCALE with chords: every scale ranked by fit to them; without: the key's list */
    memset(&song, 0, sizeof song);
    song.bars = 1;
    add(0, 90, 57); add(0, 90, 60); add(0, 90, 64);           /* Am in C major */
    bass_init(&b);
    {
        uint8_t ids[17], held[17];
        int nu, n = bass_scale_list(&song, 0, 0, ids, held, &nu);
        CHECK(nu == 3 && n == 17 && !strcmp(KIT_SCALES[ids[0]].id, "major-pentatonic") && held[0] == 3, "Am fits C major pentatonic best (%s)", KIT_SCALES[ids[0]].id);
        bass_scale_step(&b, &song, 0, 0, 1);                  /* from major, the next in fit order */
        CHECK(!strcmp(bass_scale(&b, 0)->id, "mixolydian"), "after major comes mixolydian (%s)", bass_scale(&b, 0)->id);
        bass_scale_step(&b, &song, 0, 0, -1);
        CHECK(!strcmp(bass_scale(&b, 0)->id, "major"), "and back to major");
        memset(&song, 0, sizeof song); song.bars = 1;
        n = bass_scale_list(&song, 0, 0, ids, held, &nu);
        CHECK(nu == 0 && n == 8 && ids[0] == 0, "no chords: the key's eight");
    }

    for (i = 0; i < BASS_NSTYLES; i++)
        CHECK(BASS_STYLES[i].n >= 1 && BASS_STYLES[i].line[0].n <= BASS_MAXN, "style %d has its lines", i);
    (void)pad_recorded_degree; (void)pad_next_degree; (void)pad_play; (void)pad_step_of; (void)man_walk; (void)man_next_idea;
    (void)pad_init; (void)man_init;
    (void)song_bar_chord; (void)man_set_root; (void)man_outside; (void)bass_ideas_line; (void)bass_auto_line;
    printf("%s: bass, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
