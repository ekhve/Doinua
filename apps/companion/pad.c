/* SPDX-License-Identifier: GPL-3.0-only */
/* pad.c: the chord pad's music, apart from the keys, lights and screen (D-034). app.c includes it, and so does the unit
 * test (factory/unit/test_music_chordpad.c), so the same code is what plays and what is tested.
 *
 * The pads are the key's seven chords (I..vii), twice: the lower row from C3, the upper from C4 (Sketchpad's octave,
 * shifted by OCT). A style is a list of progressions; each step of a progression is a degree with its colour. The
 * chords, colours, voicings and the likely next chord come from fm1kit (API 12, Sketchpad's theory). */
#include "fm1kit.h"

/* the tracks whose recorded notes count as "the chord" (D-046): the chords pages read their own track, the others the
 * other pitched tracks. The app sets it for the track on screen; alone (the unit tests) it is the chords track. */
static uint8_t harm_mask = 1u << KIT_TRACK_CHORDS;
#define FOR_HARM(s, t, i) for (t = 0; t < KIT_NTRACKS; t++) if (harm_mask >> t & 1u) for (i = 0; i < (s)->count[t]; i++)

#define PAD_STEPS 4
typedef struct { uint8_t degree, colour; } pad_step_t;
typedef struct { const char *name; uint8_t n; pad_step_t step[PAD_STEPS]; } pad_prog_t;
typedef struct { const char *name; uint8_t n; pad_prog_t prog[3]; } pad_style_t;

#define S_ KIT_COL_NONE
/* A starter list from the simulator the owner played (concept v0.4); the owner curates it from their artists (D-034).
 * The usual progression of a style first. */
static const pad_style_t PAD_STYLES[] = {
    { "Free", 1, { { "Free", 0, { { 0, S_ } } } } },
    { "Pop", 3, { { "I-V-vi-IV", 4, { { 0, S_ }, { 4, S_ }, { 5, S_ }, { 3, S_ } } },
                  { "I-vi-IV-V", 4, { { 0, S_ }, { 5, S_ }, { 3, S_ }, { 4, S_ } } },
                  { "vi-IV-I-V", 4, { { 5, S_ }, { 3, KIT_COL_ADD9 }, { 0, S_ }, { 4, KIT_COL_SUS4 } } } } },
    { "Neo soul", 3, { { "ii-V-I", 3, { { 1, KIT_COL_9 }, { 4, KIT_COL_13 }, { 0, KIT_COL_MAJ7 } } },
                       { "IV-iii-ii-I", 4, { { 3, KIT_COL_MAJ7 }, { 2, KIT_COL_7 }, { 1, KIT_COL_9 }, { 0, KIT_COL_MAJ7 } } },
                       { "IV-V-iii-vi", 4, { { 3, KIT_COL_MAJ7 }, { 4, KIT_COL_9 }, { 2, KIT_COL_7 }, { 5, KIT_COL_9 } } } } },
    { "Hip hop", 3, { { "vi-IV", 2, { { 5, KIT_COL_7 }, { 3, KIT_COL_MAJ7 } } },
                      { "vi-iii-IV", 3, { { 5, KIT_COL_9 }, { 2, KIT_COL_7 }, { 3, KIT_COL_MAJ7 } } },
                      { "ii-V", 2, { { 1, KIT_COL_9 }, { 4, KIT_COL_13 } } } } },
    { "West coast", 3, { { "vi-ii", 2, { { 5, KIT_COL_9 }, { 1, KIT_COL_9 } } },
                         { "vi-IV-V", 3, { { 5, KIT_COL_7 }, { 3, KIT_COL_MAJ7 }, { 4, KIT_COL_7 } } },
                         { "iii-vi", 2, { { 2, KIT_COL_7 }, { 5, KIT_COL_9 } } } } },
    { "Funk", 3, { { "I vamp", 1, { { 0, KIT_COL_9 } } },
                   { "I-IV", 2, { { 0, KIT_COL_9 }, { 3, KIT_COL_9 } } },
                   { "ii-V", 2, { { 1, KIT_COL_7 }, { 4, KIT_COL_9 } } } } },
    { "Electronic", 3, { { "vi-IV-I-V", 4, { { 5, KIT_COL_ADD9 }, { 3, KIT_COL_ADD9 }, { 0, KIT_COL_SUS2 }, { 4, KIT_COL_SUS4 } } },
                         { "I-V-vi-IV", 4, { { 0, KIT_COL_ADD9 }, { 4, S_ }, { 5, KIT_COL_ADD9 }, { 3, KIT_COL_ADD9 } } },
                         { "IV-V-vi", 3, { { 3, KIT_COL_ADD9 }, { 4, KIT_COL_SUS4 }, { 5, S_ } } } } },
    { "Jazz", 3, { { "ii-V-I", 3, { { 1, KIT_COL_7 }, { 4, KIT_COL_7 }, { 0, KIT_COL_MAJ7 } } },
                   { "I-vi-ii-V", 4, { { 0, KIT_COL_MAJ7 }, { 5, KIT_COL_7 }, { 1, KIT_COL_7 }, { 4, KIT_COL_7 } } },
                   { "iii-vi-ii-V", 4, { { 2, KIT_COL_7 }, { 5, KIT_COL_7 }, { 1, KIT_COL_7 }, { 4, KIT_COL_13 } } } } },
    /* ST-48 (the owner, 2026-11-13: "add also reggae chords, afrobeat pads ... shatta ... reggaeton ... drum and bass") */
    { "Reggae", 3, { { "I-IV", 2, { { 0, S_ }, { 3, S_ } } },
                     { "vi-ii", 2, { { 5, S_ }, { 1, S_ } } },
                     { "I-IV-V", 3, { { 0, S_ }, { 3, S_ }, { 4, S_ } } } } },
    { "Afrobeats", 3, { { "IV-V-vi", 3, { { 3, KIT_COL_MAJ7 }, { 4, S_ }, { 5, KIT_COL_7 } } },
                        { "ii-iii-IV", 3, { { 1, KIT_COL_7 }, { 2, KIT_COL_7 }, { 3, KIT_COL_MAJ7 } } },
                        { "vi-IV-I-V", 4, { { 5, KIT_COL_7 }, { 3, KIT_COL_ADD9 }, { 0, KIT_COL_ADD9 }, { 4, S_ } } } } },
    { "Shatta", 2, { { "vi-V", 2, { { 5, S_ }, { 4, S_ } } },
                     { "vi-IV-V", 3, { { 5, S_ }, { 3, S_ }, { 4, S_ } } } } },
    { "Reggaeton", 2, { { "vi-IV-I-V", 4, { { 5, S_ }, { 3, S_ }, { 0, S_ }, { 4, S_ } } },
                        { "vi-V-IV-V", 4, { { 5, S_ }, { 4, S_ }, { 3, S_ }, { 4, S_ } } } } },
    { "Drum & bass", 2, { { "vi-IV-V-iii", 4, { { 5, KIT_COL_9 }, { 3, KIT_COL_MAJ7 }, { 4, KIT_COL_9 }, { 2, KIT_COL_7 } } },
                          { "ii-V-I", 3, { { 1, KIT_COL_9 }, { 4, KIT_COL_9 }, { 0, KIT_COL_MAJ7 } } } } },
};
#undef S_
#define PAD_NSTYLES ((int)(sizeof PAD_STYLES / sizeof PAD_STYLES[0]))
#define PAD_START_STYLE 2               /* Neo soul, ii-V-I: the owner's P1 starts there */

enum { PAD_BLOCK = 0, PAD_STRUM, PAD_ARP, PAD_NFEELS };

typedef struct {
    int tonic, minor;                   /* the key the pads are in */
    int octave;                         /* OCT: -2..2 */
    int style, prog;                    /* K1, K2 */
    int voicing, feel;                  /* K3, K4 */
    int auto_on;                        /* the style's colours (AUTO) */
    int locked;                         /* a colour added to every chord, or -1 */
    int pos;                            /* the progression's next step */
    int now_degree, now_step;           /* the chord played last (-1: none), and its step in the progression (-1: none) */
    kit_vchord_t now;                   /* its notes */
} pad_t;

static void pad_init(pad_t *p)
{
    p->tonic = 0; p->minor = 0; p->octave = 0;
    p->style = PAD_START_STYLE; p->prog = 0;
    p->voicing = KIT_VOICE_ROOT; p->feel = PAD_BLOCK;
    p->auto_on = 1; p->locked = -1; p->pos = 0;
    p->now_degree = -1; p->now_step = -1;
    p->now.n = 0;
}

static const pad_prog_t *pad_prog(const pad_t *p)
{
    const pad_prog_t *pr = &PAD_STYLES[p->style].prog[p->prog];
    return pr->n ? pr : 0;              /* 0: Free, no progression */
}

/* the colour a chord gets: a held black key's, else the locked one, else the progression's for that step (AUTO on) */
static int pad_colour(const pad_t *p, int step, int held)
{
    const pad_prog_t *pr = pad_prog(p);
    if (held >= 0) return held;
    if (p->locked >= 0) return p->locked;
    if (p->auto_on && pr && step >= 0) return pr->step[step].colour;
    return KIT_COL_NONE;
}

/* the chord of degree (0..6) on a row (0 lower, 1 upper) with a colour; slash3: its 3rd in the bass */
static int pad_chord(const pad_t *p, int degree, int row, int colour, int slash3, kit_vchord_t *out)
{
    kit_chord_t tri[7];
    if (degree < 0 || degree > 6 || !kit_triads(p->tonic, p->minor ? 1 : 0, tri)) {
        out->n = 0;
        return 0;
    }
    return kit_colour_chord(tri[degree].root, tri[degree].quality, colour, slash3, p->voicing,
                            48 + 12 * row + 12 * p->octave, out);
}

/* which step of the progression a pressed degree is: the first from the next step on, round the loop; -1 if none */
static int pad_step_of(const pad_t *p, int degree)
{
    const pad_prog_t *pr = pad_prog(p);
    int i;
    if (!pr) return -1;
    for (i = 0; i < pr->n; i++) {
        int s = (p->pos + i) % pr->n;
        if (pr->step[s].degree == degree) return s;
    }
    return -1;
}

/* play degree as step (or -1): remembers it as the chord now, and moves the progression on */
static int pad_play(pad_t *p, int degree, int step, int row, int held, int slash3)
{
    const pad_prog_t *pr = pad_prog(p);
    pad_chord(p, degree, row, pad_colour(p, step, held), slash3, &p->now);
    p->now_degree = degree;
    p->now_step = step;
    if (pr && step >= 0) p->pos = (step + 1) % pr->n;
    return p->now.n;
}

/* NEXT: the progression's next step, or (Free) the likely next chord after the last; returns its degree */
static int pad_next_degree(const pad_t *p, int *step)
{
    const pad_prog_t *pr = pad_prog(p);
    if (pr) {
        *step = p->pos % pr->n;
        return pr->step[*step].degree;
    }
    *step = -1;
    return p->now_degree < 0 ? 0 : kit_next_degree(p->tonic, p->minor, p->now_degree);
}

/* the degree of the chord the chords track has at tick (the song as recorded), or -1. Notes are kept as played, so a
 * chord can start or end anywhere. The pad is the degree whose root sounds and whose chord in the key (1, 3, 5, 7, 9)
 * holds the most of the notes, one more when its root is in the bass; a tie goes to the lower degree (so Cmaj7 with E
 * in the bass is still I, and Dm9 is ii, not IV). */
static int pad_recorded_degree(const kit_song_t *s, int tick, int tonic, int minor)
{
    const kit_scale_t *sc = &KIT_SCALES[minor ? 1 : 0];
    uint16_t pcs = 0;
    int i, d, t, best = -1, best_score = 0, low = 128, len = s->bars * KIT_TICKS_PER_BAR;
    if (tick < 0) return -1;
    FOR_HARM(s, t, i) {
        const kit_song_note_t *n = &s->notes[t][i];
        int end = n->start + n->len;
        if (n->start >= len) continue;
        if ((tick >= n->start && tick < end) || (tick + len >= n->start && tick + len < end)) {
            pcs |= (uint16_t)(1u << (n->note % 12));
            if (n->note < low) low = n->note;
        }
    }
    if (!pcs) return -1;
    for (d = 0; d < 7; d++) {
        int score = 0, k, root = (tonic + sc->iv[d]) % 12;
        if (!(pcs >> root & 1u)) continue;
        for (k = 0; k < 5; k++)                 /* 1, 3, 5, 7, 9: every other note of the scale from the degree */
            score += (pcs >> ((tonic + sc->iv[(d + 2 * k) % 7]) % 12)) & 1u;
        if (low % 12 == root) score += 1;
        if (score > best_score) { best_score = score; best = d; }
    }
    return best_score >= 2 ? best : -1;         /* the root and at least one more of its notes */
}
