/* SPDX-License-Identifier: GPL-3.0-only */
/* ST-48 (R-156, D-057): reggae, afrobeats, shatta, reggaeton and drum and bass are styles of every template: the drums
 * (drumstyle.c), the chord pad (pad.c), the bass (bass.c) and the melody's AUTO phrases (melody.c). */
#include <stdio.h>
#include <string.h>
#include "../../apps/companion/pad.c"
#include "../../apps/companion/manual.c"
#include "../../apps/companion/bass.c"
#include "../../apps/companion/melody.c"
#include "../../apps/companion/drumstyle.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    static const char *const want[5] = { "Reggae", "Afrobeats", "Shatta", "Reggaeton", "Drum & bass" };
    int w, i, f;
    for (w = 0; w < 5; w++) {
        for (f = 0, i = 0; i < DS_NSTYLES; i++) f |= !strcmp(DS_STYLES[i].name, want[w]);
        CHECK(f, "the drums have %s", want[w]);
        for (f = 0, i = 0; i < PAD_NSTYLES; i++) f |= !strcmp(PAD_STYLES[i].name, want[w]);
        CHECK(f, "the chord pad has %s", want[w]);
        for (f = 0, i = 0; i < BASS_NSTYLES; i++) f |= !strcmp(BASS_STYLES[i].name, want[w]);
        CHECK(f, "the bass has %s", want[w]);
        for (f = 0, i = 0; i < MEL_NPSTYLES; i++) f |= !strcmp(MEL_PHRASES[i].name, want[w]);
        CHECK(f, "the melody has %s", want[w]);
    }
    (void)pad_recorded_degree; (void)pad_next_degree; (void)pad_play; (void)pad_step_of; (void)man_walk; (void)man_next_idea;
    (void)song_bar_chord; (void)man_set_root; (void)man_outside; (void)bass_ideas_line; (void)bass_auto_line; (void)bass_role;
    (void)bass_init; (void)man_init; (void)pad_init; (void)bass_white_note; (void)bass_line_note; (void)bass_scale_list; (void)bass_scale_step;
    (void)mel_scale_step; (void)mel_idea; (void)mel_phrase; (void)mel_init; (void)mel_plus_note; (void)mel_phrase_note; (void)mel_scale_lacks;
    (void)mel_chord_at; (void)mel_style_figs; (void)mel_scale_list; (void)ds_hits; (void)ds_variant_at; (void)DV_NAMES; (void)pad_prog;
    printf("%s: styles on every template, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
