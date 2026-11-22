/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit music: pitch classes, note names, key spelling and the scale catalogue.
 * A port of Sketchpad core/notes.mjs and core/scales.mjs (same scales, same spelling rules);
 * factory/oracle/golden.json is generated from the JavaScript and checked against this file. */
#include "fm1kit.h"

const kit_scale_t KIT_SCALES[] = {
    { "major",            "Major",            7, { 0, 2, 4, 5, 7, 9, 11 }, 0 },
    { "natural-minor",    "Natural minor",    7, { 0, 2, 3, 5, 7, 8, 10 }, 1 },
    { "major-pentatonic", "Major pentatonic", 5, { 0, 2, 4, 7, 9 },        0 },
    { "minor-pentatonic", "Minor pentatonic", 5, { 0, 3, 5, 7, 10 },       1 },
    { "dorian",           "Dorian",           7, { 0, 2, 3, 5, 7, 9, 10 }, 1 },
    { "harmonic-minor",   "Harmonic minor",   7, { 0, 2, 3, 5, 7, 8, 11 }, 1 },
    { "phrygian",         "Phrygian",         7, { 0, 1, 3, 5, 7, 8, 10 }, 1 },
    { "blues",            "Blues",            6, { 0, 3, 5, 6, 7, 10 },    1 },
    { "mixolydian",       "Mixolydian",       7, { 0, 2, 4, 5, 7, 9, 10 }, 0 },
    { "lydian",           "Lydian",           7, { 0, 2, 4, 6, 7, 9, 11 }, 0 },
    /* D-039: Sketchpad's seven more (its D-116), appended so the first ten keep their indexes */
    { "melodic-minor",    "Melodic minor",    7, { 0, 2, 3, 5, 7, 9, 11 }, 1 },
    { "hirajoshi",        "Hirajoshi",        5, { 0, 2, 3, 7, 8 },        1 },
    { "locrian",          "Locrian",          7, { 0, 1, 3, 5, 6, 8, 10 }, 1 },
    { "major-blues",      "Major blues",      6, { 0, 2, 3, 4, 7, 9 },     0 },
    { "lydian-dominant",  "Lydian dominant",  7, { 0, 2, 4, 6, 7, 9, 10 }, 0 },
    { "phrygian-dominant", "Phrygian dominant", 7, { 0, 1, 4, 5, 7, 8, 10 }, 0 },
    { "whole-tone",       "Whole tone",       6, { 0, 2, 4, 6, 8, 10 },    0 },
};
const int KIT_NSCALES = (int)(sizeof(KIT_SCALES) / sizeof(KIT_SCALES[0]));

static const char *const NAMES[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const char *const FLAT_NAMES[12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
static const char *const SOLFEGE[12] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
static const char LETTERS[7] = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
static const uint8_t LETTER_PC[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const char *const SOLFEGE_OF[7] = { "Do", "Re", "Mi", "Fa", "Sol", "La", "Si" };
/* [semitones above the tonic, scale degree 1..7]: the key's notes, then common alterations */
static const uint8_t DEG_MAJOR[11][2] = { {0,1},{2,2},{4,3},{5,4},{7,5},{9,6},{11,7},{3,3},{8,6},{10,7},{6,4} };
static const uint8_t DEG_MINOR[11][2] = { {0,1},{2,2},{3,3},{5,4},{7,5},{8,6},{10,7},{9,6},{11,7},{4,3},{1,2} };
static const uint8_t FLAT_SIDE_MAJORS[6] = { 0, 1, 3, 5, 8, 10 };

int kit_pc(int note) { return ((note % 12) + 12) % 12; }
int kit_octave(int note) { return (note - kit_pc(note)) / 12 - 1; }
int kit_is_white(int note)
{
    int p = kit_pc(note), i;
    for (i = 0; i < 7; i++)
        if (LETTER_PC[i] == p)
            return 1;
    return 0;
}

const char *kit_note_name(int note, int names)
{
    return (names == KIT_NAMES_SOLFEGE ? SOLFEGE : NAMES)[kit_pc(note)];
}

static int letter_index(char c)
{
    int i;
    for (i = 0; i < 7; i++)
        if (LETTERS[i] == c)
            return i;
    return 0;
}

static int leans_flat(int tonic, int minor)
{
    int p = minor ? kit_pc(tonic + 3) : kit_pc(tonic), i;
    for (i = 0; i < 6; i++)
        if (FLAT_SIDE_MAJORS[i] == p)
            return 1;
    return 0;
}

/* names[p] for one key: an index into a table of spellings built per call (a small static ring
 * so the returned strings stay valid for a few calls) */
static char ring[8][6];
static unsigned ring_i;

const char *kit_note_name_in_key(int note, int tonic, int minor, int names)
{
    const char *const *plain = leans_flat(tonic, minor) ? FLAT_NAMES : NAMES;
    const uint8_t (*deg)[2] = minor ? DEG_MINOR : DEG_MAJOR;
    int p = kit_pc(note), start = letter_index(plain[kit_pc(tonic)][0]), i;
    char letter = 0, acc = 0;
    char *out;
    for (i = 0; i < 11; i++) {
        int q = kit_pc(tonic + deg[i][0]);
        int li, d;
        char l;
        if (q != p)
            continue;
        li = (start + deg[i][1] - 1) % 7;
        l = LETTERS[li];
        d = kit_pc(p - LETTER_PC[li]);
        if (d == 0) {
            letter = l; acc = 0;
        } else if ((d == 1 || d == 11) && !kit_is_white(p)) {   /* no F b, C b, E#, B# */
            letter = l; acc = d == 1 ? '#' : 'b';
        }
        /* JS keeps the last matching degree that spells; continue scanning like the JS loop */
    }
    out = ring[ring_i++ & 7u];
    if (!letter) {                  /* not a degree of the key: the key's plain name */
        const char *s = plain[p];
        if (names == KIT_NAMES_SOLFEGE) {
            const char *b = SOLFEGE_OF[letter_index(s[0])];
            int k = 0;
            while (*b) out[k++] = *b++;
            if (s[1]) out[k++] = s[1];
            out[k] = 0;
            return out;
        }
        return s;
    }
    if (names == KIT_NAMES_SOLFEGE) {
        const char *b = SOLFEGE_OF[letter_index(letter)];
        int k = 0;
        while (*b) out[k++] = *b++;
        if (acc) out[k++] = acc;
        out[k] = 0;
    } else {
        out[0] = letter;
        out[1] = acc;
        out[2] = 0;
    }
    return out;
}

int kit_scale_degree(const kit_scale_t *s, int tonic, int note)
{
    int d = kit_pc(note - tonic), i;
    for (i = 0; i < s->n; i++)
        if (s->iv[i] == d)
            return i + 1;
    return 0;
}

int kit_scale_has(const kit_scale_t *s, int tonic, int note) { return kit_scale_degree(s, tonic, note) != 0; }

/* ---- run orders: Sketchpad core/arpeggio runOrder, core/voicing arpeggio, core/figures rng (D-024) ---- */

/* Sketchpad's rng(seed) (mulberry32): the same 32-bit steps, so a seed gives the same order */
static uint32_t rng_next(uint32_t *a)
{
    uint32_t t;
    *a += 0x6D2B79F5u;
    t = (*a ^ (*a >> 15)) * (1u | *a);
    t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
    return t ^ (t >> 14);
}

/* floor(r / 2^32 * m) for a 32-bit r, exactly, with no 64-bit multiply (felucca-bsp docs/CPU.md) */
static uint32_t scale_down(uint32_t r, uint32_t m)
{
    return ((r >> 16) * m + (((r & 0xFFFFu) * m) >> 16)) >> 16;
}

int kit_run_order(const uint8_t *notes, int n, int pattern, uint32_t seed, int end_on, uint8_t *out)
{
    uint8_t asc[64];
    int k = 0, i, j, m;
    if (n < 0) n = 0;
    for (i = 0; i < n && k < 64; i++) {             /* unique, ascending */
        int v = notes[i], dup = 0;
        for (j = 0; j < k; j++)
            if (asc[j] == v) dup = 1;
        if (dup) continue;
        for (j = k; j > 0 && asc[j - 1] > v; j--)
            asc[j] = asc[j - 1];
        asc[j] = (uint8_t)v;
        k++;
    }
    if (pattern == KIT_RUN_RANDOM) {
        uint32_t a = seed;
        int keep = end_on && k > 1 ? asc[--k] : -1;
        for (i = k - 1; i > 0; i--) {
            uint8_t t;
            j = (int)scale_down(rng_next(&a), (uint32_t)(i + 1));
            t = asc[i]; asc[i] = asc[j]; asc[j] = t;
        }
        for (i = 0; i < k; i++) out[i] = asc[i];
        if (keep >= 0) out[k++] = (uint8_t)keep;
        return k;
    }
    if (pattern == KIT_RUN_DOWN) {
        for (i = 0; i < k; i++) out[i] = asc[k - 1 - i];
        return k;
    }
    for (i = 0; i < k; i++) out[i] = asc[i];
    if (pattern != KIT_RUN_UPDOWN)
        return k;
    /* up, then back down without repeating the top, and without the bottom (so it loops): [...asc, ...asc
     * without its top, reversed, without its last] */
    m = k;
    for (i = k - 2; i >= 1; i--)
        out[m++] = asc[i];
    return m;
}

int kit_tempo_clamp(int bpm) { return bpm < KIT_TEMPO_MIN ? KIT_TEMPO_MIN : bpm > KIT_TEMPO_MAX ? KIT_TEMPO_MAX : bpm; }
uint32_t kit_beat_ms(int bpm) { return (uint32_t)(60000 / kit_tempo_clamp(bpm)); }

/* ---- chords and keys: Sketchpad core/harmony (harmonize, romanFor, keyFit, keysForNotes), core/chords
 * (inversions), core/voicing (voice, stackAscending) (D-030). Integer only. ---- */

static const char *const NUMERALS[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
static const uint8_t MAJOR_REF[7] = { 0, 2, 4, 5, 7, 9, 11 };

const char *kit_quality_sym(int q) { return q == KIT_Q_MAJOR ? "" : q == KIT_Q_MINOR ? "m" : q == KIT_Q_DIM ? "dim" : q == KIT_Q_AUG ? "aug" : "?"; }

static void roman_for(int tonic, int root, int degree, int q, char *out)
{
    int off = (root - tonic + 12) % 12, k = 0;
    const char *b = NUMERALS[degree];
    if (off < MAJOR_REF[degree]) out[k++] = 'b';
    else if (off > MAJOR_REF[degree]) out[k++] = '#';
    for (; *b; b++)
        out[k++] = (char)(q == KIT_Q_MINOR || q == KIT_Q_DIM ? *b - 'A' + 'a' : *b);
    if (q == KIT_Q_DIM) out[k++] = 'o';
    if (q == KIT_Q_AUG) out[k++] = '+';
    out[k] = 0;
}

int kit_triads(int tonic, int scale, kit_chord_t out[7])
{
    const kit_scale_t *s;
    int i, k;
    if (scale < 0 || scale >= KIT_NSCALES || KIT_SCALES[scale].n != 7)
        return 0;
    s = &KIT_SCALES[scale];
    for (i = 0; i < 7; i++) {
        int root = (tonic + s->iv[i]) % 12, last = -1, iv[3], q;
        for (k = 0; k < 3; k++) {                   /* stackAscending: each tone above the last */
            int t = (tonic + s->iv[(i + 2 * k) % 7]) % 12, d = (t - root + 12) % 12;
            while (d <= last) d += 12;
            iv[k] = last = d;
        }
        q = iv[1] == 4 && iv[2] == 7 ? KIT_Q_MAJOR : iv[1] == 3 && iv[2] == 7 ? KIT_Q_MINOR
          : iv[1] == 3 && iv[2] == 6 ? KIT_Q_DIM : iv[1] == 4 && iv[2] == 8 ? KIT_Q_AUG : KIT_Q_OTHER;
        out[i].root = (uint8_t)root;
        out[i].quality = (uint8_t)q;
        out[i].n = 3;
        for (k = 0; k < 3; k++)
            out[i].notes[k] = (uint8_t)(48 + root + iv[k]);
        out[i].notes[3] = 0;
        roman_for(tonic, root, i, q, out[i].roman);
    }
    return 7;
}

int kit_inversion(const kit_chord_t *c, int inv, uint8_t *out)
{
    int n = c->n, i, j, k;
    uint8_t v[4];
    for (i = 0; i < n; i++) v[i] = c->notes[i];
    for (i = 1; i < n; i++)                         /* ascending */
        for (j = i; j > 0 && v[j - 1] > v[j]; j--) { uint8_t t = v[j]; v[j] = v[j - 1]; v[j - 1] = t; }
    if (inv < 0 || inv >= n || inv >= 4)
        inv = 0;
    for (k = 0; k < inv; k++) {                     /* the lowest goes up an octave */
        uint8_t lo = v[0];
        for (i = 0; i < n - 1; i++) v[i] = v[i + 1];
        v[n - 1] = (uint8_t)(lo + 12);
    }
    for (i = 0; i < n; i++) out[i] = v[i];
    return n;
}

static uint16_t key_mask(int tonic, int minor)
{
    const kit_scale_t *s = &KIT_SCALES[minor ? 1 : 0];   /* major, natural minor */
    uint16_t m = 0;
    int i;
    for (i = 0; i < s->n; i++) m |= (uint16_t)(1u << ((tonic + s->iv[i]) % 12));
    return m;
}

static int chord_in(const kit_chord_t *c, uint16_t key)
{
    int i;
    for (i = 0; i < c->n; i++)
        if (!(key >> (c->notes[i] % 12) & 1u))
            return 0;
    return 1;
}

void kit_key_fit(const kit_chord_t *prog, int n, int tonic, int minor, kit_keyfit_t *out)
{
    uint16_t own = key_mask(tonic, minor);
    int i, m, t, best_rank = -1, best_t = 0, best_m = 0, best_n = 0;
    if (n > 32) n = 32;
    out->fits = 0;
    out->outside = 0;
    for (i = 0; i < n; i++) {
        if (chord_in(&prog[i], own)) out->fits++;
        else out->outside |= 1u << i;
    }
    out->ok = out->fits == n;
    for (m = 0; m < 2; m++)
        for (t = 0; t < 12; t++) {
            uint16_t k = key_mask(t, m);
            int c = 0, rank;
            for (i = 0; i < n; i++) c += chord_in(&prog[i], k);
            rank = c * 10 + (m == minor ? 3 : 0) + (n && prog[0].root == t ? 2 : 0) + (n && prog[n - 1].root == t ? 1 : 0);
            if (rank > best_rank) { best_rank = rank; best_t = t; best_m = m; best_n = c; }
        }
    out->suggest_tonic = -1;
    out->suggest_minor = out->suggest_fits = 0;
    if (n && best_n > out->fits) {
        out->suggest_tonic = best_t;
        out->suggest_minor = best_m;
        out->suggest_fits = best_n;
    }
}

int kit_keys_for_notes(const kit_note_ev_t *ev, int n, kit_key_cand_t *out, int limit, int *ambiguous, uint16_t *heard)
{
    uint32_t w[12] = { 0 }, total = 0, most = 0, score[24];
    kit_key_cand_t all[24];
    uint16_t hm = 0;
    int i, j, p, nheavy = 0, heavy = -1, first, last, lowest = 127, m, t;
    if (n <= 0) {
        *ambiguous = 1;
        *heard = 0;
        return 0;
    }
    for (i = 0; i < n; i++) {
        w[ev[i].note % 12] += ev[i].len;
        if (ev[i].note < lowest) lowest = ev[i].note;
    }
    for (p = 0; p < 12; p++) {
        total += w[p];
        if (w[p]) hm |= (uint16_t)(1u << p);
        if (w[p] > most) most = w[p];
    }
    for (p = 0; p < 12; p++)
        if (w[p] && w[p] == most) { nheavy++; heavy = p; }
    first = ev[0].note % 12;
    last = ev[n - 1].note % 12;
    lowest %= 12;
    for (m = 0; m < 2; m++)
        for (t = 0; t < 12; t++) {
            kit_key_cand_t *c = &all[m * 12 + t];
            uint16_t k = key_mask(t, m);
            uint32_t held = 0;
            for (p = 0; p < 12; p++) if (k >> p & 1u) held += w[p];
            c->tonic = (uint8_t)t;
            c->minor = (uint8_t)m;
            c->outside = (uint16_t)(hm & ~k);
            c->evidence = (uint8_t)((first == t) + (last == t) + (lowest == t) + (nheavy == 1 && heavy == t));
            c->held = held;
            c->total = total;
            score[m * 12 + t] = 8u * held + c->evidence * total;
        }
    for (i = 1; i < 24; i++)                        /* insertion sort: score, then major, then the tonic from the first note */
        for (j = i; j > 0; j--) {
            kit_key_cand_t *a = &all[j - 1], *b = &all[j];
            int da = (a->tonic - first + 12) % 12, db = (b->tonic - first + 12) % 12;
            int later = score[j - 1] < score[j] || (score[j - 1] == score[j] && (a->minor > b->minor || (a->minor == b->minor && da > db)));
            if (!later) break;
            { kit_key_cand_t x = *a; uint32_t y = score[j - 1]; *a = *b; *b = x; score[j - 1] = score[j]; score[j] = y; }
        }
    *ambiguous = score[0] - score[1] < total;
    *heard = hm;
    if (limit > 24) limit = 24;
    for (i = 0; i < limit; i++) out[i] = all[i];
    return limit < 0 ? 0 : limit;
}

/* ---- colour chords (API 12, D-033): Sketchpad core/chords DICTIONARY (the chords used here), core/voicing voicingsFor
 * (close, open, spread), core/chords inversions, core/harmony suggestNextChords. ---- */

typedef struct { const char *sym; uint8_t n, iv[6]; } dict_t;
static const dict_t DICT[] = {
    { "", 3, { 0, 4, 7 } },          { "m", 3, { 0, 3, 7 } },          { "dim", 3, { 0, 3, 6 } },
    { "aug", 3, { 0, 4, 8 } },       { "sus2", 3, { 0, 2, 7 } },       { "sus4", 3, { 0, 5, 7 } },
    { "7", 4, { 0, 4, 7, 10 } },     { "m7", 4, { 0, 3, 7, 10 } },     { "maj7", 4, { 0, 4, 7, 11 } },
    { "mMaj7", 4, { 0, 3, 7, 11 } }, { "m7b5", 4, { 0, 3, 6, 10 } },   { "dim7", 4, { 0, 3, 6, 9 } },
    { "6", 4, { 0, 4, 7, 9 } },      { "m6", 4, { 0, 3, 7, 9 } },
    { "add9", 4, { 0, 4, 7, 14 } },  { "m(add9)", 4, { 0, 3, 7, 14 } }, { "9", 5, { 0, 4, 7, 10, 14 } },
    { "m9", 5, { 0, 3, 7, 10, 14 } }, { "9sus4", 5, { 0, 5, 7, 10, 14 } }, { "m11", 6, { 0, 3, 7, 10, 14, 17 } },
    { "13", 6, { 0, 4, 7, 10, 14, 21 } }, { "m13", 6, { 0, 3, 7, 10, 14, 21 } },
    { "7sus4", 4, { 0, 5, 7, 10 } }, { "sus4(9)", 4, { 0, 5, 7, 14 } },
};
#define NDICT ((int)(sizeof DICT / sizeof DICT[0]))

static const char *const COLOUR_NAMES[KIT_NCOLS] = { "", "sus2", "sus4", "7", "maj7", "9", "11", "13", "add9", "6" };
/* what each triad becomes with each colour: the dictionary's chord. An 11 on a major triad leaves out the 3rd (it
 * clashes with the 11th), which Sketchpad names 9sus4; a diminished triad takes its seventh (m7b5) for any seventh
 * colour and becomes dim7 with a 6th; an augmented one stays as it is, as the dictionary has no coloured augmented chord (D-033) */
static const char *const COLOUR_SYM[7][KIT_NCOLS] = {
    { "", "sus2", "sus4", "7", "maj7", "9", "9sus4", "13", "add9", "6" },
    { "m", "sus2", "sus4", "m7", "mMaj7", "m9", "m11", "m13", "m(add9)", "m6" },
    { "dim", "sus2", "sus4", "m7b5", "m7b5", "m7b5", "m7b5", "m7b5", "dim", "dim7" },
    { "aug", "sus2", "sus4", "aug", "aug", "aug", "aug", "aug", "aug", "aug" },
    { "", "sus2", "sus4", "7", "maj7", "9", "9sus4", "13", "add9", "6" },          /* KIT_Q_OTHER: as major */
    /* the suspended types (API 13): the dictionary has 7sus4, 9sus4 and sus4(9), and no coloured sus2 */
    { "sus2", "sus2", "sus4", "sus2", "sus2", "sus2", "sus2", "sus2", "sus2", "sus2" },
    { "sus4", "sus2", "sus4", "7sus4", "sus4", "9sus4", "9sus4", "9sus4", "sus4(9)", "sus4" },
};

const char *kit_colour_name(int colour) { return colour >= 0 && colour < KIT_NCOLS ? COLOUR_NAMES[colour] : ""; }

const char *kit_colour_sym(int quality, int colour)
{
    if (quality < 0 || quality > KIT_Q_SUS4) quality = KIT_Q_MAJOR;
    if (colour < 0 || colour >= KIT_NCOLS) colour = KIT_COL_NONE;
    return COLOUR_SYM[quality][colour];
}

static const dict_t *dict_find(const char *sym)
{
    int i, j;
    for (i = 0; i < NDICT; i++) {
        for (j = 0; sym[j] && sym[j] == DICT[i].sym[j]; j++) ;
        if (sym[j] == 0 && DICT[i].sym[j] == 0) return &DICT[i];
    }
    return 0;
}

static void sort_notes(int *v, int n)
{
    int i, j;
    for (i = 1; i < n; i++)
        for (j = i; j > 0 && v[j - 1] > v[j]; j--) { int t = v[j]; v[j] = v[j - 1]; v[j - 1] = t; }
}

int kit_colour_chord(int root, int quality, int colour, int slash3, int voicing, int base, kit_vchord_t *out)
{
    const char *sym = kit_colour_sym(quality, colour);
    const dict_t *d = dict_find(sym);
    int v[KIT_CHORD_MAX], n, i, k;
    out->n = 0;
    if (!d || colour < 0 || colour >= KIT_NCOLS || voicing < 0 || voicing >= KIT_NVOICINGS)
        return 0;
    root = ((root % 12) + 12) % 12;
    n = d->n;
    for (i = 0; i < n; i++) v[i] = base + root + d->iv[i];     /* voice(): close, from base */
    if (voicing == KIT_VOICE_INV1 || voicing == KIT_VOICE_INV2) {
        for (k = 0; k < voicing; k++) {                          /* inversions(): the lowest goes up an octave */
            int lo = v[0];
            for (i = 0; i < n - 1; i++) v[i] = v[i + 1];
            v[n - 1] = lo + 12;
        }
    } else if (voicing == KIT_VOICE_OPEN) {                      /* drop 2; drop 2 and 4 from five notes */
        v[n - 2] -= 12;
        if (n >= 5) v[n - 4] -= 12;
        sort_notes(v, n);
    } else if (voicing == KIT_VOICE_WIDE) {                      /* spread: the root an octave down */
        v[0] -= 12;
    }
    out->bass = 0xFF;
    if (slash3) {                                                /* the triad's 3rd, the nearest below the chord */
        int third = (root + (quality == KIT_Q_MINOR || quality == KIT_Q_DIM ? 3 : quality == KIT_Q_SUS2 ? 2
                                    : quality == KIT_Q_SUS4 ? 5 : 4)) % 12, b = v[0] - 1;
        while (((b % 12) + 12) % 12 != third) b--;
        for (i = n; i > 0; i--) v[i] = v[i - 1];
        v[0] = b;
        n++;
        out->bass = (uint8_t)third;
    }
    for (i = 0; i < n; i++) out->notes[i] = (uint8_t)(v[i] < 0 ? 0 : v[i] > 127 ? 127 : v[i]);
    out->n = (uint8_t)n;
    out->root = (uint8_t)root;
    out->sym = d->sym;
    return n;
}

int kit_chord_label(const kit_vchord_t *c, int tonic, int minor, char *out, int len)
{
    int k = 0;
    const char *s;
    if (len <= 0) return 0;
#define PUT(str) for (s = (str); *s && k < len - 1; s++) out[k++] = *s
    PUT(kit_note_name_in_key(c->root, tonic, minor, KIT_NAMES_LETTERS));
    PUT(c->sym ? c->sym : "");
    if (c->bass != 0xFF) {
        PUT("/");
        PUT(kit_note_name_in_key(c->bass, tonic, minor, KIT_NAMES_LETTERS));
    }
#undef PUT
    out[k] = 0;
    return k;
}

int kit_next_degree(int tonic, int minor, int degree)
{
    kit_chord_t pal[7];
    int i, j, best = -1, best_score = -1;
    uint16_t a = 0;
    if (degree < 0 || degree > 6 || !kit_triads(((tonic % 12) + 12) % 12, minor ? 1 : 0, pal))
        return 0;
    for (j = 0; j < pal[degree].n; j++) a |= (uint16_t)(1u << (pal[degree].notes[j] % 12));
    for (i = 0; i < 7; i++) {                       /* suggestNextChords([last]): the first of a stable sort, best score */
        int common = 0, score, interval;
        if (pal[i].root == pal[degree].root) continue;
        for (j = 0; j < pal[i].n; j++) common += (a >> (pal[i].notes[j] % 12)) & 1;
        score = common * 2;
        interval = (pal[i].root - pal[degree].root + 12) % 12;
        if (interval == 5) score += 3;
        else if (interval == 7) score += 2;
        score += 1;                                 /* a root not used yet: true of all but the last chord's */
        if (score > best_score) { best_score = score; best = i; }
    }
    return best < 0 ? 0 : best;
}

/* ---- what played notes are (API 13, D-035): Sketchpad core/chords identifyChord, exact. SIGNATURES: the chords' pitch
 * classes above the root (bit i: i semitones), from Sketchpad's QUALITIES and DICTIONARY with their ranks, in its order. ---- */
typedef struct { uint16_t sig; uint8_t rank; const char *sym; } sig_t;
static const sig_t SIGS[] = {
    { 0x091, 3, "" }, { 0x089, 3, "m" }, { 0x049, 3, "dim" }, { 0x111, 3, "aug" }, { 0x891, 3, "maj7" },
    { 0x489, 3, "m7" }, { 0x491, 3, "7" }, { 0x449, 3, "m7b5" }, { 0x249, 3, "dim7" }, { 0x889, 3, "mMaj7" },
    { 0x911, 3, "maj7#5" }, { 0x895, 3, "maj9" }, { 0x48d, 3, "m9" }, { 0x495, 3, "9" }, { 0x44b, 3, "m7b5b9" },
    { 0x48b, 3, "m7b9" }, { 0x085, 3, "sus2" }, { 0x0a1, 3, "sus4" }, { 0x291, 3, "6" }, { 0x289, 3, "m6" },
    { 0x295, 3, "6/9" }, { 0x095, 3, "add9" }, { 0x08d, 3, "m(add9)" }, { 0x4ad, 3, "m11" }, { 0xa95, 3, "maj13" },
    { 0x695, 3, "13" }, { 0x68d, 3, "m13" }, { 0x8d5, 3, "maj7#11" }, { 0x493, 3, "7b9" }, { 0x499, 3, "7#9" },
    { 0x0a5, 3, "sus4(9)" }, { 0x8d1, 3, "maj7#11" }, { 0x4a1, 3, "7sus4" }, { 0x4a5, 3, "9sus4" },
};

int kit_identify_chord(const uint8_t *notes, int n, int tonic, int minor, char *out, int len, int *root)
{
    int pcs[12], np = 0, i, j, low = 128, hi = -1, bass, best_score = -1, best_root = -1, k = 0, prev = -1;
    const char *best_sym = 0, *s;
    if (len <= 0) return 0;
    out[0] = 0;
    for (i = 0; i < n; i++) {
        if (notes[i] < low) low = notes[i];
        if (notes[i] > hi) hi = notes[i];
    }
    for (;;) {                                       /* the pitch classes in the order of the notes, low to high */
        int nxt = 128;
        for (i = 0; i < n; i++) if (notes[i] > prev && notes[i] < nxt) nxt = notes[i];
        if (nxt > 127) break;
        for (j = 0; j < np && pcs[j] != nxt % 12; j++) ;
        if (j == np) pcs[np++] = nxt % 12;
        prev = nxt;
    }
    if (np < 2) return 0;
    bass = low % 12;
#define PUT(str) for (s = (str); *s && k < len - 1; s++) out[k++] = *s
    if (np == 2) {                                   /* an interval: "C + G", from the lowest note to the highest */
        PUT(kit_note_name_in_key(bass, tonic, minor, KIT_NAMES_LETTERS));
        PUT(" + ");
        PUT(kit_note_name_in_key(hi % 12, tonic, minor, KIT_NAMES_LETTERS));
        out[k] = 0;
        if (root) *root = bass;
        return 1;
    }
    for (i = 0; i < np; i++) {                       /* each note as the root; the first best wins (a stable sort) */
        uint16_t sig = 0;
        for (j = 0; j < np; j++) sig |= (uint16_t)(1u << ((pcs[j] - pcs[i] + 12) % 12));
        for (j = 0; j < (int)(sizeof SIGS / sizeof SIGS[0]); j++)
            if (SIGS[j].sig == sig) {
                int score = SIGS[j].rank + (pcs[i] == bass ? 2 : 0);
                if (score > best_score) { best_score = score; best_root = pcs[i]; best_sym = SIGS[j].sym; }
                break;
            }
    }
    if (best_root < 0) return 0;
    PUT(kit_note_name_in_key(best_root, tonic, minor, KIT_NAMES_LETTERS));
    PUT(best_sym);
    if (best_root != bass) {
        PUT("/");
        PUT(kit_note_name_in_key(bass, tonic, minor, KIT_NAMES_LETTERS));
    }
#undef PUT
    out[k] = 0;
    if (root) *root = best_root;
    return 1;
}

/* ---- which scales fit (API 15, D-041): Sketchpad core/scales fitScales. Its catalogue order (a stable sort keeps it on
 * a tie), as indexes into KIT_SCALES: natural minor, minor pentatonic, dorian, harmonic minor, phrygian, blues, major,
 * major pentatonic, mixolydian, lydian, then the seven of D-116 ---- */
static const uint8_t SKETCHPAD_ORDER[17] = { 1, 3, 4, 5, 6, 7, 0, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

int kit_fit_scales(uint16_t used, int tonic, uint8_t *out, uint8_t *held, int limit, int *nused)
{
    uint8_t idx[17], h[17];
    int n = 0, i, j, u = 0, p;
    for (p = 0; p < 12; p++) u += (used >> p) & 1;
    *nused = u;
    if (!u) return 0;
    for (i = 0; i < 17 && SKETCHPAD_ORDER[i] < KIT_NSCALES; i++) {
        const kit_scale_t *s = &KIT_SCALES[SKETCHPAD_ORDER[i]];
        int c = 0;
        for (p = 0; p < 12; p++) if ((used >> p) & 1 && kit_scale_has(s, tonic, p)) c++;
        idx[n] = SKETCHPAD_ORDER[i];
        h[n] = (uint8_t)c;
        n++;
    }
    for (i = 1; i < n; i++)                         /* stable: more held first, then fewer notes */
        for (j = i; j > 0; j--) {
            int better = h[j] > h[j - 1] || (h[j] == h[j - 1] && KIT_SCALES[idx[j]].n < KIT_SCALES[idx[j - 1]].n);
            uint8_t t;
            if (!better) break;
            t = idx[j]; idx[j] = idx[j - 1]; idx[j - 1] = t;
            t = h[j]; h[j] = h[j - 1]; h[j - 1] = t;
        }
    if (limit > n) limit = n;
    for (i = 0; i < limit; i++) { out[i] = idx[i]; held[i] = h[i]; }
    return limit < 0 ? 0 : limit;
}

/* ---- the closest chord (API 16, D-042): Sketchpad core/chords identifyChord(..., { missing: true }), its best reading.
 * The dictionary in Sketchpad's order (the tie rule). ---- */
static const dict_t FULL_DICT[] = {
    { "", 3, { 0, 4, 7 } }, { "m", 3, { 0, 3, 7 } }, { "dim", 3, { 0, 3, 6 } }, { "aug", 3, { 0, 4, 8 } },
    { "sus4", 3, { 0, 5, 7 } }, { "sus2", 3, { 0, 2, 7 } }, { "6", 4, { 0, 4, 7, 9 } }, { "m6", 4, { 0, 3, 7, 9 } },
    { "maj7", 4, { 0, 4, 7, 11 } }, { "m7", 4, { 0, 3, 7, 10 } }, { "7", 4, { 0, 4, 7, 10 } }, { "m7b5", 4, { 0, 3, 6, 10 } },
    { "maj9", 5, { 0, 4, 7, 11, 14 } }, { "m9", 5, { 0, 3, 7, 10, 14 } }, { "9", 5, { 0, 4, 7, 10, 14 } },
    { "add9", 4, { 0, 4, 7, 14 } }, { "m(add9)", 4, { 0, 3, 7, 14 } }, { "6/9", 5, { 0, 4, 7, 9, 14 } },
    { "m11", 6, { 0, 3, 7, 10, 14, 17 } }, { "maj13", 6, { 0, 4, 7, 11, 14, 21 } }, { "13", 6, { 0, 4, 7, 10, 14, 21 } },
    { "m13", 6, { 0, 3, 7, 10, 14, 21 } }, { "maj7#11", 5, { 0, 4, 7, 11, 18 } }, { "7b9", 5, { 0, 4, 7, 10, 13 } },
    { "7#9", 5, { 0, 4, 7, 10, 15 } }, { "sus4(9)", 4, { 0, 5, 7, 14 } }, { "dim7", 4, { 0, 3, 6, 9 } },
    { "mMaj7", 4, { 0, 3, 7, 11 } }, { "7sus4", 4, { 0, 5, 7, 10 } }, { "9sus4", 5, { 0, 5, 7, 10, 14 } },
};

int kit_closest_chord(const uint8_t *notes, int n, int tonic, int minor, char *out, int len)
{
    int pcs[12], np = 0, i, j, d, low = 128, prev = -1, best_pen = 1000, best_ord = 0, best_root = -1, best_miss = 0, k = 0;
    const char *s;
    if (len <= 0) return -1;
    out[0] = 0;
    for (i = 0; i < n; i++) if (notes[i] < low) low = notes[i];
    for (;;) {                                       /* the pitch classes, in the order of the notes low to high */
        int nxt = 128;
        for (i = 0; i < n; i++) if (notes[i] > prev && notes[i] < nxt) nxt = notes[i];
        if (nxt > 127) break;
        for (j = 0; j < np && pcs[j] != nxt % 12; j++) ;
        if (j == np) pcs[np++] = nxt % 12;
        prev = nxt;
    }
    if (np < 3) return -1;
    for (i = 0; i < np; i++)                        /* each note as the root, each chord in the dictionary's order */
        for (d = 0; d < (int)(sizeof FULL_DICT / sizeof FULL_DICT[0]); d++) {
            uint16_t tones = 0, rel = 0;
            int nt = 0, p, pen;
            for (j = 0; j < FULL_DICT[d].n; j++) tones |= (uint16_t)(1u << (FULL_DICT[d].iv[j] % 12));
            for (j = 0; j < np; j++) rel |= (uint16_t)(1u << ((pcs[j] - pcs[i] + 12) % 12));
            if ((rel & tones) != rel) continue;
            for (p = 0; p < 12; p++) nt += (tones >> p) & 1;
            pen = (nt - np) + (pcs[i] != low % 12 ? 2 : 0);
            if (pen < best_pen || (pen == best_pen && d < best_ord)) {
                best_pen = pen; best_ord = d; best_root = pcs[i]; best_miss = nt - np;
            }
        }
    if (best_root < 0) return -1;
#define PUT(str) for (s = (str); *s && k < len - 1; s++) out[k++] = *s
    PUT(kit_note_name_in_key(best_root, tonic, minor, KIT_NAMES_LETTERS));
    PUT(FULL_DICT[best_ord].sym);
    if (best_root != low % 12) {
        PUT("/");
        PUT(kit_note_name_in_key(low % 12, tonic, minor, KIT_NAMES_LETTERS));
    }
#undef PUT
    out[k] = 0;
    return best_miss;
}

/* ---- the melody guide and figures (API 21, D-049): Sketchpad core/melody melodyRole, core/figures rng, place,
 * renderFigure. Integer only: rng's draw r() in [0, 1) is the 32-bit value x / 2^32, so "r() < 0.5" is x < 2^31 and
 * floor(r() * n) is computed exactly from x's two halves (no 64-bit multiply, rule 8). ---- */
int kit_melody_role(int pc, uint16_t chord_pcs, uint16_t scale_pcs)
{
    pc = ((pc % 12) + 12) % 12;
    return (chord_pcs >> pc & 1u) ? KIT_MEL_STABLE : (scale_pcs >> pc & 1u) ? KIT_MEL_MOVES : KIT_MEL_PULLS;
}

static uint32_t fig_rng(uint32_t *a)            /* Sketchpad rng: mulberry32 */
{
    uint32_t t;
    *a += 0x6d2b79f5u;
    t = (*a ^ (*a >> 15)) * (1u | *a);
    t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
    return t ^ (t >> 14);
}
static int fig_pick(uint32_t x, int n)          /* floor(x / 2^32 * n), exactly */
{
    uint32_t hi = x >> 16, lo = x & 0xFFFFu, u = (uint32_t)n;
    return (int)((hi * u + ((lo * u) >> 16)) >> 16);
}
static int fig_place(int pcl, int near)         /* the note of pitch class pcl nearest near (Sketchpad place) */
{
    int base = near - ((((near % 12) - pcl) % 12 + 12) % 12), d0 = base - near, d1 = base + 12 - near;
    if (d0 < 0) d0 = -d0;
    if (d1 < 0) d1 = -d1;
    return d0 <= d1 ? base : base + 12;
}

int kit_render_figure(const kit_fig_step_t *steps, int n, int root_pc, const uint8_t *iv, int niv, int next_root_pc,
                      uint16_t scale_pcs, int32_t seed, int center, kit_fig_note_t *out)
{
    uint32_t a = (uint32_t)seed;
    int i, k, cnt = 0, prev = -1000, third = -1, fifth = -1;
    for (i = 0; i < niv; i++) { if (iv[i] % 12 == 3 && third < 0) third = 3; }
    for (i = 0; i < niv; i++) { if (iv[i] % 12 == 4 && third < 0) third = 4; }
    for (k = 0; k < 3 && fifth < 0; k++) {
        static const int F[3] = { 7, 6, 8 };
        for (i = 0; i < niv; i++) if (iv[i] % 12 == F[k]) fifth = F[k];
    }
    for (i = 0; i < n; i++) {
        int midi = -1000, ref = prev > -1000 ? prev : center;
        switch (steps[i].role) {
        case KIT_FIG_ROOT: midi = fig_place(root_pc, center); break;
        case KIT_FIG_OCTAVE: midi = fig_place(root_pc, center) + 12; break;
        case KIT_FIG_FIFTH: midi = fig_place(fifth < 0 ? root_pc : (root_pc + fifth) % 12, ref); break;
        case KIT_FIG_THIRD: midi = fig_place(third < 0 ? root_pc : (root_pc + third) % 12, ref); break;
        case KIT_FIG_TOP: midi = fig_place((root_pc + iv[niv - 1]) % 12, ref); break;
        case KIT_FIG_CHORDTONE: midi = fig_place((root_pc + iv[fig_pick(fig_rng(&a), niv)]) % 12, ref); break;
        case KIT_FIG_STEP: case KIT_FIG_STEPUP: case KIT_FIG_STEPDOWN: {
            int up = steps[i].role == KIT_FIG_STEPUP ? 1 : steps[i].role == KIT_FIG_STEPDOWN ? 0 : fig_rng(&a) < 0x80000000u;
            int here = prev > -1000 ? prev : fig_place(root_pc, center), cand = -1000;
            for (k = 1; k <= 12 && cand == -1000; k++) {
                int t = here + (up ? k : -k);
                if (scale_pcs >> (((t % 12) + 12) % 12) & 1u) cand = t;
            }
            midi = cand > -1000 ? cand : here;
            break; }
        case KIT_FIG_APPROACH: {
            int t = fig_place(next_root_pc >= 0 ? next_root_pc : root_pc, ref);
            midi = fig_rng(&a) < 2576980378u ? t - 1 : t + 1;   /* r() < 0.6 */
            break; }
        default: continue;
        }
        out[cnt].note = (uint8_t)(midi < 0 ? 0 : midi > 127 ? 127 : midi);
        out[cnt].pos = steps[i].pos;
        out[cnt].dur = steps[i].dur;
        out[cnt].role = steps[i].role;
        cnt++;
        prev = midi;
    }
    return cnt;
}
