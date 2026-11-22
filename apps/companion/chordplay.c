/* SPDX-License-Identifier: GPL-3.0-only */
/* chordplay.c: the chords' PLAY page, HELP in steps (ST-79, D-075, R-246..R-253), as the owner tried it in the Chord Lab
 * (v1..v5, 2026-11-19). app.c includes it before draw_play.
 *
 *   Free     the keys play notes and the FM-1 names the chord; K2 TRANSPOSE
 *   Key      the key's notes dim, its tonic half; hold a note: the rest of a chord of the key with it lights full
 *            (K3 COLOUR colours it, SELECT the next chord that holds the notes held); the key's seven chords on a row,
 *            the chord held lit; K2 TRANSPOSE
 *   Shapes   the progression's chord full, its next chord half; SELECT moves on; K2 PROGRESSION, K3 COLOUR, K4 VOICING
 *   Pads     the white keys play the key's chords by letter (C = I), the black keys held a colour, F5 NEXT, F#5 LOCK,
 *            G5 AGAIN; the keyboard drawn with what it plays; K2 PROGRESSION, K3 VOICING, K4 FEEL (ST-80): Tight,
 *            Pianist, Laid back, Rolled, Strum up, Strum down, Sloppy (feel.c, ST-81)
 * PROGRESSION (Shapes, Pads) opens a numbered menu while it turns: None, Mine, Follow bass, Follow melody, then the
 * styles' progressions and Sketchpad's sets (R-257, R-259).
 *
 * TRANSPOSE whites shifts the keys so that the white keys play the song's key: a major key on C's white keys, a minor
 * key on A's (R-251). The chords here are the lab's: thirds stacked on the key's own scale, so a colour stays in the key
 * (the key's own seventh: G7, Dm7, Cmaj7), maj7 always a major seventh, dim the diminished seventh. */

enum { CC_NONE = 0, CC_SUS2, CC_SUS4, CC_6, CC_7, CC_MAJ7, CC_9, CC_ADD9, CC_11, CC_13, CC_DIM, CC_N };
static const char *const CC_NAMES[CC_N] = { "none", "sus2", "sus4", "6", "7", "maj7", "9", "add9", "11", "13", "dim" };
static const char *const PV_NAMES[KIT_NVOICINGS] = { "Root", "Inv 1", "Inv 2", "Open", "Wide" };
static const int8_t PH_NEXT[7][3] = { { 3, 4, 5 }, { 4, -1, -1 }, { 5, 3, -1 }, { 4, 0, 1 }, { 0, 5, -1 }, { 3, 1, 4 }, { 0, -1, -1 } };

static int play_trans[KIT_NTRACKS];             /* K2 TRANSPOSE on Free and Key: 0 off, 1 whites */
static int play_colour[KIT_NTRACKS];            /* K3 COLOUR on Key and Shapes: CC_* */
static int play_cand[KIT_NTRACKS];              /* Key: which of the chords that hold the notes held (SELECT) */
static int play_step[KIT_NTRACKS];              /* Shapes: the progression's chord now (SELECT) */
static int play_last_deg[KIT_NTRACKS] = { -1, -1, -1, -1 };   /* Key: the key's chord played last, for the next ones */

static int pmod(int a, int n) { return ((a % n) + n) % n; }

/* the semitones the keys move while TRANSPOSE is whites: the key's tonic onto C (major) or A (minor), -5..+6 */
static int play_shift(void)
{
    int s;
    if (!is_play() || play_helps[track] > PH_KEY || !play_trans[track]) return 0;
    s = pmod(pads[track].tonic - (pads[track].minor ? 9 : 0), 12);
    return s > 6 ? s - 12 : s;
}

/* the key's scale note i steps above degree d's root, as semitones above the tonic (may pass 12) */
static int ph_scale(int d, int i)
{
    const kit_scale_t *sc = &KIT_SCALES[pads[track].minor ? 1 : 0];
    int j = d + i;
    return pads[track].tonic + sc->iv[j % 7] + 12 * (j / 7);
}

/* degree d's chord with a colour: pitch classes as semitones, root first (the lab's degChord) */
static int ph_deg_notes(int d, int colour, int *out)
{
    int n = 0, r = ph_scale(d, 0);
    out[n++] = r;
    switch (colour) {
    case CC_SUS2: out[n++] = ph_scale(d, 1); out[n++] = ph_scale(d, 4); break;
    case CC_SUS4: out[n++] = ph_scale(d, 3); out[n++] = ph_scale(d, 4); break;
    case CC_DIM: out[n++] = r + 3; out[n++] = r + 6; out[n++] = r + 9; break;
    default:
        out[n++] = ph_scale(d, 2); out[n++] = ph_scale(d, 4);
        if (colour == CC_6) out[n++] = ph_scale(d, 5);
        if (colour == CC_7 || colour == CC_9 || colour == CC_11 || colour == CC_13) out[n++] = ph_scale(d, 6);
        if (colour == CC_MAJ7) out[n++] = r + 11;
        if (colour == CC_9 || colour == CC_ADD9 || colour == CC_11 || colour == CC_13) out[n++] = ph_scale(d, 8);
        if (colour == CC_11) out[n++] = ph_scale(d, 10);
        if (colour == CC_13) out[n++] = ph_scale(d, 12);
    }
    return n;
}

static uint16_t ph_mask(const int *pcs, int n)
{
    uint16_t m = 0;
    int i;
    for (i = 0; i < n; i++) m |= (uint16_t)(1u << pmod(pcs[i], 12));
    return m;
}

/* a chord voiced on the keys (the lab's voice and fitKeys): the root from A3 up, the rest stacked above it, then the
 * voicing; moved by octaves, the highest notes folded down, until every note is a key */
static int ph_voice(const int *pcs, int n, int voicing, uint8_t *out)
{
    int v[KIT_CHORD_MAX + 1], i, j, m = 0, lo = kb_low(), hi = kb_low() + KIT_NKEYS - 1;
    if (n > KIT_CHORD_MAX) n = KIT_CHORD_MAX;
    v[m++] = 57 + pmod(pcs[0] - 57, 12);
    for (i = 1; i < n; i++) {
        int x = v[0] + pmod(pcs[i] - pcs[0], 12);
        while (x <= v[m - 1]) x += 12;
        v[m++] = x;
    }
    if (voicing == KIT_VOICE_INV1 && m > 2) { int r = v[0]; for (i = 1; i < m; i++) v[i - 1] = v[i]; v[m - 1] = r + 12; }
    if (voicing == KIT_VOICE_INV2 && m > 2) {
        for (j = 0; j < 2; j++) { int r = v[0]; for (i = 1; i < m; i++) v[i - 1] = v[i]; v[m - 1] = r + 12; }
    }
    if (voicing == KIT_VOICE_OPEN && m > 2) v[0] -= 12;
    if (voicing == KIT_VOICE_WIDE) { v[0] -= 12; for (i = 2; i < m; i++) v[i] += 12; }
    for (i = 0; i < m; i++) for (j = i + 1; j < m; j++) if (v[j] < v[i]) { int t = v[i]; v[i] = v[j]; v[j] = t; }
    while (v[m - 1] > hi && v[0] - 12 >= lo) for (i = 0; i < m; i++) v[i] -= 12;
    while (v[0] < lo) for (i = 0; i < m; i++) v[i] += 12;
    for (i = 0; i < m; i++) while (v[i] > hi) v[i] -= 12;
    for (i = 0; i < m; i++) for (j = i + 1; j < m; j++) if (v[j] < v[i]) { int t = v[i]; v[i] = v[j]; v[j] = t; }
    for (i = 0, j = 0; i < m; i++) if (!j || v[i] != out[j - 1]) out[j++] = (uint8_t)v[i];
    return j;
}

/* the progressions on Shapes and Pads (ST-80, R-257, R-259): None, Mine, Follow bass and Follow melody, then our styles'
 * (pad.c's, and Rock) and Sketchpad's 14 chord sets (core/chordsets.mjs), as in the Chord Lab. A step is a degree of
 * the key with a colour, or a root (semitones above the progression's home) with its intervals: Sketchpad's sets are
 * written for a mode, and one in the other mode sits on the relative key. */
typedef struct { int8_t deg; uint8_t col; int8_t off; uint16_t iv; const char *sym; } ph_step_t;
typedef struct { const char *style, *name; uint8_t mode, n; ph_step_t s[4]; } ph_prog_t;   /* mode 0 any, 1 major, 2 minor */
#define D_(d, c) { d, c, 0, 0, 0 }
#define R_(o, sym, iv) { -1, 0, o, iv, sym }
#define IV_(a, b, c, d, e, f) (uint16_t)((1u << (a)) | (1u << (b)) | (1u << (c)) | (1u << (d)) | (1u << (e)) | (1u << (f)))
#define M9_ IV_(0, 2, 3, 7, 10, 10)
#define M7_ IV_(0, 3, 7, 10, 10, 10)
#define MAJ7_ IV_(0, 4, 7, 11, 11, 11)
#define DOM7_ IV_(0, 4, 7, 10, 10, 10)
#define MAJ9_ IV_(0, 2, 4, 7, 11, 11)
#define MIN_ IV_(0, 3, 7, 7, 7, 7)
#define MAJ_ IV_(0, 4, 7, 7, 7, 7)
#define SUS4_ IV_(0, 5, 7, 7, 7, 7)
#define MAJ13_ IV_(0, 2, 4, 7, 9, 11)
#define MAJ7S11_ IV_(0, 4, 6, 7, 11, 11)
#define M11_ IV_(0, 2, 3, 5, 7, 10)
#define MADD9_ IV_(0, 2, 3, 7, 7, 7)
#define SUS49_ IV_(0, 2, 5, 7, 7, 7)
#define M13_ IV_(0, 2, 3, 7, 9, 10)
#define DOM9_ IV_(0, 2, 4, 7, 10, 10)
static const ph_prog_t PH_PROGS[] = {
    { "", "None", 0, 0, { D_(0, CC_NONE) } }, { "", "Mine", 0, 0, { D_(0, CC_NONE) } },
    { "", "Follow bass", 0, 0, { D_(0, CC_NONE) } }, { "", "Follow melody", 0, 0, { D_(0, CC_NONE) } },
    { "Pop", "I-V-vi-IV", 0, 4, { D_(0, CC_NONE), D_(4, CC_NONE), D_(5, CC_NONE), D_(3, CC_NONE) } },
    { "Pop", "I-vi-IV-V", 0, 4, { D_(0, CC_NONE), D_(5, CC_NONE), D_(3, CC_NONE), D_(4, CC_NONE) } },
    { "Pop", "vi-IV-I-V", 0, 4, { D_(5, CC_NONE), D_(3, CC_ADD9), D_(0, CC_NONE), D_(4, CC_SUS4) } },
    { "Neo soul", "ii-V-I", 0, 3, { D_(1, CC_9), D_(4, CC_13), D_(0, CC_MAJ7) } },
    { "Neo soul", "IV-iii-ii-I", 0, 4, { D_(3, CC_MAJ7), D_(2, CC_7), D_(1, CC_9), D_(0, CC_MAJ7) } },
    { "Neo soul", "IV-V-iii-vi", 0, 4, { D_(3, CC_MAJ7), D_(4, CC_9), D_(2, CC_7), D_(5, CC_9) } },
    { "Hip hop", "vi-IV", 0, 2, { D_(5, CC_7), D_(3, CC_MAJ7) } },
    { "Hip hop", "vi-iii-IV", 0, 3, { D_(5, CC_9), D_(2, CC_7), D_(3, CC_MAJ7) } },
    { "Hip hop", "ii-V", 0, 2, { D_(1, CC_9), D_(4, CC_13) } },
    { "West coast", "vi-ii", 0, 2, { D_(5, CC_9), D_(1, CC_9) } },
    { "West coast", "vi-IV-V", 0, 3, { D_(5, CC_7), D_(3, CC_MAJ7), D_(4, CC_7) } },
    { "West coast", "iii-vi", 0, 2, { D_(2, CC_7), D_(5, CC_9) } },
    { "Funk", "I vamp", 0, 1, { D_(0, CC_9) } },
    { "Funk", "I-IV", 0, 2, { D_(0, CC_9), D_(3, CC_9) } },
    { "Funk", "ii-V", 0, 2, { D_(1, CC_7), D_(4, CC_9) } },
    { "Electronic", "vi-IV-I-V", 0, 4, { D_(5, CC_ADD9), D_(3, CC_ADD9), D_(0, CC_SUS2), D_(4, CC_SUS4) } },
    { "Electronic", "I-V-vi-IV", 0, 4, { D_(0, CC_ADD9), D_(4, CC_NONE), D_(5, CC_ADD9), D_(3, CC_ADD9) } },
    { "Electronic", "IV-V-vi", 0, 3, { D_(3, CC_ADD9), D_(4, CC_SUS4), D_(5, CC_NONE) } },
    { "Jazz", "ii-V-I", 0, 3, { D_(1, CC_7), D_(4, CC_7), D_(0, CC_MAJ7) } },
    { "Jazz", "I-vi-ii-V", 0, 4, { D_(0, CC_MAJ7), D_(5, CC_7), D_(1, CC_7), D_(4, CC_7) } },
    { "Jazz", "iii-vi-ii-V", 0, 4, { D_(2, CC_7), D_(5, CC_7), D_(1, CC_7), D_(4, CC_13) } },
    { "Reggae", "I-IV", 0, 2, { D_(0, CC_NONE), D_(3, CC_NONE) } },
    { "Reggae", "vi-ii", 0, 2, { D_(5, CC_NONE), D_(1, CC_NONE) } },
    { "Reggae", "I-IV-V", 0, 3, { D_(0, CC_NONE), D_(3, CC_NONE), D_(4, CC_NONE) } },
    { "Afrobeats", "IV-V-vi", 0, 3, { D_(3, CC_MAJ7), D_(4, CC_NONE), D_(5, CC_7) } },
    { "Afrobeats", "ii-iii-IV", 0, 3, { D_(1, CC_7), D_(2, CC_7), D_(3, CC_MAJ7) } },
    { "Afrobeats", "vi-IV-I-V", 0, 4, { D_(5, CC_7), D_(3, CC_ADD9), D_(0, CC_ADD9), D_(4, CC_NONE) } },
    { "Shatta", "vi-V", 0, 2, { D_(5, CC_NONE), D_(4, CC_NONE) } },
    { "Shatta", "vi-IV-V", 0, 3, { D_(5, CC_NONE), D_(3, CC_NONE), D_(4, CC_NONE) } },
    { "Reggaeton", "vi-IV-I-V", 0, 4, { D_(5, CC_NONE), D_(3, CC_NONE), D_(0, CC_NONE), D_(4, CC_NONE) } },
    { "Reggaeton", "vi-V-IV-V", 0, 4, { D_(5, CC_NONE), D_(4, CC_NONE), D_(3, CC_NONE), D_(4, CC_NONE) } },
    { "Drum & bass", "vi-IV-V-iii", 0, 4, { D_(5, CC_9), D_(3, CC_MAJ7), D_(4, CC_9), D_(2, CC_7) } },
    { "Drum & bass", "ii-V-I", 0, 3, { D_(1, CC_9), D_(4, CC_9), D_(0, CC_MAJ7) } },
    { "Rock", "I-bVII-IV", 0, 4, { D_(0, CC_NONE), R_(10, "", MAJ_), D_(3, CC_NONE), D_(0, CC_NONE) } },
    /* Sketchpad's sets (core/chordsets.mjs): the slots its progression plays, in order */
    { "Sketchpad", "Neo-soul keys", 2, 4, { R_(0, "m9", M9_), R_(8, "maj7", MAJ7_), R_(3, "maj9", MAJ9_), R_(10, "7", DOM7_) } },
    { "Sketchpad", "Lo-fi loop", 2, 4, { R_(0, "m7", M7_), R_(3, "maj7", MAJ7_), R_(5, "m7", M7_), R_(10, "maj7", MAJ7_) } },
    { "Sketchpad", "House stabs", 2, 4, { R_(0, "m7", M7_), R_(5, "m7", M7_), R_(3, "maj7", MAJ7_), R_(10, "7", DOM7_) } },
    { "Sketchpad", "Soul changes", 1, 4, { R_(0, "maj7", MAJ7_), R_(9, "m7", M7_), R_(2, "m7", M7_), R_(7, "7", DOM7_) } },
    { "Sketchpad", "ii - V - I", 1, 4, { R_(2, "m7", M7_), R_(7, "7", DOM7_), R_(0, "maj7", MAJ7_), R_(0, "maj7", MAJ7_) } },
    { "Sketchpad", "West coast", 2, 4, { R_(0, "m9", M9_), R_(10, "maj7", MAJ7_), R_(5, "m9", M9_), R_(0, "m7", M7_) } },
    { "Sketchpad", "Dark loop", 2, 4, { R_(0, "m", MIN_), R_(1, "maj7", MAJ7_), R_(0, "m", MIN_), R_(8, "maj7", MAJ7_) } },
    { "Sketchpad", "Gospel turns", 1, 4, { R_(0, "maj7", MAJ7_), R_(4, "m7", M7_), R_(2, "m7", M7_), R_(7, "7", DOM7_) } },
    { "Sketchpad", "Bossa", 1, 4, { R_(2, "m7", M7_), R_(7, "9", DOM9_), R_(0, "maj9", MAJ9_), R_(0, "maj9", MAJ9_) } },
    { "Sketchpad", "Boom bap", 2, 4, { R_(0, "m7", M7_), R_(8, "maj9", MAJ9_), R_(10, "7", DOM7_), R_(0, "m7", M7_) } },
    { "Sketchpad", "Ambient major", 1, 4, { R_(0, "maj9", MAJ9_), R_(5, "maj7", MAJ7_), R_(9, "m9", M9_), R_(7, "sus4", SUS4_) } },
    { "Sketchpad", "Storybook", 1, 4, { R_(0, "maj13", MAJ13_), R_(5, "maj7#11", MAJ7S11_), R_(9, "m11", M11_), R_(7, "sus4(9)", SUS49_) } },
    { "Sketchpad", "Wandering minor", 2, 4, { R_(0, "m11", M11_), R_(8, "maj13", MAJ13_), R_(5, "m13", M13_), R_(3, "maj7#11", MAJ7S11_) } },
    { "Sketchpad", "Cinematic minor", 2, 4, { R_(0, "m", MIN_), R_(8, "maj7", MAJ7_), R_(10, "", MAJ_), R_(3, "maj7", MAJ7_) } },
};
#define PH_NPROGS ((int)(sizeof PH_PROGS / sizeof PH_PROGS[0]))
enum { PG_NONE = 0, PG_MINE, PG_FBASS, PG_FMEL, PG_FIRST };
#define PH_PROG_START 4                         /* 5 Pop I-V-vi-IV, where the Chord Lab starts (D-084) */
static int play_prog[KIT_NTRACKS] = { PH_PROG_START, PH_PROG_START, PH_PROG_START, PH_PROG_START };

typedef struct { int8_t deg; uint8_t col; int8_t root; uint16_t iv; } ph_chord_t;   /* deg >= 0, or root + iv */
#define PH_MINE 8
static ph_chord_t ph_mine[KIT_NTRACKS][PH_MINE];   /* Mine: the last chords played on the pads */
static int ph_nmine[KIT_NTRACKS];
static ph_chord_t ph_last_pad[KIT_NTRACKS] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

/* a chord's pitch classes, root first; and its degree in the key (-1: none) */
static int ph_chord_pcs(const ph_chord_t *c, int *pcs, int *deg)
{
    int n = 0, i;
    if (c->deg >= 0) { *deg = c->deg; return ph_deg_notes(c->deg, c->col, pcs); }
    for (i = 0; i < 12; i++) if (c->iv >> i & 1u) pcs[n++] = c->root + i;
    {
        const kit_scale_t *sc = &KIT_SCALES[pads[track].minor ? 1 : 0];
        uint16_t m = ph_mask(pcs, n), tri;
        int d;
        *deg = -1;
        for (i = 0; i < n; i++) if (!kit_scale_has(sc, pads[track].tonic, pcs[i])) return n;
        for (d = 0; d < 7; d++) {
            int t[8], k = ph_deg_notes(d, CC_NONE, t);
            tri = ph_mask(t, k);
            if (pmod(t[0], 12) == pmod(c->root, 12) && (m & tri) == tri) *deg = d;
        }
    }
    return n;
}

/* the recorded notes of the first track with role r sounding now (Follow bass, Follow melody) */
static int ph_track_notes(int r, uint8_t *out, int max)
{
    int t, n = 0;
    uint8_t keep = harm_mask;
    for (t = 0; t < KIT_NTRACKS && !n; t++)
        if (set.role[t] == r) { harm_mask = (uint8_t)(1u << t); n = song_notes_at(&song, kit_seq_playing() ? kit_seq_tick() : 0, out, max); }
    harm_mask = keep;
    return n;
}

/* the progression's chord i (any integer) */
static void ph_prog_chord(int i, ph_chord_t *c)
{
    const ph_prog_t *p = &PH_PROGS[play_prog[track]];
    uint8_t nt[16];
    int n, d, k;
    c->deg = 0; c->col = CC_NONE; c->root = 0; c->iv = 0;
    switch (play_prog[track]) {
    case PG_NONE: *c = ph_last_pad[track]; return;
    case PG_MINE: if (ph_nmine[track]) *c = ph_mine[track][pmod(i, ph_nmine[track])]; return;
    case PG_FBASS:                              /* the chord of the key on the bass's lowest note */
        n = ph_track_notes(ROLE_BASS, nt, 16);
        if (n) {
            int lo = nt[0];
            for (k = 1; k < n; k++) if (nt[k] < lo) lo = nt[k];
            for (d = 0; d < 7; d++) if (pmod(ph_scale(d, 0), 12) == lo % 12) { c->deg = (int8_t)d; return; }
        }
        return;
    case PG_FMEL: {                             /* the key's first chord, of I IV V vi ii iii vii, with the melody's note */
        static const int8_t ORDER[7] = { 0, 3, 4, 5, 1, 2, 6 };
        n = ph_track_notes(ROLE_MELODY, nt, 16);
        if (n)
            for (k = 0; k < 7; k++) {
                int pcs[8], m = ph_deg_notes(ORDER[k], CC_NONE, pcs);
                if (ph_mask(pcs, m) >> (nt[n - 1] % 12) & 1u) { c->deg = ORDER[k]; return; }
            }
        return;
    }
    }
    {
        const ph_step_t *s = &p->s[pmod(i, p->n)];
        int home = pads[track].tonic;
        if (p->mode == 2 && !pads[track].minor) home += 9;          /* a minor set in a major key: its relative minor */
        if (p->mode == 1 && pads[track].minor) home += 3;
        if (s->deg >= 0) { c->deg = s->deg; c->col = s->col; }
        else { c->deg = -1; c->root = (int8_t)pmod(home + s->off, 12); c->iv = s->iv; }
    }
}

static int ph_prog_len(void)
{
    if (play_prog[track] == PG_MINE) return ph_nmine[track] ? ph_nmine[track] : 1;
    return play_prog[track] < PG_FIRST ? 1 : PH_PROGS[play_prog[track]].n;
}

/* Shapes: the progression's chord i voiced on the keys, with K3 COLOUR over its own; its degree in *deg */
static int ph_shape(int i, uint8_t *out, int *deg)
{
    ph_chord_t c;
    int pcs[8], n;
    ph_prog_chord(i, &c);
    if (c.deg >= 0 && play_colour[track] != CC_NONE) c.col = (uint8_t)play_colour[track];
    n = ph_chord_pcs(&c, pcs, deg);
    return ph_voice(pcs, n, pads[track].voicing, out);
}

/* a progression's numerals, in the song's key: "ii9 V13 Imaj7" (R-257) */
static void ph_numerals(int pi, char *out, int len)
{
    static const char *const RB[12] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    const ph_prog_t *p = &PH_PROGS[pi];
    kit_chord_t tri[7];
    char *d = out, *end = out + len - 12;
    int i;
    out[0] = 0;
    if (pi < PG_FIRST) {
        cat(out, pi == PG_NONE ? "all 7 chords, no order" : pi == PG_MINE ? "the pads you just played" : pi == PG_FBASS ? "the chord on the bass note" : "the chord with the melody note");   /* the lab's words (D-084) */
        return;
    }
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    for (i = 0; i < p->n && d < end; i++) {
        const ph_step_t *s = &p->s[i];
        if (i) d = cat(d, "  ");
        if (s->deg >= 0) {
            d = cat(d, tri[s->deg].roman);
            if (s->col != CC_NONE && s->col != CC_DIM) d = cat(d, CC_NAMES[s->col]);
        } else {
            int home = pads[track].tonic + (p->mode == 2 && !pads[track].minor ? 9 : p->mode == 1 && pads[track].minor ? 3 : 0);
            d = cat(d, RB[pmod(home + s->off - pads[track].tonic, 12)]);
            d = cat(d, s->sym);
        }
    }
}

/* Key: the key's chords, with COLOUR, that hold every note held, those with the lowest note as root first, then as
 * 3rd, then as 5th (R-248) */
static int ph_cands(const uint8_t *held, int nh, int *out)
{
    int pos, d, n = 0, pcs[8], k;
    uint16_t hm = 0;
    for (k = 0; k < nh; k++) hm |= (uint16_t)(1u << (held[k] % 12));
    for (pos = 0; pos < 3; pos++)
        for (d = 0; d < 7; d++)
            if (pmod(ph_scale(d, pos * 2), 12) == held[0] % 12) {
                int m = ph_deg_notes(d, play_colour[track], pcs);
                if (!(hm & ~ph_mask(pcs, m))) out[n++] = d;
            }
    return n;
}

/* the key a sounding note is on, nearest to `near`, through TRANSPOSE (-1: none) */
static int ph_key_near(int pc, int near)
{
    int k, best = -1, bd = 999, sh = play_shift();
    for (k = 0; k < KIT_NKEYS; k++) {
        int s = key_note(k) + sh, dd = s > near ? s - near : near - s;
        if (pmod(s, 12) == pmod(pc, 12) && dd < bd) { bd = dd; best = k; }
    }
    return best;
}

static void ph_light(uint8_t *lv, int k, int level)   /* off < dim < half < on */
{
    static const uint8_t RANK[] = { 0, 1, 3, 0, 2 };
    if (k >= 0 && k < KIT_NKEYS && RANK[level] > RANK[lv[k]]) lv[k] = (uint8_t)level;
}

/* ---- Pads (ST-80, R-254..R-258): C = I on the white keys in every key, ten colours on the black keys ---- */
/* the white keys F3..E5 play the key's chords by their letter (C = I, D = ii ... B = vii), in their own octave; the
 * black keys F#3..D#5 held add a colour; F5 NEXT, F#5 LOCK, G5 AGAIN (the Chord Lab's layout) */
#define PK_NEXT 24
#define PK_LOCK 25
#define PK_AGAIN 26
static const int8_t PH_BLACK[KIT_NKEYS] = { -1, CC_SUS2, -1, CC_SUS4, -1, CC_6, -1, -1, CC_7, -1, CC_MAJ7, -1, -1, CC_9, -1,
                                            CC_ADD9, -1, CC_11, -1, -1, CC_13, -1, CC_DIM, -1, -1, -1, -1 };
static const char *const PH_BLACK_LAB[CC_N] = { "", "s2", "s4", "6", "7", "M7", "9", "a9", "11", "13", "o" };
static int ph_lock[KIT_NTRACKS];                /* LOCK: a colour on every pad (CC_NONE: none) */
static int ph_pad_oct[KIT_NTRACKS];             /* the octave of the last pad: -12, 0, 12 */
static int ph_pad_key = -1;                     /* the key sounding the chord */
static ph_chord_t ph_now[KIT_NTRACKS];          /* the chord played last */
static int play_feel[KIT_NTRACKS];              /* K4 FEEL on Pads: FEEL_* (ST-81, R-260) */
static struct { uint8_t note, vel; uint32_t due; } ph_q[KIT_CHORD_MAX];   /* the chord's notes still to start */
static int ph_nq;
static uint32_t ph_rng = 1u;
static int ph_now_on[KIT_NTRACKS];

static int ph_pad_deg(int key)                  /* a white key's degree by its letter (-1: none) */
{
    static const int8_t L[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    return key < 0 || key >= PK_NEXT ? -1 : L[(KIT_KEY0_NOTE + key) % 12];
}

static int ph_held_colour(void)                 /* the black key held pressed last, else LOCK */
{
    int k, best = -1;
    uint32_t at = 0;
    for (k = 0; k < PK_NEXT; k++)
        if (PH_BLACK[k] >= 0 && kit_key_held(k) && black_at[k] >= at) { at = black_at[k]; best = PH_BLACK[k]; }
    return best >= 0 ? best : ph_lock[track];
}

/* play a chord from a key: voiced on the keys, then in the pad's octave; the way FEEL says */
static void ph_play(int key, const ph_chord_t *c, int vel)
{
    int pcs[8], n, deg, i;
    uint8_t v[KIT_CHORD_MAX];
    kit_vchord_t *now = &pads[track].now;
    chord_release();
    n = ph_chord_pcs(c, pcs, &deg);
    n = ph_voice(pcs, n, pads[track].voicing, v);
    now->n = 0;
    for (i = 0; i < n; i++) {
        int x = v[i] + ph_pad_oct[track];
        if (x >= 0 && x < 128) now->notes[now->n++] = (uint8_t)x;
    }
    now->root = (uint8_t)pmod(pcs[0], 12);
    now->bass = 0xFF;
    now->sym = 0;
    pads[track].now_degree = deg;
    pads[track].now_step = -1;
    ph_now[track] = *c;
    ph_now_on[track] = 1;
    if (c->deg >= 0) ph_last_pad[track] = *c;
    chord_key = key;                            /* FEEL: each note at its time and velocity (R-260) */
    chord_vel = vel;
    strum_i = now->n;
    ph_nq = 0;
    for (i = 0; i < now->n; i++) {
        int dl, vp, vv;
        feel_note(play_feel[track], i, now->n, (int)kit_beat_ms(song.bpm) / 4, &ph_rng, &dl, &vp);
        vv = vel * vp / 100;
        vv = vv < 1 ? 1 : vv > 127 ? 127 : vv;
        if (dl <= 0) note_on(now->notes[i], vv);
        else { ph_q[ph_nq].note = now->notes[i]; ph_q[ph_nq].vel = (uint8_t)vv; ph_q[ph_nq].due = kit_ms() + (uint32_t)dl; ph_nq++; }
    }
    dirty = 1;
}

static void ph_feel_poll(uint32_t now)
{
    int i, j;
    if (chord_key < 0 || !is_play_pads()) { ph_nq = 0; return; }   /* let go, or left: the rest never starts */
    for (i = 0; i < ph_nq; )
        if ((int32_t)(now - ph_q[i].due) >= 0) {
            note_on(ph_q[i].note, ph_q[i].vel);
            for (j = i + 1; j < ph_nq; j++) ph_q[j - 1] = ph_q[j];
            ph_nq--;
        } else {
            i++;
        }
}

static void ph_pad_down(int key, int vel)
{
    int d = ph_pad_deg(key);
    ph_chord_t c;
    if (d >= 0) {
        int col = ph_held_colour(), note = KIT_KEY0_NOTE + key;
        ph_pad_oct[track] = note < 60 ? -12 : note >= 72 ? 12 : 0;   /* the pad's own octave (R-254) */
        c.deg = (int8_t)d; c.col = (uint8_t)(col >= 0 ? col : CC_NONE); c.root = 0; c.iv = 0;
        if (ph_nmine[track] == PH_MINE) { int i; for (i = 1; i < PH_MINE; i++) ph_mine[track][i - 1] = ph_mine[track][i]; ph_nmine[track]--; }
        ph_mine[track][ph_nmine[track]++] = c;  /* Mine: the last eight */
        ph_play(key, &c, vel);
    } else if (key == PK_NEXT) {                /* the progression's next chord, in the last pad's octave (R-258) */
        play_step[track]++;
        ph_prog_chord(play_step[track], &c);
        if (c.deg >= 0 && ph_lock[track] != CC_NONE) c.col = (uint8_t)ph_lock[track];
        ph_play(key, &c, vel);
    } else if (key == PK_AGAIN) {
        if (ph_now_on[track]) ph_play(key, &ph_now[track], vel);
    } else if (key == PK_LOCK) {                /* a colour held: lock it; again: unlock */
        int k, h = -1;
        uint32_t at = 0;
        for (k = 0; k < PK_NEXT; k++)
            if (PH_BLACK[k] >= 0 && kit_key_held(k) && black_at[k] >= at) { at = black_at[k]; h = PH_BLACK[k]; }
        ph_lock[track] = h >= 0 && h != ph_lock[track] ? h : CC_NONE;
    } else if (PH_BLACK[key] >= 0) {
        black_at[key] = ++presses;
    }
    dirty = 1;
}

static void ph_pad_up(int key)
{
    if (key == chord_key) chord_release();
    dirty = 1;
}

/* the lights on Pads: the pads and colours dim, the next chord's pads half, the chord now and what is held full, LOCK
 * full while locked */
static void ph_pads_lights(uint8_t *lv)
{
    ph_chord_t nx;
    int k;
    ph_prog_chord(play_step[track] + 1, &nx);
    for (k = 0; k < KIT_NKEYS; k++) {
        int d = ph_pad_deg(k);
        if (d >= 0 || PH_BLACK[k] >= 0 || k == PK_NEXT || k == PK_AGAIN) ph_light(lv, k, KIT_LIGHT_DIM);
        if (d >= 0 && play_prog[track] != PG_NONE && d == nx.deg) ph_light(lv, k, KIT_LIGHT_HALF);
        if (k == PK_NEXT && play_prog[track] != PG_NONE) ph_light(lv, k, KIT_LIGHT_HALF);
        if (PH_BLACK[k] >= 0 && PH_BLACK[k] == ph_lock[track]) ph_light(lv, k, KIT_LIGHT_ON);
        if (k == PK_LOCK && ph_lock[track] != CC_NONE) ph_light(lv, k, KIT_LIGHT_ON);
        if (kit_key_held(k)) ph_light(lv, k, KIT_LIGHT_ON);
    }
}

/* the keyboard drawn with what each key plays (R-256): a white key its chord and numeral, a black key its colour,
 * F5 NX, F#5 LK, G5 AG; the chord now lit, the next one half */
static void ph_draw_keys(int y)
{
    kit_chord_t tri[7];
    ph_chord_t nx;
    int k, wi = 0;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    ph_prog_chord(play_step[track] + 1, &nx);
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k;
        if (!kit_is_white(n)) continue;
        {
            int x = wi * 15, d = ph_pad_deg(k), on = kit_key_held(k), hf = 0;
            const char *nm, *q = "";
            char num[2] = { 0, 0 };
            if (d >= 0) {                       /* the root, its quality under it, the degree at the bottom */
                on = on || (k == chord_key);
                hf = !on && play_prog[track] != PG_NONE && d == nx.deg;
                nm = kit_note_name_in_key(tri[d].root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS);
                q = tri[d].quality == KIT_Q_MINOR ? "m" : tri[d].quality == KIT_Q_DIM ? "o" : "";
                num[0] = (char)('1' + d);
            } else {
                nm = k == PK_NEXT ? "NX" : "AG";
                on = on || k == chord_key;
            }
            kit_round_rect(x + 1, y + 26, 13, 54, 3, on ? ACC : hf ? ACC_D : CARD);
            ctext(x + 8, y + 38, nm, KIT_FONT_TINY, on ? BG : FG);
            if (q[0]) ctext(x + 8, y + 51, q, KIT_FONT_TINY, on ? BG : MUTED);
            if (num[0]) ctext(x + 8, y + 70, num, KIT_FONT_TINY, on ? BG : hf ? ACC : MUTED);
        }
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k;
        if (kit_is_white(n)) { wi++; continue; }
        {
            int x = wi * 15 - 7, c = PH_BLACK[k], on = kit_key_held(k) || (c >= 0 && c == ph_lock[track]) || (k == PK_LOCK && ph_lock[track] != CC_NONE);
            kit_round_rect(x, y, 14, 22, 3, on ? ACC : CARD2);
            ctext(x + 7, y + 11, c >= 0 ? PH_BLACK_LAB[c] : k == PK_LOCK ? "LK" : "", KIT_FONT_TINY, on ? BG : MUTED);
        }
    }
}

/* the progression menu (R-257): while PROGRESSION turns, the progressions numbered, with their style and numerals */
static uint32_t ph_menu_until;
static void ph_draw_menu(void)
{
    int i, first = play_prog[track] - 2, y = 70;
    char s[64], *d;
    if (first < 0) first = 0;
    if (first > PH_NPROGS - 6) first = PH_NPROGS - 6;
    kit_round_rect(4, 66, 232, 162, 8, KIT_RGB(22, 26, 36));
    for (i = first; i < first + 6; i++, y += 26) {   /* six rows: the number, the style and name, the numerals under */
        int cur = i == play_prog[track];
        if (cur) kit_round_rect(6, y, 228, 25, 5, ACC_D);
        d = cat_int(s, i + 1);
        cat(d, " ");
        kit_text(10, y + 3, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, cur ? ACC : MUTED);
        d = cat(s, PH_PROGS[i].style);
        if (PH_PROGS[i].style[0]) d = cat(d, " ");
        cat(d, PH_PROGS[i].name);
        kit_text(34, y + 3, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, cur ? FG : MUTED);
        ph_numerals(i, s, sizeof s);
        kit_text(34, y + 15, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, cur ? ACC : MUTED);
    }
}

/* the PLAY page on Pads */
static void ch_draw_play(void);
static void ph_draw_pads_page(void) { ch_draw_play(); }   /* the lab's Pads page (chdraw.c, D-084) */

/* PLAY's lights (R-247, R-248, R-252): what you hold full; Key: the key dim, its tonic half, the rest of the chord
 * suggested full; Shapes: the key dim, the chord full, the next one half */
static void play_levels(uint8_t *lv)            /* the keys' lights on PLAY (the screen draws them too, D-084) */
{
    const kit_scale_t *sc = &KIT_SCALES[pads[track].minor ? 1 : 0];
    uint8_t held[16], v[KIT_CHORD_MAX];
    int k, i, nh = play_held(held, 16), h = play_helps[track], sh = play_shift();
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = key_note(k) + sh;
        lv[k] = KIT_LIGHT_OFF;
        if (h == PH_KEY || h == PH_SHAPES)
            ph_light(lv, k, h == PH_KEY && n % 12 == pads[track].tonic ? KIT_LIGHT_HALF
                            : kit_scale_has(sc, pads[track].tonic, n) ? KIT_LIGHT_DIM : KIT_LIGHT_OFF);
        if (kit_key_held(k)) ph_light(lv, k, KIT_LIGHT_ON);
    }
    for (i = 0; i < nh && h != PH_PADS; i++) ph_light(lv, held[i] - sh - kb_low(), KIT_LIGHT_ON);
    if (h == PH_KEY && nh) {
        int c[21], nc = ph_cands(held, nh, c);
        if (nc) {
            int pcs[8], m = ph_deg_notes(c[pmod(play_cand[track], nc)], play_colour[track], pcs);
            for (i = 0; i < m; i++) {
                int j, have = 0;
                for (j = 0; j < nh; j++) if (held[j] % 12 == pmod(pcs[i], 12)) have = 1;
                if (!have) ph_light(lv, ph_key_near(pcs[i], held[0] + 4), KIT_LIGHT_ON);
            }
        }
    }
    if (h == PH_PADS) ph_pads_lights(lv);
    if (h == PH_SHAPES) {
        int d, n = ph_shape(play_step[track] + 1, v, &d);
        for (i = 0; i < n; i++) ph_light(lv, v[i] - kb_low(), KIT_LIGHT_HALF);
        n = ph_shape(play_step[track], v, &d);
        for (i = 0; i < n; i++) ph_light(lv, v[i] - kb_low(), KIT_LIGHT_ON);
    }
}
static void play_lights(void)
{
    uint8_t lv[KIT_NKEYS];
    int k;
    play_levels(lv);
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, lv[k]);
}

/* PLAY's knobs and SELECT (R-246, R-249, R-251, R-253) */
static void ch_turned(int id);
static void play_knob(int id, int d)
{
    int h = play_helps[track];
    ch_turned(id);
    if (id == KIT_ENC_KNOB1) {
        chord_release();
        kit_track_notes_off(track);
        clampi(&play_helps[track], d, 0, PH_N - 1);
        play_cand[track] = 0;
        return;
    }
    if (h == PH_FREE || h == PH_KEY) {
        if (id == KIT_ENC_KNOB2) { kit_track_notes_off(track); clampi(&play_trans[track], d, 0, 1); }
        if (id == KIT_ENC_KNOB3 && h == PH_KEY) clampi(&play_colour[track], d, 0, CC_N - 1);
        if (id == KIT_ENC_SELECT && h == PH_KEY) play_cand[track] += d;
        return;
    }
    if (id == KIT_ENC_KNOB2) {
        clampi(&play_prog[track], d, 0, PH_NPROGS - 1);
        play_step[track] = 0;
        ph_menu_until = kit_ms() + 1600;   /* the menu while it turns (R-257), as long as the lab's */
        if (!ph_menu_until) ph_menu_until = 1;
    }
    if (h == PH_SHAPES) {
        if (id == KIT_ENC_KNOB3) clampi(&play_colour[track], d, 0, CC_N - 1);
        if (id == KIT_ENC_KNOB4) clampi(&pads[track].voicing, d, 0, KIT_NVOICINGS - 1);
        if (id == KIT_ENC_SELECT) play_step[track] += d;
    } else {                                    /* Pads */
        if (id == KIT_ENC_KNOB3) clampi(&pads[track].voicing, d, 0, KIT_NVOICINGS - 1);
        if (id == KIT_ENC_KNOB4) clampi(&play_feel[track], d, 0, FEEL_N - 1);
    }
}

/* the key's seven chords on a row, the one played lit, those that may come lit half (R-250) */
static void ph_row(int y, int lit, uint8_t half)
{
    kit_chord_t tri[7];
    int d;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    for (d = 0; d < 7; d++) {
        char s[8], *e;
        int x = 4 + d * 33, on = d == lit, hf = !on && (half >> d & 1);
        e = cat(s, kit_note_name_in_key(tri[d].root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
        cat(e, tri[d].quality == KIT_Q_MINOR ? "m" : tri[d].quality == KIT_Q_DIM ? "o" : tri[d].quality == KIT_Q_AUG ? "+" : "");
        kit_round_rect(x, y, 31, 22, 4, on ? ACC : hf ? ACC_D : CARD);
        ctext(x + 15, y + 11, s, KIT_FONT_XSMALL, on ? BG : hf ? FG : MUTED);
    }
}

/* the degree of a chord held in the key: its root is the degree's root and every note is in the key (-1: none) */
static int ph_degree_of(const uint8_t *held, int nh, int root)
{
    const kit_scale_t *sc = &KIT_SCALES[pads[track].minor ? 1 : 0];
    int d, i;
    for (i = 0; i < nh; i++) if (!kit_scale_has(sc, pads[track].tonic, held[i])) return -1;
    for (d = 0; d < 7; d++) if (pmod(ph_scale(d, 0), 12) == pmod(root, 12)) return d;
    return -1;
}
