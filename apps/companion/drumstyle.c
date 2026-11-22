/* SPDX-License-Identifier: GPL-3.0-only */
/* The drums' styles (ST-32, ST-33, ST-48; D-054), from the drum lab the owner approved: a bar of 16 steps per drum,
 * "." a rest, 1..9 the velocity, "r" a two-hit roll, "t" a three-hit burst; busy, fill, break and drop are made from
 * the main beat, as the lab makes them (a style may have its own busy beat: dembow's is shatta). Portable, no state:
 * factory/unit/test_music_drums.c checks it. */
#include "fm1kit.h"

enum { DS_KICK = 0, DS_SNARE, DS_CLAP, DS_GHOST, DS_RIM, DS_HATC, DS_HATO, DS_SHAKER, DS_TOML, DS_TOMM, DS_TOMH, DS_PERC, DS_NSLOTS };
static const uint8_t DS_PAD[DS_NSLOTS] = { 0, 4, 9, 11, 7, 12, 15, 17, 19, 20, 21, 22 };   /* the slot's key: kick1 snare1 clap1 ... */
enum { DV_MAIN = 0, DV_BUSY, DV_FILL, DV_BREAK, DV_DROP, DV_N };
static const char *const DV_NAMES[DV_N] = { "main", "busy", "fill", "break", "drop" };

typedef struct { const char *slot[DS_NSLOTS]; } ds_bar_t;   /* each slot's 16 steps, or 0 */
typedef struct { const char *name; uint8_t kit, bpm, feel; ds_bar_t main; const ds_bar_t *busy; } ds_style_t;

#define B(k, s, c, g, r, hc, ho, sh, tl, tm, th, p) { { k, s, c, g, r, hc, ho, sh, tl, tm, th, p } }
static const ds_bar_t DS_BUSY_REGGAETON = B("9...9...9...9...", 0, "............9...", 0, 0, "6.4.6.4.6.4.6.4.", 0, 0, 0, 0, 0, "..7.......7...7.");
static const ds_bar_t DS_BUSY_SHATTA = B("9...9...9...9...", 0, "............9...", 0, "...8..8....8..8.", "..t...t...t...t.", 0, "5.5.5.5.5.5.5.5.", 0, "......6.......5.", "..5.......5..5..", 0);
/* name, kit (Dusty 808 Live Latin), tempo, feel (the REC layer's FEEL list), the main beat */
static const ds_style_t DS_STYLES[] = {
    { "Boom bap", 0, 90, 2, B("9......6..8.....", "....9.......9...", 0, "..............3.", 0, "7.5.7.5.7.5.7.5.", 0, 0, 0, 0, 0, 0), 0 },
    { "Dilla", 0, 86, 4, B("9..8......9.7..6", "....9.......9...", 0, ".....3.....3..3.", 0, "8.5.7.5.8.5.7.57", 0, 0, 0, 0, 0, 0), 0 },
    { "G-funk", 1, 92, 1, B("9.....7...9.....", 0, "....9.......9...", 0, 0, "7.5.7.5.7.5.7...", "..............6.", 0, 0, 0, 0, 0), 0 },
    { "Funk", 2, 100, 5, B("9..7......9..6..", "....9.......9...", 0, ".3.....3.3....3.", 0, "7575757575757575", 0, 0, 0, 0, 0, 0), 0 },
    { "Disco", 2, 118, 0, B("9...9...9...9...", "....9.......9...", 0, 0, 0, "6...6...6...6...", "..8...8...8...8.", 0, 0, 0, 0, 0), 0 },
    { "Half-time", 2, 76, 5, B("9.........8.....", "........9.......", 0, 0, "..............5.", "6.4.6.4.6.4.6.4.", 0, 0, 0, 0, 0, 0), 0 },
    { "Reggaeton", 3, 94, 7, B("9...9...9...9...", "...8..8....8..8.", 0, 0, 0, 0, 0, "6.4.6.4.6.4.6.4.", 0, 0, 0, "..............7."), &DS_BUSY_REGGAETON },
    { "Shatta", 1, 100, 8, B("9...9...9...9...", 0, 0, 0, "...8..8....8..8.", "..t.......t.....", 0, 0, 0, "......6.......5.", "..5.......5.....", 0), &DS_BUSY_SHATTA },
    { "Afrobeats", 3, 104, 1, B("9.....8...9.....", 0, "........9.......", 0, "...7..7.....7...", 0, 0, "5454545454545454", 0, "..........6..5..", 0, 0), 0 },
    { "Reggae", 2, 76, 5, B("........9.......", "........9.......", 0, 0, "........8.......", "6.4.6.4.6.4.6.4.", "..............5.", 0, 0, 0, 0, 0), 0 },
    { "Drum & bass", 2, 172, 0, B("9.........9.....", "....9.......9...", 0, ".......3.3....3.", 0, "7.5.7.5.7.5.7.5.", 0, 0, 0, 0, 0, 0), 0 },
    { "Trap", 1, 140, 0, B("9......8..9.....", 0, "........9.......", 0, 0, "646464646464rr64", 0, 0, 0, 0, 0, 0), 0 },
};
#undef B
#define DS_NSTYLES ((int)(sizeof DS_STYLES / sizeof DS_STYLES[0]))

typedef struct { uint8_t tick, pad, vel, slot; } ds_hit_t;   /* tick within the bar (24 a beat) */

static char ds_at(const char *s, int i) { return s ? s[i] : '.'; }

/* the bar of a style's variant: each slot's 16 steps into out[slot][16] ("." when none) */
static void ds_bar(int style, int variant, int ghosts, char out[DS_NSLOTS][16])
{
    const ds_style_t *st = &DS_STYLES[style];
    const ds_bar_t *m = variant == DV_BUSY && st->busy ? st->busy : &st->main;
    int s, i;
    for (s = 0; s < DS_NSLOTS; s++)
        for (i = 0; i < 16; i++) out[s][i] = ds_at(m->slot[s], i);
    if (variant == DV_BUSY && !st->busy) {      /* busier: the hats' gaps filled, more ghosts, a pickup kick */
        int all = 1;
        for (i = 0; i < 16; i++) if (i % 2 ? out[DS_HATC][i] != '.' : out[DS_HATC][i] == '.' || out[DS_HATC][i] == 'r' || out[DS_HATC][i] == 't') all = 0;
        if (all) for (i = 1; i < 16; i += 2) out[DS_HATC][i] = '4';
        out[DS_GHOST][7] = '3'; out[DS_GHOST][14] = '3';
        out[DS_KICK][14] = '6';
    }
    if (variant == DV_FILL) {                   /* the second half: a snare run and the toms down */
        static const char run[8] = { '7', '.', '7', '.', '9', '.', '9', '9' };
        for (s = 0; s < DS_NSLOTS; s++) for (i = 8; i < 16; i++) out[s][i] = '.';
        for (i = 8; i < 16; i++) if (run[i - 8] != '.') out[DS_SNARE][i] = run[i - 8];
        out[DS_TOMH][9] = '8'; out[DS_TOMM][11] = '8'; out[DS_TOML][13] = '8';
        out[DS_KICK][8] = '8';
    }
    if (variant == DV_BREAK)                    /* only the hats and shakers, and one kick on the one */
        for (s = 0; s < DS_NSLOTS; s++) {
            if (s == DS_HATC || s == DS_HATO || s == DS_SHAKER) continue;
            for (i = 0; i < 16; i++) out[s][i] = s == DS_KICK && i == 0 ? '9' : '.';
        }
    if (variant == DV_DROP)                     /* everything but the kick */
        for (i = 0; i < 16; i++) out[DS_KICK][i] = '.';
    if (ghosts == 0)                            /* GHOSTS off: no ghost notes */
        for (i = 0; i < 16; i++) out[DS_GHOST][i] = '.';
    if (ghosts == 2 && variant != DV_BREAK)     /* more: the busy beat's ghosts too */
        { if (out[DS_GHOST][7] == '.') out[DS_GHOST][7] = '3'; if (out[DS_GHOST][14] == '.') out[DS_GHOST][14] = '3'; }
}

/* the hits of a bar, in time order within each slot; returns how many (at most max) */
static int ds_hits(int style, int variant, int ghosts, ds_hit_t *out, int max)
{
    char bar[DS_NSLOTS][16];
    int s, i, n = 0;
    ds_bar(style, variant, ghosts, bar);
    for (s = 0; s < DS_NSLOTS; s++)
        for (i = 0; i < 16; i++) {
            char c = bar[s][i];
            if (c == '.') continue;
            if (c == 'r' || c == 't') {         /* a roll: two hits a sixteenth; a burst: three */
                int k, m = c == 'r' ? 2 : 3;
                for (k = 0; k < m && n < max; k++) {
                    out[n].tick = (uint8_t)(i * 6 + k * 6 / m);
                    out[n].pad = DS_PAD[s];
                    out[n].vel = (uint8_t)((c == 'r' ? 70 : 72) - k * (c == 'r' ? 15 : 8));
                    out[n].slot = (uint8_t)s;
                    n++;
                }
                continue;
            }
            if (n >= max) return n;
            out[n].tick = (uint8_t)(i * 6);
            out[n].pad = DS_PAD[s];
            out[n].vel = (uint8_t)((c - '0') * 127 / 9);
            out[n].slot = (uint8_t)s;
            n++;
        }
    return n;
}

/* which variant plays in this bar of a held pad: the fill in the last bar of each 4 or 8 when FILLS is on */
static int ds_variant_at(int pad_variant, int fills, int bar)
{
    int every = fills == 1 ? 4 : fills == 2 ? 8 : 0;
    if (pad_variant == DV_MAIN && every && bar % every == every - 1) return DV_FILL;
    return pad_variant;
}
