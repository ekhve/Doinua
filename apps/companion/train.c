/* SPDX-License-Identifier: GPL-3.0-only */
/* The Night Train, the Game mode's fourth game (ST-55, D-064), included by app.c after aldapa.c. Old Number 7, a 4-4-0
 * steam engine with a balloon stack, a bell and a lamp, crosses the desert at night under a huge moon, a puppy riding
 * on the logs in its tender. Each note played is a chuff: the wheels turn, it moves on, a puff in the note's colour
 * goes up; the end of each line: the whistle (choo choo), the bell, the coyote howling along. Two songs (K3): the
 * traditional I've Been Working on the Railroad (the verse and "Dinah, won't you blow", in G) and Funiculì, funiculà
 * (Luigi Denza, 1880; the verse and the chorus, in D). Easy any key, Key the next note's key (its letter over the
 * engine). A wrong key in Key: the engine coughs black smoke; two in a row: it sneezes, ah... ah... CHOO, its front
 * rearing up, the driver's cap flying off, the sooty cloud blowing back over the train; the puppy barks, the driver
 * goes red and shakes his fist. Eight right in a row in Key: the golden super train, a cowboy galloping alongside. The
 * song's end: a golden run, then Star Station, where the sheriff throws the train a star it keeps (the settings, D-045).
 * Along the line: telegraph poles and an owl, a ranch and its windmill, a covered-wagon camp, a longhorn, prairie dogs,
 * a jackrabbit racing the train, a canyon on a trestle now and then, cacti blooming in the lamplight near the song's
 * end. As the browser lab the owner shaped ("Sneeze is amazing. With maybe the dog barking and the driver kind of
 * angry?"). All drawn in code. */

#define T_STEP 50                               /* the game moves in steps of 50 ms, as the others */
#define T_RAIL 212                              /* the rail's top */
#define T_OX 20                                 /* the engine's drawing: its left and top on the screen; its rails at */
#define T_OY 100                                /* T_OY + T_G (= T_RAIL); the cab's left at T_X0 */
#define T_G 112
#define T_X0 48
#define T_NP 28
#define T_NS 40
#define T_NOFF 10

/* the songs: note, length in sixteenths, flags (bit 0: a line ends here; bits 1..3: the chord from here on, 1..) */
static const uint8_t T_RAILROAD[][3] = { {67,6,0},{62,2,0},{67,3,0},{62,1,0},{67,3,0},{69,1,0},{71,8,0},{67,8,0},{72,6,0},{72,2,0},{67,4,0},{69,4,0},{71,16,1},{67,6,0},{62,2,0},{67,3,0},{62,1,0},{67,3,0},{69,1,0},{71,8,0},{67,4,0},{71,3,0},{71,1,0},{71,4,0},{69,4,0},{69,4,0},{71,4,0},{69,16,1},{69,6,0},{69,2,0},{68,3,0},{69,1,0},{71,3,0},{69,1,0},{67,8,0},{62,8,0},{72,4,0},{72,3,0},{72,1,0},{67,3,0},{67,1,0},{69,3,0},{69,1,0},{71,16,1},{64,6,0},{66,2,0},{67,3,0},{66,1,0},{67,3,0},{64,1,0},{62,8,0},{67,8,0},{71,4,0},{72,4,0},{71,4,0},{69,4,0},{67,16,1},{62,3,0},{62,1,0},{62,3,0},{62,1,0},{67,8,0},{64,3,0},{64,1,0},{64,3,0},{64,1,0},{69,8,0},{66,3,0},{66,1,0},{66,3,0},{66,1,0},{64,4,0},{66,4,0},{67,4,0},{69,4,0},{71,16,1},{62,3,0},{62,1,0},{62,3,0},{62,1,0},{67,8,0},{64,3,0},{64,1,0},{64,3,0},{64,1,0},{69,8,0},{66,3,0},{66,1,0},{66,3,0},{66,1,0},{64,4,0},{66,4,0},{67,16,1} };
static const uint8_t T_FUNICULI[][3] = { {69,2,2},{74,16,0},{74,2,0},{73,4,0},{73,2,0},{69,4,0},{69,2,0},{71,4,0},{71,2,0},{66,6,0},{66,10,1},{66,2,0},{64,4,4},{62,2,0},{62,16,3},{66,2,0},{64,4,4},{62,2,0},{62,18,3},{73,6,4},{71,6,0},{73,6,0},{71,6,0},{74,4,0},{73,2,0},{71,4,0},{74,2,0},{73,12,1},{73,6,0},{71,6,0},{73,6,0},{71,6,0},{74,4,0},{73,2,0},{71,4,0},{74,2,0},{69,4,2},{66,2,0},{66,4,0},{66,2,0},{66,4,0},{66,2,0},{66,4,0},{66,2,0},{66,4,0},{66,2,0},{66,4,0},{66,2,0},{66,4,0},{66,2,0},{74,12,7},{76,4,8},{74,2,0},{71,4,0},{74,2,0},{69,4,2},{66,2,0},{66,4,0},{67,2,0},{69,4,4},{67,2,0},{66,4,0},{64,2,0},{62,12,3} };
typedef struct { const uint8_t (*n)[3]; int len, sixteenth, late; const char *name; const uint8_t (*ch)[4]; int nch; } t_song_t;
/* the chords: three notes and a bass. Railroad (G): G C D E A, by the note (kept while the note is in it);
 * Funiculì (D): D A7 Bm G, as its flags say */
static const uint8_t T_CH_G[5][4] = { { 55, 59, 62, 43 }, { 55, 60, 64, 48 }, { 54, 57, 62, 50 }, { 56, 59, 64, 40 }, { 57, 61, 64, 45 } };
static const uint8_t T_CH_D[4][4] = { { 54, 57, 62, 50 }, { 55, 57, 61, 45 }, { 54, 59, 62, 47 }, { 55, 59, 62, 43 } };
static const int8_t T_CHORD_OF_G[12] = { 1, 4, 0, 0, 1, 1, 2, 0, 3, 2, 0, 0 };   /* a pitch class's chord in G (0 G .. 4 A) */
static const t_song_t T_SONGS[2] = {
    { T_RAILROAD, (int)(sizeof T_RAILROAD / sizeof T_RAILROAD[0]), 110, 61, "RAILROAD", T_CH_G, 5 },
    { T_FUNICULI, (int)(sizeof T_FUNICULI / sizeof T_FUNICULI[0]), 100, 19, "FUNICULI", T_CH_D, 4 },
};
/* the voices (K2): a reedy pulse, a pluck, an organ */
static const uint8_t T_VOICE[3][2] = { { 3, 0 }, { 1, 8 }, { 5, 1 } };   /* Lo-fi PULSE LD, Analog PLUCK, Organ wheel JAZZ PERC */
static const char *const T_VOICE_NAME[3] = { "REED", "PLUCK", "ORGAN" };
#define T_KIT 2                                 /* the Live kit: shaker1 the chuff, stick1 the bridge, bell1 the bell */
enum { T_PAD_KICK = 0, T_PAD_STICK = 7, T_PAD_OPEN = 15, T_PAD_SHAKER = 17, T_PAD_TOM = 19, T_PAD_BELL = 23 };

typedef struct { int32_t x, y, vx, vy, r, grow; int16_t life, max; kit_color_t c; uint8_t used, dark, star, wind; } t_puff_t;
typedef struct { int32_t x, y, vx, vy, fall; int16_t life; kit_color_t c; uint8_t used, big; } t_spark_t;
enum { T_PH_NONE = 0, T_PH_SUPER, T_PH_ARRIVE, T_PH_STOP, T_PH_WAIT };
static struct {
    int on, mode, quiet, voice, tune, misses, streak, idx;
    uint32_t q, t;
    int32_t scroll, v;                          /* px x 256, px/s x 256 */
    int32_t wheel;                              /* a turn = 1024 */
    int32_t cough, sneeze, soot, wave, super, howl, bell, steam, angry, bark, got, pop;   /* ms (sneeze: ms since it began) */
    t_puff_t puff[T_NP]; t_spark_t spark[T_NS];
    struct { int32_t x, y, vx, vy; int used; } hat;
    struct { int32_t x; int used; } rider;
    struct { int32_t x, t; int used, tired; } rabbit;
    int32_t next_rabbit;
    int32_t tw_x, tw_a;                         /* the tumbleweed: px x 256, its turn */
    struct { int32_t x, y; int used; } shoot, bats;
    int phase; int32_t phase_t, station_wx; int station;
    struct { int32_t t, x0, y0; int used; } fly;
    int cow_was, dog_was[2]; int32_t cow_t, dog_t[2], owl_t; int owl_last;
    int32_t bloom[5];                           /* ms since each cactus opened (0: closed) */
    int listen, lk; int32_t lat;
    int ln; uint32_t lu;                        /* PLAY: the key lit for the note sounding, until */
    int chord, chord_n[4], boom_fifth;
    struct { uint32_t at; uint8_t trk, note, used; } off[T_NOFF];
    struct { uint32_t at; uint8_t what, used; } sch[6];
    int saved_e[KIT_NTRACKS], saved_p[KIT_NTRACKS], saved_rv;
    uint8_t pq_n[8]; uint32_t pq_q[8]; int npq;
    uint32_t rnd;
    int tilt, jx, jy;                           /* the engine as drawn now: its front's lift (x 1/1024 px a px), its jolt */
} T;

/* the randomness (one t_rand per expression: C leaves the order of a call's arguments to the compiler, and the FM-1's
 * and the PC's differ): the effects' sequence starts again at each note and each wrong key (so the smoke and sparks follow the
 * play, not how many steps the game has run); the sky's own chances (a shooting star, bats) come from the time */
static int t_rand(int n) { T.rnd = T.rnd * 1664525u + 1013904223u; return (int)((T.rnd >> 8) % (uint32_t)n); }
static uint32_t t_hash(uint32_t x) { x ^= x >> 16; x *= 0x45d9f3bu; x ^= x >> 16; x *= 0x45d9f3bu; x ^= x >> 16; return x; }
static int t_sin(int32_t a) { return U_SIN[(a >> 4) & 63]; }   /* sin, Q10, a turn = 1024 */
static int t_cos(int32_t a) { return U_SIN[((a >> 4) + 16) & 63]; }
static int t_wrap(int32_t v, int32_t p) { v %= p; return (int)(v < 0 ? v + p : v); }
static const t_song_t *t_sg(void) { return &T_SONGS[T.tune]; }
static int t_note_of(int k) { return t_sg()->n[k % t_sg()->len][0]; }

/* ---- sound: the tune on the voice K2 picks, a boom-chick under it, the chuffs, the bell, the effects ---- */
static void t_off_at(int trk, int note, int ms)
{
    int i, k = 0;
    for (i = 0; i < T_NOFF; i++) if (!T.off[i].used) { k = i; break; } else if (T.off[i].at < T.off[k].at) k = i;
    if (T.off[k].used) kit_track_note_off(T.off[k].trk, T.off[k].note);
    T.off[k].used = 1; T.off[k].trk = (uint8_t)trk; T.off[k].note = (uint8_t)note; T.off[k].at = T.t + (uint32_t)ms;
}
static void t_play_note(int trk, int n, int vel, int ms)
{
    int i;
    for (i = 0; i < T_NOFF; i++) if (T.off[i].used && T.off[i].trk == trk && T.off[i].note == n) { kit_track_note_off(trk, n); T.off[i].used = 0; }
    kit_track_note_on(trk, n, vel);
    t_off_at(trk, n, ms);
}
static void t_hit(int pad, int vel) { kit_track_note_on(KIT_TRACK_DRUMS, KIT_DRUM_LO + pad, vel); kit_track_note_off(KIT_TRACK_DRUMS, KIT_DRUM_LO + pad); }
enum { T_S_WOOF = 1, T_S_HOWL, T_S_NEIGH, T_S_BELL, T_S_WHISTLE, T_S_BOOM };
static void t_later(int ms, int what)
{
    int i;
    for (i = 0; i < 6; i++) if (!T.sch[i].used) { T.sch[i].used = 1; T.sch[i].at = T.t + (uint32_t)ms; T.sch[i].what = (uint8_t)what; return; }
}
static void t_boomchick(int k)                  /* the bass on the beat (the root, then the fifth), a short strum after */
{
    const t_song_t *s = t_sg();
    int n = s->n[k][0], c = (s->n[k][2] >> 1) & 7, i;
    if (T.quiet) return;
    if (c) T.chord = c - 1;
    else if (s->ch == T_CH_G) {
        int keep = 0;
        for (i = 0; i < 3; i++) if (s->ch[T.chord][i] % 12 == n % 12) keep = 1;
        if (!keep) T.chord = T_CHORD_OF_G[n % 12];
    }
    if (T.chord >= s->nch) T.chord = 0;
    {
        const uint8_t *ch = s->ch[T.chord];
        int bass = ch[3] + (T.boom_fifth ? 7 : 0);
        T.boom_fifth = !T.boom_fifth;
        t_play_note(KIT_TRACK_BASS, bass, 90, 260);
        for (i = 0; i < 3; i++) t_play_note(KIT_TRACK_CHORDS, ch[i], 34, 140);
    }
}
static void t_whistle(void) { kit_sfx(KIT_SFX_WHISTLE, 7); T.steam = 1200; }

static void t_enter(void)
{
    int t, i;
    for (t = 0; t < KIT_NTRACKS; t++) { T.saved_e[t] = kit_track_engine(t); T.saved_p[t] = kit_track_preset(t); }
    kit_seq_stop();
    kit_track_sound(KIT_TRACK_MELODY, T_VOICE[T.voice][0], T_VOICE[T.voice][1]);
    kit_track_sound(KIT_TRACK_DRUMS, KIT_ENGINE_DRUMS, T_KIT);
    kit_track_sound(KIT_TRACK_CHORDS, 1, 8);    /* Analog: PLUCK, the strum */
    kit_track_sound(KIT_TRACK_BASS, 1, 7);      /* Analog: SUB BASS, the boom */
    T.saved_rv = kit_reverb_of(); kit_reverb(T.quiet ? 0 : 30);
    T.on = 1; T.q = kit_ms() / T_STEP; T.t = T.q * T_STEP; T.rnd = 4242;
    T.idx = 0; T.misses = T.streak = 0; T.scroll = 0; T.v = 0; T.wheel = 0; T.listen = 0; T.npq = 0; T.chord = 0;
    T.cough = T.sneeze = T.soot = T.wave = T.super = T.howl = T.bell = T.steam = T.angry = T.bark = T.got = T.pop = 0;
    for (i = 0; i < T_NP; i++) T.puff[i].used = 0;
    for (i = 0; i < T_NS; i++) T.spark[i].used = 0;
    for (i = 0; i < T_NOFF; i++) T.off[i].used = 0;
    for (i = 0; i < 6; i++) T.sch[i].used = 0;
    for (i = 0; i < 5; i++) T.bloom[i] = 0;
    T.hat.used = T.rider.used = T.rabbit.used = T.fly.used = 0; T.next_rabbit = 9000;
    T.tw_x = 300 << 8; T.tw_a = 0; T.shoot.used = T.bats.used = 0;
    T.phase = T_PH_NONE; T.station = 0; T.cow_was = 0; T.dog_was[0] = T.dog_was[1] = 0; T.cow_t = T.dog_t[0] = T.dog_t[1] = T.owl_t = 0; T.owl_last = -1;
}
static void t_leave(void)
{
    int t, i;
    for (i = 0; i < T_NOFF; i++) if (T.off[i].used) { kit_track_note_off(T.off[i].trk, T.off[i].note); T.off[i].used = 0; }
    for (t = 0; t < KIT_NTRACKS; t++) kit_track_sound(t, T.saved_e[t], T.saved_p[t]);
    kit_reverb(T.saved_rv);
    kit_lights_off();
    T.on = 0;
}

/* ---- smoke and sparks ---- */
static void t_puff(int x, int y, kit_color_t c, int r, int vx, int vy, int life, int dark, int star, int wind, int grow)
{
    int i;
    for (i = 0; i < T_NP; i++)
        if (!T.puff[i].used) {
            t_puff_t *p = &T.puff[i];
            p->used = 1; p->x = x << 8; p->y = y << 8; p->vx = vx << 8; p->vy = vy << 8; p->r = r << 8; p->grow = grow << 8;
            p->life = p->max = (int16_t)life; p->c = c; p->dark = (uint8_t)dark; p->star = (uint8_t)star; p->wind = (uint8_t)wind;
            return;
        }
}
static void t_spark(int x, int y, int vx, int vy, int fall, int life, kit_color_t c, int big)
{
    int i;
    for (i = 0; i < T_NS; i++)
        if (!T.spark[i].used) {
            t_spark_t *s = &T.spark[i];
            s->used = 1; s->x = x << 8; s->y = y << 8; s->vx = vx << 8; s->vy = vy << 8; s->fall = fall << 8; s->life = (int16_t)life; s->c = c; s->big = (uint8_t)big;
            return;
        }
}

/* ---- where things are on the engine (as drawn: its front lifts when it sneezes) ---- */
static int t_lift(int x) { int d = x - (T_X0 + 18); return d > 0 ? T.tilt * d / 1024 : 0; }
static int t_sx(int x) { return T_OX + x + T.jx; }
static int t_sy(int x, int y) { return T_OY + y + T.jy - t_lift(x); }
#define T_STACK_X (T_X0 + 88)
#define T_STACK_Y (T_G - 97)
#define T_LAMP_X (T_X0 + 106)
#define T_LAMP_Y (T_G - 76)

/* ---- playing ---- */
static int t_in_canyon(int x);
static void t_super(int ms)
{
    T.super = ms; if (T.v < (120 << 8)) T.v = 120 << 8;
    T.bell = 1500; t_whistle(); t_hit(T_PAD_BELL, 90);
    T.rider.used = 1; T.rider.x = -(40 << 8);
    t_later(900, T_S_NEIGH);
}
static void t_finale(void)                      /* the song's end: a golden run, then the station, then a star */
{
    T.phase = T_PH_SUPER; T.phase_t = 0; t_super(3600);
    t_later(500, T_S_BOOM); t_later(1200, T_S_BOOM); t_later(1900, T_S_BOOM); t_later(2600, T_S_BOOM);
}
static void t_play(int k, int ms)               /* note k: a chuff, the voice sings it, a puff goes up */
{
    const t_song_t *s = t_sg();
    int n = s->n[k][0], len = s->n[k][1];
    T.rnd = 0x9E3779B9u ^ ((uint32_t)k * 2654435761u) ^ ((uint32_t)T.tune << 20);
    t_play_note(KIT_TRACK_MELODY, n, 100, ms);
    t_hit(T_PAD_SHAKER, 46 + t_rand(20));
    t_boomchick(k);
    T.v += (18 + len * 3 / 2) << 8; if (T.v > ((T.super > 0 ? 160 : 75) << 8)) T.v = (T.super > 0 ? 160 : 75) << 8;
    T.pop = 150;
    {
        int sx = t_sx(T_STACK_X), sy = t_sy(T_STACK_X, T_STACK_Y), i;
        if (T.super > 0) for (i = 0; i < 3; i++) t_puff(sx, sy, U_COL[(n + i * 4) % 12], 3, -20 + i * 8, -40 - i * 10, 1400, 0, 1, 0, 0);
        else { int vx = -6 - t_rand(6), vy = -26 - t_rand(10); t_puff(sx, sy, U_COL[n % 12], 4 + len / 4, vx, vy, 1600, 0, 0, 0, 5); }
    }
    if (t_in_canyon(T_OX + T_X0 + 47)) t_hit(T_PAD_STICK, 70);   /* on the bridge: the wheels clatter on the timbers */
    if (s->n[k][2] & 1) {                       /* a line's end: the whistle, the bell, the coyote */
        t_whistle(); t_hit(T_PAD_BELL, 70); T.bell = 1200; T.wave = 1800; T.howl = 1600; t_later(1100, T_S_HOWL);
    }
    if (k == s->len - 1) t_finale();
}
static void t_wrong(void)
{
    int sx = t_sx(T_STACK_X), sy = t_sy(T_STACK_X, T_STACK_Y), i;
    T.rnd = 1234567u + (uint32_t)T.idx * 7919u + (uint32_t)T.misses * 31u;
    T.streak = 0;
    if (++T.misses == 1) {                      /* a cough: black smoke */
        kit_sfx(KIT_SFX_COUGH, 9); T.cough = 900; T.v = T.v * 4 / 10;
        for (i = 0; i < 6; i++) { int ox = t_rand(5) - 2, vx = t_rand(21) - 10, vy = -30 - t_rand(15); t_puff(sx + ox, sy - i * 3, KIT_RGB(26, 26, 30), 4, vx, vy, 1500, 1, 0, 0, 7); }
    } else {                                    /* the sneeze: ah... ah... CHOO */
        T.misses = 0; T.sneeze = 1; T.v = 0; kit_sfx(KIT_SFX_SNEEZE, 10);
    }
}
static void t_sneeze_step(int32_t prev)        /* the sneeze's moments, as its clock passes them */
{
    int32_t s = T.sneeze;
#define T_AT(ms) (prev < (ms) && s >= (ms))
    if (T_AT(300) || T_AT(600)) t_puff(t_sx(T_STACK_X), t_sy(T_STACK_X, T_STACK_Y), KIT_RGB(200, 200, 208), 2, -4, -14, 500, 0, 0, 0, 3);
    if (T_AT(900)) {                            /* CHOO: a great dark cloud out of the stack and the front */
        int sx = t_sx(T_STACK_X), sy = t_sy(T_STACK_X, T_STACK_Y), lx = t_sx(T_LAMP_X), ly = t_sy(T_LAMP_X, T_LAMP_Y), i;
        for (i = 0; i < 14; i++) { int r = 4 + t_rand(4), vx = 70 + t_rand(90), vy = -30 + t_rand(50); t_puff(sx + 4, sy + 4, i % 3 ? KIT_RGB(26, 26, 30) : KIT_RGB(58, 58, 68), r, vx, vy, 2600, 1, 0, 1, 7); }
        for (i = 0; i < 6; i++) { int r = 3 + t_rand(3), vx = 60 + t_rand(60), vy = -10 + t_rand(30); t_puff(lx + 4, ly + 4, KIT_RGB(42, 42, 48), r, vx, vy, 2200, 1, 0, 1, 6); }
        T.hat.used = 1; T.hat.x = t_sx(T_X0 + 11) << 8; T.hat.y = t_sy(T_X0 + 11, T_G - 72) << 8; T.hat.vx = -(35 << 8); T.hat.vy = -(95 << 8);
        for (i = 0; i < 12; i++) { int ox = t_rand(40), vx = t_rand(61) - 30, vy = -40 - t_rand(40); t_spark(T_OX + T_X0 + 18 + ox, 210, vx, vy, 160, 600, KIT_RGB(255, 216, 74), 0); }
    }
    if (T_AT(1200)) T.angry = 4500;              /* the driver: HMPH! */
    if (T_AT(1350)) { T.bark = 1400; kit_sfx(KIT_SFX_WOOF, 9); t_later(450, T_S_WOOF); t_later(900, T_S_WOOF); }
    if (T_AT(2300)) T.soot = 4500;               /* the cloud has blown back over the puppy */
#undef T_AT
}
static void t_press_now(int note)
{
    int k = T.idx % t_sg()->len;
    if (T.listen || T.sneeze) return;
    if (T.phase && T.phase != T_PH_WAIT) return;
    if (T.phase == T_PH_WAIT) { T.phase = T_PH_NONE; T.idx = 0; t_whistle(); T.v = 40 << 8; return; }   /* off again */
    if (T.mode == 1 && note != t_note_of(k)) { t_wrong(); return; }
    T.misses = 0; T.streak++;
    t_play(k, t_sg()->n[k][1] * t_sg()->sixteenth * 9 / 10 + 80);
    T.idx = k + 1 >= t_sg()->len ? 0 : k + 1;
    if (T.mode == 1 && T.streak >= 8 && T.super <= 0 && T.idx != 0) { T.streak = 0; t_super(6000); }
}
static void t_press(int note) { if (T.npq < 8) { T.pq_n[T.npq] = (uint8_t)note; T.pq_q[T.npq] = kit_ms() / T_STEP; T.npq++; } }

#define T_CANYON_P 1700
#define T_CANYON_A 1150
#define T_CANYON_B 1430
static int t_in_canyon(int x) { int k = t_wrap((T.scroll >> 8) + x, T_CANYON_P); return k > T_CANYON_A && k < T_CANYON_B; }

static void t_phases(int32_t dt)
{
    if (!T.phase) return;
    T.phase_t += dt;
    if (T.phase == T_PH_SUPER && T.super <= 300) { T.phase = T_PH_ARRIVE; T.phase_t = 0; T.station = 1; T.station_wx = (T.scroll >> 8) + 330; }
    else if (T.phase == T_PH_ARRIVE) {
        int32_t target = (T.station_wx - 150) << 8, d = target - T.scroll;
        T.v = d * 12 / 10; if (T.v < 0) T.v = 0; if (T.v > (110 << 8)) T.v = 110 << 8;
        if (d < 200) { T.scroll = target; T.v = 0; T.phase = T_PH_STOP; T.phase_t = 0; t_whistle(); t_later(300, T_S_BELL); t_later(700, T_S_BELL); T.wave = 3000; }
    } else if (T.phase == T_PH_STOP) {
        if (!T.fly.used && T.phase_t >= 1000 && T.phase_t - dt < 1000) { T.fly.used = 1; T.fly.t = 0; T.fly.x0 = T.station_wx - (T.scroll >> 8) + 52; T.fly.y0 = 166; }
        if (T.phase_t > 5000) T.phase = T_PH_WAIT;
    }
    if (T.fly.used) {
        T.fly.t += dt;
        if (T.fly.t >= 1100) {
            int i, sx = T_OX + 4 + (set.train_stars < 5 ? set.train_stars : 4) * 8 + 3, sy = T_OY + T_G - 30;
            T.fly.used = 0; if (set.train_stars < 255) set.train_stars++; settings_save();
            t_hit(T_PAD_BELL, 100); t_later(200, T_S_BELL); t_later(400, T_S_WHISTLE); T.wave = 3000; T.got = 2500;
            for (i = 0; i < 16; i++) t_spark(sx, sy, 40 * t_cos(i * 64) / 1024, 40 * t_sin(i * 64) / 1024, 30, 800, KIT_RGB(255, 216, 74), 0);
        }
    }
}

static void t_triggers(void);
static void t_step(void)
{
    const int32_t dt = T_STEP;
    int i;
    T.t += (uint32_t)dt;
    for (i = 0; i < T_NOFF; i++) if (T.off[i].used && (int32_t)(T.t - T.off[i].at) >= 0) { kit_track_note_off(T.off[i].trk, T.off[i].note); T.off[i].used = 0; }
    for (i = 0; i < 6; i++)
        if (T.sch[i].used && (int32_t)(T.t - T.sch[i].at) >= 0) {
            int w = T.sch[i].what;
            T.sch[i].used = 0;
            if (w == T_S_WOOF) kit_sfx(KIT_SFX_WOOF, 9);
            else if (w == T_S_HOWL) kit_sfx(KIT_SFX_HOWL, 6);
            else if (w == T_S_NEIGH) kit_sfx(KIT_SFX_NEIGH, 7);
            else if (w == T_S_BELL) t_hit(T_PAD_BELL, 90);
            else if (w == T_S_WHISTLE) t_whistle();
            else if (w == T_S_BOOM) {
                int x = 30 + t_rand(180), y = 34 + t_rand(56), c = t_rand(12), j;
                for (j = 0; j < 14; j++) t_spark(x, y, 36 * t_cos(j * 73) / 1024, 36 * t_sin(j * 73) / 1024, 22, 1300, U_COL[c], 1);
                t_hit(T_PAD_TOM, 80);
            }
        }
    if (T.listen) {                             /* PLAY: the song, in its own time */
        T.lat -= dt;
        if (T.lat <= 0) {
            const t_song_t *s = t_sg();
            int len = s->n[T.lk][1] * s->sixteenth;
            T.ln = s->n[T.lk][0]; T.lu = T.t + (uint32_t)(len * 92 / 100);
            t_play(T.lk, len * 92 / 100); T.lat += len; T.lk++; T.idx = T.lk % s->len;
            if (T.lk >= s->len) T.listen = 0;
        }
    }
    if (T.sneeze) { int32_t p = T.sneeze; T.sneeze += dt; t_sneeze_step(p); if (T.sneeze > 3000) T.sneeze = 0; }
    if (T.super > 0) {
        int32_t floor_v = (70 + 80 * (T.super > 2000 ? 2000 : T.super) / 2000) << 8;
        if (T.v < floor_v) T.v = floor_v;
        if (t_rand(100) < 70) t_puff(t_sx(T_STACK_X), t_sy(T_STACK_X, T_STACK_Y), U_COL[t_rand(12)], 3, -30, -40, 1200, 0, 1, 0, 0);
        for (i = 0; i < 2; i++) { int ox = t_rand(90), vx = -40 - t_rand(60), vy = -20 - t_rand(30), hot = t_rand(2); t_spark(T_OX + T_X0 + 10 + ox, 211, vx, vy, 120, 500, hot ? KIT_RGB(255, 225, 77) : KIT_RGB(255, 154, 58), 0); }
    }
    T.v = T.v * 979 / 1024; if (T.v < 128) T.v = 0;
    t_phases(dt);
    if (T.idx < t_sg()->late && !T.phase) for (i = 0; i < 5; i++) T.bloom[i] = 0;
    T.scroll += T.v * dt / 1000;
    T.wheel += (T.v >> 8) * 163 * dt / 8000;
    if (T.v == 0 && T.t % 6000 < 50 && !T.sneeze && !T.phase && !T.listen) {   /* waiting: a hiss of steam */
        t_hit(T_PAD_OPEN, 30);
        for (i = 0; i < 4; i++) t_puff(T_OX + T_X0 + 92 + i * 2, 204, KIT_RGB(232, 232, 244), 2, 24 + i * 6, -4, 900, 0, 0, 0, 6);
    }
    if (T.v > (30 << 8) && T.super <= 0 && t_rand(100) < 30) t_puff(t_sx(T_STACK_X), t_sy(T_STACK_X, T_STACK_Y), KIT_RGB(216, 216, 232), 2, -6, -26, 1000, 0, 0, 0, 5);
    for (i = 0; i < T_NP; i++)
        if (T.puff[i].used) {
            t_puff_t *p = &T.puff[i];
            if (p->wind) { p->vx += ((-(55 << 8)) - p->vx) * 2 * dt / 1000; p->vy = p->vy * 9 / 10; }
            else p->vy = p->vy * 96 / 100;
            p->x += (p->vx - T.v * 7 / 10) * dt / 1000; p->y += p->vy * dt / 1000; p->r += p->grow * dt / 1000; p->life -= (int16_t)dt;
            if (p->life <= 0 || p->x < -(30 << 8) || p->x > (280 << 8)) p->used = 0;
        }
    for (i = 0; i < T_NS; i++)
        if (T.spark[i].used) {
            t_spark_t *s = &T.spark[i];
            s->x += s->vx * dt / 1000; s->y += s->vy * dt / 1000; s->vy += s->fall * dt / 1000; s->life -= (int16_t)dt;
            if (s->life <= 0 || s->y > (250 << 8)) s->used = 0;
        }
    if (T.hat.used) { T.hat.x += T.hat.vx * dt / 1000; T.hat.y += T.hat.vy * dt / 1000; T.hat.vy += (160 << 8) * dt / 1000; if (T.hat.vy > 0 && (T.hat.y >> 8) >= t_sy(T_X0 + 11, T_G - 72)) T.hat.used = 0; }
    if (T.rider.used) {
        int32_t want = (T.super > 800 ? 120 : -80) << 8;
        T.rider.x += (want - T.rider.x) * (T.super > 800 ? 6 : 4) / 100;
        if (T.super <= 0 && T.rider.x < -(40 << 8)) T.rider.used = 0;
    }
    if (!T.rabbit.used && !T.phase && T.v > (15 << 8) && (T.next_rabbit -= dt) <= 0) { T.rabbit.used = 1; T.rabbit.x = -(14 << 8); T.rabbit.t = 0; T.rabbit.tired = 0; T.next_rabbit = 20000 + t_rand(10000); }
    if (T.rabbit.used) {
        int32_t rs;
        T.rabbit.t += dt;
        if (!T.rabbit.tired && (T.rabbit.t > 7000 || T.v > (95 << 8))) { T.rabbit.tired = 1; kit_sfx(KIT_SFX_CHIRP, 4); }
        rs = T.rabbit.tired ? 0 : T.v * 13 / 10 + (30 << 8); if (rs > (95 << 8)) rs = 95 << 8;
        T.rabbit.x += (rs - T.v * 13 / 10) * dt / 1000;
        if (!T.rabbit.tired && T.rabbit.x > (226 << 8)) T.rabbit.x = 226 << 8;
        if (T.rabbit.tired && T.rabbit.x < -(20 << 8)) T.rabbit.used = 0;
    }
    {                                           /* the tumbleweed: rolling with the wind, and by as the train moves */
        int32_t d = (int32_t)(T.t * 16 / 1000 % 4160) + (T.scroll >> 8) * 13 / 10;
        T.tw_x = (int32_t)(300 - t_wrap(d, 520)) << 8; T.tw_a = -d * 163 / 8;
    }
    { uint32_t h = t_hash(T.t / T_STEP); if (!T.shoot.used && h % 1000 < 7) { T.shoot.used = 1; T.shoot.x = (int32_t)(120 + (h >> 12) % 100) << 8; T.shoot.y = (int32_t)(8 + (h >> 20) % 30) << 8; } }
    if (T.shoot.used) { T.shoot.x -= (120 << 8) * dt / 1000; T.shoot.y += (50 << 8) * dt / 1000; if (T.shoot.x < (70 << 8)) T.shoot.used = 0; }
    { uint32_t h = t_hash(T.t / T_STEP + 777777u); if (!T.bats.used && h % 1000 < 5) { T.bats.used = 1; T.bats.x = 250 << 8; T.bats.y = (int32_t)(30 + (h >> 12) % 12) << 8; } }
    if (T.bats.used) { T.bats.x -= (40 << 8) * dt / 1000; if (T.bats.x < -(40 << 8)) T.bats.used = 0; }
    {
        int32_t *tm[] = { &T.cough, &T.soot, &T.wave, &T.super, &T.howl, &T.bell, &T.steam, &T.angry, &T.bark, &T.got, &T.pop, &T.cow_t, &T.dog_t[0], &T.dog_t[1], &T.owl_t };
        for (i = 0; i < (int)(sizeof tm / sizeof tm[0]); i++) if (*tm[i] > 0) *tm[i] -= dt;
    }
    for (i = 0; i < 5; i++) if (T.bloom[i] > 0) T.bloom[i] += dt;
    /* the engine's pose: rearing up as it sneezes, slamming down, shaking as it coughs, rocking as it runs */
    T.tilt = 0; T.jx = 0; T.jy = 0;
    if (T.sneeze > 0 && T.sneeze < 900) { T.tilt = 133 * T.sneeze / 900; if (T.sneeze > 600) T.jx = (T.t / 50) % 2 ? 1 : -1; }
    else if (T.sneeze >= 900 && T.sneeze < 1400) { int32_t r = 1400 - T.sneeze; T.tilt = -31 * r / 500; T.jx = -6 * r / 500; T.jy = 2 * r / 500; }
    if (T.cough > 0) T.jx += (T.t / 50) % 2 ? 1 : -1;
    if (T.v > (4 << 8) && !T.sneeze) T.jy += t_sin(T.wheel * 2) > 512 ? 1 : 0;
    t_triggers();
}

/* ---- drawing: helpers ---- */
#define TC(r, g, b) KIT_RGB(r, g, b)
static const kit_color_t T_OUT = TC(12, 10, 16), T_WHITE = TC(255, 255, 255), T_GOLDC = TC(255, 216, 74);
static int t_x(int wx, int par, int period, int margin)   /* a thing at wx (repeating), seen with parallax par/100 */
{
    int x = t_wrap(wx - (T.scroll >> 8) * par / 100 + margin, period) - margin;
    return x < KIT_W + margin ? x : -9999;
}
static void t_text(int x, int y, const char *s, int font, kit_color_t c, int center)
{
    static const int8_t RING[8][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 } };
    int j, al = center ? KIT_ALIGN_CENTER : KIT_ALIGN_LEFT;
    for (j = 0; j < 8; j++) kit_text(x + RING[j][0], y + RING[j][1], s, font, al, T_OUT);
    kit_text(x, y, s, font, al, c);
}
static void t_pixstar(int x, int y, kit_color_t c)   /* a small sheriff's star, 7 x 7 */
{
    static const int8_t S7[9][3] = { { 3, 0, 1 }, { 3, 1, 1 }, { 0, 2, 7 }, { 1, 3, 5 }, { 2, 4, 3 }, { 1, 5, 2 }, { 4, 5, 2 }, { 0, 6, 1 }, { 6, 6, 1 } };
    int i;
    for (i = 0; i < 9; i++) kit_rect(x + S7[i][0], y + S7[i][1], S7[i][2], 1, c);
}
static void t_bigstar(int cx, int cy, int r, kit_color_t c)   /* a big star: a disc and five points */
{
    int i;
    kit_circle(cx, cy, r * 45 / 100, c);
    for (i = 0; i < 5; i++) { int a = i * 205 - 256, k; for (k = 0; k < r; k++) { int w = (r - k) * 4 / r; kit_rect(cx + k * t_cos(a) / 1024 - w / 2, cy + k * t_sin(a) / 1024 - w / 2, w + 1, w + 1, c); } }
}

/* ---- the engine: drawn in parts, outlines first, each lifted as the front rears up ---- */
typedef struct { int16_t x, y, w, h; uint8_t circle; kit_color_t c; } t_part_t;
static t_part_t t_parts[40]; static int t_np;
static void t_pr(int x, int y, int w, int h, kit_color_t c) { if (t_np < 40) { t_part_t *p = &t_parts[t_np++]; p->x = (int16_t)x; p->y = (int16_t)y; p->w = (int16_t)w; p->h = (int16_t)h; p->circle = 0; p->c = c; } }
static void t_pc(int x, int y, int r, kit_color_t c) { if (t_np < 40) { t_part_t *p = &t_parts[t_np++]; p->x = (int16_t)x; p->y = (int16_t)y; p->w = (int16_t)r; p->h = 0; p->circle = 1; p->c = c; } }
static void t_er(int x, int y, int w, int h, kit_color_t c)   /* a rect on the engine, lifted strip by strip */
{
    int xs;
    if (!T.tilt) { kit_rect(t_sx(x), t_sy(x, y), w, h, c); return; }
    for (xs = x; xs < x + w; xs += 4) { int ww = x + w - xs < 4 ? x + w - xs : 4; kit_rect(t_sx(xs), t_sy(xs + ww / 2, y), ww, h, c); }
}
static void t_ec(int x, int y, int r, kit_color_t c) { kit_circle(t_sx(x), t_sy(x, y), r, c); }
static void t_el(int x0, int y0, int x1, int y1, kit_color_t c) { kit_line(t_sx(x0), t_sy(x0, y0), t_sx(x1), t_sy(x1, y1), c); }
static void t_flush(void)
{
    int i;
    for (i = 0; i < t_np; i++) { t_part_t *p = &t_parts[i]; if (p->circle) t_ec(p->x, p->y, p->w + 1, T_OUT); else t_er(p->x - 1, p->y - 1, p->w + 2, p->h + 2, T_OUT); }
    for (i = 0; i < t_np; i++) { t_part_t *p = &t_parts[i]; if (p->circle) t_ec(p->x, p->y, p->w, p->c); else t_er(p->x, p->y, p->w, p->h, p->c); }
    t_np = 0;
}
static void t_engine(void)
{
    const int G = T_G, x0 = T_X0, gold = T.super > 0;
    const kit_color_t body = gold ? TC(184, 134, 42) : TC(35, 34, 46), shine = gold ? TC(255, 233, 160) : TC(74, 74, 98),
        smoke = gold ? TC(138, 94, 26) : TC(23, 22, 30), trim = gold ? TC(255, 90, 58) : TC(200, 48, 42), trimd = TC(122, 26, 24),
        brass = gold ? TC(255, 240, 176) : TC(216, 168, 58), brassl = gold ? T_WHITE : TC(255, 224, 138),
        wood = TC(138, 90, 42), woodd = TC(90, 56, 32), logc = TC(154, 106, 58), rod = TC(200, 200, 216);
    static const int8_t LOGS[9][2] = { { 7, 47 }, { 14, 47 }, { 21, 47 }, { 28, 47 }, { 35, 47 }, { 10, 53 }, { 17, 53 }, { 24, 53 }, { 31, 53 } };
    int i, k, a = T.wheel, sooty = T.soot > 0 || (T.sneeze > 900 && T.sneeze < 3000), cross = T.angry > 0;
    /* the dark undercarriage first, so nothing behind shows through; the tender; the frame, cab, boiler, domes, stack,
     * lamp, cylinder; the wheels */
    t_pr(x0 + 26, G - 40, 76, 14, smoke); t_pr(x0 + 2, G - 24, 100, 15, smoke); t_pr(3, G - 16, 40, 8, smoke); t_pr(42, G - 30, 8, 7, smoke);
    t_pr(2, G - 40, 42, 26, wood); t_pr(0, G - 43, 46, 4, woodd); t_pr(2, G - 15, 42, 3, shine);
    for (i = 0; i < 9; i++) t_pc(LOGS[i][0], G - LOGS[i][1], 3, logc);
    t_pc(11, G - 7, 7, trim); t_pc(35, G - 7, 7, trim);
    t_pr(x0 - 2, G - 27, 108, 5, trim);
    t_pr(x0, G - 80, 30, 55, trim); t_pr(x0 - 4, G - 84, 38, 5, body);
    t_pr(x0 + 28, G - 64, 70, 27, body); t_pr(x0 + 96, G - 66, 8, 31, smoke);
    t_pr(x0 + 44, G - 73, 11, 9, brass); t_pr(x0 + 68, G - 71, 9, 7, brass);
    t_pr(x0 + 84, G - 78, 9, 14, body); t_pr(x0 + 77, G - 96, 23, 3, body);
    t_pr(x0 + 91, G - 82, 15, 13, body); t_pr(x0 + 70, G - 37, 21, 11, shine);
    t_pc(x0 + 18, G - 14, 14, trim); t_pc(x0 + 47, G - 14, 14, trim); t_pc(x0 + 81, G - 6, 6, trim); t_pc(x0 + 95, G - 6, 6, trim);
    t_flush();
    for (k = 0; k < 15; k++) { int w = 21 - k * 85 / 100; t_er(x0 + 89 - w / 2, G - 93 + k, w, 1, body); }   /* the balloon stack */
    t_er(x0 + 77, G - 93, 1, 15, T_OUT); t_er(x0 + 99, G - 93, 1, 15, T_OUT);
    /* the details */
    for (k = 0; k < 4; k++) t_er(2, G - 34 + k * 6, 42, 1, woodd);   /* planks, and the stars it has won */
    for (i = 0; i < (set.train_stars < 5 ? set.train_stars : 5); i++) { t_pixstar(t_sx(4 + i * 8), t_sy(4, G - 32), T_OUT); t_pixstar(t_sx(4 + i * 8), t_sy(4, G - 33), T_GOLDC); }
    for (i = 0; i < 9; i++) { t_ec(LOGS[i][0], G - LOGS[i][1], 2, TC(200, 148, 90)); t_er(LOGS[i][0], G - LOGS[i][1], 1, 1, woodd); }
    t_er(x0 + 28, G - 62, 70, 2, shine);
    t_er(x0 + 40, G - 64, 2, 27, brass); t_er(x0 + 62, G - 64, 2, 27, brass); t_er(x0 + 82, G - 64, 2, 27, brass);
    if (gold) { int bx = x0 + 28 + (int)(T.t * 90 / 1000 % 90); if (bx < x0 + 94) { t_er(bx, G - 63, 3, 25, T_WHITE); t_er(bx + 4, G - 63, 1, 25, brassl); } }
    t_er(x0 + 45, G - 72, 9, 2, brassl); t_er(x0 + 69, G - 70, 7, 2, brassl);
    {                                           /* the bell, swinging when it rings */
        int sw = T.bell > 0 ? 2 * t_sin((int32_t)(T.t * 18 * 1024 / 6283)) / 1024 : 0;
        t_er(x0 + 58, G - 74, 2, 3, T_OUT); t_ec(x0 + 59 + sw, G - 69, 4, T_OUT); t_ec(x0 + 59 + sw, G - 69, 3, brass); t_er(x0 + 56 + sw, G - 66, 7, 2, brass);
    }
    t_er(x0 + 32, G - 70, 3, 6, T_OUT); t_er(x0 + 33, G - 69, 1, 5, brass);   /* the whistle */
    t_er(x0 + 79, G - 95, 19, 1, shine); t_er(x0 + 84, G - 76, 2, 12, shine);
    t_er(x0 + 93, G - 80, 11, 9, brass); t_er(x0 + 103, G - 79, 3, 7, TC(255, 246, 200)); t_er(x0 + 95, G - 78, 6, 5, TC(255, 242, 176));   /* the lamp */
    t_er(x0 + 70, G - 35, 21, 1, brassl); t_er(x0 + 70, G - 28, 21, 1, smoke);
    t_er(x0 + 2, G - 78, 26, 1, TC(224, 120, 112));   /* the cab: its window, the driver, the number */
    t_er(x0 + 6, G - 74, 17, 15, T_OUT); t_er(x0 + 7, G - 73, 15, 13, TC(255, 210, 122)); t_er(x0 + 14, G - 73, 1, 13, T_OUT);
    t_er(x0 + 8, G - 68, 6, 6, sooty ? TC(42, 34, 40) : cross ? TC(240, 138, 114) : TC(241, 199, 163));
    if (sooty) { t_er(x0 + 9, G - 66, 1, 1, T_WHITE); t_er(x0 + 12, G - 66, 1, 1, T_WHITE); }
    else { int bl = T.t % 3300 < 120; t_er(x0 + 9, G - 66, 1, bl ? 1 : 2, T_OUT); t_er(x0 + 12, G - 66, 1, bl ? 1 : 2, T_OUT); }
    if (cross) {                                /* frowning, a fist shaking */
        int sh = (T.t / 50) % 2 ? 1 : -1;
        t_er(x0 + 8, G - 68, 2, 1, T_OUT); t_er(x0 + 12, G - 68, 2, 1, T_OUT); t_er(x0 + 9, G - 63, 4, 1, TC(90, 26, 26));
        t_er(x0 + 17, G - 72 + sh, 4, 4, T_OUT); t_er(x0 + 18, G - 71 + sh, 3, 3, TC(241, 199, 163)); t_er(x0 + 18, G - 68 + sh, 2, 3, TC(58, 90, 168));
    }
    t_er(x0 + 8, G - 62, 6, 2, TC(200, 48, 42));
    if (T.cough > 0) t_er(x0 + 10, G - 64, 4, 2, TC(241, 199, 163));
    if (!T.hat.used) { t_er(x0 + 7, G - 71, 8, 3, TC(58, 90, 168)); t_er(x0 + 7, G - 70, 8, 1, TC(232, 232, 248)); t_er(x0 + 13, G - 69, 3, 1, TC(58, 90, 168)); }
    if (T.wave > 0 && !cross) t_er(x0 + 15, G - 70 - (int)((T.t / 70) % 3), 3, 6, TC(241, 199, 163));
    t_er(x0 + 8, G - 50, 14, 9, brass); t_er(x0 + 9, G - 49, 12, 7, trimd); t_er(x0 + 12, G - 48, 6, 1, brassl); t_el(x0 + 17, G - 48, x0 + 14, G - 43, brassl);
    for (i = 0; i < 17; i++) { int h = 21 - i * 125 / 100; t_er(x0 + 101 + i, G - 22, 1, h + 1, T_OUT); }   /* the cowcatcher */
    for (i = 0; i < 16; i++) { int h = 21 - i * 125 / 100 - 1; t_er(x0 + 101 + i, G - 21, 1, h, i % 3 == 2 ? trimd : trim); }
    t_er(x0 + 100, G - 23, 18, 2, brass);
    {                                           /* the wheels' spokes turning, the rods */
        static const int16_t WH[6][3] = { { T_X0 + 18, T_G - 14, 14 }, { T_X0 + 47, T_G - 14, 14 }, { T_X0 + 81, T_G - 6, 6 }, { T_X0 + 95, T_G - 6, 6 }, { 11, T_G - 7, 7 }, { 35, T_G - 7, 7 } };
        int cx1, cy1, cx2, cy2, chx;
        for (i = 0; i < 6; i++) {
            int wx = WH[i][0], wy = WH[i][1], rr = WH[i][2], ns = rr > 8 ? 10 : 6, s;
            t_ec(wx, wy, rr - 2, trimd);
            for (s = 0; s < ns; s++) { int an = a * 14 / rr + s * 1024 / ns; t_el(wx + 2 * t_cos(an) / 1024, wy + 2 * t_sin(an) / 1024, wx + (rr - 2) * t_cos(an) / 1024, wy + (rr - 2) * t_sin(an) / 1024, trim); }
            t_ec(wx, wy, rr > 8 ? 3 : 2, brass);
        }
        cx1 = x0 + 18 + 8 * t_cos(a) / 1024; cy1 = G - 14 + 8 * t_sin(a) / 1024; cx2 = x0 + 47 + 8 * t_cos(a) / 1024; cy2 = G - 14 + 8 * t_sin(a) / 1024;
        t_er(cx1 - 1, cy1 - 2, cx2 - cx1 + 3, 4, T_OUT); t_er(cx1, cy1 - 1, cx2 - cx1 + 1, 2, rod);
        chx = x0 + 66 + 8 * t_cos(a) / 1024;
        t_er(chx, G - 33, x0 + 70 - chx, 3, rod); t_er(chx - 2, G - 34, 4, 5, brass);
        t_el(chx, G - 32, cx2, cy2, T_OUT); t_el(chx, G - 31, cx2, cy2 + 1, rod); t_el(chx + 1, G - 31, cx2 + 1, cy2 + 1, rod);
        t_er(cx1 - 1, cy1 - 1, 2, 2, brass); t_er(cx2 - 1, cy2 - 1, 2, 2, brass);
    }
}
static void t_puppy(void)                       /* the puppy riding on the logs */
{
    int x = T_OX + 23 + T.jx, y = T_OY + T_G - 58 + T.jy, ear = T.v > (25 << 8) || T.super > 0 ? -4 : 0;
    int barking = T.bark > 0 && (T.bark / 150) % 3 != 0;
    if (T.bark > 0) y -= (T.bark / 120) % 2 ? 4 : 0;
    kit_circle(x, y, 8, T_OUT); kit_circle(x, y, 7, TC(232, 216, 184)); kit_circle(x - 1, y + 7, 6, T_OUT); kit_circle(x - 1, y + 7, 5, TC(232, 216, 184));
    kit_rect(x - 10, y - 4 + ear, 4, 9, T_OUT); kit_rect(x + 6, y - 4 + ear, 4, 9, T_OUT); kit_rect(x - 9, y - 3 + ear, 2, 8, TC(122, 74, 42)); kit_rect(x + 7, y - 3 + ear, 2, 8, TC(122, 74, 42));
    if (T.soot > 0) {
        int b = T.t % 900 < 150;
        kit_circle(x, y, 7, TC(30, 26, 30));
        kit_rect(x - 4, y - 2, 3, 3, b ? T_OUT : T_WHITE); kit_rect(x + 2, y - 2, 3, 3, b ? T_OUT : T_WHITE);
        if (!b) { kit_rect(x - 3, y - 1, 2, 2, T_OUT); kit_rect(x + 3, y - 1, 2, 2, T_OUT); }
        if (barking) kit_rect(x - 2, y + 5, 5, 3, TC(255, 107, 138));
        return;
    }
    kit_circle(x + 3, y - 2, 3, TC(176, 122, 68)); kit_circle(x, y + 2, 3, TC(255, 244, 224));
    { int bl = (T.t + 1300) % 3700 < 120; kit_rect(x - 3, y - 2, 2, bl ? 1 : 2, T_OUT); kit_rect(x + 2, y - 2, 2, bl ? 1 : 2, T_OUT); }
    kit_rect(x - 1, y + 1, 3, 2, T_OUT);
    if (barking) { kit_rect(x - 2, y + 4, 5, 3, TC(90, 26, 26)); kit_rect(x - 1, y + 6, 3, 1, TC(255, 107, 138)); }
    else if (T.wave > 0 || T.super > 0) kit_rect(x, y + 3, 3, 3 + (int)((T.t / 80) % 3), TC(255, 107, 138));
    if (T.super > 0) kit_rect(x - 6, y - 12 - (int)((T.t / 60) % 4), 2, 5, TC(232, 216, 184));
}

/* ---- the world around the line ---- */
static void t_cactus(int x, int base, int h, int id, int lit)
{
    const kit_color_t c = TC(30, 74, 50), d = TC(22, 58, 38), hi = TC(46, 106, 70);
    if (x < -40 || x > KIT_W + 40) return;
    (void)lit;
    kit_rect(x - 3, base - h - 1, 7, h + 1, T_OUT); kit_rect(x - 2, base - h, 5, h, c); kit_rect(x - 2, base - h, 1, h, d); kit_rect(x + 1, base - h + 1, 1, h - 2, hi);
    kit_rect(x - 8, base - h * 7 / 10, 4, 9, T_OUT); kit_rect(x - 7, base - h * 7 / 10, 2, 8, c); kit_rect(x - 7, base - h * 7 / 10 + 6, 6, 2, c);
    kit_rect(x + 4, base - h * 55 / 100, 4, 8, T_OUT); kit_rect(x + 5, base - h * 55 / 100, 2, 7, c); kit_rect(x + 2, base - h * 55 / 100 + 5, 5, 2, c);
    if (id >= 0 && T.bloom[id]) {               /* a flower opening on its top and on an arm */
        int p = T.bloom[id] > 700 ? 1024 : T.bloom[id] * 1024 / 700, f, i;
        for (f = 0; f < 2; f++) {
            int fx = f ? x - 6 : x + 1, fy = f ? base - h * 7 / 10 - 2 : base - h - 2, rr = 1 + p * (f ? 2 : 3) / 1024;
            for (i = 0; i < 5; i++) { int an = i * 205 - 256; kit_circle(fx + rr * t_cos(an) / 1024, fy + rr * t_sin(an) / 1024, 1 + p / 1024, i % 2 ? TC(255, 107, 198) : TC(255, 138, 216)); }
            kit_circle(fx, fy, p > 512 ? 1 : 0, TC(255, 225, 77));
            if (p < 1024) for (i = 0; i < 4; i++) kit_pixel(fx + 6 * t_cos(i * 260 + (int32_t)T.t) / 1024, fy + 6 * t_sin(i * 260 + (int32_t)T.t) / 1024, T_WHITE);
        }
    }
}
static int t_ahead(int x) { return x > 186 && x < 236; }   /* in the lamplight, just ahead of the engine */
static void t_triggers(void)                    /* what the lamplight and the train's coming set off, step by step */
{
    static const int16_t CA[4][2] = { { 40, 85 }, { 220, 85 }, { 380, 85 }, { 560, 85 } };
    int i, x, late = T.idx >= t_sg()->late || T.phase, lit;
    x = t_x(560, 85, 700, 60); lit = x != -9999 && t_ahead(x);
    if (lit && !T.cow_was) { T.cow_t = 2200; kit_sfx(KIT_SFX_MOO, 8); }
    T.cow_was = lit;
    for (i = 0; i < 2; i++) {                   /* the prairie dogs hear it coming */
        int near;
        x = t_x(i ? 300 : 90, 130, 520, 40);
        near = x != -9999 && !t_in_canyon(x) && !t_in_canyon(x + 14) && x > 140 && x < 240;
        if (near && !T.dog_was[i]) { T.dog_t[i] = 2000; kit_sfx(KIT_SFX_CHIRP, 5); }
        T.dog_was[i] = near;
    }
    {                                           /* the owl hoots as it goes by */
        int off = t_wrap(T.scroll >> 8, 170), k;
        for (k = 0; k < 4; k++) {
            int px = (k - 1) * 170 - off + 40, idx = ((T.scroll >> 8) + px) / 170;
            if (idx % 3 == 1 && px > 40 && px < 190 && T.owl_last != idx) { T.owl_last = idx; T.owl_t = 1600; kit_sfx(KIT_SFX_HOOT, 7); }
        }
    }
    for (i = 0; i < 5; i++) {                   /* near the song's end, each cactus the lamp lights opens */
        if (T.bloom[i] || !late) continue;
        if (i < 4) { x = t_x(CA[i][0], CA[i][1], 700, 40); lit = x != -9999 && t_ahead(x); }
        else { x = t_x(220, 130, 520, 40); lit = x != -9999 && !t_in_canyon(x) && x > 150 && x < 236; }
        if (lit) { T.bloom[i] = 1; t_hit(T_PAD_BELL, 40); }
    }
}
static void t_sky(void)
{
    static const kit_color_t SKY[9] = { TC(10, 8, 38), TC(16, 12, 58), TC(24, 18, 74), TC(36, 26, 86), TC(52, 32, 90), TC(74, 38, 92), TC(98, 42, 90), TC(128, 50, 90), TC(160, 62, 90) };
    int i;
    for (i = 0; i < 9; i++) kit_rect(0, i * 18, KIT_W, 18, SKY[i]);
    for (i = 0; i < 56; i++) kit_pixel((i * 53 + (i >> 3) * 7) % 240, (i * 31) % 118, ((T.t / 400 + (uint32_t)i * 3) % 9) == 0 ? TC(90, 90, 144) : i % 7 ? TC(232, 224, 255) : TC(255, 232, 176));
    if (T.shoot.used) { int k, sx = T.shoot.x >> 8, sy = T.shoot.y >> 8; for (k = 0; k < 12; k++) kit_pixel(sx + k * 2, sy - k, k < 2 ? T_WHITE : TC(168, 168, 224)); }
    kit_circle(58, 50, 38, TC(40, 30, 80)); kit_circle(58, 50, 32, TC(70, 56, 104));   /* the huge moon and its halo */
    kit_circle(58, 50, 27, TC(255, 243, 208));
    kit_circle(49, 44, 6, TC(239, 226, 188)); kit_circle(66, 59, 5, TC(239, 226, 188)); kit_circle(65, 40, 3, TC(239, 226, 188)); kit_circle(52, 62, 2, TC(239, 226, 188));
    if (T.bats.used) {                          /* bats across the moon */
        for (i = 0; i < 3; i++) {
            int bx = (T.bats.x >> 8) + i * 14, by = (T.bats.y >> 8) + i * 7 + 2 * t_sin((int32_t)T.t / 2 + i * 300) / 1024, up = (T.t / 110 + (uint32_t)i) % 2;
            kit_rect(bx - 1, by, 3, 2, T_OUT);
            kit_line(bx - 5, by + (up ? -3 : 2), bx - 1, by, T_OUT); kit_line(bx + 5, by + (up ? -3 : 2), bx + 1, by, T_OUT);
        }
    }
}
static void t_mesa(int wx, int w, int top, kit_color_t c, kit_color_t c2)
{
    int x0 = t_x(wx, 6, 640, 140), k;
    if (x0 == -9999) return;
    for (k = 0; k < w; k++) {
        int x = x0 + k, e = k < w - k ? k : w - k, y = top + (e < 7 ? (7 - e) * 3 : 0);
        if (x < 0 || x >= KIT_W) continue;
        kit_rect(x, y, 1, 162 - y, (e > 7 && k % 9 == 0) ? c2 : c);
    }
}
static void t_hills(void)
{
    int x, ho = (T.scroll >> 8) / 5, k;
    t_mesa(140, 90, 102, TC(74, 32, 72), TC(60, 26, 59)); t_mesa(250, 50, 116, TC(62, 28, 64), TC(50, 22, 52));
    t_mesa(430, 76, 110, TC(74, 32, 72), TC(60, 26, 59)); t_mesa(540, 40, 122, TC(62, 28, 64), TC(50, 22, 52));
    for (x = 0; x < KIT_W; x++) { int y = 148 + 4 * t_sin((x + ho) * 1024 / 214) / 1024 + 3 * t_sin((x + ho) * 1024 / 82) / 2048; kit_rect(x, y, 1, 18, TC(44, 21, 48)); }
    x = t_x(60, 20, 520, 60);                   /* the coyote on its rock */
    if (x != -9999) {
        static const int16_t C[8][4] = { { 2, 128, 14, 8 }, { 14, 120, 5, 10 }, { 16, 116, 4, 4 }, { 18, 112, 2, 4 }, { 0, 130, 3, 8 }, { 12, 134, 3, 4 }, { 4, 134, 2, 4 }, { -2, 128, 4, 2 } };
        int up = T.howl > 0;
        kit_rect(x - 6, 142, 36, 10, TC(26, 14, 26)); kit_rect(x - 2, 138, 28, 6, TC(26, 14, 26));
        for (k = 0; k < 8; k++) { int dy = k == 1 ? -up * 4 : k == 2 ? -up * 6 : k == 3 ? -up * 8 : 0; kit_rect(x + C[k][0] + (k == 3 ? up : 0), C[k][1] + dy, C[k][2], C[k][3] + (k == 1 ? up * 2 : 0), TC(26, 14, 26)); }
        if (T.howl > 0) t_text(x + 20, 96 + (1600 - T.howl) / 300, "ow-oo", KIT_FONT_XSMALL, TC(232, 208, 255), 0);
    }
}
static void t_ranch(void)                       /* a ranch house, its window lit, the windmill turning */
{
    int x = t_x(150, 40, 560, 80), wx, b, lit;
    if (x == -9999) return;
    kit_rect(x - 1, 140, 40, 25, T_OUT); kit_rect(x, 141, 38, 23, TC(90, 52, 40));
    for (b = 0; b < 5; b++) kit_rect(x, 144 + b * 4, 38, 1, TC(74, 42, 32));
    for (b = 0; b < 12; b++) kit_rect(x - 3 + b * 2, 141 - b, 44 - b * 4 > 0 ? 44 - b * 4 : 1, 1, b ? TC(58, 30, 26) : T_OUT);
    lit = T.t % 11000 < 8500;
    kit_rect(x + 6, 148, 9, 8, T_OUT); kit_rect(x + 7, 149, 7, 6, lit ? TC(255, 207, 106) : TC(42, 32, 64)); kit_rect(x + 10, 149, 1, 6, T_OUT);
    kit_rect(x + 24, 150, 8, 14, T_OUT); kit_rect(x + 25, 151, 6, 13, TC(58, 30, 22));
    wx = x + 56;
    kit_line(wx - 7, 165, wx - 1, 110, TC(42, 26, 32)); kit_line(wx + 7, 165, wx + 1, 110, TC(42, 26, 32));
    for (b = 0; b < 4; b++) kit_rect(wx - 6 + b * 13 / 10, 152 - b * 13, 12 - b * 26 / 10, 1, TC(42, 26, 32));
    {
        int32_t ang = (int32_t)(T.t * (12 + (T.v >> 8) / 3) / 60);
        for (b = 0; b < 12; b++) { int an = ang + b * 85; kit_line(wx, 108, wx + 13 * t_cos(an) / 1024, 108 + 13 * t_sin(an) / 1024, b % 2 ? TC(74, 58, 72) : TC(94, 74, 88)); }
    }
    kit_circle(wx, 108, 2, TC(122, 106, 120)); kit_rect(wx, 106, 14, 2, TC(42, 26, 32)); kit_rect(wx + 12, 102, 5, 8, TC(74, 58, 72));
}
static void t_ground(void)
{
    int k, fo = t_wrap((T.scroll >> 8) * 85 / 100, 48);
    kit_rect(0, 162, KIT_W, 78, TC(58, 30, 48));
    for (k = -1; k < 7; k++) { kit_rect(k * 48 - fo + 7, 172, 3, 1, TC(74, 42, 58)); kit_rect(k * 48 - fo + 30, 184, 2, 1, TC(74, 42, 58)); kit_rect(k * 48 - fo + 18, 198, 4, 1, TC(74, 42, 58)); }
    {
        static const int16_t CA[4][3] = { { 40, 192, 26 }, { 220, 186, 18 }, { 380, 188, 22 }, { 560, 194, 28 } };
        for (k = 0; k < 4; k++) { int x = t_x(CA[k][0], 85, 700, 40); if (x != -9999) t_cactus(x, CA[k][1], CA[k][2], k, t_ahead(x)); }
    }
}
static void t_camp(void)                        /* a covered wagon, a campfire, a cowboy with a guitar, his horse */
{
    int x = t_x(300, 60, 760, 120), gy = 176, fx, cx, hx, i, fl;
    if (x == -9999) return;
    fl = (int)((T.t / 60) % 3) - 1; fx = x + 62;
    kit_circle(fx, gy - 4, 20 + fl, TC(84, 40, 44));    /* the firelight on the ground round them */
    kit_rect(x - 1, gy - 19, 46, 12, T_OUT); kit_rect(x, gy - 18, 44, 10, TC(122, 74, 36)); kit_rect(x, gy - 18, 44, 2, TC(154, 106, 58));
    for (i = 2; i < 43; i++) { int h = 15 + 4 * t_sin((i - 2) * 512 / 40) / 1024; kit_rect(x + i, gy - 18 - h, 1, h, i % 9 == 2 ? TC(200, 184, 152) : i < 8 ? TC(216, 200, 168) : TC(240, 230, 208)); kit_pixel(x + i, gy - 19 - h, T_OUT); }
    kit_rect(x + 1, gy - 33, 2, 15, T_OUT); kit_rect(x + 41, gy - 33, 2, 15, T_OUT); kit_circle(x + 22, gy - 26, 5, TC(232, 216, 184)); kit_circle(x + 22, gy - 26, 3, TC(42, 26, 20));
    for (i = 0; i < 2; i++) {
        int wx = x + (i ? 36 : 8), s;
        kit_circle(wx, gy - 6, 7, T_OUT); kit_circle(wx, gy - 6, 6, TC(106, 68, 36)); kit_circle(wx, gy - 6, 4, TC(42, 26, 20));
        for (s = 0; s < 6; s++) kit_line(wx, gy - 6, wx + 5 * t_cos(s * 171) / 1024, gy - 6 + 5 * t_sin(s * 171) / 1024, TC(138, 90, 42));
    }
    for (i = -3; i <= 3; i++) kit_rect(fx + i * 3 - 1, gy - 3, 3, 3, i % 2 ? TC(90, 80, 96) : TC(106, 96, 112));   /* the fire */
    kit_rect(fx - 6, gy - 5, 12, 2, TC(90, 58, 42));
    kit_circle(fx, gy - 8, 4 + fl, TC(255, 90, 26)); kit_circle(fx, gy - 9 - fl, 2, TC(255, 184, 74)); kit_rect(fx - 1, gy - 14 - fl, 2, 4, TC(255, 225, 77));
    for (i = 0; i < 5; i++) { int sy = (int)((T.t / 40 + (uint32_t)i * 9) % 26); kit_pixel(fx - 3 + (i * 7) % 7 + (sy / 6) % 2, gy - 14 - sy, sy < 14 ? TC(255, 225, 77) : TC(255, 122, 58)); }
    cx = fx + 14;                               /* the cowboy on a log, strumming */
    kit_rect(cx - 5, gy - 4, 12, 4, T_OUT); kit_rect(cx - 4, gy - 3, 10, 2, TC(106, 68, 36));
    kit_rect(cx - 3, gy - 17, 8, 12, T_OUT); kit_rect(cx - 2, gy - 16, 6, 10, TC(184, 48, 42)); kit_rect(cx - 3, gy - 6, 4, 6, TC(58, 74, 122)); kit_rect(cx + 2, gy - 6, 5, 3, TC(58, 74, 122));
    kit_circle(cx + 1, gy - 20, 4, T_OUT); kit_circle(cx + 1, gy - 20, 3, TC(232, 176, 136)); kit_rect(cx - 4, gy - 23, 11, 2, TC(138, 90, 42)); kit_rect(cx - 1, gy - 27, 5, 4, TC(138, 90, 42));
    kit_circle(cx - 4, gy - 10, 3, TC(200, 138, 58)); kit_rect(cx - 4, gy - 11, 12, 1, TC(90, 58, 26));
    kit_rect(cx - 2, gy - 12 + (int)((T.t / 120) % 3) - 1, 2, 3, TC(232, 176, 136));
    hx = fx + 40;                               /* the horse, tied up, its tail swishing */
    kit_rect(hx - 9, gy - 21, 22, 10, T_OUT); kit_rect(hx - 8, gy - 20, 20, 8, TC(90, 52, 36)); kit_rect(hx + 10, gy - 30, 6, 12, TC(90, 52, 36)); kit_rect(hx + 11, gy - 33, 8, 5, TC(90, 52, 36));
    kit_rect(hx + 17, gy - 31, 3, 3, TC(58, 32, 20)); kit_rect(hx + 11, gy - 35, 2, 3, TC(90, 52, 36)); kit_rect(hx + 9, gy - 31, 2, 10, TC(42, 24, 16)); kit_pixel(hx + 14, gy - 32, T_OUT);
    kit_rect(hx - 7, gy - 12, 2, 12, TC(74, 42, 28)); kit_rect(hx - 3, gy - 12, 2, 12, TC(74, 42, 28)); kit_rect(hx + 6, gy - 12, 2, 12, TC(74, 42, 28)); kit_rect(hx + 10, gy - 12, 2, 12, TC(74, 42, 28));
    kit_line(hx - 9, gy - 19, hx - 12 + 2 * t_sin((int32_t)T.t / 2) / 1024, gy - 8, TC(42, 24, 16));
    kit_rect(hx + 26, gy - 18, 2, 18, TC(74, 46, 34)); kit_line(hx + 17, gy - 30, hx + 26, gy - 14, TC(200, 168, 120));
}
static void t_cow(void)                         /* a longhorn grazing; the lamplight makes her look up and moo */
{
    int x = t_x(560, 85, 700, 60), lit, up, gy = 190, hx, hy;
    if (x == -9999) return;
    (void)lit; up = T.cow_t > 0;
    kit_rect(x - 13, gy - 16, 26, 12, T_OUT); kit_rect(x - 12, gy - 15, 24, 10, TC(232, 220, 200)); kit_circle(x - 5, gy - 11, 4, TC(106, 58, 36)); kit_circle(x + 6, gy - 13, 3, TC(106, 58, 36));
    kit_rect(x - 11, gy - 5, 2, 7, TC(200, 184, 160)); kit_rect(x - 7, gy - 5, 2, 7, TC(200, 184, 160)); kit_rect(x + 6, gy - 5, 2, 7, TC(200, 184, 160)); kit_rect(x + 10, gy - 5, 2, 7, TC(200, 184, 160));
    hx = x + 13; hy = up ? gy - 21 : gy - 9;
    kit_rect(hx - 1, hy - 1, 9, 9, T_OUT); kit_rect(hx, hy, 7, 7, TC(232, 220, 200)); kit_rect(hx + 3, hy + 5, 4, 3, TC(232, 168, 160));
    kit_rect(hx - 7, hy - 3, 21, 2, T_OUT); kit_rect(hx - 6, hy - 2, 19, 1, TC(240, 232, 208)); kit_rect(hx - 7, hy - 4, 2, 2, TC(240, 232, 208)); kit_rect(hx + 12, hy - 4, 2, 2, TC(240, 232, 208));
    kit_pixel(hx + 2, hy + 2, T_OUT); kit_pixel(hx + 5, hy + 2, T_OUT);
    kit_line(x - 13, gy - 14, x - 16 + 2 * t_sin((int32_t)T.t * 2 / 5) / 1024, gy - 6, TC(200, 184, 160));
    if (T.cow_t > 1000) t_text(hx - 4, hy - 16, "MOO!", KIT_FONT_SMALL, T_WHITE, 0);
}
static void t_poles(void)                       /* telegraph poles going by, their wires, an owl that hoots as we pass */
{
    const int P = 170, base = 206, top = 88;
    int off = t_wrap(T.scroll >> 8, P), k, x, xs[4];
    for (k = 0; k < 4; k++) xs[k] = (k - 1) * P - off + 40;
    for (k = 0; k < 3; k++)
        for (x = xs[k] < 0 ? 0 : xs[k]; x < (xs[k + 1] > KIT_W ? KIT_W : xs[k + 1]); x++) {
            int t2 = (x - xs[k]) * 1024 / P, sag = 8 * (1024 * 1024 - (2 * t2 - 1024) * (2 * t2 - 1024) / 1) / (1024 * 1024);
            kit_pixel(x, top + 5 + sag, TC(74, 58, 88)); kit_pixel(x, top + 10 + sag, TC(74, 58, 88));
        }
    for (k = 0; k < 4; k++) {
        int px = xs[k], idx, ix;
        if (px < -20 || px > KIT_W + 20) continue;
        kit_rect(px - 2, top, 5, base - top, T_OUT); kit_rect(px - 1, top, 3, base - top, TC(74, 46, 34)); kit_rect(px - 12, top + 4, 25, 3, T_OUT); kit_rect(px - 11, top + 5, 23, 1, TC(90, 58, 42));
        for (ix = 0; ix < 4; ix++) kit_rect(px - 10 + ix * 6 + (ix > 1 ? 2 : 0), top + 2, 2, 3, TC(138, 200, 192));
        idx = ((T.scroll >> 8) + px) / P;
        if (idx % 3 == 1) {                     /* the owl */
            int oy = top - 9, bl = (T.t + (uint32_t)idx * 700) % 4100 < 150, ex;
            kit_circle(px, oy, 6, T_OUT); kit_circle(px, oy, 5, TC(122, 90, 58)); kit_circle(px, oy + 4, 4, TC(154, 122, 90));
            kit_rect(px - 5, oy - 7, 2, 3, TC(122, 90, 58)); kit_rect(px + 4, oy - 7, 2, 3, TC(122, 90, 58));
            for (ex = -2; ex <= 2; ex += 4) { kit_circle(px + ex, oy - 1, 2, TC(255, 216, 74)); if (bl) kit_rect(px + ex - 1, oy - 1, 3, 1, TC(122, 90, 58)); else kit_pixel(px + ex, oy - 1, T_OUT); }
            kit_rect(px, oy + 1, 1, 2, TC(232, 168, 58));
            if (T.owl_t > 600 && T.owl_last == idx) t_text(px + 8, oy - 14, "hoo", KIT_FONT_XSMALL, TC(232, 224, 255), 0);
        }
    }
}
static void t_station(void)                     /* Star Station, where the song's end brings the train */
{
    int x, k, cheer, sx1, sy = 197, lx2, kx, jump, wv;
    if (!T.station) return;
    x = T.station_wx - (T.scroll >> 8);
    if (x < -300) { if (!T.phase) T.station = 0; return; }
    if (x > KIT_W + 200) return;
    kit_rect(x - 170, 197, 270, 9, T_OUT); kit_rect(x - 169, 198, 268, 7, TC(122, 74, 42)); for (k = -168; k < 98; k += 6) kit_rect(x + k, 198, 1, 7, TC(90, 56, 32));
    kit_rect(x - 1, 117, 86, 81, T_OUT); kit_rect(x, 140, 84, 57, TC(138, 90, 58)); for (k = 0; k < 9; k++) kit_rect(x, 144 + k * 6, 84, 1, TC(106, 64, 36));
    kit_rect(x + 4, 118, 76, 22, TC(154, 106, 64)); kit_rect(x + 2, 116, 80, 3, TC(90, 56, 32));
    kit_rect(x + 8, 121, 68, 13, TC(42, 24, 16)); kit_text(x + 14, 123, "STAR", KIT_FONT_SMALL, KIT_ALIGN_LEFT, T_GOLDC); t_pixstar(x + 64, 124, T_GOLDC);
    for (k = 0; k < 2; k++) { int wx = x + (k ? 58 : 10); kit_rect(wx, 152, 16, 14, T_OUT); kit_rect(wx + 1, 153, 14, 12, TC(255, 207, 106)); kit_rect(wx + 8, 153, 1, 12, T_OUT); kit_rect(wx + 1, 159, 14, 1, T_OUT); }
    kit_rect(x + 34, 160, 14, 37, T_OUT); kit_rect(x + 35, 161, 12, 36, TC(74, 42, 26)); kit_rect(x + 44, 178, 2, 2, TC(216, 168, 58));
    kit_rect(x - 14, 145, 112, 4, T_OUT); kit_rect(x - 13, 146, 110, 2, TC(90, 56, 32));
    kit_rect(x - 12, 148, 2, 50, TC(90, 56, 32)); kit_rect(x + 94, 148, 2, 50, TC(90, 56, 32));
    for (k = 0; k < 2; k++) { int lx = x + (k ? 88 : -6); kit_rect(lx, 149, 1, 4, T_OUT); kit_rect(lx - 2, 153, 5, 6, T_OUT); kit_rect(lx - 1, 154, 3, 4, TC(255, 224, 138)); }
    cheer = T.phase == T_PH_STOP || T.phase == T_PH_WAIT; wv = (int)((T.t / 100) % 3) * 2 - 2;
    sx1 = x + 46;                               /* the sheriff, a star on his vest */
    kit_rect(sx1 - 4, sy - 22, 9, 14, T_OUT); kit_rect(sx1 - 3, sy - 21, 7, 13, TC(90, 106, 138)); kit_rect(sx1 - 3, sy - 21, 2, 13, TC(58, 42, 36)); kit_rect(sx1 + 2, sy - 21, 2, 13, TC(58, 42, 36)); t_pixstar(sx1 - 3, sy - 19, T_GOLDC);
    kit_rect(sx1 - 3, sy - 8, 3, 8, TC(58, 48, 64)); kit_rect(sx1 + 1, sy - 8, 3, 8, TC(58, 48, 64));
    kit_circle(sx1, sy - 26, 4, T_OUT); kit_circle(sx1, sy - 26, 3, TC(232, 176, 136)); kit_rect(sx1 - 2, sy - 25, 5, 1, TC(90, 58, 42));
    kit_rect(sx1 - 6, sy - 30, 13, 2, TC(58, 36, 24)); kit_rect(sx1 - 3, sy - 34, 7, 4, TC(58, 36, 24));
    if (cheer && !T.fly.used) kit_rect(sx1 + 5, sy - 32 + wv, 2, 10, TC(232, 176, 136)); else kit_rect(sx1 + 5, sy - 21, 2, 9, TC(232, 176, 136));
    lx2 = x + 62;                               /* a lady swinging a lantern */
    for (k = 0; k < 14; k++) kit_rect(lx2 - 3 - k / 3, sy - 14 + k, 7 + 2 * (k / 3), 1, k == 13 ? T_OUT : TC(138, 58, 106));
    kit_rect(lx2 - 3, sy - 22, 7, 9, TC(138, 58, 106)); kit_circle(lx2, sy - 26, 3, TC(241, 199, 163)); kit_rect(lx2 - 5, sy - 30, 11, 3, TC(232, 224, 208)); kit_rect(lx2 - 4, sy - 31, 9, 2, TC(232, 224, 208));
    {
        int lanx = lx2 + 7 + (cheer ? 6 * t_sin((int32_t)T.t) / 1024 : 0), lany = sy - (cheer ? 34 : 20);
        kit_line(lx2 + 3, sy - 20, lanx, lany + 3, TC(241, 199, 163)); kit_rect(lanx - 2, lany, 5, 6, T_OUT); kit_rect(lanx - 1, lany + 1, 3, 4, TC(255, 224, 138));
    }
    kx = x + 76; jump = cheer ? (int)((T.t / 80) % 4) * 2 : 0;   /* a boy, jumping */
    kit_rect(kx - 3, sy - 15 - jump, 7, 9, T_OUT); kit_rect(kx - 2, sy - 14 - jump, 5, 8, TC(58, 138, 74)); kit_rect(kx - 2, sy - 6 - jump, 2, 6, TC(58, 74, 122)); kit_rect(kx + 1, sy - 6 - jump, 2, 6, TC(58, 74, 122));
    kit_circle(kx, sy - 18 - jump, 3, TC(232, 176, 136)); kit_rect(kx - 3, sy - 22 - jump, 7, 2, TC(200, 74, 42));
    if (cheer) { kit_rect(kx - 5, sy - 22 - jump, 2, 7, TC(232, 176, 136)); kit_rect(kx + 4, sy - 22 - jump, 2, 7, TC(232, 176, 136)); }
}
static void t_canyon(void)                      /* now and then the line crosses a canyon */
{
    int x, s = T.scroll >> 8;
    for (x = 0; x < KIT_W; x++) {
        int k = t_wrap(s + x, T_CANYON_P), edge, top;
        if (k <= T_CANYON_A || k >= T_CANYON_B) continue;
        edge = k - T_CANYON_A < T_CANYON_B - k ? k - T_CANYON_A : T_CANYON_B - k;
        top = 200 + (edge < 6 ? (3 - edge / 2) * 4 : 0) + (k * 7) % 5;
        kit_rect(x, top, 1, 240 - top, TC(22, 10, 24));
        if (220 > top) kit_pixel(x, 220 + (k >> 3) % 2, TC(42, 20, 38));
        if (edge > 6) { kit_rect(x, 234, 1, 6, TC(30, 58, 106)); if ((k + (int)(T.t / 50)) % 23 < 3) kit_pixel(x, 235, TC(138, 184, 232)); }
        kit_pixel(x, top, TC(90, 48, 64));
    }
}
static void t_track(void)
{
    int x, off = t_wrap(T.scroll >> 8, 8), o16 = t_wrap(T.scroll >> 8, 16);
    for (x = 0; x < KIT_W; x += 4) { int c = t_in_canyon(x); if (!c) kit_rect(x, 213, 4, 7, TC(42, 22, 36)); }
    for (x = -off; x < KIT_W; x += 8) kit_rect(x, 213, 6, 3, t_in_canyon(x) ? TC(122, 74, 42) : TC(90, 58, 42));
    for (x = -o16; x < KIT_W; x += 16) {        /* the trestle under the rails, over the canyon */
        if (!t_in_canyon(x) || !t_in_canyon(x + 16)) continue;
        kit_rect(x, 216, 2, 24, T_OUT); kit_rect(x + 1, 216, 1, 24, TC(122, 74, 42));
        kit_line(x + 1, 218, x + 15, 238, TC(90, 56, 32)); kit_line(x + 15, 218, x + 1, 238, TC(90, 56, 32)); kit_rect(x, 227, 16, 1, TC(106, 64, 36));
    }
    kit_rect(0, 211, KIT_W, 2, TC(184, 176, 200)); kit_rect(0, 216, KIT_W, 1, TC(138, 128, 152));
}
static void t_foreground(void)                  /* nearest of all: prairie dogs, a cactus, rocks, the tumbleweed */
{
    int i, x, d;
    for (i = 0; i < 2; i++) {
        int near;
        x = t_x(i ? 300 : 90, 130, 520, 40);
        if (x == -9999 || t_in_canyon(x) || t_in_canyon(x + 14)) continue;
        (void)near;
        for (d = 0; d < 2; d++) {
            int dx = x + d * 14, up = T.dog_t[i] > 0 ? (T.dog_t[i] > 1800 ? (2000 - T.dog_t[i]) / 20 : T.dog_t[i] < 300 ? T.dog_t[i] / 30 : 10) : 0;
            if (up > 10) up = 10;
            if (up > 0) { kit_rect(dx - 3, 231 - up, 7, up + 2, T_OUT); kit_rect(dx - 2, 232 - up, 5, up, TC(184, 136, 90)); kit_pixel(dx - 1, 233 - up, T_OUT); kit_pixel(dx + 1, 233 - up, T_OUT); }
            kit_rect(dx - 6, 232, 13, 3, TC(90, 52, 36)); kit_rect(dx - 3, 231, 7, 2, TC(42, 20, 16));
        }
    }
    x = t_x(220, 130, 520, 40); if (x != -9999 && !t_in_canyon(x)) t_cactus(x, 240, 24, 4, x > 150 && x < 236);
    {
        static const int16_t RK[3][2] = { { 30, 6 }, { 160, 4 }, { 410, 8 } };
        for (i = 0; i < 3; i++) { x = t_x(RK[i][0], 130, 520, 20); if (x != -9999 && !t_in_canyon(x)) { kit_rect(x, 234, RK[i][1], 3, TC(90, 58, 74)); kit_rect(x + 1, 233, RK[i][1] - 2, 1, TC(106, 74, 90)); } }
    }
    {                                           /* the tumbleweed */
        int tx = T.tw_x >> 8, ty = (t_in_canyon(tx) ? 212 : 230) - (t_sin((int32_t)T.t * 3 / 6) > 0 ? 5 * t_sin((int32_t)T.t * 3 / 6) / 1024 : -5 * t_sin((int32_t)T.t * 3 / 6) / 1024), k, r;
        for (k = 0; k < 8; k++) for (r = 2; r < 8; r += 2) kit_pixel(tx + r * t_cos(T.tw_a + k * 128) / 1024, ty + r * t_sin(T.tw_a + k * 128) / 1024, k % 2 ? TC(154, 122, 74) : TC(122, 90, 58));
    }
    if (T.rabbit.used) {                        /* the jackrabbit racing the train, then sitting down, puffing */
        int rx = T.rabbit.x >> 8, ry = t_in_canyon(rx) ? 214 : 231;
        if (!T.rabbit.tired) {
            int hop = (int)((T.rabbit.t / 40) % 6), f = (T.rabbit.t / 80) % 2;
            hop = hop < 3 ? hop * 2 : (6 - hop) * 2;
            kit_rect(rx - 7, ry - 6 - hop, 13, 6, T_OUT); kit_rect(rx - 6, ry - 5 - hop, 11, 4, TC(184, 148, 106)); kit_rect(rx - 8, ry - 6 - hop, 3, 3, T_WHITE);
            kit_rect(rx + 3, ry - 9 - hop, 6, 5, T_OUT); kit_rect(rx + 4, ry - 8 - hop, 4, 3, TC(184, 148, 106)); kit_pixel(rx + 6, ry - 7 - hop, T_OUT);
            kit_line(rx + 4, ry - 9 - hop, rx - 1, ry - 15 - hop, TC(184, 148, 106)); kit_line(rx + 6, ry - 9 - hop, rx + 2, ry - 16 - hop, TC(168, 132, 90));
            kit_line(rx - 5, ry - 2 - hop, rx - (f ? 9 : 3), ry + 1 - hop, TC(184, 148, 106)); kit_line(rx + 3, ry - 2 - hop, rx + (f ? 7 : 1), ry + 1 - hop, TC(184, 148, 106));
        } else {
            int hv = (T.t / 80) % 2;
            kit_rect(rx - 4, ry - 11 + hv, 9, 11, T_OUT); kit_rect(rx - 3, ry - 10 + hv, 7, 10, TC(184, 148, 106)); kit_rect(rx - 2, ry - 6 + hv, 5, 5, TC(232, 212, 176));
            kit_rect(rx - 3, ry - 16, 8, 7, T_OUT); kit_rect(rx - 2, ry - 15, 6, 5, TC(184, 148, 106)); kit_pixel(rx - 1, ry - 13, T_OUT); kit_pixel(rx + 2, ry - 13, T_OUT); kit_rect(rx, ry - 11, 2, 2, TC(255, 107, 138));
            kit_line(rx - 2, ry - 15, rx - 7, ry - 12, TC(168, 132, 90)); kit_line(rx + 3, ry - 15, rx + 8, ry - 12, TC(168, 132, 90));
            if (T.t % 800 < 500) t_text(rx - 10, ry - 28, "pant", KIT_FONT_XSMALL, TC(232, 224, 255), 0);
        }
    }
    if (T.rider.used) {                         /* the super train's escort: a cowboy galloping alongside */
        int rx = T.rider.x >> 8, ry = 226, f = (T.t / 100) % 2, l;
        static const int8_t LG[4] = { -10, -6, 6, 10 };
        kit_rect(rx - 12, ry - 10, 24, 9, T_OUT); kit_rect(rx - 11, ry - 9, 22, 7, TC(232, 224, 216)); kit_circle(rx - 4, ry - 6, 3, TC(58, 42, 36));
        kit_rect(rx + 9, ry - 18, 6, 11, T_OUT); kit_rect(rx + 10, ry - 17, 4, 10, TC(232, 224, 216)); kit_rect(rx + 11, ry - 20, 9, 5, T_OUT); kit_rect(rx + 12, ry - 19, 7, 3, TC(232, 224, 216)); kit_pixel(rx + 17, ry - 18, T_OUT);
        kit_rect(rx + 8, ry - 17, 2, 9, TC(200, 168, 120));
        for (l = 0; l < 4; l++) { int sk = (l % 2) ^ f ? 3 : -3; kit_line(rx + LG[l], ry - 2, rx + LG[l] + sk, ry + 6, TC(200, 192, 184)); kit_line(rx + LG[l] + 1, ry - 2, rx + LG[l] + sk + 1, ry + 6, TC(200, 192, 184)); }
        kit_line(rx - 12, ry - 8, rx - 18, ry - 4 + (f ? -2 : 2), TC(200, 184, 168));
        kit_rect(rx - 3, ry - 22, 7, 12, T_OUT); kit_rect(rx - 2, ry - 21, 5, 10, TC(42, 106, 168)); kit_circle(rx + 1, ry - 25, 3, TC(232, 176, 136));
        { int hu = (int)((T.t / 70) % 4) - 2; kit_rect(rx + 3, ry - 34 + hu, 2, 8, TC(232, 176, 136)); kit_rect(rx, ry - 37 + hu, 10, 2, TC(138, 90, 42)); kit_rect(rx + 3, ry - 40 + hu, 4, 3, TC(138, 90, 42)); }
        if (T.super > 3000 || (T.phase == T_PH_SUPER && T.super > 1500)) t_text(rx - 22, ry - 54, "YEE-HAW!", KIT_FONT_XSMALL, TC(255, 225, 77), 0);
    }
}
static void t_puffs(void)
{
    int i;
    for (i = 0; i < T_NP; i++)
        if (T.puff[i].used) {
            t_puff_t *p = &T.puff[i];
            int x = p->x >> 8, y = p->y >> 8, r = p->r >> 8;
            if (p->life * 4 < p->max && !p->dark && !p->star) r = r * 2 / 3;
            if (p->star) { kit_rect(x - 1, y - 3, 3, 7, p->c); kit_rect(x - 3, y - 1, 7, 3, p->c); kit_pixel(x, y, T_WHITE); continue; }
            kit_circle(x, y, r + 1, p->dark ? T_OUT : TC(40, 36, 56));
            kit_circle(x - 1, y - 1, r, p->c);
            if (!p->dark && r > 3) kit_rect(x - r / 2, y - r / 2, 2, 2, T_WHITE);   /* a glint */
        }
}
static void t_beam(void)                        /* the lamp's light: forward and a little down, in dots */
{
    int lx = t_sx(T_LAMP_X), ly = t_sy(T_LAMP_X, T_LAMP_Y), dx, y, len = 90, sup = T.super > 0;
    kit_color_t c = sup ? TC(255, 242, 192) : TC(255, 226, 160);
    for (dx = 1; dx < len && lx + dx < KIT_W; dx++) {   /* the dots: closer together near the lamp and in the beam's middle */
        int half = 3 + dx * (sup ? 42 : 34) / 100, cy = ly + dx * (22 * 1024 + T.tilt * 12 / 10) / 102400, step = dx < 30 ? 2 : dx < 60 ? 3 : 4;
        int x = lx + dx, inner = half / 2, y0 = cy - half, y1 = cy + half;
        for (y = y0 + (x + y0) % (step + 1); y < cy - inner; y += step + 1) kit_pixel(x, y, c);
        for (y = cy - inner + (x + cy - inner) % step; y <= cy + inner; y += step) kit_pixel(x, y, c);
        for (y = cy + inner + 1 + (x + cy + inner + 1) % (step + 1); y <= y1; y += step + 1) kit_pixel(x, y, c);
    }
    kit_circle(lx, ly, 6, TC(255, 236, 170)); kit_circle(lx, ly, 3, T_WHITE);
}

static void t_draw(void)
{
    int i, k = T.idx % t_sg()->len;
    char s[12];
    t_sky(); t_hills(); t_ranch(); t_ground(); t_camp(); t_cow(); t_poles(); t_station(); t_canyon(); t_track();
    if (T.super > 0) for (i = 0; i < 14; i++) { int y = 20 + (i * 37) % 200, x = (int)((240 * 2 + 50 - (T.t * 600 / 1000 + (uint32_t)i * 97) % 340)) - 50 - 240; kit_rect(x, y, 40 + (i % 3) * 20, 1, TC(255, 255, 255)); }
    t_engine(); t_puppy();
    if (T.steam > 0) { int wx = t_sx(T_X0 + 33), wy = t_sy(T_X0 + 33, T_G - 71), e = (1200 - T.steam) / 150; kit_circle(wx, wy - 4 - e, 3 + e / 2, T_WHITE); }
    if (T.hat.used) { int hx = T.hat.x >> 8, hy = T.hat.y >> 8; kit_rect(hx - 4, hy, 8, 3, TC(58, 90, 168)); kit_rect(hx - 4, hy + 1, 8, 1, TC(232, 232, 248)); }
    t_foreground();
    t_puffs();
    t_beam();
    for (i = 0; i < T_NS; i++) if (T.spark[i].used) kit_rect(T.spark[i].x >> 8, T.spark[i].y >> 8, T.spark[i].big ? 2 : 1, T.spark[i].big ? 2 : 1, T.spark[i].c);
    if (T.mode == 1 && !T.listen && !T.phase && !T.sneeze) {   /* Key: the next note's letter, with a halo, over the engine */
        static const int8_t RING[8][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } };
        int n = t_note_of(k), lx = 160, ly = 86 + 2 * t_sin((int32_t)(T.t * 1024 / 2094)) / 1024 - (T.pop > 0 ? 4 : 0), j;
        const char *nm = kit_note_name(n, KIT_NAMES_LETTERS);
        for (j = 0; j < 8; j++) kit_text(lx + RING[j][0], ly - 12 + RING[j][1], nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, (T.t / 250) % 2 ? T_WHITE : TC(255, 233, 194));
        for (j = 0; j < 4; j++) kit_text(lx + RING[j][0] / 2, ly - 12 + RING[j][1] / 2, nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, T_OUT);
        kit_text(lx, ly - 12, nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, U_COL[n % 12]);
    }
    if (T.sneeze) {                             /* ah... ah... CHOO! */
        int sx = t_sx(T_STACK_X), sy = t_sy(T_STACK_X, T_STACK_Y);
        if (T.sneeze < 900) t_text(sx - 34, sy - 18 - T.sneeze / 150, T.sneeze < 300 ? "AH" : T.sneeze < 600 ? "AH..AH" : "AAAH..", KIT_FONT_SMALL, T_WHITE, 0);
        else if (T.sneeze < 2000) t_text(sx + 4 - (T.sneeze - 900) / 250, sy - 16, "CHOO!", KIT_FONT_MEDIUM, TC(255, 225, 77), 0);
    }
    if (T.angry > 0) {                          /* the driver: steam from his ears, HMPH! */
        int cx = t_sx(T_X0 + 11), cy = t_sy(T_X0 + 11, T_G - 72), e = (int)((T.t / 100) % 5);
        kit_circle(cx - 5 - e, cy - 4 - e * 2, 1 + e / 2, T_WHITE); kit_circle(cx + 6 + e, cy - 4 - e * 2, 1 + e / 2, T_WHITE);
        if (T.angry > 2500) t_text(cx - 26, cy - 24, "HMPH!", KIT_FONT_SMALL, TC(255, 138, 114), 0);
    }
    if (T.bark > 0) t_text(T_OX + 2, T_OY + T_G - 84 - (1400 - T.bark) / 350, "WOOF!", KIT_FONT_SMALL, T_WHITE, 0);
    if (T.fly.used) {                           /* the sheriff's star, flying onto the tender */
        int i5 = set.train_stars < 5 ? set.train_stars : 4, tx = T_OX + 4 + i5 * 8 + 3, ty = T_OY + T_G - 30, t = T.fly.t * 1024 / 1100;
        int fx = T.fly.x0 + (tx - T.fly.x0) * t / 1024, fy = T.fly.y0 + (ty - T.fly.y0) * t / 1024 - 60 * t_sin(t / 2) / 1024;
        t_bigstar(fx, fy, 9, T_OUT); t_bigstar(fx, fy - 1, 8, T_GOLDC);
    }
    if (set.train_stars > 5) { s[0] = 'x'; s[1] = 0; { int n = set.train_stars, j = 1; char d[4]; int nd = 0; while (n) { d[nd++] = (char)('0' + n % 10); n /= 10; } while (nd) s[j++] = d[--nd]; s[j] = 0; } t_text(T_OX + 46, T_OY + T_G - 34, s, KIT_FONT_XSMALL, T_GOLDC, 0); }
    if (T.got > 0) ctext(120, 64, "A STAR!", KIT_FONT_MEDIUM, T_GOLDC);
    if (T.phase == T_PH_STOP || T.phase == T_PH_WAIT) ctext(120, 36, "YAY!", KIT_FONT_LARGE, U_COL[(T.t / 100) % 12]);
    else if (T.super > 0) ctext(120, 36, T.phase ? "CHOO CHOO!" : "SUPER TRAIN!", KIT_FONT_MEDIUM, U_COL[(T.t / 100) % 12]);
    kit_rect(0, 0, KIT_W, 16, TC(22, 16, 44)); kit_rect(0, 0, KIT_W, 2, MODE_COL);   /* the top line */
    {
        int w = kit_text(4, 4, "TRAIN", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, TC(255, 207, 90));
        w += kit_text(10 + w, 4, t_sg()->name, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, TC(232, 224, 255));
        kit_text(16 + w, 4, T.listen ? "LISTEN" : T.mode ? "KEY" : "EASY", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, TC(201, 191, 224));
    }
    kit_rect(168, 7, 66, 4, TC(58, 47, 94)); kit_rect(168, 7, 66 * k / t_sg()->len, 4, TC(255, 207, 90));
}

static void t_lights(void)                      /* Key: the next note's key; Easy: every key breathing */
{
    int k, want = T.mode == 1 && !T.listen ? t_note_of(T.idx) : -1;
    if (T.listen) want = (int32_t)(T.t - T.lu) < 0 ? T.ln : -1;   /* PLAY: the key of the note sounding */
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, T.mode == 0 && !T.listen ? KIT_LIGHT_BREATH : KIT_DRUM_LO + k == want ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_PLAY, T.listen ? KIT_LIGHT_ON : KIT_LIGHT_DIM);
}

static int t_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_KEY_DOWN) { t_press(KIT_DRUM_LO + e->id); return 1; }
    if (e->type == KIT_EV_MIDI_NOTE_ON) { t_press(e->id); return 1; }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_PLAY) {   /* listen first: the song played, the train running it */
        if (T.phase) return 1;
        T.listen = !T.listen; T.lk = 0; T.lat = 0; T.idx = 0; T.npq = 0;
        return 1;
    }
    if (e->type == KIT_EV_ENC) {
        if (e->id == KIT_ENC_KNOB1) { T.mode = e->delta > 0 ? 1 : 0; T.streak = 0; }
        if (e->id == KIT_ENC_KNOB2) {
            T.voice = (T.voice + (e->delta > 0 ? 1 : 2)) % 3;
            kit_track_notes_off(KIT_TRACK_MELODY);
            kit_track_sound(KIT_TRACK_MELODY, T_VOICE[T.voice][0], T_VOICE[T.voice][1]);
        }
        if (e->id == KIT_ENC_KNOB3 && !T.listen && !T.phase) { T.tune = !T.tune; T.idx = 0; T.streak = T.misses = 0; T.chord = 0; t_whistle(); }
        if (e->id == KIT_ENC_KNOB4) {
            int b = e->delta > 0;
            if (b == T.quiet) { T.quiet = !b; kit_reverb(b ? 30 : 0); }
        }
    }
    return 1;
}

static void t_frame(uint32_t now)
{
    int moved = 0;
    if (!T.on) { t_enter(); moved = 1; }
    while (T.q < now / T_STEP) {                /* step by step to now: the same states on every FM-1 */
        int j, k = 0;
        for (j = 0; j < T.npq; j++)
            if (T.pq_q[j] <= T.q) t_press_now(T.pq_n[j]);
            else { T.pq_n[k] = T.pq_n[j]; T.pq_q[k] = T.pq_q[j]; k++; }
        T.npq = k;
        T.q++; t_step(); moved = 1;
    }
    if (!moved) return;
    t_draw();
    t_lights();
}
