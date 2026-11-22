/* SPDX-License-Identifier: GPL-3.0-only */
/* companion: the melody template's music (ST-26 to ST-29, D-050). Pure: no screen, no sound; included by app.c and by
 * factory/unit/test_music_melody.c. Uses bass.c (the scale fit, the chord a track hears) and fm1kit's Sketchpad melody
 * guide and figures (kit_melody_role, kit_render_figure, checked by the oracle).
 *
 *   PLAY   the keys where they are; K1 HELP Free / Key / Chord / Whites; K2 SCALE (the key's pentatonic first; by fit
 *          when chords are recorded); K3 OUTSIDE play / mute; K4 PLUS off / 3rd / 6th / octave / echo
 *   IDEAS  Sketchpad's figures fall onto the keys over the chords: K1 STYLE, K2 IDEA, K3 SPEED, K4 HEAR; SELECT the
 *          version (the figure's seed: the same rhythm, other notes)
 *   AUTO   a degree pad plays a style's phrase over its chord, a call bar and an answer bar: K1 STYLE, K2 PHRASE,
 *          K3 FEEL straight / swing, K4 FOLLOW the recorded chords */
enum { MEL_PLAY = 0, MEL_IDEAS, MEL_AUTO, MEL_NPAGES };
enum { MHELP_FREE = 0, MHELP_KEY, MHELP_CHORD, MHELP_WHITES, MEL_NHELPS };
enum { MOUT_PLAY = 0, MOUT_MUTE, MEL_NOUTS };
enum { MPLUS_OFF = 0, MPLUS_3RD, MPLUS_6TH, MPLUS_OCT, MPLUS_ECHO, MEL_NPLUS };
#define MEL_NSPEEDS 7
static const uint8_t MEL_SPEEDS[MEL_NSPEEDS] = { 50, 60, 70, 75, 80, 90, 100 };
/* the key's scales, its pentatonic first (the owner, 2026-11-12): KIT_SCALES indexes */
static const uint8_t MEL_MINOR_SCALES[] = { 3, 1, 7, 4, 6, 5, 10, 11, 12 };
static const uint8_t MEL_MAJOR_SCALES[] = { 2, 0, 13, 8, 9, 14, 15, 16 };

typedef struct {
    int page, help, scale_id, outside, plus;    /* PLAY: scale_id a KIT_SCALES index, -1 the key's pentatonic */
    int style, fig, version, speed, hear;       /* IDEAS */
    int astyle, aphrase, swing, follow;         /* AUTO */
} mel_t;

static void mel_init(mel_t *m)
{
    m->page = MEL_PLAY; m->help = MHELP_KEY; m->scale_id = -1; m->outside = MOUT_PLAY; m->plus = MPLUS_OFF;
    m->style = 0; m->fig = 0; m->version = 1; m->speed = MEL_NSPEEDS - 1; m->hear = HEAR_LISTEN;
    m->astyle = 0; m->aphrase = 0; m->swing = 0; m->follow = 0;
}

static const kit_scale_t *mel_scale(const mel_t *m, int minor)
{
    if (m->scale_id >= 0 && m->scale_id < KIT_NSCALES) return &KIT_SCALES[m->scale_id];
    return &KIT_SCALES[minor ? MEL_MINOR_SCALES[0] : MEL_MAJOR_SCALES[0]];
}
static uint16_t scale_mask(const kit_scale_t *sc, int tonic)
{
    uint16_t m = 0;
    int i;
    for (i = 0; i < sc->n; i++) m |= (uint16_t)(1u << ((tonic + sc->iv[i]) % 12));
    return m;
}

/* the list SCALE walks: with the harmony's notes recorded, every scale by how much of them it holds (Sketchpad
 * fitScales); else the key's own, its pentatonic first */
static int mel_scale_list(const kit_song_t *s, int tonic, int minor, uint8_t *ids, uint8_t *held, int *nused)
{
    int n, i;
    uint16_t pcs = bass_chord_pcs(s);
    *nused = 0;
    if (pcs) return kit_fit_scales(pcs, tonic, ids, held, KIT_NSCALES, nused);
    n = minor ? (int)sizeof MEL_MINOR_SCALES : (int)sizeof MEL_MAJOR_SCALES;
    for (i = 0; i < n; i++) { ids[i] = minor ? MEL_MINOR_SCALES[i] : MEL_MAJOR_SCALES[i]; held[i] = 0; }
    return n;
}
static void mel_scale_step(mel_t *m, const kit_song_t *s, int tonic, int minor, int d)
{
    uint8_t ids[KIT_NSCALES], held[KIT_NSCALES];
    int nu, n = mel_scale_list(s, tonic, minor, ids, held, &nu), cur = (int)(mel_scale(m, minor) - KIT_SCALES), i, at = 0;
    for (i = 0; i < n; i++) if (ids[i] == cur) at = i;
    at = ((at + (d > 0 ? 1 : -1)) % n + n) % n;
    m->scale_id = ids[at];
}
/* the harmony's pitch classes a scale lacks (ST-49: "lacks F#"); 0 when it fits */
static uint16_t mel_scale_lacks(const kit_song_t *s, int id, int tonic) { return (uint16_t)(bass_chord_pcs(s) & ~scale_mask(&KIT_SCALES[id], tonic)); }

/* PLUS: a second voice a 3rd or a 6th above in the key's own scale (or the octave); -1 when none */
static int mel_plus_note(const mel_t *m, int tonic, int minor, int n)
{
    const kit_scale_t *k = &KIT_SCALES[minor ? 1 : 0];
    int i, at = -1, up, steps;
    if (m->plus == MPLUS_OCT) return n + 12;
    if (m->plus != MPLUS_3RD && m->plus != MPLUS_6TH) return -1;
    steps = m->plus == MPLUS_3RD ? 2 : 5;
    for (i = 0; i < 7; i++) if ((tonic + k->iv[i]) % 12 == pc12(n)) at = i;
    if (at < 0) return n + (m->plus == MPLUS_3RD ? 4 : 9);   /* outside the key: a major 3rd or 6th */
    up = pc12(tonic + k->iv[(at + steps) % 7] - n);
    return n + (up ? up : 12);
}

/* ---- IDEAS: Sketchpad's melody figures (core/figures PATTERNS, kind "melody") and its six styles ---- */
typedef struct { const char *name; uint8_t styles; uint8_t n; kit_fig_step_t st[8]; } mel_fig_t;
enum { MS_HIPHOP = 1, MS_SOUL = 2, MS_FUNK = 4, MS_HOUSE = 8, MS_WEST = 16, MS_CINE = 32 };
static const char *const MEL_STYLE_NAMES[6] = { "Hip hop", "Soul", "Funk", "House", "West coast", "Cinematic" };
static const mel_fig_t MEL_FIGS[] = {
    { "Pentatonic hook", MS_HIPHOP | MS_WEST, 5, { { 0, 2, KIT_FIG_CHORDTONE }, { 2, 2, KIT_FIG_STEP }, { 6, 2, KIT_FIG_CHORDTONE }, { 10, 2, KIT_FIG_STEP }, { 12, 4, KIT_FIG_CHORDTONE } } },
    { "Sparse and late", MS_SOUL | MS_CINE, 3, { { 2, 4, KIT_FIG_TOP }, { 8, 2, KIT_FIG_STEP }, { 11, 5, KIT_FIG_CHORDTONE } } },
    { "Stabs", MS_FUNK | MS_HOUSE, 5, { { 0, 1, KIT_FIG_TOP }, { 3, 1, KIT_FIG_CHORDTONE }, { 7, 1, KIT_FIG_TOP }, { 10, 1, KIT_FIG_CHORDTONE }, { 11, 1, KIT_FIG_STEP } } },
    { "Rising line", MS_CINE | MS_HOUSE, 4, { { 0, 2, KIT_FIG_ROOT }, { 4, 2, KIT_FIG_STEPUP }, { 8, 2, KIT_FIG_STEPUP }, { 12, 4, KIT_FIG_APPROACH } } },
    { "Call and answer", MS_SOUL | MS_HIPHOP, 4, { { 0, 2, KIT_FIG_TOP }, { 2, 2, KIT_FIG_STEP }, { 8, 2, KIT_FIG_CHORDTONE }, { 10, 4, KIT_FIG_STEPDOWN } } },
    { "Two-note motif", MS_CINE | MS_WEST, 4, { { 0, 3, KIT_FIG_CHORDTONE }, { 4, 3, KIT_FIG_STEP }, { 8, 3, KIT_FIG_CHORDTONE }, { 12, 3, KIT_FIG_STEP } } },
    { "Arpeggio up", MS_HOUSE | MS_CINE, 6, { { 0, 2, KIT_FIG_ROOT }, { 2, 2, KIT_FIG_THIRD }, { 4, 2, KIT_FIG_FIFTH }, { 6, 2, KIT_FIG_TOP }, { 8, 2, KIT_FIG_FIFTH }, { 10, 2, KIT_FIG_THIRD } } },
    { "Tumbling", MS_FUNK | MS_HIPHOP, 6, { { 0, 1, KIT_FIG_TOP }, { 1, 1, KIT_FIG_STEPDOWN }, { 2, 1, KIT_FIG_STEPDOWN }, { 3, 1, KIT_FIG_STEPDOWN }, { 6, 2, KIT_FIG_CHORDTONE }, { 12, 4, KIT_FIG_CHORDTONE } } },
    { "Falling line", MS_SOUL | MS_WEST, 4, { { 0, 2, KIT_FIG_TOP }, { 4, 2, KIT_FIG_STEPDOWN }, { 8, 2, KIT_FIG_STEPDOWN }, { 12, 4, KIT_FIG_CHORDTONE } } },
};
#define MEL_NFIGS ((int)(sizeof MEL_FIGS / sizeof MEL_FIGS[0]))
static int mel_style_figs(int style, int *out)  /* the style's figures, in Sketchpad's order; how many */
{
    int i, n = 0;
    for (i = 0; i < MEL_NFIGS; i++) if (MEL_FIGS[i].styles & (1u << style)) out[n++] = i;
    return n;
}
static const mel_fig_t *mel_idea(const mel_t *m)
{
    int f[MEL_NFIGS], n = mel_style_figs(m->style, f);
    return &MEL_FIGS[f[m->fig % (n ? n : 1)]];
}

/* the chord a melody hears at tick (the harmony's notes recorded there): its root and its intervals from it (Sketchpad
 * renderFigure's chord); with none recorded, the key's I-V-vi-IV triads, a bar each. How many intervals */
static int mel_chord_at(const kit_song_t *s, int tick, int tonic, int minor, int *root, uint8_t *iv)
{
    uint8_t nt[16];
    int third, n = song_notes_at(s, tick, nt, 16), r = bass_chord_at(s, tick, tonic, minor, &third), i, k, m = 0;
    if (r < 0) return 0;
    *root = r;
    iv[m++] = 0;
    if (n >= 2) {
        uint16_t pcs = 0;
        for (i = 0; i < n; i++) pcs |= (uint16_t)(1u << pc12(nt[i] - r));
        for (k = 1; k < 12; k++) if (pcs >> k & 1u) iv[m++] = (uint8_t)k;
        return m;
    }
    iv[m++] = (uint8_t)third;
    iv[m++] = 7;
    return m;
}

/* ---- AUTO: phrases by style (the lab's, approved by the owner on 2026-11-13). A note: its sixteenth, a degree from
 * the chord's root in the key's scale (0 root, 2 third, 4 fifth, 7 octave, -1 below), its length. Bars alternate the
 * call (A) and the answer (B). ---- */
typedef struct { int8_t step, deg; uint8_t len; } mel_pnote_t;
#define MEL_PMAX 8
typedef struct { const char *name; uint8_t na, nb; mel_pnote_t a[MEL_PMAX], b[MEL_PMAX]; } mel_phrase_t;
typedef struct { const char *name; uint8_t n; mel_phrase_t ph[2]; } mel_pstyle_t;
static const mel_pstyle_t MEL_PHRASES[] = {
    { "Pop", 2, { { "Hook", 6, 5, { { 0, 4, 2 }, { 2, 4, 2 }, { 4, 5, 2 }, { 6, 4, 2 }, { 10, 2, 2 }, { 12, 0, 4 } }, { { 0, 4, 2 }, { 2, 4, 2 }, { 4, 5, 2 }, { 6, 7, 4 }, { 12, 4, 4 } } },
                  { "Climb", 6, 6, { { 0, 0, 2 }, { 2, 2, 2 }, { 4, 4, 4 }, { 8, 5, 2 }, { 10, 4, 2 }, { 12, 2, 4 } }, { { 0, 4, 2 }, { 2, 5, 2 }, { 4, 7, 4 }, { 8, 5, 2 }, { 10, 4, 2 }, { 12, 2, 4 } } } } },
    { "G-funk", 1, { { "Whistle", 4, 4, { { 0, 7, 6 }, { 6, 6, 2 }, { 8, 4, 6 }, { 14, 6, 2 } }, { { 0, 7, 4 }, { 4, 9, 4 }, { 8, 7, 6 }, { 14, 4, 2 } } } } },
    { "Ghibli", 2, { { "Arpeggio", 8, 6, { { 0, 0, 2 }, { 2, 2, 2 }, { 4, 4, 2 }, { 6, 7, 2 }, { 8, 9, 2 }, { 10, 7, 2 }, { 12, 4, 2 }, { 14, 2, 2 } }, { { 0, 0, 2 }, { 2, 2, 2 }, { 4, 4, 2 }, { 6, 7, 2 }, { 8, 11, 4 }, { 12, 9, 4 } } },
                     { "Fanfare", 5, 5, { { 0, 4, 3 }, { 3, 4, 1 }, { 4, 5, 2 }, { 6, 7, 6 }, { 12, 9, 4 } }, { { 0, 9, 3 }, { 3, 7, 1 }, { 4, 5, 2 }, { 6, 4, 6 }, { 12, 2, 4 } } } } },
    { "Neo soul", 1, { { "Lick", 7, 5, { { 2, 4, 1 }, { 3, 5, 1 }, { 4, 4, 2 }, { 7, 2, 1 }, { 8, 0, 3 }, { 12, -2, 2 }, { 14, 0, 2 } }, { { 0, 7, 3 }, { 4, 6, 1 }, { 5, 4, 1 }, { 6, 2, 2 }, { 10, 4, 6 } } } } },
    { "Reggaeton", 1, { { "Perreo", 6, 5, { { 0, 4, 3 }, { 3, 4, 3 }, { 6, 2, 2 }, { 8, 4, 3 }, { 11, 5, 3 }, { 14, 4, 2 } }, { { 0, 2, 3 }, { 3, 2, 3 }, { 6, 0, 2 }, { 8, 2, 3 }, { 11, 4, 5 } } } } },
    { "Shatta", 1, { { "De notes", 6, 6, { { 0, 4, 2 }, { 3, 5, 1 }, { 4, 4, 2 }, { 8, 4, 2 }, { 11, 5, 1 }, { 12, 4, 4 } }, { { 0, 4, 2 }, { 3, 5, 1 }, { 4, 4, 2 }, { 8, 5, 3 }, { 12, 4, 2 }, { 14, 3, 2 } } } } },
    { "Afrobeats", 1, { { "Pluck", 6, 5, { { 0, 4, 1 }, { 3, 4, 1 }, { 6, 5, 1 }, { 8, 4, 2 }, { 11, 2, 1 }, { 14, 0, 2 } }, { { 0, 7, 1 }, { 3, 7, 1 }, { 6, 5, 1 }, { 8, 4, 2 }, { 11, 2, 3 } } } } },
    { "Reggae", 1, { { "One drop", 4, 4, { { 2, 4, 2 }, { 6, 4, 2 }, { 8, 2, 4 }, { 14, 0, 2 } }, { { 2, 7, 2 }, { 6, 5, 2 }, { 8, 4, 6 }, { 14, 2, 2 } } } } },
    { "Cinematic", 1, { { "Long line", 3, 2, { { 0, 4, 8 }, { 8, 2, 4 }, { 12, 0, 4 } }, { { 0, 5, 8 }, { 8, 4, 8 } } } } },
    { "Boom bap", 1, { { "Sample riff", 6, 5, { { 0, 7, 2 }, { 3, 6, 1 }, { 4, 4, 2 }, { 8, 2, 2 }, { 10, 4, 2 }, { 14, 0, 2 } }, { { 0, 0, 2 }, { 3, 2, 1 }, { 4, 4, 2 }, { 8, 7, 4 }, { 14, 6, 2 } } } } },
    { "Disco", 1, { { "Octaves", 8, 7, { { 0, 0, 1 }, { 2, 7, 1 }, { 4, 0, 1 }, { 6, 7, 1 }, { 8, 4, 1 }, { 10, 5, 1 }, { 12, 4, 2 }, { 14, 2, 2 } }, { { 0, 0, 1 }, { 2, 7, 1 }, { 4, 0, 1 }, { 6, 7, 1 }, { 8, 9, 2 }, { 10, 7, 2 }, { 12, 4, 4 } } } } },
    { "Drum & bass", 1, { { "Liquid", 5, 4, { { 0, 7, 3 }, { 3, 4, 3 }, { 6, 5, 2 }, { 8, 4, 4 }, { 12, 2, 4 } }, { { 0, 9, 3 }, { 3, 7, 3 }, { 6, 4, 6 }, { 12, 2, 4 } } } } },
};
#define MEL_NPSTYLES ((int)(sizeof MEL_PHRASES / sizeof MEL_PHRASES[0]))
static const mel_phrase_t *mel_phrase(const mel_t *m) { const mel_pstyle_t *s = &MEL_PHRASES[m->astyle]; return &s->ph[m->aphrase % s->n]; }

/* a phrase note over a chord root: the degree in the key's scale counted from the root, moved into the melody's scale
 * (the nearest note of it: down a semitone first); near the keys' middle (C4) */
static int mel_phrase_note(const mel_t *m, int tonic, int minor, int deg, int root)
{
    const kit_scale_t *k = &KIT_SCALES[minor ? 1 : 0], *sc = mel_scale(m, minor);
    int i, at = -1, j, n, base = 60 + pc12(root);
    for (i = 0; i < 7; i++) if ((tonic + k->iv[i]) % 12 == pc12(root)) at = i;
    if (at < 0) at = 0;
    if (base > 67) base -= 12;
    j = at + deg;
    n = base + (k->iv[((j % 7) + 7) % 7] + 12 * (j >= 0 ? j / 7 : (j - 6) / 7)) - k->iv[at];
    {
        static const int8_t D[5] = { 0, -1, 1, -2, 2 };
        uint16_t sm = scale_mask(sc, tonic);
        for (i = 0; i < 5; i++) if (sm >> pc12(n + D[i]) & 1u) return n + D[i];
    }
    return n;
}
