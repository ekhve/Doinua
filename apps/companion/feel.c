/* SPDX-License-Identifier: GPL-3.0-only */
/* feel.c: how a chord's notes are played, FEEL (ST-81, D-077, R-260), the Chord Lab's seven feels. app.c includes it,
 * and so does the unit test (factory/unit/test_music_feel.c), so the same code is what plays and what is tested.
 * For note i of n (low to high): when it starts after the chord (ms) and its velocity, % of the key's. Integer only;
 * the randomness is a 32-bit xorshift the caller keeps, so the host and the FM-1 play the same. */
#include <stdint.h>

enum { FEEL_TIGHT = 0, FEEL_PIANIST, FEEL_LAIDBACK, FEEL_ROLLED, FEEL_STRUM_UP, FEEL_STRUM_DOWN, FEEL_SLOPPY, FEEL_N };
static const char *const FEEL_WORDS[FEEL_N] = { "Tight", "Pianist", "Laid back", "Rolled", "Strum up", "Strum dn", "Sloppy" };

static int feel_rand(uint32_t *s, int n)        /* 0..n-1 */
{
    uint32_t x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return n > 0 ? (int)(x % (uint32_t)n) : 0;
}

/* sixteenth: a sixteenth note at the song's tempo, ms */
static void feel_note(int feel, int i, int n, int sixteenth, uint32_t *rng, int *delay_ms, int *vel_pct)
{
    int top = i == n - 1, r = n - 1 - i;
    switch (feel) {
    case FEEL_PIANIST:                          /* the bass lands, the inner notes a little after and soft, the top sings */
        if (i == 0) { *delay_ms = feel_rand(rng, 5); *vel_pct = 85; }
        else if (top) { *delay_ms = 8 + feel_rand(rng, 13); *vel_pct = 92; }
        else { *delay_ms = 14 + feel_rand(rng, 23); *vel_pct = 42 + feel_rand(rng, 17); }
        break;
    case FEEL_LAIDBACK:                         /* the whole hand late, by a third of a sixteenth, soft, a bit spread */
        *delay_ms = sixteenth * (30 + feel_rand(rng, 11)) / 100 + i * (6 + feel_rand(rng, 7));
        *vel_pct = 50 + feel_rand(rng, 13) + (top ? 10 : 0);
        break;
    case FEEL_ROLLED:                           /* a harp, getting louder */
        *delay_ms = i * sixteenth * (22 + feel_rand(rng, 6)) / 100;
        *vel_pct = 45 + 45 * i / (n > 1 ? n - 1 : 1);
        break;
    case FEEL_STRUM_UP:                         /* the low string first and hardest */
        *delay_ms = i * 13 + feel_rand(rng, 6);    /* 13 to 18 ms a string, always in order */
        *vel_pct = 95 - i * 10;
        break;
    case FEEL_STRUM_DOWN:                       /* the top string first and hardest */
        *delay_ms = r * 13 + feel_rand(rng, 6);
        *vel_pct = 95 - r * 10;
        break;
    case FEEL_SLOPPY:                           /* anyone's first try */
        *delay_ms = feel_rand(rng, sixteenth * 45 / 100 + 1);
        *vel_pct = 35 + feel_rand(rng, 61);
        break;
    default:                                    /* tight: together, within 3 ms */
        *delay_ms = feel_rand(rng, 4);
        *vel_pct = 78 + feel_rand(rng, 5);
    }
    if (*vel_pct < 10) *vel_pct = 10;
}
