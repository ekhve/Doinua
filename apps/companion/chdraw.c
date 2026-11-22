/* SPDX-License-Identifier: GPL-3.0-only */
/* chdraw.c: the chords track's PLAY pages drawn as the Chord Lab draws them (its draw(): the owner's contract, D-084, #125).
 * app.c includes it after seqsong.c; draw_play and ph_draw_pads_page call ch_draw_play.
 *
 * The lab's screen, 240 x 240: the top line (CHORDS, the mode, the key and tempo), the four knob cards, the chord big, its
 * numeral, a quiet line, then on Free nothing more, on Key and Shapes the key's seven chords and (Shapes) the
 * progression, on Pads the progression and every key with what it plays; the FM-1's keys at the bottom, lit as the
 * panel. The FM-1 has no degree sign or arrows in its fonts, so ch_text draws them: \1 a degree sign, \2 an arrow,
 * \3 a play triangle, \4 a tick. */

#define CH_HALF KIT_RGB(42, 79, 143)
#define CH_GOOD KIT_RGB(60, 207, 122)
#define CH_WARN KIT_RGB(242, 163, 60)
#define CH_PANEL KIT_RGB(20, 25, 34)

static int ch_glyph_w(char c, int font)
{
    int h = kit_font_height(font);
    if (c == 1) return h / 3 + 2;
    if (c == 2) return h * 2 / 3 + 3;
    if (c == 3) return h / 2 + 3;
    if (c == 4) return h * 2 / 3 + 2;
    if (c == 5) return 6;
    return 0;
}

static int ch_text_w(const char *s, int font)
{
    char run[64];
    int w = 0, n = 0;
    for (;; s++) {
        if (*s && *s >= ' ' && n < 63) { run[n++] = *s; continue; }
        run[n] = 0;
        if (n) w += kit_text_width(run, font);
        n = 0;
        if (!*s) break;
        w += ch_glyph_w(*s, font);
    }
    return w;
}

static void ch_glyph(char c, int x, int cy, int font, kit_color_t col)
{
    int h = kit_font_height(font), i;
    if (c == 1) {                               /* a small ring, raised */
        int s = h / 4 + 1, y = cy - h / 3;
        kit_rect(x + 2, y, s - 2, 1, col); kit_rect(x + 2, y + s - 1, s - 2, 1, col);
        kit_rect(x + 1, y + 1, 1, s - 2, col); kit_rect(x + s, y + 1, 1, s - 2, col);
    }
    if (c == 2) { int w = h / 2 + 1; kit_rect(x + 1, cy, w, 1, col); for (i = 0; i < 3; i++) kit_rect(x + w - i, cy - i, 1, 2 * i + 1, col); }
    if (c == 3) { int s = h / 4 + 1; for (i = 0; i <= s; i++) kit_rect(x + 1 + i, cy - (s - i), 1, 2 * (s - i) + 1, col); }
    if (c == 5) kit_rect(x + 2, cy, 2, 2, col);
    if (c == 4) { int s = h / 3; for (i = 0; i < s / 2 + 1; i++) kit_rect(x + 1 + i, cy + i - 1, 1, 2, col); for (i = 0; i <= s; i++) kit_rect(x + s / 2 + 1 + i, cy + s / 2 - i - 1, 1, 2, col); }
}

/* text with the lab's glyphs; y is the line's centre */
static void ch_text(int x, int cy, const char *s, int font, int align, kit_color_t col)
{
    char run[64];
    int n = 0, w = ch_text_w(s, font), top = cy - kit_font_height(font) / 2;
    if (align == KIT_ALIGN_CENTER) x -= w / 2;
    if (align == KIT_ALIGN_RIGHT) x -= w;
    for (;; s++) {
        if (*s && *s >= ' ' && n < 63) { run[n++] = *s; continue; }
        run[n] = 0;
        if (n) x += kit_text(x, top, run, font, KIT_ALIGN_LEFT, col);
        n = 0;
        if (!*s) break;
        ch_glyph(*s, x, cy, font, col);
        x += ch_glyph_w(*s, font);
    }
}

/* the lab's fit(): cut a text to a width */
static void ch_fit(char *s, int font, int w)
{
    int n = 0;
    while (s[n]) n++;
    while (n > 0 && ch_text_w(s, font) > w) s[--n] = 0;
}

/* a degree's chord as the lab names it: "Dm", "B" + a degree sign; its numeral "vii" + a degree sign */
static void ch_deg_label(int d, char *out)
{
    kit_chord_t tri[7];
    char *e;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    e = cat(out, kit_note_name_in_key(tri[d].root, pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
    if (tri[d].quality == KIT_Q_MINOR) cat(e, "m");
    else if (tri[d].quality == KIT_Q_DIM) { e[0] = 1; e[1] = 0; }
    else if (tri[d].quality == KIT_Q_AUG) cat(e, "+");
}
static void ch_roman(const char *roman, char *out)   /* "viio" -> "vii" + a degree sign */
{
    char *e = cat(out, roman);
    if (e > out && e[-1] == 'o') e[-1] = 1;
}

/* a chord of the progression, named as on the screen */
static void ch_chord_name(const ph_chord_t *c, char *out, int len)
{
    int pcs[8], deg, n = ph_chord_pcs(c, pcs, &deg), r;
    uint8_t v[KIT_CHORD_MAX];
    out[0] = 0;
    if (n <= 0) return;
    n = ph_voice(pcs, n, KIT_VOICE_ROOT, v);
    kit_identify_chord(v, n, pads[track].tonic, pads[track].minor, out, len, &r);
    if (out[0]) { char *e = out; while (*e) e++; if (e - out >= 3 && e[-3] == 'd' && e[-2] == 'i' && e[-1] == 'm') { e[-3] = 1; e[-2] = 0; } }
}

/* the top line, the lab's: CHORDS, the mode, the key and tempo (a play triangle while playing; a red dot while REC is on) */
static void ch_top(const char *mode_name)
{
    char s[32], *d;
    int w;
    kit_rect(0, 0, KIT_W, 2, SQ_TC[track]);
    w = kit_text(6, 5, track_name(track), KIT_FONT_XSMALL, KIT_ALIGN_LEFT, FG);
    kit_text(6 + w + 8 > 56 ? 6 + w + 8 : 56, 5, mode_name, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, SQ_MUTE);
    key_name(s);
    d = s; while (*d) d++;
    d = cat_int(cat(d, "   "), song.bpm);
    d = cat(d, octave > 0 ? "   Oct +" : "   Oct ");   /* the keys' octave: every page whose keys play notes (the owner, 2026-11-21) */
    d = cat_int(d, octave);
    if (kit_seq_playing()) { d[0] = ' '; d[1] = 3; d[2] = 0; }
    w = ch_text_w(s, KIT_FONT_XSMALL);
    ch_text(234, 12, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, SQ_MUTE);
    if (armed) kit_circle(234 - w - 7, 12, 3, REC);
    if (steal_until) kit_circle(234 - w - (armed ? 17 : 7), 12, 3, WARN);   /* a voice was taken: the FM-1 is full (ST-37, kept from the track pages) */
    kit_rect(6, 21, 228, 1, KIT_RGB(34, 41, 54));
}

/* the lab's smallest capitals (6.5 to 7 px): a 3 x 5 pixel font, upright or turned a quarter (CALM and ENERGY up the side) */
static const char CH_SL[] = "ABCDEGHIKLMNOPRSTUVYFW-1";
static const uint8_t CH_SF[24][5] = {   /* rows top to bottom, 3 bits */
    { 2, 5, 7, 5, 5 }, { 6, 5, 6, 5, 6 }, { 3, 4, 4, 4, 3 }, { 6, 5, 5, 5, 6 }, { 7, 4, 6, 4, 7 }, { 3, 4, 5, 5, 3 },
    { 5, 5, 7, 5, 5 }, { 7, 2, 2, 2, 7 }, { 5, 5, 6, 5, 5 }, { 4, 4, 4, 4, 7 }, { 5, 7, 7, 5, 5 }, { 5, 7, 7, 7, 5 },
    { 2, 5, 5, 5, 2 }, { 6, 5, 6, 4, 4 }, { 6, 5, 6, 5, 5 }, { 3, 4, 2, 1, 6 }, { 7, 2, 2, 2, 2 }, { 5, 5, 5, 5, 7 },
    { 5, 5, 5, 5, 2 }, { 5, 5, 2, 2, 2 }, { 7, 4, 6, 4, 4 }, { 5, 5, 7, 7, 5 }, { 0, 0, 7, 0, 0 }, { 2, 6, 2, 2, 7 } };
static const uint8_t *ch_sglyph(char c)
{
    int k;
    for (k = 0; CH_SL[k]; k++) if (CH_SL[k] == c) return CH_SF[k];
    return 0;
}
static void ch_small(int cx, int cy, const char *w, kit_color_t col)   /* upright, centred */
{
    int n = 0, i, r, c, x;
    while (w[n]) n++;
    x = cx - (n * 4 - 1) / 2;
    for (i = 0; i < n; i++, x += 4) {
        const uint8_t *g = ch_sglyph(w[i]);
        if (!g) continue;
        for (r = 0; r < 5; r++) for (c = 0; c < 3; c++) if (g[r] >> (2 - c) & 1) kit_pixel(x + c, cy - 2 + r, col);
    }
}
static void ch_vword(int cx, int cy, const char *w, kit_color_t col)   /* turned a quarter, reading upwards */
{
    int n = 0, i, r, c, y;
    while (w[n]) n++;
    y = cy + (n * 4 - 1) / 2;
    for (i = 0; i < n; i++, y -= 4) {
        const uint8_t *g = ch_sglyph(w[i]);
        if (!g) continue;
        for (r = 0; r < 5; r++) for (c = 0; c < 3; c++) if (g[r] >> (2 - c) & 1) kit_pixel(cx - 2 + r, y - c, col);
    }
}

static int ch_knob_i = -1;                      /* the card of the knob turned last, lit for a moment (the lab) */
static uint32_t ch_knob_until;
static void ch_turned(int id) { if (id >= KIT_ENC_KNOB1 && id <= KIT_ENC_KNOB4) { ch_knob_i = id - KIT_ENC_KNOB1; ch_knob_until = kit_ms() + 900; if (!ch_knob_until) ch_knob_until = 1; } }

static void ch_card(int i, const char *label, const char *value)   /* label NULL: the lab's empty card */
{
    char v[24];
    int x = 4 + i * 58, lit = ch_knob_i == i && ch_knob_until && (int32_t)(kit_ms() - ch_knob_until) < 0;
    if (label) exp_card(i, label, value, 0);
    kit_round_rect(x, 26, 54, 30, 5, lit ? SQ_ACCD : SQ_CARD);
    ch_small(x + 27, 34, label ? label : "-", SQ_MUTE);   /* the lab's small capitals: every name fits its card */
    if (label && value) {
        cat(v, value);
        ch_fit(v, KIT_FONT_XSMALL, 50);
        ch_text(x + 27, 47, v, KIT_FONT_XSMALL, KIT_ALIGN_CENTER, SQ_ACC);
    }
}

/* the chord big, its numeral, a quiet line (the lab's 88, 118 and 138) */
static void ch_hero(const char *big, const char *sub, kit_color_t sub_col, const char *sub2, kit_color_t sub2_col)
{
    char s[64];
    if (big && big[0]) { cat(s, big); ch_fit(s, KIT_FONT_LARGE, 228); ch_text(8, 88, s, KIT_FONT_LARGE, KIT_ALIGN_LEFT, FG); }
    if (sub && sub[0]) ch_text(10, 118, sub, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, sub_col);
    if (sub2 && sub2[0]) { cat(s, sub2); ch_fit(s, KIT_FONT_XSMALL, 226); ch_text(10, 136, s, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, sub2_col); }
}

/* the key's seven chords: the numeral over the chord; full the chord now, half the suggestions */
static void ch_row(int lit, uint8_t half)
{
    kit_chord_t tri[7];
    int d;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    for (d = 0; d < 7; d++) {
        char nm[12], ro[10];
        int x = 4 + d * 33, y = 146, on = d == lit, hf = !on && (half >> d & 1);
        ch_deg_label(d, nm);
        ch_roman(tri[d].roman, ro);
        kit_round_rect(x, y, 30, 32, 5, on ? SQ_ACC : hf ? CH_HALF : SQ_CARD);
        ch_text(x + 15, y + 9, ro, KIT_FONT_TINY, KIT_ALIGN_CENTER, on || hf ? FG : SQ_MUTE);
        ch_text(x + 15, y + 22, nm, KIT_FONT_TINY, KIT_ALIGN_CENTER, on || hf ? FG : FG);
    }
}

/* the progression's chords, the one now marked (Shapes: a quiet line at 182; Pads: tiles at 142, the next one half) */
static void ch_strip(int y, int tiles, int cur)
{
    int n = ph_prog_len(), i, w;
    if (n <= 0 || play_prog[track] < PG_FIRST) {
        if (!(play_prog[track] == PG_MINE && ph_nmine[track])) return;
    }
    if (n > 8) n = 8;
    if (n <= 0) return;
    w = 228 / n;
    for (i = 0; i < n; i++) {
        ph_chord_t c;
        char nm[20];
        int x = 6 + i * w, on = i == cur, nx = tiles && cur >= 0 && i == (cur + 1) % n;
        ph_prog_chord(i, &c);
        ch_chord_name(&c, nm, sizeof nm);
        ch_fit(nm, KIT_FONT_TINY, w - 6);
        if (tiles) {
            kit_round_rect(x, y, w - 3, 18, 4, on ? SQ_ACC : nx ? CH_HALF : CH_PANEL);
            ch_text(x + w / 2 - 1, y + 9, nm, KIT_FONT_TINY, KIT_ALIGN_CENTER, on || nx ? FG : SQ_MUTE);
        } else {
            if (on) kit_round_rect(x, y, w - 3, 14, 3, SQ_ACCD);
            ch_text(x + w / 2 - 1, y + 7, nm, KIT_FONT_TINY, KIT_ALIGN_CENTER, on ? FG : SQ_MUTE);
        }
    }
}

/* the FM-1's keys at the bottom, lit as the panel (lv: the keys' lights); labels: Pads' chords and colours, else the
 * notes' names */
static void ch_keys(int y, int h, const uint8_t *lv, int labels)
{
    kit_chord_t tri[7];
    int k, wi = 0, nw = 0;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    for (k = 0; k < KIT_NKEYS; k++) nw += kit_is_white(KIT_KEY0_NOTE + k);
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k, x, xe, s = lv[k];
        kit_color_t c, tc;
        if (!kit_is_white(n)) continue;
        x = 2 + wi * 236 / nw; xe = 2 + (wi + 1) * 236 / nw - 1;
        c = kit_key_held(k) ? CH_GOOD : s == KIT_LIGHT_ON || s == KIT_LIGHT_BREATH ? SQ_ACC : s == KIT_LIGHT_HALF ? CH_HALF : s == KIT_LIGHT_DIM ? KIT_RGB(58, 74, 102) : KIT_RGB(207, 213, 223);
        tc = c == KIT_RGB(207, 213, 223) ? KIT_RGB(74, 81, 96) : FG;
        kit_rect(x, y, xe - x, h, c);
        if (labels) {
            char t[12];
            int d = ph_pad_deg(k);
            t[0] = 0;
            if (d >= 0) ch_deg_label(d, t);
            if (k == PK_NEXT || k == PK_AGAIN) {         /* NEXT and AGAIN, written down the key */
                const char *w = k == PK_NEXT ? "NEXT" : "AGAIN";
                int i;
                int nl = 0; while (w[nl]) nl++;   /* the last letter at the bottom (the owner, 2026-11-20) */
                for (i = 0; w[i]; i++) { char one[2]; one[0] = w[i]; one[1] = 0; ch_text(x + (xe - x) / 2, y + h - 8 - (nl - 1 - i) * 9, one, KIT_FONT_TINY, KIT_ALIGN_CENTER, c == KIT_RGB(207, 213, 223) ? KIT_RGB(28, 32, 41) : FG); }
                t[0] = 0;
            }
            ch_fit(t, KIT_FONT_TINY, xe - x + 2);
            ch_text(x + (xe - x) / 2, y + h - 9, t, KIT_FONT_TINY, KIT_ALIGN_CENTER, c == KIT_RGB(207, 213, 223) ? KIT_RGB(28, 32, 41) : FG);
        } else {
            char t[4];
            cat(t, kit_note_name_in_key(n + play_shift(), pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
            ch_text(x + (xe - x) / 2, y + h - 6, t, KIT_FONT_TINY, KIT_ALIGN_CENTER, tc);
        }
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_KEY0_NOTE + k, s = lv[k], ww = 236 / nw, x, bw, bh;
        kit_color_t c;
        if (kit_is_white(n)) { wi++; continue; }
        x = 2 + wi * 236 / nw - ww * 32 / 100; bw = ww * 64 / 100; bh = h * 58 / 100;
        c = kit_key_held(k) ? CH_GOOD : s == KIT_LIGHT_ON || s == KIT_LIGHT_BREATH ? SQ_ACC : s == KIT_LIGHT_HALF ? CH_HALF : KIT_RGB(32, 36, 44);
        kit_rect(x, y, bw, bh, c);
        if (labels) {
            int cl = PH_BLACK[k];
            if (k == PK_LOCK) {
                const char *w = "LOCK";
                int i;
                for (i = 0; w[i]; i++) { char one[2]; one[0] = w[i]; one[1] = 0; ch_text(x + bw / 2, y + 5 + i * 9, one, KIT_FONT_TINY, KIT_ALIGN_CENTER, FG); }
            } else if (cl >= 0) {
                char t[8];
                cat(t, PH_BLACK_LAB[cl]);
                if (t[0] == 'o' && !t[1]) t[0] = 1;   /* dim: a degree sign, as the lab */
                ch_fit(t, KIT_FONT_TINY, bw + 8);
                ch_text(x + bw / 2, y + bh - 7, t, KIT_FONT_TINY, KIT_ALIGN_CENTER, FG);
            }
        }
    }
}

/* the progression menu, the lab's: "PROGRESSION n / N", seven rows, each its number, style and numerals */
static void ch_menu(void)
{
    int r, cur = play_prog[track];
    char s[64], *d;
    kit_round_rect(4, 60, 232, 176, 8, SQ_ACC);
    kit_round_rect(5, 61, 230, 174, 7, KIT_RGB(12, 15, 22));
    d = cat_int(cat(s, "PROGRESSION  "), cur + 1);
    cat_int(cat(d, " / "), PH_NPROGS);
    ch_text(12, 72, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, SQ_MUTE);
    for (r = -3; r <= 3; r++) {
        int i = cur + r, y = 92 + (r + 3) * 20, on = !r;
        const ph_prog_t *p;
        if (i < 0 || i >= PH_NPROGS) continue;
        p = &PH_PROGS[i];
        if (on) kit_round_rect(8, y - 9, 224, 18, 4, SQ_ACCD);
        cat_int(s, i + 1);
        ch_text(i + 1 < 10 ? 16 : 12, y, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? FG : SQ_MUTE);
        cat(s, i < PG_FIRST || !p->style[0] || p->style[0] == 'S' ? p->name : p->style);
        ch_fit(s, KIT_FONT_TINY, 82);
        ch_text(30, y, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? FG : FG);
        ph_numerals(i, s, sizeof s);
        if (i >= PG_FIRST) { char *e; for (e = s; *e; e++) if (*e == 'o' && (e[1] == 0 || e[1] == ' ')) *e = 1; }   /* viio: a degree sign (not the specials' words) */
        ch_fit(s, KIT_FONT_TINY, 124);
        ch_text(108, y, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? KIT_RGB(207, 224, 255) : SQ_MUTE);
    }
}

static void play_levels(uint8_t *lv);

/* the chords' PLAY page, every HELP step (R-247..R-260), as the lab draws it */
static void ch_draw_play(void)
{
    uint8_t held[16], lv[KIT_NKEYS];
    char big[32], sub[48], sub2[64], v[24], *d;
    kit_color_t sub_col = SQ_MUTE, sub2_col = SQ_MUTE;
    int i, nh = play_held(held, 16), root = -1, h = play_helps[track], row_lit = -1, inkey = 1, cur = -1;
    uint8_t row_half = 0;
    kit_chord_t tri[7];
    const kit_scale_t *sc = &KIT_SCALES[pads[track].minor ? 1 : 0];
    big[0] = sub[0] = sub2[0] = 0;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    play_levels(lv);
    kit_clear(BG);
    ch_top("PLAY");
    ch_card(0, "HELP", PH_NAMES[h]);
    if (h == PH_FREE || h == PH_KEY) {
        ch_card(1, "TRANSPOSE", play_trans[track] ? "whites" : "off");
        if (h == PH_KEY) ch_card(2, "COLOUR", CC_NAMES[play_colour[track]]); else ch_card(2, 0, 0);
        ch_card(3, 0, 0);                       /* the lab's PLUS (hold, octave): no story builds it yet (#125) */
    } else {
        cat_int(v, play_prog[track] + 1);
        ch_card(1, "PROGRESSION", v);
        if (h == PH_SHAPES) { ch_card(2, "COLOUR", CC_NAMES[play_colour[track]]); ch_card(3, "VOICING", PV_NAMES[pads[track].voicing]); }
        else { ch_card(2, "VOICING", PV_NAMES[pads[track].voicing]); ch_card(3, "FEEL", FEEL_WORDS[play_feel[track]]); }
    }
    for (i = 0; i < nh; i++) if (!kit_scale_has(sc, pads[track].tonic, held[i])) inkey = 0;
    if (h != PH_PADS && nh) {                   /* what is held: a note, a chord, or notes */
        if (nh == 1) {
            d = cat(big, kit_note_name_in_key(held[0], pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS));
            cat_int(d, kit_octave(held[0]));
            if (!inkey) cat(sub, "outside");
        } else if (kit_identify_chord(held, nh, pads[track].tonic, pads[track].minor, big, sizeof big, &root)) {
            int dg = ph_degree_of(held, nh, root);
            if (dg >= 0) { ch_roman(tri[dg].roman, sub); row_lit = dg; play_last_deg[track] = dg; } else cat(sub, "outside");
        } else {
            int miss;
            char cl[20];
            for (i = 0, d = big; i < nh && i < 6; i++) { if (i) d = cat(d, " "); d = cat(d, kit_note_name_in_key(held[i], pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)); }
            miss = kit_closest_chord(held, nh, pads[track].tonic, pads[track].minor, cl, sizeof cl);
            if (miss > 0) {                     /* the closest chord, smaller under the notes (the approved scenario) */
                d = cat(cat(sub, "closest "), cl);
                d = cat_int(cat(d, " ("), miss);
                cat(d, " missing)");
                sub_col = SQ_ACC;
            } else {
                cat(sub, "no chord");
            }
        }
        if (nh >= 2 && sub2[0] == 0 && h != PH_SHAPES && sub[0] && sub[0] != 'n' && sub[0] != 'c') {   /* the notes held, low to high (the approved chord play scenario) */
            for (i = 0, d = sub2; i < nh && i < 8; i++) { if (i) d = cat(d, " "); d = cat(d, kit_note_name_in_key(held[i], pads[track].tonic, pads[track].minor, KIT_NAMES_LETTERS)); }
        }
        { char *e = big; while (*e) e++; if (e - big >= 3 && e[-3] == 'd' && e[-2] == 'i' && e[-1] == 'm') { e[-3] = 1; e[-2] = 0; } }
    }
    if (play_shift() && (h == PH_FREE || h == PH_KEY)) {   /* TRANSPOSE: which key the white keys play */
        d = cat(sub2, pads[track].minor ? "keys: A minor " : "keys: C major ");
        d[0] = 2; d[1] = ' '; d[2] = 0; d += 2;
        key_name(d);
        while (*d) d++;
        if (d - sub2 > 4 && d[-3] == 'm' && d[-2] == 'a' && d[-1] == 'j') cat(d - 3, "major");
        else if (d - sub2 > 4 && d[-3] == 'm' && d[-2] == 'i' && d[-1] == 'n') cat(d - 3, "minor");
    }
    if (h == PH_KEY) {
        if (nh) {                               /* the key's chords with every note held; the one suggested and the next */
            int cs[21], nc = ph_cands(held, nh, cs);
            for (i = 0; i < nc; i++) row_half |= (uint8_t)(1u << cs[i]);
            if (nc) {
                int cd = cs[pmod(play_cand[track], nc)], pcs[8], m = ph_deg_notes(cd, play_colour[track], pcs), miss = 0, j;
                uint8_t cv[KIT_CHORD_MAX];
                char nm[20];
                for (i = 0; i < m; i++) { int have = 0; for (j = 0; j < nh; j++) if (held[j] % 12 == pmod(pcs[i], 12)) have = 1; miss += !have; }
                if (nh == 1 || miss) {
                    m = ph_voice(pcs, m, KIT_VOICE_ROOT, cv);
                    kit_identify_chord(cv, m, pads[track].tonic, pads[track].minor, nm, sizeof nm, &j);
                    d = sub2; d[0] = miss ? 2 : 4; d[1] = ' '; d[2] = 0; d = cat(d + 2, nm);
                    if (nc > 1) {
                        int c2 = cs[pmod(play_cand[track] + 1, nc)];
                        m = ph_voice(pcs, ph_deg_notes(c2, play_colour[track], pcs), KIT_VOICE_ROOT, cv);
                        kit_identify_chord(cv, m, pads[track].tonic, pads[track].minor, nm, sizeof nm, &j);
                        d = cat(d, "    "); d[0] = 3; d[1] = ' '; d[2] = 0; cat(d + 2, nm);
                    }
                    row_lit = cd;
                }
            }
        } else if (play_last_deg[track] >= 0) { /* nothing held: the chords that may come next, half */
            d = cat(sub2, "next:");
            for (i = 0; i < 3; i++) if (PH_NEXT[play_last_deg[track]][i] >= 0) {
                char nm[12];
                row_half |= (uint8_t)(1u << PH_NEXT[play_last_deg[track]][i]);
                ch_deg_label(PH_NEXT[play_last_deg[track]][i], nm);
                d = cat(cat(d, "  "), nm);
            }
        }
    }
    if (h == PH_SHAPES) {                       /* the chord to play, the next under it (R-252) */
        uint8_t sv[KIT_CHORD_MAX], nv[KIT_CHORD_MAX];
        int sd, nd, sn = ph_shape(play_step[track], sv, &sd), nn = ph_shape(play_step[track] + 1, nv, &nd), ok = nh >= 3;
        uint16_t want = 0, have = 0;
        char nx[20];
        for (i = 0; i < sn; i++) want |= (uint16_t)(1u << (sv[i] % 12));
        for (i = 0; i < nh; i++) have |= (uint16_t)(1u << (held[i] % 12));
        ok = ok && want == have;
        kit_identify_chord(nv, nn, pads[track].tonic, pads[track].minor, nx, sizeof nx, &i);
        if (sn && (!nh || ok)) {
            kit_identify_chord(sv, sn, pads[track].tonic, pads[track].minor, big, sizeof big, &root);
            sub[0] = 0;
            if (sd >= 0) ch_roman(tri[sd].roman, sub);
        }
        d = sub2;
        if (ok) { d[0] = 4; d[1] = ' '; d[2] = ' '; d[3] = 0; d += 3; }
        cat(cat(d, "next "), nx);
        sub2_col = ok ? CH_GOOD : SQ_MUTE;
        row_lit = sd;
        cur = pmod(play_step[track], ph_prog_len() > 0 ? ph_prog_len() : 1);
    }
    if (h == PH_PADS) {                         /* the pad playing, the progression under it */
        if (ph_now_on[track] && pads[track].now.n) {
            kit_identify_chord(pads[track].now.notes, pads[track].now.n, pads[track].tonic, pads[track].minor, big, sizeof big, &root);
            if (!big[0]) cat(big, "?");
            if (pads[track].now_degree >= 0) ch_roman(tri[pads[track].now_degree].roman, sub); else cat(sub, "outside");
        } else {
            cat(big, "Pads");
        }
        d = cat_int(sub2, play_prog[track] + 1);
        d = cat(d, " ");
        d = cat(d, PH_PROGS[play_prog[track]].style[0] && play_prog[track] >= PG_FIRST && PH_PROGS[play_prog[track]].style[0] != 'S' ? PH_PROGS[play_prog[track]].style : "");
        if (PH_PROGS[play_prog[track]].style[0] && play_prog[track] >= PG_FIRST && PH_PROGS[play_prog[track]].style[0] != 'S') d = cat(d, " ");
        d = cat(d, PH_PROGS[play_prog[track]].name);
        if (ph_lock[track] != CC_NONE) cat(cat(d, "   lock "), CC_NAMES[ph_lock[track]]);
        cur = pmod(play_step[track], ph_prog_len() > 0 ? ph_prog_len() : 1);
    }
    { char *e = big; while (*e) e++; if (e - big >= 3 && e[-3] == 'd' && e[-2] == 'i' && e[-1] == 'm') { e[-3] = 1; e[-2] = 0; } }
    if (sub[0] && sub[0] == 'o') sub_col = CH_WARN;
    ch_hero(big, sub, sub_col, sub2, sub2_col);
    if (h == PH_PADS) {
        ch_strip(142, 1, cur);
        ch_keys(166, 72, lv, 1);
    } else {
        if (h != PH_FREE) ch_row(row_lit, row_half);
        if (h == PH_SHAPES) ch_strip(182, 0, cur);
        ch_keys(200, 38, lv, 0);
    }
    if (last_note >= 0 && last_channel) {       /* a note from a MIDI keyboard or a computer (until a key is played) */
        d = cat_int(cat(sub2, "MIDI channel "), last_channel);
        cat(cat(d, "  "), (last_mtrack >= 0 ? track_name(last_mtrack) : "-"));
        kit_round_rect(4, 146, 232, 22, 6, SQ_ACCD);
        ctext(120, 157, sub2, KIT_FONT_XSMALL, FG);
    }
    if (kit_ms() < ph_menu_until) ch_menu();    /* PROGRESSION turning (R-257) */
}

static void ci_levels(uint8_t *lv);

/* IDEAS, as the lab draws it: the chord now big, its numeral, listen / your turn and the next chord, the key's chords,
 * the idea's chords, the keys */
static void ch_draw_ideas(void)
{
    ci_ev_t ev[CI_MAXEV];
    uint8_t v[KIT_CHORD_MAX], held[16], lv[KIT_NKEYS];
    char big[32], sub[16], sub2[64], w[24], nx[20], *d;
    kit_chord_t tri[7];
    int now, next, pos, len, nev = ci_where(&now, &next, &pos, &len), p = ci_prog(ci_style[track], ci_idea[track]), i, deg = -1, root, nh;
    kit_color_t sub2_col = SQ_MUTE;
    big[0] = sub[0] = sub2[0] = 0;
    kit_triads(pads[track].tonic, pads[track].minor ? 1 : 0, tri);
    ci_levels(lv);
    kit_clear(BG);
    ch_top("IDEAS");
    ch_card(0, "STYLE", CI_STYLES[ci_style[track]]);
    d = cat_int(w, ci_idea[track] + 1);
    cat_int(cat(d, " / "), ci_nideas(ci_style[track]));
    ch_card(1, "IDEA", w);
    cat(cat_int(w, MEL_SPEEDS[ci_speed[track]]), "%");
    ch_card(2, "SPEED", w);
    ch_card(3, "HEAR", CI_HEAR[ci_hear[track]]);
    if (nev && p >= 0) {
        int n = ci_voiced(p, (ci_events(ci_style[track], ci_idea[track], ev, &nev), ev[now].step), v, &deg);
        uint16_t want = 0, have = 0;
        kit_identify_chord(v, n, pads[track].tonic, pads[track].minor, big, sizeof big, &root);
        for (i = 0; i < n; i++) want |= (uint16_t)(1u << (v[i] % 12));
        i = ci_voiced(p, ev[next].step, v, &root);
        kit_identify_chord(v, i, pads[track].tonic, pads[track].minor, nx, sizeof nx, &root);
        nh = play_held(held, 16);
        for (i = 0; i < nh; i++) have |= (uint16_t)(1u << (held[i] % 12));
        if (deg >= 0) ch_roman(tri[deg].roman, sub);
        if (!kit_seq_playing()) {
            d = cat(sub2, PH_PROGS[p].name);
            d = cat(cat(cat(d, ", "), CR_NAMES[CI_RHY[ci_style[track]][ci_idea[track] % 2]]), "   ");
            d[0] = 3; d[1] = ' '; d[2] = 0; cat(d + 2, "to play");
        } else if (nh >= 3 && (want & have) == want) {
            d = sub2; d[0] = 4; d[1] = ' '; d[2] = ' '; d[3] = 0; cat(cat(d + 3, "next "), nx);
            sub2_col = CH_GOOD;
        } else {
            cat(cat(sub2, ci_hear[track] == 0 && ci_abs < (uint32_t)len ? "listen   next " : "your turn   next "), nx);
        }
        { char *e = big; while (*e) e++; if (e - big >= 3 && e[-3] == 'd' && e[-2] == 'i' && e[-1] == 'm') { e[-3] = 1; e[-2] = 0; } }
        ch_hero(big, sub, SQ_MUTE, sub2, sub2_col);
        ch_row(deg, 0);
        {                                       /* the idea's chords, the one now marked (the lab's quiet line) */
            int m = nev > 8 ? 8 : nev, bw = m ? 228 / m : 228;
            for (i = 0; i < m; i++) {
                uint8_t vv[KIT_CHORD_MAX];
                char nm[20];
                int r, k = ci_voiced(p, ev[i].step, vv, &r), x = 6 + i * bw;
                kit_identify_chord(vv, k, pads[track].tonic, pads[track].minor, nm, sizeof nm, &r);
                ch_fit(nm, KIT_FONT_TINY, bw - 5);
                if (i == now) kit_round_rect(x, 182, bw - 3, 14, 3, SQ_ACCD);
                ch_text(x + bw / 2 - 1, 189, nm, KIT_FONT_TINY, KIT_ALIGN_CENTER, i == now ? FG : SQ_MUTE);
            }
        }
    } else {
        ch_hero(CI_STYLES[ci_style[track]], "", SQ_MUTE, "", SQ_MUTE);
        ch_row(-1, 0);
    }
    ch_keys(200, 38, lv, 0);
}


/* AUTO, as the lab draws it: the chord now big, the part (and the next), the part's chords as tiles, then the fourteen
 * parts on their keys, calm and energy */
static void ch_draw_auto(void)
{
    uint8_t v[KIT_CHORD_MAX];
    char big[32], sub[64], *d;
    int deg = -1, root, n, k, b = 0;
    kit_clear(BG);
    ch_top("AUTO");
    ch_card(0, "STYLE", CI_STYLES[au_style[track]]);
    ch_card(1, "DENSITY", AU_DENS[au_dens[track]]);
    ch_card(2, "SWING", AU_SWING[au_swing[track]]);
    ch_card(3, "FOLLOW", AU_FOLLOW[au_follow[track]]);
    big[0] = sub[0] = 0;
    if (au_part >= 0) {
        int8_t bars[8];
        int prog, nb = au_part_bars(au_part, &prog, bars), i, bw;
        b = kit_seq_playing() ? (int)(au_abs / KIT_TICKS_PER_BAR) - au_bar0 : 0;
        n = au_chord(b, v, &deg);
        kit_identify_chord(v, n, pads[track].tonic, pads[track].minor, big, sizeof big, &root);
        d = cat(sub, PT_NAMES[au_part]);
        d = cat(d, au_hi ? " - energy" : " - calm");
        if (au_next >= 0) { d = cat(d, "   "); d[0] = 2; d[1] = ' '; d[2] = 0; cat(d + 2, PT_NAMES[au_next]); }
        { char *e = sub; while (*e && !(e[0] == ' ' && e[1] == '-')) e++; if (*e) { e[1] = 5; } }
        ch_hero(big[0] ? big : "-", sub, SQ_MUTE, "", SQ_MUTE);
        if (nb > 8) nb = 8;
        bw = nb ? 228 / nb : 228;
        for (i = 0; i < nb && !au_follow[track]; i++) {   /* the part's chords: the one now full, the next half */
            uint8_t vv[KIT_CHORD_MAX];
            char nm[20];
            int r, m, pcs[8], on = kit_seq_playing() && i == pmod(b, nb), nx = kit_seq_playing() && i == pmod(b + 1, nb);
            ph_chord_t c;
            if (bars[i] < 0) { c.deg = 0; c.col = CC_NONE; c.root = 0; c.iv = 0; } else ci_step_chord(prog, bars[i], &c);
            m = ph_voice(pcs, ph_chord_pcs(&c, pcs, &r), KIT_VOICE_ROOT, vv);
            kit_identify_chord(vv, m, pads[track].tonic, pads[track].minor, nm, sizeof nm, &r);
            ch_fit(nm, KIT_FONT_TINY, bw - 6);
            kit_round_rect(6 + i * bw, 140, bw - 3, 18, 4, on ? SQ_ACC : nx ? CH_HALF : CH_PANEL);
            ch_text(6 + i * bw + bw / 2 - 1, 149, nm, KIT_FONT_TINY, KIT_ALIGN_CENTER, on || nx ? FG : SQ_MUTE);
        }
    } else {
        ch_hero(CI_STYLES[au_style[track]], "a white key: a part", SQ_MUTE, "", SQ_MUTE);
    }
    for (k = 0; k < 14; k++) {                  /* the parts on their keys: calm F3..E4, energy F4..E5 */
        static const char *const KL = "FGABCDE";
        int x = 22 + (k % 7) * 61 / 2, y = 166 + (k / 7) * 36, hi = k >= 7, p = k % 7;
        int on = au_part == p && au_hi == hi, nx = au_next == p && au_next_hi == hi;
        char kn[4];
        kit_round_rect(x, y, 28, 32, 5, on ? SQ_ACC : nx ? CH_HALF : hi ? KIT_RGB(29, 36, 51) : SQ_CARD);
        ch_small(x + 14, y + 11, PT_NAMES[p], FG);
        kn[0] = KL[p]; kn[1] = (char)('0' + (p < 4 ? 3 : 4) + hi); kn[2] = 0;
        ch_text(x + 14, y + 24, kn, KIT_FONT_TINY, KIT_ALIGN_CENTER, on ? KIT_RGB(223, 232, 255) : SQ_MUTE);
    }
    ch_vword(10, 182, "CALM", SQ_MUTE);
    ch_vword(10, 218, "ENERGY", SQ_MUTE);
}
