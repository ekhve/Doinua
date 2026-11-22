/* SPDX-License-Identifier: GPL-3.0-only */
/* chideas.c: the chords' IDEAS page (ST-82, D-079, R-261..R-264), as the owner tried it in the Chord Lab (v4, v5).
 * It replaces the manual chord builder (D-036, R-085..R-094, retired by the owner in #108). app.c includes it after
 * chordplay.c.
 *
 * An idea is a style's progression with a rhythm, 2 or 4 bars long (R-261): K1 STYLE (our 13 styles), K2 IDEA (each
 * progression in the style's two rhythms), K3 SPEED (the whole song slower while practising, back to full when the page
 * closes, R-264), K4 HEAR (once, on, off). PLAY: the idea plays over the song, heard and never recorded; the chord now
 * lights full and the next one half from the bar before it (R-262). SELECT walks the idea's chords and lights them
 * (R-263); stopped, it plays the chord once. The keys play notes, to play along.
 *
 * How many bars each chord gets, so that an idea is 4 bars (the lab's PLAN): 1 chord 4; 2 chords A B A B; 3 chords
 * A B C-C; 4 chords one each. The rhythms, in beats: bar (a chord a bar), push (each chord half a beat early), halves
 * (a chord every two beats: 2 bars), hip hop (the first chord twice, a long one, the last chord on beat 8: 2 bars),
 * stabs (beats 1, 2.5 and 3.5 of each bar). */

enum { CR_BAR = 0, CR_PUSH, CR_HALVES, CR_HIPHOP, CR_STABS };
static const char *const CR_NAMES[] = { "bar", "push", "halves", "hip hop", "stabs" };
static const char *const CI_STYLES[] = { "Pop", "Neo soul", "Hip hop", "West coast", "Funk", "Electronic", "Jazz", "Reggae",
                                         "Afrobeats", "Shatta", "Reggaeton", "Drum & bass", "Rock" };
#define CI_NSTYLES ((int)(sizeof CI_STYLES / sizeof CI_STYLES[0]))
static const uint8_t CI_RHY[CI_NSTYLES][2] = {
    { CR_BAR, CR_HALVES }, { CR_PUSH, CR_HIPHOP }, { CR_HIPHOP, CR_PUSH }, { CR_HIPHOP, CR_BAR }, { CR_STABS, CR_HALVES },
    { CR_BAR, CR_STABS }, { CR_HALVES, CR_PUSH }, { CR_HALVES, CR_STABS }, { CR_PUSH, CR_STABS }, { CR_STABS, CR_BAR },
    { CR_HALVES, CR_PUSH }, { CR_BAR, CR_PUSH }, { CR_BAR, CR_HALVES } };
static const char *const CI_HEAR[3] = { "once", "on", "off" };

typedef struct { uint8_t step; uint16_t start, len; } ci_ev_t;   /* a chord of the idea: its step, ticks */
#define CI_MAXEV 16
static int ci_style[KIT_NTRACKS] = { 1, 1, 1, 1 };   /* Neo soul: where the pads have started (D-034) */
static int ci_idea[KIT_NTRACKS], ci_hear[KIT_NTRACKS] = { 1, 1, 1, 1 }, ci_sel[KIT_NTRACKS];
static int ci_speed[KIT_NTRACKS] = { MEL_NSPEEDS - 1, MEL_NSPEEDS - 1, MEL_NSPEEDS - 1, MEL_NSPEEDS - 1 };
static int ci_prev = -1, ci_cur = -1, ci_nnow;  /* the song's last tick seen; the event sounding (round * 64 + k) */
static uint32_t ci_abs, ci_off_at;              /* ticks since PLAY; when the chord sounding stops */
static uint8_t ci_now_notes[KIT_CHORD_MAX];
static uint32_t ci_aud_until;                   /* SELECT stopped: the chord heard until then */

static int ci_same(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

static int ci_nideas(int style)                 /* each progression of the style, in its two rhythms */
{
    int p, n = 0;
    for (p = PG_FIRST; p < PH_NPROGS; p++) if (ci_same(PH_PROGS[p].style, CI_STYLES[style])) n += 2;
    return n;
}

static int ci_prog(int style, int idea)         /* the PH_PROGS index of an idea (-1: none) */
{
    int p, k = 0;
    for (p = PG_FIRST; p < PH_NPROGS; p++)
        if (ci_same(PH_PROGS[p].style, CI_STYLES[style]) && k++ == idea / 2) return p;
    return -1;
}

/* the idea's chords in time (the lab's RHY over PLAN); the idea's length in ticks */
static int ci_events(int style, int idea, ci_ev_t *ev, int *nev)
{
    static const uint8_t PLAN[4][4][2] = { { { 0, 4 } }, { { 0, 1 }, { 1, 1 }, { 0, 1 }, { 1, 1 } },
                                           { { 0, 1 }, { 1, 1 }, { 2, 2 } }, { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } };
    static const uint8_t PLAN_N[4] = { 1, 4, 3, 4 };
    const int B = KIT_TICKS_PER_BEAT, h = B / 2;
    int p = ci_prog(style, idea), r, n, i, k = 0, total = 0, per;
    *nev = 0;
    if (p < 0) return 0;
    r = CI_RHY[style][idea % 2];
    n = PH_PROGS[p].n < 1 ? 1 : PH_PROGS[p].n > 4 ? 4 : PH_PROGS[p].n;
    per = r == CR_HALVES ? 2 : 4;               /* beats a plan bar takes */
    if (r == CR_HIPHOP) {
        static const uint8_t HH[4][3] = { { 0, 0, 3 }, { 0, 3, 5 }, { 0, 8, 6 }, { 1, 14, 2 } };   /* step (1: last), half beats */
        for (i = 0; i < 4; i++) {
            ev[k].step = (uint8_t)(HH[i][0] ? n - 1 : 0);
            ev[k].start = (uint16_t)(HH[i][1] * h);
            ev[k].len = (uint16_t)(HH[i][2] * h);
            k++;
        }
        *nev = k;
        return 8 * B;
    }
    for (i = 0; i < PLAN_N[n - 1]; i++) {
        int c = PLAN[n - 1][i][0], bars = PLAN[n - 1][i][1], b0 = total, l = bars * per * B;
        total += l;
        if (r == CR_STABS) {
            int x;
            for (x = b0; x < b0 + l && k + 3 <= CI_MAXEV; x += 4 * B) {
                ev[k].step = (uint8_t)c; ev[k].start = (uint16_t)x; ev[k].len = (uint16_t)h; k++;
                ev[k].step = (uint8_t)c; ev[k].start = (uint16_t)(x + 3 * h); ev[k].len = (uint16_t)h; k++;
                ev[k].step = (uint8_t)c; ev[k].start = (uint16_t)(x + 5 * h); ev[k].len = (uint16_t)B; k++;
            }
        } else {
            ev[k].step = (uint8_t)c;
            ev[k].start = (uint16_t)b0;
            ev[k].len = (uint16_t)l;
            if (r == CR_PUSH) {                 /* each chord after the first half a beat early; the last held on */
                if (i) { ev[k].start = (uint16_t)(b0 - h); ev[k].len = (uint16_t)(l + h); }
                else ev[k].len = (uint16_t)(l - h);
            }
            k++;
        }
    }
    *nev = k;
    return total;
}

/* a progression step as a chord in the song's key (the relative key for a set in the other mode) */
static void ci_step_chord(int p, int s, ph_chord_t *c)
{
    const ph_step_t *st = &PH_PROGS[p].s[pmod(s, PH_PROGS[p].n ? PH_PROGS[p].n : 1)];
    int home = pads[track].tonic;
    if (PH_PROGS[p].mode == 2 && !pads[track].minor) home += 9;
    if (PH_PROGS[p].mode == 1 && pads[track].minor) home += 3;
    if (st->deg >= 0) { c->deg = st->deg; c->col = st->col; c->root = 0; c->iv = 0; }
    else { c->deg = -1; c->col = 0; c->root = (int8_t)pmod(home + st->off, 12); c->iv = st->iv; }
}

static int ci_voiced(int p, int s, uint8_t *out, int *deg)    /* on the keys, root position */
{
    ph_chord_t c;
    int pcs[8], n;
    ci_step_chord(p, s, &c);
    n = ph_chord_pcs(&c, pcs, deg);
    return ph_voice(pcs, n, KIT_VOICE_ROOT, out);
}

static void ci_release(void)
{
    int i;
    for (i = 0; i < ci_nnow; i++) kit_track_note_off(track, ci_now_notes[i]);
    ci_nnow = 0;
}

static void ci_sound(int p, int s, int vel)     /* heard, never recorded */
{
    int i, d;
    ci_release();
    ci_nnow = ci_voiced(p, s, ci_now_notes, &d);
    for (i = 0; i < ci_nnow; i++) kit_track_note_on(track, ci_now_notes[i], vel);
}

/* where the idea is: the event now and the next (indexes), the position in the idea and its length (ticks) */
static int ci_where(int *now, int *next, int *pos, int *len)
{
    ci_ev_t ev[CI_MAXEV];
    int nev, k;
    *len = ci_events(ci_style[track], ci_idea[track], ev, &nev);
    if (!nev || !*len) { *now = *next = -1; return 0; }
    if (!kit_seq_playing()) {
        *now = pmod(ci_sel[track], nev);
        *pos = ev[*now].start;
    } else {
        *pos = (int)(ci_abs % (uint32_t)*len);
        *now = nev - 1;                         /* the last event that started (the first one's before it wraps) */
        for (k = 0; k < nev; k++) if (ev[k].start <= *pos) *now = k;
        if (*pos < ev[0].start) *now = nev - 1;
    }
    *next = (*now + 1) % nev;
    return nev;
}

/* each frame: follow the song, sound the idea's chords on their beats (R-261) */
static void ci_poll(uint32_t now_ms)
{
    ci_ev_t ev[CI_MAXEV];
    int nev, len, k, p, round, tick;
    if (ci_aud_until && (int32_t)(now_ms - ci_aud_until) >= 0) { ci_aud_until = 0; ci_release(); }
    if (!is_chords() || pages[track] != PAGE_IDEAS || !kit_seq_playing()) {
        if (ci_prev >= 0) { ci_release(); ci_prev = -1; ci_cur = -1; ci_abs = 0; }
        return;
    }
    tick = kit_seq_tick();
    if (ci_prev < 0) ci_abs = (uint32_t)tick;
    else ci_abs += (uint32_t)(tick >= ci_prev ? tick - ci_prev : tick + song.bars * KIT_TICKS_PER_BAR - ci_prev);
    ci_prev = tick;
    len = ci_events(ci_style[track], ci_idea[track], ev, &nev);
    p = ci_prog(ci_style[track], ci_idea[track]);
    if (!nev || !len || p < 0) return;
    {
        int pos = (int)(ci_abs % (uint32_t)len);
        round = (int)(ci_abs / (uint32_t)len);
        if (ci_nnow && ci_abs >= ci_off_at) ci_release();
        for (k = nev - 1; k >= 0; k--) if (ev[k].start <= pos) break;
        if (k >= 0 && round * 64 + k != ci_cur && pos < ev[k].start + ev[k].len) {
            int hear = ci_hear[track] == 1 || (ci_hear[track] == 0 && round == 0);
            ci_cur = round * 64 + k;
            if (hear) {
                ci_sound(p, ev[k].step, 80);
                ci_off_at = ci_abs + (uint32_t)(ev[k].len * 95 / 100);
            }
            dirty = 1;
        }
    }
}

static void ch_turned(int id);
static void ci_knob(int id, int d)
{
    ch_turned(id);
    if (id == KIT_ENC_KNOB1) { clampi(&ci_style[track], d, 0, CI_NSTYLES - 1); ci_idea[track] = 0; ci_sel[track] = 0; ci_release(); ci_cur = -1; }
    if (id == KIT_ENC_KNOB2) { clampi(&ci_idea[track], d, 0, ci_nideas(ci_style[track]) - 1); ci_sel[track] = 0; ci_release(); ci_cur = -1; }
    if (id == KIT_ENC_KNOB3) clampi(&ci_speed[track], d, 0, MEL_NSPEEDS - 1);
    if (id == KIT_ENC_KNOB4) clampi(&ci_hear[track], d, 0, 2);
    if (id == KIT_ENC_SELECT) {                 /* the idea's next chord, lit; stopped: heard once (R-263) */
        ci_ev_t ev[CI_MAXEV];
        int nev, p = ci_prog(ci_style[track], ci_idea[track]);
        ci_events(ci_style[track], ci_idea[track], ev, &nev);
        if (!nev || p < 0) return;
        ci_sel[track] = pmod(ci_sel[track] + d, nev);
        if (!kit_seq_playing()) {
            ci_sound(p, ev[ci_sel[track]].step, 80);
            ci_aud_until = kit_ms() + 700;
            if (!ci_aud_until) ci_aud_until = 1;
        }
    }
}

/* the page closes (SCL, another track): SPEED back to full (R-264), the idea's chord let go */
static void ci_leave(void)
{
    ci_speed[track] = MEL_NSPEEDS - 1;
    ci_release();
    ci_cur = -1;
}

/* the lights: the chord now full, the next one half from the bar before it (R-262), what you hold full */
static void ci_levels(uint8_t *lv)              /* the keys' lights on IDEAS (the screen draws them too, D-084) */
{
    ci_ev_t ev[CI_MAXEV];
    uint8_t v[KIT_CHORD_MAX];
    int k, i, n, d, now, next, pos, len, nev = ci_where(&now, &next, &pos, &len), p = ci_prog(ci_style[track], ci_idea[track]);
    for (k = 0; k < KIT_NKEYS; k++) lv[k] = kit_key_held(k) ? KIT_LIGHT_ON : KIT_LIGHT_OFF;
    if (nev && p >= 0) {
        ci_events(ci_style[track], ci_idea[track], ev, &nev);
        if (!kit_seq_playing() || pmod(ev[next].start - pos, len) <= KIT_TICKS_PER_BAR || next == now) {
            n = ci_voiced(p, ev[next].step, v, &d);
            for (i = 0; i < n; i++) ph_light(lv, v[i] - kb_low(), KIT_LIGHT_HALF);
        }
        n = ci_voiced(p, ev[now].step, v, &d);
        for (i = 0; i < n; i++) ph_light(lv, v[i] - kb_low(), KIT_LIGHT_ON);
    }
}
static void ci_lights(void)
{
    uint8_t lv[KIT_NKEYS];
    int k;
    ci_levels(lv);
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, lv[k]);
    kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF);
}

static void ch_draw_ideas(void);
static void ci_draw(void) { ch_draw_ideas(); }   /* the lab's IDEAS page (chdraw.c, D-084) */
