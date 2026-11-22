/* SPDX-License-Identifier: GPL-3.0-only */
/* chauto.c: the chords' AUTO page, song parts (ST-83, D-080, R-265..R-270), as the owner tried it in the Chord Lab
 * (v5, "I like the chords auto concept"). It replaces the chord pad on AUTO (D-034, R-075..R-080, R-082; retired by
 * the owner in #108). app.c includes it after chideas.c.
 *
 * The white keys start a part of the style's song: INTRO, VERSE, PRE (the pre-chorus), CHORUS, BRIDGE, BREAK, OUTRO, calm
 * on F3..E4 and with energy on F4..E5 (R-265). Stopped, a part's key starts the song with that part (R-267); playing,
 * another part's key starts it on the next bar (R-266). The black keys held move the part (R-268): F#3 / F#4 fill (busy,
 * the last beat in sixteenths), G#3 / G#4 build (busier and louder, up an octave after two seconds), A#3 / A#4 half time,
 * C#4 / C#5 octave up, D#4 / D#5 stop. Each part is a progression (or part of one) laid out in 2 or 4 bars (R-269), played
 * by one of the style's players (the lab's nine: pop piano, ballad, arp, Alberti, reggae, house, funk, neo soul, pads) at
 * the part's density. K1 STYLE, K2 DENSITY (sparse, normal, busy), K3 SWING (straight, light, medium, heavy), K4 FOLLOW
 * (off, or the chord of the key on the recorded bass or melody at that moment, R-270). What it plays is recorded while
 * REC is armed, as the pads were. */

typedef struct { uint8_t at, len; int8_t what; } au_ev_t;   /* a 16th of the bar, its length in 16ths, what plays */
enum { AW_END = -9, AW_C = -1, AW_R = -2, AW_B = -3 };      /* the chord, rolled, the root an octave down; 0..3 a tone */
/* generated from the Chord Lab's AUTO_STYLES (scratchpad st83/gen.py): two patterns a player, 16ths */
static const au_ev_t AU_PATS[9][2][17] = {
    /* Pop piano */ {
        { { 0, 4, AW_B }, { 0, 4, AW_C }, { 4, 2, AW_C }, { 6, 2, AW_C }, { 8, 4, AW_B }, { 8, 4, AW_C }, { 12, 2, AW_C }, { 14, 2, AW_C }, { 0, 0, AW_END } },
        { { 0, 6, AW_C }, { 6, 4, AW_C }, { 10, 6, AW_C }, { 0, 0, AW_END } },
    },
    /* Ballad */ {
        { { 0, 3, 0 }, { 2, 3, 1 }, { 4, 3, 2 }, { 6, 3, 1 }, { 8, 3, 3 }, { 10, 3, 2 }, { 12, 3, 1 }, { 14, 3, 2 }, { 0, 0, AW_END } },
        { { 0, 16, AW_B }, { 0, 4, 1 }, { 4, 4, 2 }, { 8, 4, 3 }, { 12, 4, 2 }, { 0, 0, AW_END } },
    },
    /* Arp */ {
        { { 0, 1, 0 }, { 1, 1, 1 }, { 2, 1, 2 }, { 3, 1, 3 }, { 4, 1, 0 }, { 5, 1, 1 }, { 6, 1, 2 }, { 7, 1, 3 }, { 8, 1, 0 }, { 9, 1, 1 }, { 10, 1, 2 }, { 11, 1, 3 }, { 12, 1, 0 }, { 13, 1, 1 }, { 14, 1, 2 }, { 15, 1, 3 }, { 0, 0, AW_END } },
        { { 0, 2, 0 }, { 2, 2, 1 }, { 4, 2, 2 }, { 6, 2, 3 }, { 8, 2, 2 }, { 10, 2, 1 }, { 12, 2, 0 }, { 14, 2, 1 }, { 0, 0, AW_END } },
    },
    /* Alberti */ {
        { { 0, 2, 0 }, { 2, 2, 2 }, { 4, 2, 1 }, { 6, 2, 2 }, { 8, 2, 0 }, { 10, 2, 2 }, { 12, 2, 1 }, { 14, 2, 2 }, { 0, 0, AW_END } },
        { { 0, 1, 0 }, { 1, 1, 2 }, { 2, 1, 1 }, { 3, 1, 2 }, { 4, 1, 0 }, { 5, 1, 2 }, { 6, 1, 1 }, { 7, 1, 2 }, { 8, 1, 0 }, { 9, 1, 2 }, { 10, 1, 1 }, { 11, 1, 2 }, { 12, 1, 0 }, { 13, 1, 2 }, { 14, 1, 1 }, { 15, 1, 2 }, { 0, 0, AW_END } },
    },
    /* Reggae */ {
        { { 4, 2, AW_C }, { 12, 2, AW_C }, { 0, 0, AW_END } },
        { { 4, 1, AW_C }, { 6, 1, AW_C }, { 12, 1, AW_C }, { 14, 1, AW_C }, { 0, 0, AW_END } },
    },
    /* House */ {
        { { 2, 1, AW_C }, { 6, 1, AW_C }, { 10, 1, AW_C }, { 14, 1, AW_C }, { 0, 0, AW_END } },
        { { 0, 1, AW_C }, { 3, 1, AW_C }, { 6, 1, AW_C }, { 10, 2, AW_C }, { 14, 1, AW_C }, { 0, 0, AW_END } },
    },
    /* Funk */ {
        { { 0, 1, AW_C }, { 3, 1, AW_C }, { 6, 1, AW_C }, { 10, 1, AW_C }, { 11, 1, AW_C }, { 14, 1, AW_C }, { 0, 0, AW_END } },
        { { 0, 2, AW_C }, { 4, 1, AW_C }, { 7, 1, AW_C }, { 8, 1, AW_C }, { 12, 1, AW_C }, { 15, 1, AW_C }, { 0, 0, AW_END } },
    },
    /* Neo soul */ {
        { { 0, 10, AW_R }, { 14, 2, AW_C }, { 0, 0, AW_END } },
        { { 0, 6, AW_R }, { 7, 7, AW_R }, { 0, 0, AW_END } },
    },
    /* Pads */ {
        { { 0, 16, AW_C }, { 0, 0, AW_END } },
        { { 0, 8, AW_C }, { 8, 8, AW_C }, { 0, 0, AW_END } },
    },
};
static const char *const AU_PLAYERS[9] = { "Pop piano", "Ballad", "Arp", "Alberti", "Reggae", "House", "Funk", "Neo soul", "Pads" };

enum { PL_POP = 0, PL_BALLAD, PL_ARP, PL_ALBERTI, PL_REGGAE, PL_HOUSE, PL_FUNK, PL_NEOSOUL, PL_PADS };
static const uint8_t AU_GENRE[CI_NSTYLES][3] = {   /* each style's players (the lab's GENRE_PLAYERS), by CI_STYLES */
    { PL_POP, PL_BALLAD, PL_ARP }, { PL_NEOSOUL, PL_PADS, PL_BALLAD }, { PL_NEOSOUL, PL_PADS, PL_ARP },
    { PL_PADS, PL_NEOSOUL, PL_ARP }, { PL_FUNK, PL_HOUSE, PL_POP }, { PL_HOUSE, PL_ARP, PL_PADS },
    { PL_BALLAD, PL_NEOSOUL, PL_ALBERTI }, { PL_REGGAE, PL_POP, PL_PADS }, { PL_HOUSE, PL_ARP, PL_POP },
    { PL_REGGAE, PL_HOUSE, PL_PADS }, { PL_REGGAE, PL_HOUSE, PL_ARP }, { PL_PADS, PL_ARP, PL_HOUSE },
    { PL_POP, PL_BALLAD, PL_PADS } };
enum { PT_INTRO = 0, PT_VERSE, PT_PRE, PT_CHORUS, PT_BRIDGE, PT_BREAK, PT_OUTRO, PT_N };
static const char *const PT_NAMES[PT_N] = { "INTRO", "VERSE", "PRE", "CHORUS", "BRIDGE", "BREAK", "OUTRO" };
static const char *const PT_SHORT[PT_N] = { "IN", "VE", "PR", "CH", "BR", "BK", "OU" };
/* each part (the lab's partsOf): which of the style's progressions, which player (0..2, or 3: pads), its density, and
 * take (only the first n chords), from (start at that chord), home (end on I) */
static const struct { uint8_t prog, pl, dens, take, from, home; } AU_PARTS[PT_N] = {
    { 0, 3, 0, 2, 0, 0 }, { 0, 0, 1, 0, 0, 0 }, { 1, 1, 1, 0, 0, 0 }, { 2, 0, 2, 0, 0, 0 },
    { 1, 2, 1, 0, 1, 0 }, { 0, 3, 0, 1, 0, 0 }, { 0, 1, 0, 0, 0, 1 } };
static const char *const AU_DENS[3] = { "sparse", "normal", "busy" };
static const char *const AU_SWING[4] = { "straight", "light", "medium", "heavy" };
static const uint8_t AU_SWING_PCT[4] = { 0, 22, 40, 60 };   /* the off-beat 16th later by this % of a 16th */
static const char *const AU_FOLLOW[3] = { "off", "bass", "melody" };
enum { MV_FILL = 1, MV_BUILD = 2, MV_HALF = 4, MV_OCT = 8, MV_STOP = 16 };
static const uint8_t AU_MOVE[KIT_NKEYS] = { 0, MV_FILL, 0, MV_BUILD, 0, MV_HALF, 0, 0, MV_OCT, 0, MV_STOP, 0,
                                            0, MV_FILL, 0, MV_BUILD, 0, MV_HALF, 0, 0, MV_OCT, 0, MV_STOP, 0, 0, 0, 0 };
static const char *const AU_MOVE_LAB[17] = { "", "fl", "bu", "", "1/2", "", "", "", "8va", "", "", "", "", "", "", "", "st" };

static int au_style[KIT_NTRACKS] = { 1, 1, 1, 1 }, au_dens[KIT_NTRACKS] = { 1, 1, 1, 1 }, au_swing[KIT_NTRACKS], au_follow[KIT_NTRACKS];
static int au_part = -1, au_hi, au_next = -1, au_next_hi, au_bar0;   /* the part playing (-1: none), from which bar */
static int au_prev = -1, au_last16 = -1;
static uint32_t au_abs, au_build_ms;
static struct { uint8_t note, vel, on; uint32_t at, off; } au_q[24];
static int au_nq;

static int au_part_key(int key, int *hi)           /* a white key F3..E5: its part (-1: none) */
{
    int k, w = 0;
    if (key < 0 || key >= 24 || !kit_is_white(KIT_KEY0_NOTE + key)) return -1;
    for (k = 0; k < key; k++) if (kit_is_white(KIT_KEY0_NOTE + k)) w++;
    *hi = w >= 7;
    return w % 7;
}

static int au_moves(void)
{
    int k, m = 0;
    for (k = 0; k < KIT_NKEYS; k++) if (AU_MOVE[k] && kit_key_held(k)) m |= AU_MOVE[k];
    return m;
}

/* a part's chords, one a bar, laid out in 2 or 4 bars (R-269): the progression's steps (-1: home, I) */
static int au_part_bars(int part, int *prog, int8_t *bars)
{
    int8_t st[8];
    int n = 0, i, k, np, nb = 0;
    static const uint8_t PLAN[4][4][2] = { { { 0, 4 } }, { { 0, 1 }, { 1, 1 }, { 0, 1 }, { 1, 1 } },
                                           { { 0, 1 }, { 1, 1 }, { 2, 2 } }, { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } };
    static const uint8_t PLAN_N[4] = { 1, 4, 3, 4 };
    *prog = -1;
    for (i = AU_PARTS[part].prog; i >= 0 && *prog < 0; i--) *prog = ci_prog(au_style[track], i * 2);   /* the style may have fewer */
    if (*prog < 0) return 0;
    np = PH_PROGS[*prog].n;
    for (i = 0; i < np; i++) st[n++] = (int8_t)((i + AU_PARTS[part].from) % np);
    if (AU_PARTS[part].take && AU_PARTS[part].take < n) n = AU_PARTS[part].take;
    if (AU_PARTS[part].home) {                  /* the outro ends home on I: at most 3 of its chords before it */
        if (n > 3) n = 3;
        st[n++] = -1;
    }
    if (n > 4) n = 4;
    for (i = 0; i < PLAN_N[n - 1]; i++)
        for (k = 0; k < PLAN[n - 1][i][1]; k++) bars[nb++] = st[PLAN[n - 1][i][0]];
    return nb;
}

/* the part's chord in bar b of it, voiced on the keys; FOLLOW: the key's chord on the recorded bass or melody (R-270) */
static int au_chord(int b, uint8_t *out, int *deg)
{
    int8_t bars[8];
    int prog, nb = au_part_bars(au_part, &prog, bars), pcs[8], n;
    ph_chord_t c;
    if (au_follow[track]) {
        int keep = play_prog[track];
        play_prog[track] = au_follow[track] == 1 ? PG_FBASS : PG_FMEL;
        ph_prog_chord(0, &c);
        play_prog[track] = keep;
    } else if (!nb) {
        return 0;
    } else if (bars[pmod(b, nb)] < 0) {
        c.deg = 0; c.col = CC_NONE; c.root = 0; c.iv = 0;
    } else {
        ci_step_chord(prog, bars[pmod(b, nb)], &c);
    }
    n = ph_chord_pcs(&c, pcs, deg);
    return ph_voice(pcs, n, KIT_VOICE_ROOT, out);
}

static void au_release(void)
{
    int i;
    for (i = 0; i < au_nq; i++) if (au_q[i].on) { kit_seq_note_off(track, au_q[i].note); kit_midi_note_off(channel_of(track), au_q[i].note); }
    au_nq = 0;
}

static void au_push(int note, int vel, uint32_t at, uint32_t off)
{
    if (au_nq >= 24 || note < 0 || note > 127) return;
    vel = vel < 1 ? 1 : vel > 127 ? 127 : vel;
    au_q[au_nq].note = (uint8_t)note; au_q[au_nq].vel = (uint8_t)vel; au_q[au_nq].on = 0;
    au_q[au_nq].at = at; au_q[au_nq].off = off;
    au_nq++;
}

/* one 16th of the part (the lab's autoTick): the player's pattern at its density, with the moves held */
static void au_step(int s16, int bar, uint32_t now)
{
    const au_ev_t *pat[2];
    uint8_t v[KIT_CHORD_MAX];
    int deg, n, i, j, k, np = 1, mv = au_moves(), dens, pl, sixteenth = (int)kit_beat_ms(song.bpm) / 4, gain, shift = 0;
    if (s16 == 0 && au_next >= 0) { au_part = au_next; au_hi = au_next_hi; au_next = -1; au_bar0 = bar; dirty = 1; }
    if (au_part < 0 || (mv & MV_STOP)) return;
    if (s16 == 0) dirty = 1;
    pl = AU_PARTS[au_part].pl == 3 ? PL_PADS : AU_GENRE[au_style[track]][AU_PARTS[au_part].pl];
    if (au_hi && pl == PL_PADS) pl = AU_GENRE[au_style[track]][0];   /* energy: the style's own player, not pads */
    dens = AU_PARTS[au_part].dens + au_hi + au_dens[track] - 1;
    dens = dens < 0 ? 0 : dens > 2 ? 2 : dens;
    if (mv & (MV_FILL | MV_BUILD)) dens = 2;
    pat[0] = AU_PATS[pl][0];
    if (dens == 2) { pat[1] = AU_PATS[pl][1]; np = 2; }
    n = au_chord(bar - au_bar0, v, &deg);
    if (!n) return;
    gain = au_hi ? 100 : 72;
    if (mv & MV_BUILD) {
        uint32_t t = now - au_build_ms;
        gain = gain * (100 + (int)(t > 4000 ? 40 : t / 100)) / 100;
        if (t > 2000) shift = 12;
    }
    if (mv & MV_OCT) shift = 12;
    if (!au_hi && AU_PARTS[au_part].dens == 0) shift -= 12;
    {
        int sw = AU_SWING_PCT[au_swing[track]] * sixteenth / 100;
        uint32_t at0 = now + (uint32_t)(s16 % 4 == 2 ? sw : s16 % 2 ? sw / 2 : 0);
        int any_on_beat = 0;
        for (k = 0; k < np; k++) for (j = 0; pat[k][j].what != AW_END; j++) if (pat[k][j].at % 8 == 0) any_on_beat = 1;
        for (k = 0; k < np; k++)
            for (j = 0; pat[k][j].what != AW_END; j++) {
                const au_ev_t *e = &pat[k][j];
                int len = e->len, at = e->at;
                if (dens == 0 && (any_on_beat ? at % 8 != 0 : j > 0)) continue;   /* sparse: the strong beats only */
                if (mv & MV_HALF) { if (at % 8) continue; len *= 2; }                /* half time */
                if (at != s16) continue;
                {
                    uint32_t off = at0 + (uint32_t)(sixteenth * len * 92 / 100);
                    if (e->what == AW_B) au_push(v[0] + shift - 12, 70 * gain / 100, at0, off);
                    else if (e->what >= 0) au_push(v[e->what < n ? e->what : n - 1] + shift, (60 + feel_rand(&ph_rng, 9)) * gain / 100, at0 + (uint32_t)feel_rand(&ph_rng, 7), off);
                    else
                        for (i = 0; i < n; i++) {
                            int dl, vp;
                            feel_note(e->what == AW_R ? FEEL_ROLLED : FEEL_PIANIST, i, n, sixteenth, &ph_rng, &dl, &vp);
                            au_push(v[i] + shift, vp * 85 / 100 * gain / 100, at0 + (uint32_t)dl, off);
                        }
                }
            }
        if ((mv & MV_FILL) && s16 >= 12)       /* the fill: the last beat in 16ths */
            for (i = 0; i < n; i++) au_push(v[i] + shift, 80 * gain / 100, at0, at0 + (uint32_t)(sixteenth * 92 / 100));
    }
}

/* each frame: the song's 16ths as they pass (recorded while REC is armed), and the queued notes on time */
static void au_poll(uint32_t now)
{
    int i, j, tick, len = song.bars * KIT_TICKS_PER_BAR;
    if (!is_chords() || pages[track] != PAGE_AUTO || !kit_seq_playing()) {
        if (au_prev >= 0 || au_nq) { au_release(); au_prev = -1; au_last16 = -1; au_abs = 0; }
        return;
    }
    tick = kit_seq_tick();
    if (au_prev < 0) au_abs = (uint32_t)tick;
    else au_abs += (uint32_t)(tick >= au_prev ? tick - au_prev : tick + len - au_prev);
    au_prev = tick;
    if ((int)(au_abs / 6) != au_last16) {
        au_last16 = (int)(au_abs / 6);
        au_step(au_last16 % 16, au_last16 / 16, now);
    }
    for (i = 0; i < au_nq; ) {
        if (!au_q[i].on && (int32_t)(now - au_q[i].at) >= 0) {
            au_q[i].on = 1;
            kit_seq_note_on(track, au_q[i].note, au_q[i].vel);
            kit_midi_note_on(channel_of(track), au_q[i].note, au_q[i].vel);
        }
        if (au_q[i].on && (int32_t)(now - au_q[i].off) >= 0) {
            kit_seq_note_off(track, au_q[i].note);
            kit_midi_note_off(channel_of(track), au_q[i].note);
            for (j = i + 1; j < au_nq; j++) au_q[j - 1] = au_q[j];
            au_nq--;
        } else {
            i++;
        }
    }
}

static void au_key_down(int key)
{
    int hi, p = au_part_key(key, &hi);
    if (AU_MOVE[key] & MV_BUILD) au_build_ms = kit_ms();
    if (p >= 0) {
        if (kit_seq_playing() && au_part >= 0) { au_next = p; au_next_hi = hi; }   /* on the next bar (R-266) */
        else {                                  /* on its own: the part starts the song (R-267) */
            au_part = p; au_hi = hi; au_next = -1; au_bar0 = 0;
            if (!kit_seq_playing()) kit_seq_play(&song);
            else au_bar0 = (int)(au_abs / KIT_TICKS_PER_BAR) + 1;
        }
    }
    dirty = 1;
}

static void ch_turned(int id);
static void au_knob(int id, int d)
{
    ch_turned(id);
    if (id == KIT_ENC_KNOB1) clampi(&au_style[track], d, 0, CI_NSTYLES - 1);
    if (id == KIT_ENC_KNOB2) clampi(&au_dens[track], d, 0, 2);
    if (id == KIT_ENC_KNOB3) clampi(&au_swing[track], d, 0, 3);
    if (id == KIT_ENC_KNOB4) clampi(&au_follow[track], d, 0, 2);
}

/* the lights: the part playing full, the next one half, the other parts and the moves dim, what is held full */
static void au_lights(void)
{
    int k, hi;
    for (k = 0; k < KIT_NKEYS; k++) {
        int p = au_part_key(k, &hi), lv = KIT_LIGHT_OFF;
        if (p >= 0) lv = p == au_part && hi == au_hi ? KIT_LIGHT_ON : p == au_next && hi == au_next_hi ? KIT_LIGHT_HALF : KIT_LIGHT_DIM;
        if (AU_MOVE[k]) lv = KIT_LIGHT_DIM;
        if (kit_key_held(k)) lv = KIT_LIGHT_ON;
        kit_key_light(k, lv);
    }
    kit_btn_light(KIT_BTN_ARP, KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_EDIT, KIT_LIGHT_OFF);
}

/* the keys drawn with what they do: the parts (calm low, energy high) and the moves (R-265) */
static void au_draw_keys(int y)
{
    int k, wi = 0, hi;
    for (k = 0; k < KIT_NKEYS; k++) {
        int p, on;
        if (!kit_is_white(KIT_KEY0_NOTE + k)) continue;
        p = au_part_key(k, &hi);
        on = p >= 0 && p == au_part && hi == au_hi;
        kit_round_rect(wi * 15 + 1, y + 26, 13, 54, 3, on ? ACC : p >= 0 && p == au_next && hi == au_next_hi ? ACC_D : p >= 0 ? (hi ? CARD : CARD2) : BG);
        if (p >= 0) {
            ctext(wi * 15 + 8, y + 44, PT_SHORT[p], KIT_FONT_TINY, on ? BG : FG);
            ctext(wi * 15 + 8, y + 66, hi ? "+" : "", KIT_FONT_TINY, on ? BG : ACC);
        }
        wi++;
    }
    wi = 0;
    for (k = 0; k < KIT_NKEYS; k++) {
        if (kit_is_white(KIT_KEY0_NOTE + k)) { wi++; continue; }
        if (!AU_MOVE[k]) continue;
        kit_round_rect(wi * 15 - 7, y, 14, 22, 3, kit_key_held(k) ? ACC : CARD2);
        ctext(wi * 15, y + 11, AU_MOVE_LAB[AU_MOVE[k]], KIT_FONT_TINY, kit_key_held(k) ? BG : MUTED);
    }
}

static void ch_draw_auto(void);
static void au_draw(void) { ch_draw_auto(); }   /* the lab's AUTO page (chdraw.c, D-084) */
