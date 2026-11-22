/* SPDX-License-Identifier: GPL-3.0-only */
/* Prints what fm1kit's music.c answers, in golden.json's shape, for check.py to compare. */
#include <stdio.h>
#include <string.h>
#include "fm1kit.h"

static void names(int solfege)
{
    int minor, tonic, p;
    printf("\"%s\": [", solfege ? "solfege" : "names");
    for (minor = 0; minor < 2; minor++)
        for (tonic = 0; tonic < 12; tonic++) {
            printf("%s{\"tonic\": %d, \"minor\": %d, \"names\": [", minor || tonic ? ", " : "", tonic, minor);
            for (p = 0; p < 12; p++)
                printf("%s\"%s\"", p ? ", " : "", kit_note_name_in_key(60 + p, tonic, minor, solfege ? KIT_NAMES_SOLFEGE : KIT_NAMES_LETTERS));
            printf("]}");
        }
    printf("], ");
}

static int runs(void)                  /* stdin: "pattern end_on seed n note..." per line; stdout: the order */
{
    int pat, end_on, n, i, m;
    unsigned seed;
    uint8_t notes[64], out[128];
    while (scanf("%d %d %u %d", &pat, &end_on, &seed, &n) == 4) {
        for (i = 0; i < n; i++) { int v; if (scanf("%d", &v) != 1) return 1; notes[i] = (uint8_t)v; }
        m = kit_run_order(notes, n, pat, seed, end_on, out);
        for (i = 0; i < m; i++) printf("%s%d", i ? " " : "", out[i]);
        printf("\n");
    }
    return 0;
}

static int chords(void)                 /* every seven-note scale and tonic: root q roman n0 n1 n2 / inv1 / inv2 per chord */
{
    int sc, t, i, k, v;
    for (sc = 0; sc < KIT_NSCALES; sc++)
        for (t = 0; t < 12; t++) {
            kit_chord_t c[7];
            if (!kit_triads(t, sc, c))
                continue;
            printf("%s %d", KIT_SCALES[sc].id, t);
            for (i = 0; i < 7; i++) {
                printf(" | %d %s %s", c[i].root, kit_quality_sym(c[i].quality), c[i].roman[0] ? c[i].roman : "-");
                for (v = 0; v < 3; v++) {
                    uint8_t o[4];
                    kit_inversion(&c[i], v, o);
                    for (k = 0; k < 3; k++) printf(" %d", o[k]);
                }
            }
            printf("\n");
        }
    return 0;
}

static int keyfit(void)                 /* stdin: "tonic minor n" then n x "root k notes..."; stdout: fits ok outside s_t s_m s_f */
{
    int t, m, n, i, k;
    while (scanf("%d %d %d", &t, &m, &n) == 3) {
        kit_chord_t c[32];
        kit_keyfit_t r;
        for (i = 0; i < n; i++) {
            int root, cnt;
            if (scanf("%d %d", &root, &cnt) != 2) return 1;
            c[i].root = (uint8_t)root;
            c[i].n = (uint8_t)cnt;
            for (k = 0; k < cnt; k++) { int v; if (scanf("%d", &v) != 1) return 1; c[i].notes[k] = (uint8_t)v; }
        }
        kit_key_fit(c, n, t, m, &r);
        printf("%d %d %u %d %d %d\n", r.fits, r.ok, (unsigned)r.outside, r.suggest_tonic, r.suggest_minor, r.suggest_fits);
    }
    return 0;
}

static int keys(void)                   /* stdin: "limit n" then n x "note len"; stdout: amb heard k, then per key: t m out ev held total */
{
    int limit, n, i;
    while (scanf("%d %d", &limit, &n) == 2) {
        kit_note_ev_t ev[64];
        kit_key_cand_t c[24];
        int amb, k;
        uint16_t heard;
        for (i = 0; i < n; i++) { int a, b; if (scanf("%d %d", &a, &b) != 2) return 1; ev[i].note = (uint8_t)a; ev[i].len = (uint16_t)b; }
        k = kit_keys_for_notes(ev, n, c, limit, &amb, &heard);
        printf("%d %u %d", amb, (unsigned)heard, k);
        for (i = 0; i < k; i++)
            printf(" %d %d %u %d %u %u", c[i].tonic, c[i].minor, (unsigned)c[i].outside, c[i].evidence, (unsigned)c[i].held, (unsigned)c[i].total);
        printf("\n");
    }
    return 0;
}

/* colour chords (D-033): every triad quality, colour, root, base and voicing; labels; the next chord */
static int colour(void)
{
    int q, c, r, b, v, i, t, m, d;
    static const int BASES[3] = { 36, 48, 60 };
    static const int QS[6] = { KIT_Q_MAJOR, KIT_Q_MINOR, KIT_Q_DIM, KIT_Q_AUG, KIT_Q_SUS2, KIT_Q_SUS4 };
    int qi;
    for (qi = 0; qi < 6 && (q = QS[qi], 1); qi++)
        for (c = 0; c < KIT_NCOLS; c++)
            for (r = 0; r < 12; r++)
                for (b = 0; b < 3; b++)
                    for (v = 0; v < KIT_NVOICINGS; v++) {
                        kit_vchord_t ch;
                        int n = kit_colour_chord(r, q, c, 0, v, BASES[b], &ch);
                        printf("chord [%s] %d %d %d", ch.sym, r, BASES[b], v);
                        for (i = 0; i < n; i++) printf(" %d", ch.notes[i]);
                        printf("\n");
                    }
    for (m = 0; m < 2; m++)
        for (t = 0; t < 12; t++)
            for (r = 0; r < 12; r++) {
                kit_vchord_t ch;
                char lab[24];
                kit_colour_chord(r, KIT_Q_MINOR, KIT_COL_9, 0, KIT_VOICE_ROOT, 48, &ch);
                kit_chord_label(&ch, t, m, lab, sizeof lab);
                printf("label %d %d %d %s\n", t, m, r, lab);
            }
    for (m = 0; m < 2; m++)
        for (t = 0; t < 12; t++) {
            printf("next %d %d", t, m);
            for (d = 0; d < 7; d++) printf(" %d", kit_next_degree(t, m, d));
            printf("\n");
        }
    return 0;
}

/* what played notes are (D-035): "tonic minor n notes..." in, "root label" out */
static int ident(void)
{
    int t, m, n, i, v, root;
    while (scanf("%d %d %d", &t, &m, &n) == 3) {
        uint8_t notes[16];
        char lab[32];
        for (i = 0; i < n; i++) { if (scanf("%d", &v) != 1) return 1; notes[i] = (uint8_t)v; }
        root = -1;
        if (!kit_identify_chord(notes, n, t, m, lab, sizeof lab, &root)) root = -1;
        printf("%d [%s]\n", root, lab);
    }
    return 0;
}

/* which scales fit (D-041): "tonic mask" in, "n id:held ..." out (the top five) */
static int fit(void)
{
    int t, m, i, nu, n;
    uint8_t out[17], held[17];
    while (scanf("%d %d", &t, &m) == 2) {
        n = kit_fit_scales((uint16_t)m, t, out, held, 5, &nu);
        printf("%d", n);
        for (i = 0; i < n; i++) printf(" %s:%d", KIT_SCALES[out[i]].id, held[i]);
        printf("\n");
    }
    return 0;
}

/* the closest chord (D-042): "tonic minor n notes..." in, "missing [label]" out */
static int closest(void)
{
    int t, m, n, i, v, miss;
    while (scanf("%d %d %d", &t, &m, &n) == 3) {
        uint8_t notes[16];
        char lab[32];
        for (i = 0; i < n; i++) { if (scanf("%d", &v) != 1) return 1; notes[i] = (uint8_t)v; }
        miss = kit_closest_chord(notes, n, t, m, lab, sizeof lab);
        printf("%d [%s]\n", miss, lab);
    }
    return 0;
}

/* the melody guide: "pc chord scale" in, the role out; figures: "nsteps (pos dur role)* root niv iv* next scale seed
 * center" in, "n (note pos dur)*" out (D-049) */
static int mguide(void)
{
    int p, c, s2;
    while (scanf("%d %d %d", &p, &c, &s2) == 3) printf("%d\n", kit_melody_role(p, (uint16_t)c, (uint16_t)s2));
    return 0;
}
static int figs(void)
{
    int ns, i, root, niv, next, scale, seed, center, n;
    while (scanf("%d", &ns) == 1) {
        kit_fig_step_t st[32];
        kit_fig_note_t out[32];
        uint8_t iv[16];
        for (i = 0; i < ns; i++) { int a, b, c; if (scanf("%d %d %d", &a, &b, &c) != 3) return 1; st[i].pos = (uint8_t)a; st[i].dur = (uint8_t)b; st[i].role = (uint8_t)c; }
        if (scanf("%d %d", &root, &niv) != 2) return 1;
        for (i = 0; i < niv; i++) { int v; if (scanf("%d", &v) != 1) return 1; iv[i] = (uint8_t)v; }
        if (scanf("%d %d %d %d", &next, &scale, &seed, &center) != 4) return 1;
        n = kit_render_figure(st, ns, root, iv, niv, next, (uint16_t)scale, seed, center, out);
        printf("%d", n);
        for (i = 0; i < n; i++) printf(" %d %d %d", out[i].note, out[i].pos, out[i].dur);
        printf("\n");
    }
    return 0;
}

int main(int argc, char **argv)
{
    int i, t, p, first = 1;
    if (argc > 1 && !strcmp(argv[1], "chords"))
        return chords();
    if (argc > 1 && !strcmp(argv[1], "mguide"))
        return mguide();
    if (argc > 1 && !strcmp(argv[1], "figs"))
        return figs();
    if (argc > 1 && !strcmp(argv[1], "closest"))
        return closest();
    if (argc > 1 && !strcmp(argv[1], "fit"))
        return fit();
    if (argc > 1 && !strcmp(argv[1], "ident"))
        return ident();
    if (argc > 1 && !strcmp(argv[1], "colour"))
        return colour();
    if (argc > 1 && !strcmp(argv[1], "keyfit"))
        return keyfit();
    if (argc > 1 && !strcmp(argv[1], "keys"))
        return keys();
    if (argc > 1 && !strcmp(argv[1], "runs"))
        return runs();
    if (argc > 1 && !strcmp(argv[1], "tempo")) {
        int b;
        while (scanf("%d", &b) == 1)
            printf("%d %u\n", kit_tempo_clamp(b), (unsigned)kit_beat_ms(b));
        return 0;
    }
    printf("{");
    names(0);
    names(1);
    printf("\"plain\": {\"letters\": [");
    for (p = 0; p < 12; p++) printf("%s\"%s\"", p ? ", " : "", kit_note_name(p, KIT_NAMES_LETTERS));
    printf("], \"solfege\": [");
    for (p = 0; p < 12; p++) printf("%s\"%s\"", p ? ", " : "", kit_note_name(p, KIT_NAMES_SOLFEGE));
    printf("]}, \"scales\": [");
    for (i = 0; i < KIT_NSCALES; i++)
        for (t = 0; t < 12; t++) {
            int n, k = 0;
            printf("%s{\"id\": \"%s\", \"tonic\": %d, \"pcs\": [", first ? "" : ", ", KIT_SCALES[i].id, t);
            first = 0;
            for (n = 0; n < KIT_SCALES[i].n; n++)
                printf("%s%d", k++ ? ", " : "", kit_pc(t + KIT_SCALES[i].iv[n]));
            printf("], \"has\": [");
            for (p = 0; p < 12; p++) printf("%s%d", p ? ", " : "", kit_scale_has(&KIT_SCALES[i], t, 60 + p));
            printf("]}");
        }
    printf("]}\n");
    return 0;
}
