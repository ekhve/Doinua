/* SPDX-License-Identifier: GPL-3.0-only */
/* The melody template's music (apps/companion/melody.c, D-050) on the real theory (music.c, checked by the oracle):
 * the key's pentatonic first, PLUS a voice in the key, phrases kept in the scale, what a scale lacks of the chords,
 * the chord a figure plays over, and Sketchpad's figures by style. */
#include <stdio.h>
#include <string.h>
#include "../../apps/companion/pad.c"
#include "../../apps/companion/manual.c"
#include "../../apps/companion/bass.c"
#include "../../apps/companion/melody.c"

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
    mel_t m;
    uint8_t ids[KIT_NSCALES], held[KIT_NSCALES], iv[12];
    int nu, n, i, root, f[MEL_NFIGS];

    mel_init(&m);
    CHECK(!strcmp(mel_scale(&m, 0)->id, "major-pentatonic") && !strcmp(mel_scale(&m, 1)->id, "minor-pentatonic"), "the key's pentatonic first");
    memset(&song, 0, sizeof song);
    song.bars = 4;
    n = mel_scale_list(&song, 0, 0, ids, held, &nu);
    CHECK(n == 8 && ids[0] == 2 && ids[1] == 0 && nu == 0, "no chords: the key's list, its pentatonic first (%d %d)", n, ids[0]);

    /* PLUS: a 3rd or a 6th above in the key */
    m.plus = MPLUS_3RD;
    CHECK(mel_plus_note(&m, 0, 0, 60) == 64 && mel_plus_note(&m, 0, 0, 62) == 65 && mel_plus_note(&m, 0, 0, 64) == 67, "3rd in C major: C E, D F, E G");
    m.plus = MPLUS_6TH;
    CHECK(mel_plus_note(&m, 0, 0, 64) == 72, "6th in C major: E and the C above (%d)", mel_plus_note(&m, 0, 0, 64));
    m.plus = MPLUS_OCT;
    CHECK(mel_plus_note(&m, 0, 0, 60) == 72, "the octave");
    m.plus = MPLUS_ECHO;
    CHECK(mel_plus_note(&m, 0, 0, 60) == -1, "echo adds no second voice");

    /* phrases stay in the melody's scale */
    mel_init(&m);
    for (i = 0; i < MEL_NPSTYLES; i++) {
        const mel_pstyle_t *s = &MEL_PHRASES[i];
        int p, k;
        for (p = 0; p < s->n; p++)
            for (k = 0; k < s->ph[p].na; k++) {
                int note = mel_phrase_note(&m, 9, 1, s->ph[p].a[k].deg, 2);   /* over Dm in A minor */
                CHECK(scale_mask(mel_scale(&m, 1), 9) >> pc12(note) & 1u, "%s %s: note %d in A minor pentatonic", s->name, s->ph[p].name, note);
                CHECK(note >= 45 && note <= 84, "%s: note %d near the keys", s->name, note);
            }
    }
    CHECK(mel_phrase_note(&m, 0, 0, 0, 0) == 60 && mel_phrase_note(&m, 0, 0, 7, 0) == 72, "degree 0 is the root (C4), 7 its octave");

    /* what a scale lacks of the chords: Am9 and Dm9 in A minor */
    add(0, 96, 57); add(0, 96, 60); add(0, 96, 64); add(0, 96, 67); add(0, 96, 71);   /* Am9 */
    add(96, 96, 50); add(96, 96, 53); add(96, 96, 57); add(96, 96, 60); add(96, 96, 64); /* Dm9 */
    CHECK(mel_scale_lacks(&song, 1, 9) == 0, "natural minor fits Am9 Dm9");
    CHECK(mel_scale_lacks(&song, 4, 9) == (1u << 5), "dorian lacks F");
    n = mel_scale_list(&song, 9, 1, ids, held, &nu);
    CHECK(nu == 7 && held[0] == 7, "with chords: the best fit holds all seven of their notes (%d of %d)", held[0], nu);

    /* the chord a figure hears */
    CHECK(mel_chord_at(&song, 120, 9, 1, &root, iv) >= 4 && root == 2, "bar 2: Dm9's root D, with its intervals (%d)", root);
    memset(&song, 0, sizeof song);
    song.bars = 4;
    n = mel_chord_at(&song, 100, 0, 0, &root, iv);
    CHECK(n == 3 && root == 7 && iv[1] == 4, "no chords, bar 2: the key's V, G major (%d %d)", n, root);

    /* Sketchpad's styles have their figures */
    for (i = 0; i < 6; i++) CHECK(mel_style_figs(i, f) >= 2, "%s has figures", MEL_STYLE_NAMES[i]);
    (void)pad_recorded_degree; (void)pad_next_degree; (void)pad_play; (void)pad_step_of; (void)man_walk; (void)man_next_idea;
    (void)song_bar_chord; (void)man_set_root; (void)man_outside; (void)bass_ideas_line; (void)bass_auto_line; (void)bass_role;
    (void)bass_init; (void)man_init; (void)pad_init; (void)bass_white_note; (void)bass_line_note; (void)bass_scale_list; (void)bass_scale_step; (void)mel_scale_step; (void)mel_idea; (void)mel_phrase;
    printf("%s: melody, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
