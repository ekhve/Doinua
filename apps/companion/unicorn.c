/* SPDX-License-Identifier: GPL-3.0-only */
/* Unicorn Jump, the Game mode's first game (ST-51, D-059), included by app.c before app_frame. For a child of four:
 * a winged unicorn runs to the right through a night meadow; fences come, each with a note of a song. It waits for
 * her: it stops in front of a fence until she plays, and a key played early makes it dash there, so the rhythm is
 * hers. K1 the mode: Easy (plain fences, any key jumps, the fence's note sings, so the song still plays) or Key (the
 * note's letter over the fence, its key lit: play it). In Key mode a wrong key is a bump with stars, and it
 * waits on; two in a row: it poops ("pfrrt ... plop"), and the poop comes round again, in a fence's place, to hop (any
 * key). At the song's end (in Key, once eight good jumps in a row have earned the wings) it flies away to the moon,
 * smaller and smaller, round it and back, a rainbow behind it (only then); the next round waits for it to land. Left waiting, it neighs at
 * 8 s and falls asleep at 20 s, until a key. A soft pad and a bass follow the song's notes, with a reverb. The song done: YAY, and again.
 * Nothing ends. K2 how fast it runs, K3 the song. As the browser lab, changed as the owner asked after trying it
 * ("too hard for small kids ... it waits for me"; "the unicorn should be bigger ... it ends up on top of them ...
 * fly to the moon and come back, leaving a rainbow trail"; "the flying bonus is too easy for easy mode ... middle of
 * the song?"; "in easy mode we dont need anything ... just the note in a nice way"; "the horse... sleeps?"). */

#define U_SC 3                                  /* fences and poops drawn 3x: a fence 42 x 48 */
#define U_USC 2                                 /* the unicorn drawn 2x, from 44 x 42: 88 x 84 */
#define U_GROUND 196                            /* the meadow's top */
#define U_X 96                                  /* the unicorn's front, on screen */
#define U_W (44 * U_USC)                        /* the unicorn's width */
#define U_GY (U_GROUND - 80 + 2)                /* its top while on the ground (its hooves, outlined, on the grass) */
#define U_LEGS 44                               /* from its front to the middle of its legs */
#define U_FW (14 * U_SC)                        /* a fence's width */
#define U_PW (10 * U_SC)                        /* a poop's width */
#define U_SPACING 190                           /* between fences, px: room to land between two */
#define U_HOP 84                                /* how high it jumps, px */
#define U_FLYN 5                                /* a flight lasts five fences: four play themselves below */
#define U_NT 24                                 /* the rainbow's points: a tail of 1.2 s, gone behind it */
#define U_NEAR 130                              /* a fence this near: its key counts */
#define U_NF 6
#define U_NP 4
#define U_NS 8
static const char *const U_NAMES[7] = { "C", "D", "E", "F", "G", "A", "B" };
static const uint8_t U_WHITE[12] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };   /* a note's letter (sharps as their natural) */
static const kit_color_t U_COL[12] = {          /* a colour a note, as children's bells have them */
    KIT_RGB(255, 77, 77), KIT_RGB(255, 122, 89), KIT_RGB(255, 159, 28), KIT_RGB(255, 191, 71), KIT_RGB(255, 225, 77),
    KIT_RGB(94, 211, 107), KIT_RGB(56, 201, 164), KIT_RGB(46, 196, 240), KIT_RGB(77, 140, 255), KIT_RGB(138, 107, 255),
    KIT_RGB(194, 107, 255), KIT_RGB(255, 107, 198) };
/* the songs: traditional, free, and the unicorn's own; one fence a note */
static const uint8_t U_TWINKLE[] = { 60, 60, 67, 67, 69, 69, 67, 65, 65, 64, 64, 62, 62, 60, 67, 67, 65, 65, 64, 64, 62,
                                     67, 67, 65, 65, 64, 64, 62, 60, 60, 67, 67, 69, 69, 67, 65, 65, 64, 64, 62, 62, 60 };
static const uint8_t U_MARY[] = { 64, 62, 60, 62, 64, 64, 64, 62, 62, 62, 64, 67, 67, 64, 62, 60, 62, 64, 64, 64, 64, 62, 62, 64, 62, 60 };
static const uint8_t U_JACQUES[] = { 60, 62, 64, 60, 60, 62, 64, 60, 64, 65, 67, 64, 65, 67, 67, 69, 67, 65, 64, 60,
                                     67, 69, 67, 65, 64, 60, 60, 55, 60, 60, 55, 60 };
static const uint8_t U_ROW[] = { 60, 60, 60, 62, 64, 64, 62, 64, 65, 67, 72, 72, 72, 67, 67, 67, 64, 64, 64, 60, 60, 60, 67, 65, 64, 62, 60 };
static const uint8_t U_ODE[] = { 64, 64, 65, 67, 67, 65, 64, 62, 60, 60, 62, 64, 64, 62, 62, 64, 64, 65, 67, 67, 65, 64, 62, 60, 60, 62, 64, 62, 60, 60 };
static const uint8_t U_BDAY[] = { 67, 67, 69, 67, 72, 71, 67, 67, 69, 67, 74, 72, 67, 67, 79, 76, 72, 71, 69, 77, 77, 76, 72, 74, 72 };
/* the unicorn's own song, written for the game (D-059): a skip up, a gallop round, and home */
static const uint8_t U_UNICORN[] = { 67, 64, 67, 72, 71, 67, 69, 67, 65, 69, 67, 64, 62, 64, 60,
                                     67, 64, 67, 72, 74, 72, 71, 72, 69, 67, 65, 64, 62, 67, 60 };
typedef struct { const char *name; const uint8_t *n; uint8_t len; } u_song_t;
static const u_song_t U_SONGS[] = {
    { "TWINKLE TWINKLE", U_TWINKLE, sizeof U_TWINKLE }, { "MARY'S LAMB", U_MARY, sizeof U_MARY },
    { "FRERE JACQUES", U_JACQUES, sizeof U_JACQUES }, { "ROW YOUR BOAT", U_ROW, sizeof U_ROW },
    { "ODE TO JOY", U_ODE, sizeof U_ODE }, { "HAPPY BIRTHDAY", U_BDAY, sizeof U_BDAY },
    { "UNICORN SONG", U_UNICORN, sizeof U_UNICORN } };
#define U_NSONGS 7
static const uint8_t U_FPM[3] = { 42, 56, 72 };  /* fences a minute: slow, medium, fast */

enum { UF_AHEAD = 0, UF_OK, UF_CLEARED, UF_DOWN };   /* OK: played, the unicorn dashes to it and jumps it */
enum { UM_EASY = 0, UM_KEY, UM_N };            /* Easy: plain fences, any key; Key: the note's letter, its key lit */
typedef struct { int32_t wx; uint8_t note, state, used, si; } u_fence_t;   /* si: its place in the song */
typedef struct { int32_t wx; uint8_t used, back, ahead, hop; } u_poop_t;
static struct {
    int on, running;
    uint32_t last, t;                           /* ms */
    int32_t x;                                  /* how far the meadow has moved, px x 256 */
    u_fence_t f[U_NF];
    u_poop_t p[U_NP];
    int idx, misses, poops, score, streak, fly, wings, at_moon; int32_t wing_msg;   /* wing_msg: WINGS! showing, ms */
    int32_t jump, jump_len, jump_v, bonk, cheer;   /* ms left (jump: done so far; jump_v: its speed, kept to the end) */
    int32_t fly_x0;                             /* where the flight began (U.x) */
    int16_t tr_x[U_NT], tr_y[U_NT]; uint8_t tr_w[U_NT]; int ntr;    /* the rainbow: where the unicorn's tail was, in the sky */
    uint32_t plop_at;                           /* a poop on its way (the PFFT now, the PLOP then) */
    int chord, chord_n[4];                      /* the pad's chord (0 none), its notes */
    int saved_rv, lastn;
    int tonic, minor;                           /* the song's key, for the pad */
    int teach, tn, dance, learned; uint8_t tbuf[MY_SONG_MAX];   /* teaching a MY SONG: the notes so far; the dance step's time left */
    int fl_n[8], fl_x[8], fl_y[8];              /* the notes floating up to the unicorn as it learns */                        /* the reverb before the game; the last fence's note */
    int mode, speed, tune, quiet;               /* K1, K2, K3, K4 (the music: the pad, the bass, the reverb; on at first) */
    int listen, ln; uint32_t lu;                /* PLAY: the song run by itself; the key lit for the note sounding, until */
    int wait;                                   /* stopped in front of a fence or a poop, waiting for a key */
    int32_t idle; int sleep;                    /* how long it has waited with no key; asleep */
    int sp_x[U_NS], sp_y[U_NS], sp_vx[U_NS], sp_vy[U_NS], sp_life[U_NS], sp_c[U_NS];
    uint32_t arp_at; int arp_n;                 /* the cheer's little tune */
    int saved_e[KIT_NTRACKS], saved_p[KIT_NTRACKS];
    uint32_t q;                                 /* the game's clock: whole steps of U_STEP ms */
    uint8_t off_n[4]; uint32_t off_at[4];       /* bell notes to let go */
    uint8_t pq_n[8]; uint32_t pq_q[8]; int npq; /* keys played, each taken at the step it came in */
} U;
#define U_STEP 50                               /* the game moves in steps of 50 ms (20 a second), the same on every FM-1 */

static int32_t u_sx(int32_t wx) { return wx - (U.x >> 8); }             /* world to screen */
static int32_t u_speed(void) { return U_SPACING * 256 * U_FPM[U.speed] / 60; }   /* px x 256 a second: running */
#define U_STOP 18                               /* it stops this far before a fence or a poop it has not been told to jump */

/* the songs: the seven built in, then MY SONG 1..4, taught (#84, D-061) and kept in the settings record */
#define U_NALL (U_NSONGS + MY_SONGS)
static int u_my(void) { return U.tune >= U_NSONGS ? U.tune - U_NSONGS : -1; }   /* which MY SONG, or -1 */
static int u_len(void) { return u_my() >= 0 ? set.my_len[u_my()] : U_SONGS[U.tune].len; }
static int u_note(int k) { return u_my() >= 0 ? set.my_song[u_my()][k] : U_SONGS[U.tune].n[k]; }
static const char *u_name(void)
{
    static const char *const MY[MY_SONGS] = { "MY SONG 1", "MY SONG 2", "MY SONG 3", "MY SONG 4" };
    return u_my() >= 0 ? MY[u_my()] : U_SONGS[U.tune].name;
}
static void u_add(int32_t wx)
{
    int i;
    if (!u_len()) return;                       /* a MY SONG not taught yet: no fences */
    if (U.idx >= u_len()) { U.idx = 0; U.cheer = 2500; U.arp_at = U.t; U.arp_n = 0; }   /* the song done: YAY, and again */
    for (i = 0; i < U_NF; i++)
        if (!U.f[i].used) { U.f[i].used = 1; U.f[i].wx = wx; U.f[i].si = (uint8_t)U.idx; U.f[i].note = (uint8_t)u_note(U.idx++); U.f[i].state = UF_AHEAD; return; }
}
static u_fence_t *u_next(void)                  /* the fence to clear next: the nearest one ahead */
{
    u_fence_t *b = 0;
    int i;
    for (i = 0; i < U_NF; i++) if (U.f[i].used && U.f[i].state == UF_AHEAD && (!b || U.f[i].wx < b->wx)) b = &U.f[i];
    return b;
}
static int32_t u_far(void)                      /* the furthest fence, or poop come round, ahead */
{
    int i;
    int32_t m = -1;
    for (i = 0; i < U_NF; i++) if (U.f[i].used && U.f[i].wx > m) m = U.f[i].wx;
    for (i = 0; i < U_NP; i++) if (U.p[i].used && U.p[i].ahead && U.p[i].wx > m) m = U.p[i].wx;
    return m;
}

static void u_reset(void)
{
    int i;
    for (i = 0; i < U_NF; i++) U.f[i].used = 0;
    for (i = 0; i < U_NP; i++) U.p[i].used = 0;
    for (i = 0; i < U_NS; i++) U.sp_life[i] = 0;
    U.idx = 0; U.x = 0; U.jump = U.jump_len = 0; U.bonk = 0; U.misses = 0; U.streak = 0; U.fly = 0; U.wings = 0; U.wing_msg = 0; U.cheer = 0;
    U.arp_n = -1; U.wait = 0; U.ntr = 0; U.plop_at = 0; U.at_moon = 0; U.idle = 0; U.sleep = 0;
    for (i = 0; i < 4; i++) u_add(320 + i * U_SPACING);
}

/* sounds: the notes a bell on the melody track, the knocks the drum kit's */
#define U_BELL KIT_TRACK_MELODY
#define U_DRUM KIT_TRACK_DRUMS
static void u_bell(int n)                      /* a bell, held 300 ms */
{
    int i, k = 0;
    for (i = 0; i < 4; i++) if (U.off_at[i] && U.off_n[i] == n) { kit_track_note_off(U_BELL, n); U.off_at[i] = 0; }
    kit_track_note_on(U_BELL, n, 110);
    for (i = 0; i < 4; i++) if (!U.off_at[i]) { k = i; break; } else if (U.off_at[i] < U.off_at[k]) k = i;
    if (U.off_at[k]) kit_track_note_off(U_BELL, U.off_n[k]);
    U.off_n[k] = (uint8_t)n; U.off_at[k] = kit_ms() + 300; if (!U.off_at[k]) U.off_at[k] = 1;
}
static void u_hit(int pad, int vel) { kit_track_note_on(U_DRUM, KIT_DRUM_LO + pad, vel); kit_track_note_off(U_DRUM, KIT_DRUM_LO + pad); }

/* the pad and the bass: the song's I, IV or V (i, iv, v in a minor key), the one with the fence's note (kept while the
 * note is in it), voiced close between B2 and A#3 (in C: C3 E3 G3, C3 F3 A3, B2 D3 G3), the root an octave and a half
 * below. The built-in songs are in C; a taught one's key is worked out from its notes (kit_keys_for_notes). */
#ifndef U_PADV
#define U_PADV 24                               /* the pad and the bass, soft under the bell */
#define U_BASSV 12
#endif
static const uint8_t U_ROOT[3] = { 0, 5, 7 };   /* I, IV, V above the tonic */
static const uint8_t U_CHORD_OF[2][12] = {       /* a pitch class above the tonic: its chord, 1 I, 2 IV, 3 V */
    { 1, 1, 3, 3, 1, 2, 2, 1, 1, 2, 2, 3 }, { 1, 1, 3, 1, 1, 2, 2, 1, 2, 2, 3, 3 } };
static void u_chord_notes(int c, int *n)        /* chord c (1..3) in the song's key: three notes and the bass */
{
    static const uint8_t TONES[2][3] = { { 0, 4, 7 }, { 0, 3, 7 } };
    int i, root = (U.tonic + U_ROOT[c - 1]) % 12;
    for (i = 0; i < 3; i++) { n[i] = 48 + (root + TONES[U.minor][i]) % 12; if (n[i] > 58) n[i] -= 12; }
    n[3] = 36 + root; if (n[3] > 47) n[3] -= 12;
}
static void u_key(void)                         /* the song's key: C for the built-in songs; a taught one's, from its notes */
{
    U.tonic = 0; U.minor = 0;
    if (u_my() >= 0 && u_len() > 0) {
        static kit_note_ev_t ev[MY_SONG_MAX];
        kit_key_cand_t c[1];
        int k, amb = 0;
        uint16_t heard = 0;
        for (k = 0; k < u_len(); k++) { ev[k].note = (uint8_t)u_note(k); ev[k].len = 1; }
        if (kit_keys_for_notes(ev, u_len(), c, 1, &amb, &heard) > 0) { U.tonic = c[0].tonic; U.minor = c[0].minor ? 1 : 0; }
    }
}
static void u_pad_off(void)
{
    int i;
    if (U.chord) { for (i = 0; i < 3; i++) kit_track_note_off(KIT_TRACK_CHORDS, U.chord_n[i]); kit_track_note_off(KIT_TRACK_BASS, U.chord_n[3]); }
    U.chord = 0;
}
static void u_pad(int note)
{
    int c = U_CHORD_OF[U.minor][(note - U.tonic + 120) % 12], i, n[4];
    if (U.quiet) return;
    if (U.chord) { u_chord_notes(U.chord, n); for (i = 0; i < 3; i++) if (n[i] % 12 == note % 12) c = U.chord; }   /* the note is in it: keep it */
    if (c == U.chord) return;
    u_pad_off();
    U.chord = c;
    u_chord_notes(c, n);
    for (i = 0; i < 4; i++) U.chord_n[i] = n[i];
    for (i = 0; i < 3; i++) kit_track_note_on(KIT_TRACK_CHORDS, U.chord_n[i], U_PADV);
    kit_track_note_on(KIT_TRACK_BASS, U.chord_n[3], U_BASSV);
}

static void u_enter(void)                       /* the game opens: a bell and Dusty; what the tracks were is kept */
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++) { U.saved_e[t] = kit_track_engine(t); U.saved_p[t] = kit_track_preset(t); }
    kit_seq_stop();
    kit_track_sound(U_BELL, 2, 4);              /* Phase: BELL */
    kit_track_sound(U_DRUM, KIT_ENGINE_DRUMS, 0);
    kit_track_sound(KIT_TRACK_CHORDS, 1, 1);    /* Analog: SOFT PAD */
    kit_track_sound(KIT_TRACK_BASS, 1, 7);      /* Analog: SUB BASS */
    U.saved_rv = kit_reverb_of(); kit_reverb(U.quiet ? 0 : 40);
    U.chord = 0;
    U.on = 1; U.running = 0; U.q = kit_ms() / U_STEP; U.t = U.q * U_STEP;
    u_reset(); u_key();
}
static void u_leave(void)
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++) kit_track_sound(t, U.saved_e[t], U.saved_p[t]);
    kit_reverb(U.saved_rv);
    U.chord = 0;
    U.teach = 0;                                /* left while teaching: not kept */
    kit_lights_off();
    U.on = 0;
}

static void u_spark(int c, int x, int y)
{
    int i, k = 0;
    for (i = 0; i < U_NS && k < 6; i++)
        if (U.sp_life[i] <= 0) {
            U.sp_x[i] = x << 8; U.sp_y[i] = y << 8;
            U.sp_vx[i] = (int)(((k * 37) % 60) - 10) << 8; U.sp_vy[i] = -((20 + (k * 23) % 60) << 8);
            U.sp_life[i] = 600; U.sp_c[i] = c; k++;
        }
}
/* a jump over something w wide that starts d ahead: at its height when its legs are over the middle, the same speed
 * all the way (a dash), so it comes down well past it */
static void u_jump(int32_t d, int32_t w)
{
    int32_t len = 2 * (U_LEGS + d + w / 2);     /* px */
    U.jump_v = u_speed() * 2;
    U.jump = 0; U.jump_len = len * 256 * 1000 / U.jump_v;
}
static void u_bump(void)                        /* a wrong key: a bump with stars; it waits on; two in a row, a poop */
{
    U.bonk = 600; U.streak = 0;
    u_hit(19, 110);                             /* the low tom */
    if (++U.misses >= 2) {                      /* twice: it poops, and the poop stays on the meadow */
        U.misses = 0; U.poops++;
        kit_sfx(KIT_SFX_PFFT, 9);               /* pfrrt ... */
        U.plop_at = U.t + 450;                  /* ... plop, and there it is */
    }
}
static void u_plop(void)
{
    int i;
    kit_sfx(KIT_SFX_PLOP, 9);
    for (i = 0; i < U_NP; i++)
        if (!U.p[i].used) { U.p[i].used = 1; U.p[i].wx = (U.x >> 8) + U_X - U_W - 6; U.p[i].back = 0; U.p[i].ahead = 0; U.p[i].hop = 0; break; }
}
static u_poop_t *u_poop_ahead(void)             /* the nearest poop ahead it has not been told to hop */
{
    u_poop_t *b = 0;
    int i;
    for (i = 0; i < U_NP; i++)
        if (U.p[i].used && U.p[i].ahead && !U.p[i].hop && u_sx(U.p[i].wx) > U_X - 4 && (!b || U.p[i].wx < b->wx)) b = &U.p[i];
    return b;
}
static void u_ok(u_fence_t *f)                  /* a fence played: the note sings, a star, it dashes there and jumps */
{
    f->state = UF_OK;
    u_bell(f->note);
    U.lastn = f->note; u_pad(f->note);
    U.misses = 0; U.score++;
    u_spark(f->note % 12, U_X - 20, U_GROUND - 70);
    if (!U.fly && ++U.streak == 8 && U.mode != UM_EASY && !U.wings) {   /* Key: eight good jumps in a row earn the wings */
        U.wings = 1; U.wing_msg = 1500;         /* (it flies when the song ends: never in the middle of a phrase) */
        u_spark(5, U_X - 40, U_GROUND - 60);
    }
}

/* teaching (#84, D-061): REC on a MY SONG; the unicorn stands and dances a step to each note, which floats up to it;
 * K1 back takes back the last note; SAVE (or REC) keeps the song; holding HOME leaves without keeping it */
#define U_UNDO 0                                /* (in the key queue: K1 turned back) */
static void u_teach_start(void)
{
    int i;
    U.teach = 1; U.tn = 0; U.dance = 0; U.running = 0; U.fly = 0; U.wings = 0;
    u_pad_off();
    for (i = 0; i < 8; i++) U.fl_n[i] = -1;
}
static void u_teach_note(int n)
{
    int i;
    if (n == U_UNDO) { if (U.tn > 0) U.tn--; return; }
    while (n < KIT_DRUM_LO) n += 12;            /* into the keys, F3 .. G5, by octaves */
    while (n > KIT_DRUM_LO + KIT_NKEYS - 1) n -= 12;
    u_bell(n);
    if (U.tn < MY_SONG_MAX) U.tbuf[U.tn++] = (uint8_t)n;
    U.dance = 300;                              /* a dance step */
    for (i = 0; i < 8; i++)                     /* the note floats up from its key to the unicorn */
        if (U.fl_n[i] < 0) { U.fl_n[i] = n; U.fl_x[i] = 4 + (n - KIT_DRUM_LO) * 232 / (KIT_NKEYS - 1); U.fl_y[i] = 0; break; }
}
static void u_teach_save(void)
{
    int k, m = u_my();
    U.teach = 0;
    if (U.tn > 0 && m >= 0) {                   /* kept: in the settings record, over power-off */
        for (k = 0; k < U.tn; k++) set.my_song[m][k] = U.tbuf[k];
        set.my_len[m] = (uint8_t)U.tn;
        settings_save();
        u_reset(); u_key();
        U.cheer = 2500; U.arp_at = U.t; U.arp_n = 0; U.learned = 1;   /* LEARNED!, and the cheer's tune */
        return;
    }
    u_reset(); u_key();
}

static void u_press_now(int note)
{
    U.idle = 0; U.sleep = 0;                    /* a key: awake */
    u_fence_t *f;
    u_poop_t *p;
    if (U.teach) { u_teach_note(note); return; }
    if (U.listen) return;                       /* PLAY runs the song: the keys wait */
    if (!U.running && !u_len()) { if (note != U_UNDO) u_bell(note); return; }   /* a MY SONG not taught: the key just sings */
    if (note == U_UNDO) return;
    if (!U.running) { U.running = 1; U.learned = 0; u_reset(); U.q = kit_ms() / U_STEP; u_bell(note); U.lastn = 60; u_pad(60); return; }   /* any key starts it, on a step */
    if (U.fly) { u_bell(note); return; }        /* flying: the keys just sing */
    f = u_next();
    p = u_poop_ahead();
    if (p && (!f || p->wx < f->wx) && u_sx(p->wx) < KIT_W) { p->hop = 1; u_bell(note); return; }   /* a poop first: any key hops it */
    if (!f || u_sx(f->wx) >= KIT_W) { u_bell(note); return; }   /* no fence on the screen: the key just sings */
    if (U.bonk > 0) return;
    if (U.mode == UM_EASY || note == f->note || (U.fly && note % 12 == f->note % 12)) u_ok(f);   /* Easy: any key */
    else u_bump();
}

static void u_press(int note)                   /* a key: taken at its step (the same on every FM-1) */
{
    if (!U.running && !U.teach) { u_press_now(note); return; }
    if (U.npq < 8) { U.pq_n[U.npq] = (uint8_t)note; U.pq_q[U.npq] = kit_ms() / U_STEP; U.npq++; }
}

static int32_t u_fly_p(void)                    /* how far through the flight, 0..1024 */
{
    int32_t p = ((U.x - U.fly_x0) >> 8) * 1024 / (U_FLYN * U_SPACING);
    return p > 1024 ? 1024 : p;
}
static int u_ease(int32_t q) { return (int)(q * q / 1024 * (3072 - 2 * q) / 2048); }   /* 0..1024, slow at both ends */
static int u_rise(int32_t q) { return (int)(q * (2048 - q) / 1024); }   /* 0..1024, fast at first: up off the fence */
/* the flight (the owner: "it becomes smaller and smaller as it is farther away, goes around the moon, and comes
 * back"): off from the meadow, smaller as it flies away to the moon; once round the moon, behind it at the top and in
 * front at the bottom; then back, bigger as it comes, and down again. Its middle, how wide it is, which way it faces. */
static const int16_t U_SIN[64] = { 0, 100, 200, 297, 392, 483, 569, 650, 724, 792, 851, 903, 946, 980, 1004, 1019, 1024, 1019, 1004, 980, 946, 903, 851, 792, 724, 650, 569, 483, 392, 297, 200, 100, 0, -100, -200, -297, -392, -483, -569, -650, -724, -792, -851, -903, -946, -980, -1004, -1019, -1024, -1019, -1004, -980, -946, -903, -851, -792, -724, -650, -569, -483, -392, -297, -200, -100 };   /* sin, Q10, 64 a turn */
#define U_MCX 197                               /* the moon's middle */
#define U_MCY 39
#define U_GCX (U_X - U_W / 2)                   /* the unicorn's middle on the meadow */
#define U_GCY (U_GY + 42)
#define U_OUT 380                               /* the flight: out to the moon until 380/1024, round it until 700 */
#define U_ROUND 700
#define U_RX 46                                 /* the orbit: 46 px across each side, 20 up and down */
#define U_RY 20
typedef struct { int cx, cy, w, left, behind; } u_flypos_t;
static u_flypos_t u_fly_at(void)
{
    u_flypos_t f;
    int32_t p = u_fly_p(), q;
    int ox = U_MCX - U_RX, oy = U_MCY;            /* where it meets the moon's orbit: on the left */
    f.left = 0; f.behind = 0;
    if (p < U_OUT) {                            /* away: a quick rise off the fence, smaller as it goes */
        q = p * 1024 / U_OUT;
        f.cx = U_GCX + (ox - U_GCX) * (int)q / 1024;
        f.cy = U_GCY + (oy - U_GCY) * u_rise(q) / 1024;
        f.w = 88 - (88 - 22) * (int)q / 1024;   /* 88 px wide on the meadow, 22 at the moon */
    } else if (p < U_ROUND) {                   /* once round: left, over the top (behind), right, under (in front) */
        int a = (int)((p - U_OUT) * 64 / (U_ROUND - U_OUT)), s, c;
        a = (a + 32) & 63;                      /* from the left (half a turn) */
        s = U_SIN[a]; c = U_SIN[(a + 16) & 63];
        f.cx = U_MCX + U_RX * c / 1024; f.cy = U_MCY + U_RY * s / 1024;
        f.behind = s < 0;
        f.left = s > 0;                          /* under the moon it comes back the other way */
        f.w = 22 + 8 * s / 1024;                 /* nearer under the moon, further over it */
    } else {                                    /* back: bigger as it comes, down to the meadow */
        q = (p - U_ROUND) * 1024 / (1024 - U_ROUND);
        f.cx = ox + (U_GCX - ox) * (int)q / 1024;
        f.cy = oy + (U_GCY - oy) * u_ease(q) / 1024;
        f.w = 22 + (88 - 22) * (int)q / 1024;
        f.left = q < 850;                       /* coming back, then turning to run on */
    }
    f.cy += (int)((U.t / 60) % 12 < 6 ? (U.t / 60) % 6 : 6 - (U.t / 60) % 6) * f.w / 88;   /* the wings' bob */
    return f;
}
/* the frame for a width: the nearest drawn size (88 and 44 are the 44 x 42 frames at 2x and 1x) */
static const uint8_t U_FW_SIZES[9] = { 88, 66, 52, 44, 40, 30, 22, 16, 11 };
static const kit_image_t *const U_FLY_IMG[9][2][2] = {   /* [size][frame][0 right, 1 left] */
    { { &IMG_UNI_FLY1, &IMG_UNI_FLY1_L }, { &IMG_UNI_FLY2, &IMG_UNI_FLY2_L } },
    { { &IMG_UF66_1R, &IMG_UF66_1L }, { &IMG_UF66_2R, &IMG_UF66_2L } },
    { { &IMG_UF52_1R, &IMG_UF52_1L }, { &IMG_UF52_2R, &IMG_UF52_2L } },
    { { &IMG_UNI_FLY1, &IMG_UNI_FLY1_L }, { &IMG_UNI_FLY2, &IMG_UNI_FLY2_L } },
    { { &IMG_UF40_1R, &IMG_UF40_1L }, { &IMG_UF40_2R, &IMG_UF40_2L } },
    { { &IMG_UF30_1R, &IMG_UF30_1L }, { &IMG_UF30_2R, &IMG_UF30_2L } },
    { { &IMG_UF22_1R, &IMG_UF22_1L }, { &IMG_UF22_2R, &IMG_UF22_2L } },
    { { &IMG_UF16_1R, &IMG_UF16_1L }, { &IMG_UF16_2R, &IMG_UF16_2L } },
    { { &IMG_UF11_1R, &IMG_UF11_1L }, { &IMG_UF11_2R, &IMG_UF11_2L } } };
static int u_fly_size(int w)                    /* the nearest drawn size */
{
    int i, b = 0;
    for (i = 1; i < 9; i++) if ((w > U_FW_SIZES[i] ? w - U_FW_SIZES[i] : U_FW_SIZES[i] - w) < (w > U_FW_SIZES[b] ? w - U_FW_SIZES[b] : U_FW_SIZES[b] - w)) b = i;
    return b;
}

static void u_step(void)                        /* one step of U_STEP ms */
{
    const int32_t dt = U_STEP;
    int32_t v;
    u_fence_t *f;
    int i;
    U.t += (uint32_t)dt;
    if (U.arp_n >= 0 && U.arp_n < 5 && U.t - U.arp_at >= (uint32_t)(U.arp_n * 70)) {   /* the cheer: up an arpeggio */
        static const uint8_t ARP[5] = { 72, 76, 79, 84, 88 };
        u_bell(ARP[U.arp_n++]);
    }
    if (U.plop_at && U.t >= U.plop_at) { U.plop_at = 0; u_plop(); }
    if (U.dance > 0) U.dance -= dt;
    for (i = 0; i < 8; i++) if (U.fl_n[i] >= 0 && (U.fl_y[i] += dt) >= 700) U.fl_n[i] = -1;   /* the floating notes: 0.7 s up */
    if (U.cheer > 0 && !U.running) { U.cheer -= dt; if (U.cheer <= 0) U.learned = 0; }
    if (!U.running) return;
    if (U.listen && !U.fly) {                   /* PLAY: as each fence comes near, its note sounds and the unicorn jumps it */
        u_poop_t *p = u_poop_ahead();
        f = u_next();
        if (p && (!f || p->wx < f->wx) && u_sx(p->wx) - U_X < 40) p->hop = 1;
        else if (f && f->state == UF_AHEAD && u_sx(f->wx) - U_X < 40) {
            int last = f->si == u_len() - 1;
            U.ln = f->note; U.lu = U.t + 350; u_ok(f);
            if (last) U.listen = 0;             /* the song's last fence: the run is over */
        }
    }
    if (U.fly) {                                /* flying: the fences below play themselves, the poops are passed */
        int32_t land = (U.fly_x0 >> 8) + U_FLYN * U_SPACING + U_X;   /* where its front comes down */
        f = u_next();
        if (f && u_sx(f->wx) - U_X < 40 && f->wx < land - 40) { u_ok(f); f->state = UF_CLEARED; }
        for (i = 0; i < U_NP; i++) if (U.p[i].used && U.p[i].ahead && u_sx(U.p[i].wx) - U_X < 20 && U.p[i].wx < land - 40) U.p[i].ahead = 0;
        v = u_speed();
        U.wait = 0;
        if (!U.at_moon && u_fly_p() >= U_OUT) { U.at_moon = 1; U.arp_at = U.t; U.arp_n = 0; u_spark(9, U_MCX - 30, U_MCY); }
        if (u_fly_p() >= 1024) U.fly = 0;       /* down again: it runs on */
    } else {                                    /* how fast: dashing to a fence played early, else running; it stops before one not played */
        int block = 0;
        u_poop_t *p = u_poop_ahead();
        f = u_next();
        if (!U.jump_len && ((f && u_sx(f->wx) - U_X <= U_STOP) || (p && u_sx(p->wx) - U_X <= U_STOP))) block = 1;
        v = 0;
        if (!block) {
            int dash = 0;
            for (i = 0; i < U_NF; i++) if (U.f[i].used && U.f[i].state == UF_OK) dash = 1;
            for (i = 0; i < U_NP; i++) if (U.p[i].used && U.p[i].ahead && U.p[i].hop) dash = 1;
            v = U.jump_len ? U.jump_v : dash ? u_speed() * 2 : u_speed();   /* in the air: the jump's own speed */
        }
        U.wait = block;
        if (block) {                            /* waiting with no key: a neigh at 8 s, asleep at 20 s */
            U.idle += dt;
            if (U.idle >= 8000 && U.idle - dt < 8000) kit_sfx(KIT_SFX_NEIGH, 8);
            if (U.idle >= 20000) U.sleep = 1;
        }
    }
    U.x += v * dt / 1000;
    if (U.bonk > 0) U.bonk -= dt;
    if (U.jump_len) { U.jump += dt; if (U.jump >= U.jump_len) U.jump = U.jump_len = 0; }
    if (U.cheer > 0) U.cheer -= dt;
    for (i = 0; i < U_NF; i++)                  /* a fence played reaching the unicorn: over it (or, with wings, away) */
        if (U.f[i].used && U.f[i].state == UF_OK && u_sx(U.f[i].wx) - U_X < U_STOP + 4) {
            U.f[i].state = UF_CLEARED;
            if (U.f[i].si == u_len() - 1 && (U.mode == UM_EASY || U.wings)) {   /* the song's last fence: up and away */
                int k;
                U.wings = 0; U.fly = 1; U.fly_x0 = U.x; U.at_moon = 0; U.ntr = 0; U.jump_len = 0;
                for (k = 0; k < U_NF; k++) if (U.f[k].used && U.f[k].state == UF_AHEAD) U.f[k].used = 0;   /* the next round waits for its landing */
                U.idx = 0; U.cheer = 2500; U.arp_at = U.t; U.arp_n = 0;   /* YAY, and off */
            }
            else u_jump(u_sx(U.f[i].wx) - U_X, U_FW);
        }
    for (i = 0; i < U_NF; i++) if (U.f[i].used && u_sx(U.f[i].wx) < -U_FW - 4) U.f[i].used = 0;
    if (!U.fly) {                               /* the next fence, one spacing on; after a flight, the first just off the screen */
        int32_t far = u_far();
        if (far < 0) u_add((U.x >> 8) + KIT_W + 20);
        else if (u_sx(far) < KIT_W + 40 - U_SPACING) u_add(far + U_SPACING);
    }
    if (U.wing_msg > 0) U.wing_msg -= dt;
    for (i = 0; i < U_NP; i++) {                /* a poop left behind comes round again, in the place of the next fence */
        u_poop_t *p = &U.p[i];
        if (!p->used) continue;
        if (!p->back && u_sx(p->wx) < -U_PW - 4) { p->back = 1; p->ahead = 1; p->wx = u_far() + U_SPACING; }
        else if (p->back && u_sx(p->wx) < -U_PW - 4) p->used = 0;
        if (p->used && p->ahead && p->hop && u_sx(p->wx) - U_X < U_STOP + 4) { p->ahead = 0; p->hop = 0; u_jump(u_sx(p->wx) - U_X, U_PW); }
    }
    if (!U.fly && U.ntr > 0) {                  /* down again: the rainbow fades from its start */
        int k, n = U.ntr < 4 ? U.ntr : 4;
        for (k = n; k < U.ntr; k++) { U.tr_x[k - n] = U.tr_x[k]; U.tr_y[k - n] = U.tr_y[k]; U.tr_w[k - n] = U.tr_w[k]; }
        U.ntr -= n;
    }
    if (U.fly) {                                /* the rainbow: where its tail is, each step */
        u_flypos_t fp = u_fly_at();
        int k, dx = (fp.left ? 34 : -34) * fp.w / 88;   /* its tail */
        if (U.ntr >= U_NT) { for (k = 1; k < U_NT; k++) { U.tr_x[k - 1] = U.tr_x[k]; U.tr_y[k - 1] = U.tr_y[k]; U.tr_w[k - 1] = U.tr_w[k]; } U.ntr = U_NT - 1; }
        U.tr_x[U.ntr] = (int16_t)(fp.cx + dx); U.tr_y[U.ntr] = (int16_t)(fp.cy - 2 * fp.w / 88);
        U.tr_w[U.ntr] = (uint8_t)fp.w; U.ntr++;
    }
    for (i = 0; i < U_NS; i++)
        if (U.sp_life[i] > 0) {
            U.sp_x[i] += U.sp_vx[i] * dt / 1000; U.sp_y[i] += U.sp_vy[i] * dt / 1000;
            U.sp_vy[i] += (120 << 8) * dt / 1000; U.sp_life[i] -= dt;
        }
}

static const kit_color_t U_RB[6] = { KIT_RGB(255, 77, 77), KIT_RGB(255, 159, 28), KIT_RGB(255, 225, 77), KIT_RGB(94, 211, 107),
                                     KIT_RGB(46, 196, 240), KIT_RGB(138, 107, 255) };
/* the screen: the night sky in bands (8-bit), twinkling stars, the moon, clouds, hills, the meadow, the poops, the
 * fences with their notes, the unicorn, the top line */
static void u_draw(void)
{
    static const kit_color_t BANDS[6] = { KIT_RGB(13, 11, 38), KIT_RGB(20, 17, 58), KIT_RGB(28, 23, 80), KIT_RGB(38, 32, 95),
                                          KIT_RGB(48, 40, 109), KIT_RGB(59, 48, 121) };
    u_fence_t *nf = u_next();
    int i, x, y;
    int32_t sx = U.x >> 8;
    const kit_image_t *spr;
    char s[24];
    for (i = 0; i < 6; i++) kit_rect(0, i * 36, KIT_W, 36, BANDS[i]);
    for (i = 0; i < 46; i++) {
        int on = ((U.t / 300 + (uint32_t)i * 7) % 9) != 0;
        kit_rect((int)(((i * 53) - sx / 20) % 240 + 240) % 240, (i * 37) % 150 + 16, 1, 1, on ? KIT_RGB(255, 246, 196) : KIT_RGB(111, 104, 168));
    }
    kit_image(176, 20, &IMG_MOON, 3);
    kit_image(164, 30, &IMG_STAR, 1);
    kit_image(226, 64, &IMG_STAR, 1);
    for (i = 0; i < 3; i++) {
        int cx = (int)(((i * 110 - sx / 5) % 330 + 330) % 330) - 50;
        kit_image(cx, 64 + i * 26, &IMG_CLOUD, 2);
    }
    for (x = 0; x < KIT_W; x += 4) {            /* hills, far */
        int ph = (int)((x + sx * 3 / 10) % 188), hh = 14 + (ph < 94 ? ph : 188 - ph) / 6;
        kit_rect(x, U_GROUND - hh, 4, hh, KIT_RGB(31, 58, 79));
    }
    kit_rect(0, U_GROUND, KIT_W, 36, KIT_RGB(47, 138, 74));
    for (x = -8; x < KIT_W + 8; x += 8) { int gx = x - (int)(sx % 8); kit_rect(gx, U_GROUND, 4, 2, KIT_RGB(58, 166, 90)); kit_rect(gx + 4, U_GROUND + 6, 2, 2, KIT_RGB(58, 166, 90)); }
    kit_rect(0, U_GROUND + 18, KIT_W, 18, KIT_RGB(31, 107, 55));
    if (!U.teach)
    for (i = 0; i < U_NP; i++) if (U.p[i].used) kit_image((int)u_sx(U.p[i].wx), U_GROUND - 7 * U_SC + 2, &IMG_POOP, U_SC);
    for (i = 0; i < U_NF && !U.teach; i++) {   /* (teaching: no fences) */
        u_fence_t *f = &U.f[i];
        if (!f->used) continue;
        x = (int)u_sx(f->wx);
        kit_image(x, U_GROUND - 16 * U_SC + 3, f->state == UF_DOWN ? &IMG_FENCE_DOWN : &IMG_FENCE, U_SC);
        if (U.mode == UM_KEY && f->state == UF_AHEAD) {   /* Key: just the note's letter in its colour, bobbing; Easy: none */
            static const int8_t RING[16][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 },
                                                { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };
            const char *nm = U_NAMES[U_WHITE[f->note % 12]];
            int cy = U_GROUND - 100 + (int)(((U.t / 100) + f->si) % 8 < 4 ? ((U.t / 100) + f->si) % 4 : 4 - ((U.t / 100) + f->si) % 4), k;
            kit_color_t halo = f == nf ? ((U.t / 250) % 2 ? KIT_RGB(255, 255, 255) : KIT_RGB(255, 246, 196)) : KIT_RGB(13, 11, 38);
            for (k = 0; k < 8; k++)             /* a halo round the letter: light on the next fence, dark on the others */
                kit_text(x + 21 + RING[k][0], cy + RING[k][1], nm, KIT_FONT_LARGE, KIT_ALIGN_CENTER, halo);
            for (k = 8; k < 16; k++)            /* and a thin dark line inside it */
                kit_text(x + 21 + RING[k][0], cy + RING[k][1], nm, KIT_FONT_LARGE, KIT_ALIGN_CENTER, KIT_RGB(13, 11, 38));
            kit_text(x + 21, cy, nm, KIT_FONT_LARGE, KIT_ALIGN_CENTER, U_COL[f->note % 12]);
        }
    }
    for (i = 0; i + 1 < U.ntr; i++) {           /* the flight's rainbow, left in the sky */
        int ax = U.tr_x[i], ay = U.tr_y[i], dx = U.tr_x[i + 1] - ax, dy = U.tr_y[i + 1] - ay, k, j, n;
        int th = U.tr_w[i] * 3 * (i + 1) / (88 * U.ntr);   /* as thick as the unicorn was big there, thinner to its end */
        if (th < 1) th = 1;
        n = ((dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy)) / 2 + 1;
        for (j = 0; j < n; j++)                 /* from this point to the next, 2 px at a time: no gaps */
            for (k = 0; k < 6; k++) kit_rect(ax + dx * j / n, ay + dy * j / n + (k - 3) * th, 3, th, U_RB[k]);
    }
    y = U_GY;                                   /* the unicorn: running, jumping (an arc), bonked, flying */
    spr = U.wait ? &IMG_UNI_B : (U.t / 125) % 2 ? &IMG_UNI_A : &IMG_UNI_B;   /* waiting: it stands */
    if (U.sleep) { spr = &IMG_UNI_SLEEP; y += 10; }   /* asleep: lying down, eyes shut */
    if (U.teach) {                              /* listening: it sways; each note, a little hop with its wings up */
        spr = (U.t / 400) % 2 ? &IMG_UNI_A : &IMG_UNI_B;
        if (U.dance > 0) { spr = &IMG_UNI_UP; y -= (int)(U.dance > 150 ? 300 - U.dance : U.dance) / 12; }
        x = U_X - U_W + 20;
    }
    x = U_X - U_W;
    if (U.jump_len) {
        int32_t p = U.jump * 1024 / U.jump_len, a = p * (1024 - p) / 256;   /* a parabola, 0..1024 */
        y -= (int)(a * U_HOP / 1024); spr = &IMG_UNI_UP;
    }
    if (U.fly) {                                /* flying: smaller the further away; behind the moon over its top */
        u_flypos_t fp = u_fly_at();
        int si = u_fly_size(fp.w), w = U_FW_SIZES[si], h = (w * 42 + 22) / 44;
        spr = U_FLY_IMG[si][(U.t / 150) % 2][fp.left];
        kit_image(fp.cx - w / 2, fp.cy - h / 2, spr, w == 88 ? 2 : 1);
        if (fp.behind) kit_image(176, 20, &IMG_MOON, 3);
    }
    if (U.bonk > 0) x += (U.bonk / 40) % 2 ? 2 : -2;
    if (!U.fly) kit_image(x, y, spr, U_USC);
    if (U.sleep)                                /* z z Z, floating up from its head */
        for (i = 0; i < 3; i++) {
            int ph = (int)((U.t / 60 + (uint32_t)i * 12) % 36);
            kit_text(x + 58 + ph / 3 + i * 5, y - 4 - ph, i == 2 ? "Z" : "z", i == 2 ? KIT_FONT_MEDIUM : KIT_FONT_SMALL, KIT_ALIGN_CENTER, KIT_RGB(201, 195, 238));
        }
    if (U.bonk > 0)                             /* little stars round its head */
        for (i = 0; i < 3; i++) kit_image(x + 66 + ((int)(U.t / 80) + i * 5) % 18 - 9, y - 2 + (i * 4) % 9, &IMG_STAR, U_SC);
    for (i = 0; i < U_NS; i++) if (U.sp_life[i] > 0) kit_rect(U.sp_x[i] >> 8, U.sp_y[i] >> 8, 3, 3, U_COL[U.sp_c[i]]);
    kit_rect(0, 0, KIT_W, 16, KIT_RGB(13, 11, 38));   /* the top line: the song, the stars won, the poops */
    kit_rect(0, 0, KIT_W, 2, MODE_COL);
    {                                           /* the song, and the mode */
        static const char *const MODES[UM_N] = { "EASY", "KEY" };
        int w = kit_text(4, 4, u_name(), KIT_FONT_XSMALL, KIT_ALIGN_LEFT, KIT_RGB(255, 210, 63));
        kit_text(12 + w, 4, U.teach ? "LISTENING" : U.listen ? "LISTEN" : MODES[U.mode], KIT_FONT_XSMALL, KIT_ALIGN_LEFT, U.teach ? KIT_RGB(255, 246, 196) : KIT_RGB(160, 154, 210));
    }
    if (U.teach) {                              /* teaching: the notes floating up, the notes learned, what to press */
        static const char *const SHARP[12] = { "", "#", "", "#", "", "", "#", "", "#", "", "#", "" };
        int from = U.tn > 12 ? U.tn - 12 : 0;
        for (i = 0; i < 8; i++)
            if (U.fl_n[i] >= 0) {               /* from its key, up to the unicorn's ear, in its colour */
                int p = U.fl_y[i] * 1024 / 700, fx = U.fl_x[i] + (100 - U.fl_x[i]) * p / 1024, fy = 226 - (226 - 124) * p / 1024;
                kit_circle(fx, fy, 4, U_COL[U.fl_n[i] % 12]);
                kit_rect(fx + 3, fy - 12, 2, 12, U_COL[U.fl_n[i] % 12]);
                kit_rect(fx + 3, fy - 12, 6, 2, U_COL[U.fl_n[i] % 12]);
            }
        for (i = from; i < U.tn; i++) {         /* the last twelve notes learned, along the meadow */
            char nm[3];
            nm[0] = U_NAMES[U_WHITE[U.tbuf[i] % 12]][0]; nm[1] = SHARP[U.tbuf[i] % 12][0]; nm[2] = 0;
            kit_text(12 + (i - from) * 19, U_GROUND + 6, nm, KIT_FONT_SMALL, KIT_ALIGN_CENTER, U_COL[U.tbuf[i] % 12]);
        }
        kit_text(120, U_GROUND + 26, U.tn ? "SAVE keeps it    K1 back: undo" : "Play the song: the unicorn listens", KIT_FONT_TINY, KIT_ALIGN_CENTER, KIT_RGB(220, 240, 220));
        cat_int(s, U.tn);
        x = kit_text(236, 4, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, KIT_RGB(255, 246, 196));
        kit_text(236 - x - 4, 4, "notes", KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, KIT_RGB(160, 154, 210));
    } else {
    cat_int(s, U.score);
    x = kit_text(U.poops ? 196 : 236, 4, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, KIT_RGB(255, 246, 196));
    kit_image((U.poops ? 196 : 236) - x - 9, 6, &IMG_STAR, 2);
    }
    if (U.poops && !U.teach) { cat_int(s, U.poops); x = kit_text(236, 4, s, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, KIT_RGB(255, 246, 196)); kit_image(236 - x - 22, 4, &IMG_POOP, 2); }
    if (U.cheer > 0) ctext(120, U.learned ? 108 : 60, U.learned ? "LEARNED!" : "YAY!", U.learned ? KIT_FONT_MEDIUM : KIT_FONT_LARGE, U_COL[(U.t / 120) % 12]);
    if (U.fly) ctext(120, U_GROUND + 8, U.at_moon ? "THE MOON!" : "UP AND AWAY!", KIT_FONT_MEDIUM, KIT_RGB(255, 246, 196));
    else if (U.wing_msg > 0) ctext(120, U_GROUND + 8, "WINGS!", KIT_FONT_MEDIUM, KIT_RGB(255, 246, 196));
    if (!U.running && !U.teach) {               /* the start card; on a MY SONG not taught yet: TEACH ME */
        kit_round_rect(24, 40, 192, 56, 8, KIT_RGB(13, 11, 38));
        ctext(120, 60, u_len() ? "PRESS A KEY" : "TEACH ME", KIT_FONT_MEDIUM, KIT_RGB(255, 210, 63));
        ctext(120, 84, !u_len() ? "REC, then play me a song" : u_my() >= 0 ? "K3 song   REC teach it again"
                       : !U.quiet ? "K1 mode  K2 speed  K3 song  K4 music" : "K1 mode  K2 speed  K3 song  K4 music off", KIT_FONT_TINY, KIT_RGB(201, 195, 238));
    }
}

static void u_lights(void)                      /* Key modes: the next fence's key lights; Easy: every key, breathing when it waits */
{
    u_fence_t *f = U.running ? u_next() : 0;
    int k, want = f ? f->note : -1;
    if (U.listen) {                             /* PLAY: the key of the note sounding, the others dark */
        for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, KIT_DRUM_LO + k == U.ln && (int32_t)(U.t - U.lu) < 0 ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
        kit_btn_light(KIT_BTN_PLAY, KIT_LIGHT_ON);
        return;
    }
    kit_btn_light(KIT_BTN_PLAY, U.teach ? KIT_LIGHT_OFF : KIT_LIGHT_DIM);
    for (k = 0; k < KIT_NKEYS; k++) {
        int n = KIT_DRUM_LO + k;                /* the keys as they are: F3 .. G5, no octave shift */
        int lv = !U.running || U.teach ? KIT_LIGHT_BREATH
               : U.mode == UM_EASY ? (U.wait ? KIT_LIGHT_BREATH : KIT_LIGHT_DIM)
               : n == want ? (U.wait ? KIT_LIGHT_BREATH : KIT_LIGHT_ON) : KIT_LIGHT_OFF;
        kit_key_light(k, lv);
    }
}

/* the game's events: keys, K1..K3; 1 when taken */
static int u_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_KEY_DOWN) { u_press(KIT_DRUM_LO + e->id); return 1; }
    if (e->type == KIT_EV_MIDI_NOTE_ON) { u_press(e->id); return 1; }
    if (e->type == KIT_EV_BTN_DOWN && (e->id == KIT_BTN_REC || e->id == KIT_BTN_SAVE)) {   /* teach a MY SONG, and keep it */
        if (U.teach) u_teach_save();
        else if (e->id == KIT_BTN_REC && u_my() >= 0) u_teach_start();
        return 1;
    }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_PLAY && !U.teach) {   /* PLAY: the unicorn runs the song by itself, or stops */
        if (U.listen) { U.listen = 0; return 1; }
        if (!u_len()) return 1;
        if (!U.running) { U.running = 1; U.learned = 0; u_reset(); U.q = kit_ms() / U_STEP; U.lastn = 60; u_pad(60); }
        U.listen = 1; U.npq = 0;
        return 1;
    }
    if (U.teach && e->type == KIT_EV_ENC) {     /* teaching: K1 back takes back the last note; K2, K4 as ever */
        if (e->id == KIT_ENC_KNOB1 && e->delta < 0) u_press(U_UNDO);
        if (e->id != KIT_ENC_KNOB2 && e->id != KIT_ENC_KNOB4) return 1;
    }
    if (e->type == KIT_EV_ENC) {
        if (e->id == KIT_ENC_KNOB1) { U.mode += e->delta > 0 ? 1 : -1; U.mode = U.mode < 0 ? 0 : U.mode >= UM_N ? UM_N - 1 : U.mode; }
        if (e->id == KIT_ENC_KNOB2) { U.speed += e->delta > 0 ? 1 : -1; U.speed = U.speed < 0 ? 0 : U.speed > 2 ? 2 : U.speed; }
        if (e->id == KIT_ENC_KNOB4) {         /* the music on or off */
            int b = e->delta > 0;
            if (b == U.quiet) { U.quiet = !b; kit_reverb(b ? 40 : 0); if (!b) u_pad_off(); else if (U.running) u_pad(U.lastn); }
        }
        if (e->id == KIT_ENC_KNOB3) { U.tune = ((U.tune + (e->delta > 0 ? 1 : -1)) % U_NALL + U_NALL) % U_NALL; u_pad_off(); u_reset(); u_key(); U.listen = 0; if (!u_len()) U.running = 0; }
        return 1;
    }
    return 1;                                   /* the rest does nothing in the game */
}

static void u_frame(uint32_t now)
{
    int i, moved = 0;
    if (!U.on) { u_enter(); moved = 1; }
    for (i = 0; i < 4; i++) if (U.off_at[i] && (int32_t)(now - U.off_at[i]) >= 0) { kit_track_note_off(U_BELL, U.off_n[i]); U.off_at[i] = 0; }
    while (U.q < now / U_STEP) {                /* step by step to now: the same states on every FM-1 */
        int j, k = 0;
        for (j = 0; j < U.npq; j++)             /* the keys played in this step, first */
            if (U.pq_q[j] <= U.q) u_press_now(U.pq_n[j]);
            else { U.pq_n[k] = U.pq_n[j]; U.pq_q[k] = U.pq_q[j]; k++; }
        U.npq = k;
        U.q++; u_step(); moved = 1;
    }
    if (!moved) return;                         /* drawn only when it moves */
    u_draw();
    u_lights();
}
