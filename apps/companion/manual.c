/* SPDX-License-Identifier: GPL-3.0-only */
/* manual.c: manual chords' music, apart from the keys, lights and screen (D-036). app.c includes it after pad.c, and so
 * does the unit test (factory/unit/test_music_manual.c).
 *
 * The knobs build a chord: K1 the root (the key's seven chords, each with its type, then the five roots outside the
 * key), K2 the type, K3 the colour, K4 the voicing. SELECT walks the ideas: the chord pad's progression, or I-V-vi-IV.
 * The chord is placed on the keys: its root the first of its name from the lowest key up. */

#define MAN_NROOTS 12
#define MAN_NTYPES 6
#define MAN_NCOLS 8
static const uint8_t MAN_TYPES[MAN_NTYPES] = { KIT_Q_MAJOR, KIT_Q_MINOR, KIT_Q_SUS2, KIT_Q_SUS4, KIT_Q_DIM, KIT_Q_AUG };
static const char *const MAN_TYPE_NAMES[MAN_NTYPES] = { "maj", "m", "sus2", "sus4", "dim", "aug" };
static const uint8_t MAN_COLS[MAN_NCOLS] = { KIT_COL_NONE, KIT_COL_7, KIT_COL_MAJ7, KIT_COL_9, KIT_COL_ADD9, KIT_COL_6,
                                             KIT_COL_11, KIT_COL_13 };
static const pad_step_t MAN_STANDARD[4] = { { 0, KIT_COL_NONE }, { 4, KIT_COL_NONE }, { 5, KIT_COL_NONE }, { 3, KIT_COL_NONE } };

typedef struct {
    int root_i;                 /* K1: 0..6 the key's chords, 7..11 the roots outside the key */
    int type, colour, voicing;  /* K2 (MAN_TYPES), K3 (MAN_COLS), K4 (KIT_VOICE_*) */
    int idea;                   /* the step of the ideas the knobs took last (-1: none yet) */
    int idea_src;               /* the pad's style and progression the ideas came from: another one starts afresh */
} man_t;

static void man_init(man_t *m) { m->root_i = 0; m->type = 0; m->colour = 0; m->voicing = KIT_VOICE_ROOT; m->idea = -1; m->idea_src = -1; }

static int man_type_of(int quality)
{
    int i;
    for (i = 0; i < MAN_NTYPES; i++) if (MAN_TYPES[i] == quality) return i;
    return 0;
}

static int man_root_pc(const man_t *m, int tonic, int minor)
{
    const kit_scale_t *s = &KIT_SCALES[minor ? 1 : 0];
    int i, k = 7;
    if (m->root_i < 7) return (tonic + s->iv[m->root_i]) % 12;
    for (i = 1; i < 12; i++) {              /* the roots outside the key, upwards from the tonic */
        int pc = (tonic + i) % 12;
        if (kit_scale_has(s, tonic, pc)) continue;
        if (k++ == m->root_i) return pc;
    }
    return tonic;
}

static int man_outside(const man_t *m) { return m->root_i >= 7; }

/* K1: a chord of the key takes its type with it; a root outside the key keeps the type */
static void man_set_root(man_t *m, int root_i, int tonic, int minor)
{
    kit_chord_t tri[7];
    m->root_i = ((root_i % MAN_NROOTS) + MAN_NROOTS) % MAN_NROOTS;
    if (m->root_i < 7 && kit_triads(tonic, minor ? 1 : 0, tri)) m->type = man_type_of(tri[m->root_i].quality);
}

/* the chord on the keys: its root the first note of its name at or above low (the lowest key) */
static int man_chord(const man_t *m, int tonic, int minor, int low, kit_vchord_t *out)
{
    int pc = man_root_pc(m, tonic, minor), root_note = low + ((pc - low) % 12 + 12) % 12;
    return kit_colour_chord(pc, MAN_TYPES[m->type], MAN_COLS[m->colour], 0, m->voicing, root_note - pc, out);
}

static int man_col_of(int colour)
{
    int i;
    for (i = 0; i < MAN_NCOLS; i++) if (MAN_COLS[i] == colour) return i;
    return 0;
}

/* the ideas: the chord pad's progression (as chosen there), or I-V-vi-IV */
static const pad_step_t *man_ideas(const pad_t *p, int *n)
{
    const pad_prog_t *pr = pad_prog(p);
    if (pr) { *n = pr->n; return pr->step; }
    *n = 4;
    return MAN_STANDARD;
}

/* set the knobs to the idea `i` steps away (+1 the next, -1 the previous) */
static void man_walk(man_t *m, const pad_t *p, int dir, int tonic, int minor)
{
    int n;
    const pad_step_t *st = man_ideas(p, &n);
    if (m->idea_src != p->style * 8 + p->prog) { m->idea = -1; m->idea_src = p->style * 8 + p->prog; }
    m->idea = m->idea < 0 ? (dir > 0 ? 0 : n - 1) : ((m->idea + dir) % n + n) % n;
    man_set_root(m, st[m->idea].degree, tonic, minor);
    if (st[m->idea].colour == KIT_COL_SUS2 || st[m->idea].colour == KIT_COL_SUS4) {
        m->type = man_type_of(st[m->idea].colour == KIT_COL_SUS2 ? KIT_Q_SUS2 : KIT_Q_SUS4);
        m->colour = 0;
    } else {
        m->colour = man_col_of(st[m->idea].colour);
    }
}

/* the idea after the one the knobs have (for the screen) */
static void man_next_idea(const man_t *m, const pad_t *p, int tonic, int minor, int low, kit_vchord_t *out)
{
    man_t t = *m;
    man_walk(&t, p, 1, tonic, minor);
    man_chord(&t, tonic, minor, low, out);
}

/* the harmony's notes sounding at tick (as recorded; harm_mask's tracks), up to max; how many */
static int song_notes_at(const kit_song_t *s, int tick, uint8_t *out, int max)
{
    int i, t, n = 0, len = s->bars * KIT_TICKS_PER_BAR;
    if (tick < 0) return 0;
    FOR_HARM(s, t, i) {
        const kit_song_note_t *x = &s->notes[t][i];
        int end = x->start + x->len;
        if (n >= max) break;
        if (x->start >= len) continue;
        if ((tick >= x->start && tick < end) || (tick + len >= x->start && tick + len < end)) out[n++] = x->note;
    }
    return n;
}

/* the first chord played in a bar: the notes that start within a sixteenth of the bar's first one; how many */
static int song_bar_chord(const kit_song_t *s, int bar, uint8_t *out, int max)
{
    int i, t, n = 0, first = -1, a = bar * KIT_TICKS_PER_BAR, b = a + KIT_TICKS_PER_BAR;
    FOR_HARM(s, t, i) {
        int st = s->notes[t][i].start;
        if (st >= a && st < b && (first < 0 || st < first)) first = st;
    }
    if (first < 0) return 0;
    FOR_HARM(s, t, i) {
        int st = s->notes[t][i].start;
        if (n < max && st >= first && st < first + KIT_TICKS_PER_BEAT / 4) out[n++] = s->notes[t][i].note;
    }
    return n;
}
