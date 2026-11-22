/* SPDX-License-Identifier: GPL-3.0-only */
/* bass.c: the bass track's music, apart from the keys, lights and screen (D-040). app.c includes it after pad.c and
 * manual.c, and so does the unit test (factory/unit/test_music_bass.c).
 *
 * Three pages (SCL), as tried in the bass lab with the owner (docs/concepts/v0.5/BASS.md):
 *   PLAY   the keys play the bass an octave down; HELP lights the scale (Key), the chord now's root over it
 *          (Key+root), or makes the white keys the scale (Whites); OUTSIDE plays the notes outside it, softly or not
 *   IDEAS  a style's bassline falls onto its keys over the chord now; HEAR plays it (every other loop, always, never)
 *   AUTO   a degree pad plays a style's pattern on its root; FOLLOW plays it on the chords recorded
 * One scale for the track (SCALE), on the song's key: what PLAY lights, what Whites plays, and what IDEAS and AUTO keep
 * their notes inside. */

enum { BASS_PLAY = 0, BASS_IDEAS, BASS_AUTO, BASS_NPAGES };
enum { HELP_FREE = 0, HELP_KEY, HELP_ROOT, HELP_WHITES, BASS_NHELPS };
enum { OUT_PLAY = 0, OUT_SOFT, OUT_MUTE, BASS_NOUTS };
enum { HEAR_LISTEN = 0, HEAR_ON, HEAR_OFF, BASS_NHEARS };
enum { BPLUS_OFF = 0, BPLUS_OCT, BPLUS_FIFTH, BASS_NPLUS };   /* PLAY's K4 (ST-84, R-272): a second note with each */

/* the scales SCALE offers, by the key's mode, the key's own first (indexes into KIT_SCALES) */
static const uint8_t BASS_MINOR_SCALES[] = { 1, 3, 7, 4, 6, 5, 10, 11, 12 };
static const uint8_t BASS_MAJOR_SCALES[] = { 0, 2, 13, 8, 9, 14, 15, 16 };
#define BASS_NMINOR ((int)sizeof BASS_MINOR_SCALES)
#define BASS_NMAJOR ((int)sizeof BASS_MAJOR_SCALES)

/* the lines and patterns: a step (16ths) in the bar, the interval from the chord's root (BASS_THIRD: the chord's 3rd,
 * BASS_APPROACH: a step below the next chord's root), a length in 16ths. A starter set per style (the owner curates). */
#define BASS_THIRD 100
#define BASS_APPROACH 101
#define BASS_MAXN 8
typedef struct { int8_t step, iv; uint8_t len; } bass_note_t;
typedef struct { const char *name; uint8_t n; bass_note_t note[BASS_MAXN]; } bass_line_t;
typedef struct { const char *name; uint8_t n; bass_line_t line[2]; } bass_style_t;
static const bass_style_t BASS_STYLES[] = {
    { "G-funk", 2, { { "Slide line", 6, { { 0, 0, 3 }, { 3, 0, 2 }, { 6, 10, 2 }, { 8, 12, 3 }, { 11, 7, 2 }, { 14, 10, 2 } } },
                     { "Low and long", 4, { { 0, 0, 6 }, { 7, 7, 2 }, { 10, 10, 2 }, { 12, 12, 4 } } } } },
    { "Boom bap", 2, { { "Roots, pickup", 4, { { 0, 0, 4 }, { 7, 0, 1 }, { 10, 7, 2 }, { 14, -2, 2 } } },
                       { "Lazy roots", 4, { { 0, 0, 6 }, { 10, 0, 2 }, { 12, 3, 2 }, { 14, 5, 2 } } } } },
    { "Latin", 2, { { "Tresillo", 4, { { 0, 0, 3 }, { 6, 0, 2 }, { 8, 7, 3 }, { 14, 7, 2 } } },
                    { "Reggaeton", 6, { { 0, 0, 3 }, { 3, 0, 3 }, { 6, 0, 2 }, { 8, 0, 3 }, { 11, 0, 3 }, { 14, 0, 2 } } } } },
    { "Disco", 2, { { "Octaves", 8, { { 0, 0, 1 }, { 2, 12, 1 }, { 4, 0, 1 }, { 6, 12, 1 }, { 8, 0, 1 }, { 10, 12, 1 }, { 12, 0, 1 }, { 14, 12, 1 } } },
                    { "Root-fifth", 6, { { 0, 0, 2 }, { 3, 0, 1 }, { 6, 7, 2 }, { 8, 12, 2 }, { 11, 7, 1 }, { 14, 0, 2 } } } } },
    { "Blues", 2, { { "Boogie", 8, { { 0, 0, 3 }, { 3, 0, 1 }, { 4, 7, 3 }, { 7, 7, 1 }, { 8, 9, 3 }, { 11, 9, 1 }, { 12, 10, 3 }, { 15, 10, 1 } } },
                    { "Two-note", 8, { { 0, 0, 3 }, { 3, 0, 1 }, { 4, 0, 3 }, { 7, 0, 1 }, { 8, 7, 3 }, { 11, 7, 1 }, { 12, 7, 3 }, { 15, 7, 1 } } } } },
    { "Shatta", 2, { { "Bounce", 5, { { 0, 0, 3 }, { 3, 0, 3 }, { 6, 0, 2 }, { 10, 7, 2 }, { 12, 10, 4 } } },
                     { "Stab", 4, { { 0, 0, 2 }, { 6, 0, 2 }, { 8, 0, 2 }, { 14, -2, 2 } } } } },
    { "Jazz walk", 1, { { "Walking", 4, { { 0, 0, 4 }, { 4, BASS_THIRD, 4 }, { 8, 7, 4 }, { 12, BASS_APPROACH, 4 } } } } },
    /* ST-48 */
    { "Reggae", 2, { { "One drop", 5, { { 2, 0, 2 }, { 6, 0, 2 }, { 8, 7, 3 }, { 12, 5, 2 }, { 14, 3, 2 } } },
                     { "Roots, fifths", 4, { { 0, 0, 4 }, { 8, 7, 4 }, { 12, 0, 2 }, { 14, 12, 2 } } } } },
    { "Afrobeats", 2, { { "Bounce", 5, { { 0, 0, 2 }, { 3, 0, 1 }, { 6, 7, 2 }, { 10, 0, 2 }, { 12, 10, 2 } } },
                        { "Sub", 3, { { 0, 0, 6 }, { 10, 7, 2 }, { 12, 0, 4 } } } } },
    { "Reggaeton", 2, { { "Dembow", 4, { { 0, 0, 3 }, { 3, 0, 2 }, { 8, 0, 3 }, { 11, 0, 2 } } },
                        { "Climb", 4, { { 0, 0, 3 }, { 3, 7, 2 }, { 8, 0, 3 }, { 11, 10, 2 } } } } },
    { "Drum & bass", 2, { { "Sub roller", 3, { { 0, 0, 8 }, { 10, 0, 2 }, { 12, 7, 4 } } },
                          { "Reese", 2, { { 0, 0, 12 }, { 12, -2, 4 } } } } },
};
#define BASS_NSTYLES ((int)(sizeof BASS_STYLES / sizeof BASS_STYLES[0]))

typedef struct {
    int page;
    int help, scale, outside;           /* PLAY: scale is a place in the key's list */
    int scale_id;                       /* the scale chosen from the list of those that fit the chords (KIT_SCALES), or -1 */
    int style, line, hear, speed;       /* IDEAS (and AUTO's STYLE and PATTERN are style, line too); speed: an index into MEL_SPEEDS */
    int plus;                           /* PLAY's PLUS: off, the octave, the fifth (ST-84) */
    int astyle, aline, swing, follow;   /* AUTO */
} bass_t;

static void bass_init(bass_t *b)
{
    b->page = BASS_PLAY;
    b->help = HELP_KEY; b->scale = 0; b->outside = OUT_SOFT; b->scale_id = -1;
    b->style = 0; b->line = 0; b->hear = HEAR_LISTEN; b->speed = 6; b->plus = BPLUS_OFF;   /* speed 6: MEL_SPEEDS' 100 % */
    b->astyle = 2; b->aline = 0; b->swing = 0; b->follow = 0;
}

static int bass_nscales(int minor) { return minor ? BASS_NMINOR : BASS_NMAJOR; }
static const kit_scale_t *bass_scale(const bass_t *b, int minor)
{
    if (b->scale_id >= 0 && b->scale_id < KIT_NSCALES) return &KIT_SCALES[b->scale_id];
    int n = bass_nscales(minor), i = b->scale < n ? b->scale : n - 1;
    return &KIT_SCALES[minor ? BASS_MINOR_SCALES[i] : BASS_MAJOR_SCALES[i]];
}

static int pc12(int n) { return ((n % 12) + 12) % 12; }

/* a note's place on the bass track at this moment: 2 the chord now's root (Key+root), 1 in the scale (3 its tonic),
 * 0 outside it */
enum { ROLE_OUT = 0, ROLE_SCALE, ROLE_ROOT, ROLE_TONIC };
static int bass_role(const bass_t *b, int tonic, int minor, int pc, int chord_root)
{
    const kit_scale_t *s = bass_scale(b, minor);
    if (b->help == HELP_ROOT && chord_root >= 0 && pc12(pc) == chord_root) return ROLE_ROOT;
    if (!kit_scale_has(s, tonic, pc)) return ROLE_OUT;
    return pc12(pc) == tonic && b->help == HELP_KEY ? ROLE_TONIC : ROLE_SCALE;
}

/* Whites: white key w (0..15 from F3) plays the scale from its tonic, the first at or above low (the lowest bass note) */
static int bass_white_note(const bass_t *b, int tonic, int minor, int w, int low)
{
    const kit_scale_t *s = bass_scale(b, minor);
    int t0 = low + pc12(tonic - low);
    if (w < 0) return -1;
    return t0 + s->iv[w % s->n] + 12 * (w / s->n);
}

/* a line's note over a chord (root pitch class, minor 3rd or not), the next chord's root, placed from low (the
 * lowest bass note): the root the first of its name at or above low; a note outside the scale moves to the nearest */
static int bass_line_note(const bass_t *b, int tonic, int minor, const bass_note_t *x, int root, int third, int next_root, int low)
{
    const kit_scale_t *s = bass_scale(b, minor);
    int iv = x->iv, n;
    if (iv == BASS_THIRD) iv = third;
    else if (iv == BASS_APPROACH) iv = (pc12(next_root - root) ? pc12(next_root - root) : 12) - 1;
    n = low + pc12(root - low) + iv;
    if (!kit_scale_has(s, tonic, n)) n += kit_scale_has(s, tonic, n - 1) ? -1 : 1;
    if (n < low) n += 12;
    return n;
}

static const bass_line_t *bass_ideas_line(const bass_t *b) { const bass_style_t *s = &BASS_STYLES[b->style]; return &s->line[b->line % s->n]; }
static const bass_line_t *bass_auto_line(const bass_t *b) { const bass_style_t *s = &BASS_STYLES[b->astyle]; return &s->line[b->aline % s->n]; }

/* the chord the harmony's tracks (harm_mask) have around tick: its root and whether its 3rd is minor; -1 when none sounds. With no
 * chords recorded, the key's I-V-vi-IV, a bar each (so IDEAS and AUTO have something to follow) */
static int bass_chord_at(const kit_song_t *s, int tick, int tonic, int minor, int *third)
{
    uint8_t nt[16];
    int n = song_notes_at(s, tick, nt, 16), root = -1, i, t, has_chords = 0;
    FOR_HARM(s, t, i) { has_chords = 1; break; }
    *third = minor ? 3 : 4;
    if (n >= 2) {
        char lab[24];
        if (!kit_identify_chord(nt, n, tonic, minor, lab, sizeof lab, &root)) {
            int low = 128;
            for (i = 0; i < n; i++) if (nt[i] < low) low = nt[i];
            root = low % 12;
        }
        *third = 4;
        for (i = 0; i < n; i++) if (pc12(nt[i] - root) == 3) *third = 3;
        return root;
    }
    if (has_chords) return -1;
    {
        static const uint8_t DEG[4] = { 0, 4, 5, 3 };
        const kit_scale_t *k = &KIT_SCALES[minor ? 1 : 0];
        int d = DEG[(tick / KIT_TICKS_PER_BAR) % 4], r = (tonic + k->iv[d]) % 12, t = (tonic + k->iv[(d + 2) % 7]) % 12;
        *third = pc12(t - r);
        return r;
    }
}

/* the pitch classes the harmony's tracks (harm_mask) hold (in the song's length), for fitting scales to them */
static uint16_t bass_chord_pcs(const kit_song_t *s)
{
    uint16_t m = 0;
    int i, t, len = s->bars * KIT_TICKS_PER_BAR;
    FOR_HARM(s, t, i)
        if (s->notes[t][i].start < len) m |= (uint16_t)(1u << (s->notes[t][i].note % 12));
    return m;
}

/* the list SCALE walks: with chords recorded, every scale ranked by how much of them it holds (Sketchpad fitScales);
 * else the scales of the key's mode. ids: KIT_SCALES indexes; held, *nused: the fit (nused 0: no chords) */
static int bass_scale_list(const kit_song_t *s, int tonic, int minor, uint8_t *ids, uint8_t *held, int *nused)
{
    int n, i;
    uint16_t pcs = bass_chord_pcs(s);
    *nused = 0;
    if (pcs) return kit_fit_scales(pcs, tonic, ids, held, KIT_NSCALES, nused);
    n = bass_nscales(minor);
    for (i = 0; i < n; i++) { ids[i] = minor ? BASS_MINOR_SCALES[i] : BASS_MAJOR_SCALES[i]; held[i] = 0; }
    return n;
}

/* K2: the next or previous scale of that list */
static void bass_scale_step(bass_t *b, const kit_song_t *s, int tonic, int minor, int d)
{
    uint8_t ids[17], held[17];
    int nu, n = bass_scale_list(s, tonic, minor, ids, held, &nu), cur = (int)(bass_scale(b, minor) - KIT_SCALES), i, at = 0;
    for (i = 0; i < n; i++) if (ids[i] == cur) at = i;
    at = ((at + (d > 0 ? 1 : -1)) % n + n) % n;
    b->scale_id = ids[at];
    for (i = 0; i < bass_nscales(minor); i++)
        if ((minor ? BASS_MINOR_SCALES[i] : BASS_MAJOR_SCALES[i]) == ids[at]) b->scale = i;
}
