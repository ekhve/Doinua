/* SPDX-License-Identifier: GPL-3.0-only */
/* Hiru Damatxo, the Game mode's second game (ST-53, D-062), included by app.c after unicorn.c. The three little ladies
 * of Donostia dance on La Concha's promenade to the traditional tune (Donostiako hiru damatxo, variant A of the
 * Bertsolaritza database: the old songbooks' tune, danced by 1784), a jump for each note played: Easy any key, Key the
 * next note's key (its letter floats over the dancer). They wait for her. The long note that ends each line: all three
 * jump; the song's end: they turn together, fireworks over the bay, ZORIONAK!, and again. A wrong key in Key: the
 * dancer whose note it was sits down hard and the other two giggle; two in a row: a seagull flies in and poops on her
 * head. PLAY: listen to the song first, danced. K1 the mode, K4 the music. As the browser lab the owner shaped
 * ("the 3 women do not have castañuelas ... I dont like them spinning so much ... they fall if we do once wrong. Twice
 * comes a seagull and poops on her head"), dressed as the owner's photo of the dance costume. All drawn in code. */

#define D_STEP 50                               /* the game moves in steps of 50 ms, as the unicorn (the same on every FM-1) */
#define D_FLOOR 200                             /* the promenade's top */
#define D_NR 12
#define D_NF 6
#define D_ND 12
/* the tune: note, length in eighths */
static const uint8_t D_TUNE[][2] = {
    {62,1},{62,1},{67,2},{69,1},{70,1},{69,1},{67,1},{69,2},{66,1},{62,1},{74,1},{72,1},{70,2},{69,1},{67,1},{69,1},{67,1},{66,1},{67,3},
    {62,1},{62,1},{67,2},{69,1},{70,1},{69,1},{67,1},{69,2},{66,1},{62,1},{74,1},{72,1},{70,2},{69,1},{67,1},{69,1},{67,1},{66,1},{67,3},
    {70,1},{72,1},{74,2},{74,1},{74,1},{75,2},{74,1},{72,2},{70,1},{69,1},{70,1},{72,1},{74,2},{74,1},{70,1},{67,2},{70,1},{69,3},
    {62,1},{62,1},{67,2},{69,1},{70,1},{69,1},{67,1},{69,2},{65,1},{62,1},{74,1},{72,1},{70,2},{69,1},{67,1},{69,1},{67,1},{65,1},{67,3} };
#define D_LEN ((int)(sizeof D_TUNE / sizeof D_TUNE[0]))
static int d_phrase_end(int k) { return k == 18 || k == 37 || k == 55 || k == D_LEN - 1; }

typedef struct { int32_t x, y, vx, vy; int16_t life; uint8_t c, used; } d_bit_t;   /* x, y: px x 256 */
static struct {
    int on, mode, quiet, misses, idx;
    uint32_t q, t;
    int32_t dance[3], spin[3], fall[3], giggle[3], splat[3], shake[3], cheer;   /* ms left */
    d_bit_t roses[D_NR], dust[D_ND];
    struct { int32_t x, y, r, age; uint8_t c, used; } fw[D_NF];
    struct { int32_t x; int tx, who, dropped, used; } gull;
    struct { int32_t x, y; int who, used; } poop;
    int32_t trainera, next_trainera;            /* x px x 256 (0: none), ms to the next */
    int32_t cloud[3];
    int listen, lk; int32_t lat;                /* PLAY: the song played and danced, the note, ms to the next */
    int ln; uint32_t lu;                        /* PLAY: the key lit for the note sounding, until */
    uint8_t off_n[4]; uint32_t off_at[4];
    int chord, chord_n[4];
    int saved_e[KIT_NTRACKS], saved_p[KIT_NTRACKS], saved_rv;
    uint8_t pq_n[8]; uint32_t pq_q[8]; int npq;
    uint32_t rnd;
} D;

static int d_rand(int n) { D.rnd = D.rnd * 1664525u + 1013904223u; return (int)((D.rnd >> 8) % (uint32_t)n); }
static const int D_LX[3] = { 62, 120, 178 };    /* where the three stand */
static int d_shift;                             /* a head shaken: the whole of her, a pixel either way */
static int d_isqrt(int v) { int r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }

/* ---- sound: the tune on a soft flute, G minor chords under it, a soft step, the falls and the gull ---- */
#define D_MEL KIT_TRACK_MELODY
static void d_note(int n, int ms)
{
    int i, k = 0;
    for (i = 0; i < 4; i++) if (D.off_at[i] && D.off_n[i] == n) { kit_track_note_off(D_MEL, n); D.off_at[i] = 0; }
    kit_track_note_on(D_MEL, n, 100);
    for (i = 0; i < 4; i++) if (!D.off_at[i]) { k = i; break; } else if (D.off_at[i] < D.off_at[k]) k = i;
    if (D.off_at[k]) kit_track_note_off(D_MEL, D.off_n[k]);
    D.off_n[k] = (uint8_t)n; D.off_at[k] = D.t + (uint32_t)ms; if (!D.off_at[k]) D.off_at[k] = 1;
}
static void d_hit(int sound, int vel) { int pad = kit_drum_key_of(sound); kit_track_note_on(KIT_TRACK_DRUMS, KIT_DRUM_LO + pad, vel); kit_track_note_off(KIT_TRACK_DRUMS, KIT_DRUM_LO + pad); }
/* G minor: Gm, Cm, D, F; the one holding the note (kept while the note is in it) */
static const uint8_t D_CH[4][4] = { { 55, 58, 62, 43 }, { 55, 60, 63, 48 }, { 54, 57, 62, 50 }, { 53, 57, 60, 41 } };
static const int8_t D_CHORD_OF[12] = { 2, 1, 3, 2, 1, 4, 3, 1, 1, 3, 1, 1 };   /* a pitch class's chord: 1 Gm 2 Cm 3 D 4 F */
static void d_pad_off(void)
{
    int i;
    if (D.chord) { for (i = 0; i < 3; i++) kit_track_note_off(KIT_TRACK_CHORDS, D.chord_n[i]); kit_track_note_off(KIT_TRACK_BASS, D.chord_n[3]); }
    D.chord = 0;
}
static void d_pad(int n)
{
    int c = D_CHORD_OF[n % 12], i;
    if (D.quiet) return;
    if (D.chord) for (i = 0; i < 3; i++) if (D_CH[D.chord - 1][i] % 12 == n % 12) c = D.chord;
    if (c == D.chord) return;
    d_pad_off();
    D.chord = c;
    for (i = 0; i < 4; i++) D.chord_n[i] = D_CH[c - 1][i];
    for (i = 0; i < 3; i++) kit_track_note_on(KIT_TRACK_CHORDS, D.chord_n[i], 24);
    kit_track_note_on(KIT_TRACK_BASS, D.chord_n[3], 12);
}

static void d_enter(void)
{
    int t, i;
    for (t = 0; t < KIT_NTRACKS; t++) { D.saved_e[t] = kit_track_engine(t); D.saved_p[t] = kit_track_preset(t); }
    kit_seq_stop();
    kit_track_sound(D_MEL, 5, 3);               /* Organ wheel: SOFT FLUTE (a txistu's breath) */
    kit_track_sound(KIT_TRACK_DRUMS, KIT_ENGINE_DRUMS, 0);
    kit_track_sound(KIT_TRACK_CHORDS, 1, 1);    /* Analog: SOFT PAD */
    kit_track_sound(KIT_TRACK_BASS, 1, 7);      /* Analog: SUB BASS */
    D.saved_rv = kit_reverb_of(); kit_reverb(D.quiet ? 0 : 40);
    D.on = 1; D.q = kit_ms() / D_STEP; D.t = D.q * D_STEP; D.rnd = 12345;
    D.idx = 0; D.misses = 0; D.chord = 0; D.listen = 0; D.cheer = 0; D.npq = 0;
    for (i = 0; i < 3; i++) { D.dance[i] = D.spin[i] = D.fall[i] = D.giggle[i] = D.splat[i] = D.shake[i] = 0; }
    for (i = 0; i < D_NR; i++) D.roses[i].used = 0;
    for (i = 0; i < D_ND; i++) D.dust[i].used = 0;
    for (i = 0; i < D_NF; i++) D.fw[i].used = 0;
    D.gull.used = 0; D.poop.used = 0; D.trainera = 0; D.next_trainera = 6000;
    D.cloud[0] = 20 << 8; D.cloud[1] = 120 << 8; D.cloud[2] = 200 << 8;
}
static void d_leave(void)
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++) kit_track_sound(t, D.saved_e[t], D.saved_p[t]);
    kit_reverb(D.saved_rv);
    D.chord = 0;
    kit_lights_off();
    D.on = 0;
}

/* ---- playing ---- */
static void d_rose(int x, int c)
{
    int i, k = 0;
    for (i = 0; i < D_NR && k < 3; i++)
        if (!D.roses[i].used) {
            D.roses[i].used = 1; D.roses[i].x = (x + d_rand(17) - 8) << 8; D.roses[i].y = (D_FLOOR - 70) << 8;
            D.roses[i].vx = (d_rand(41) - 20) << 8; D.roses[i].vy = -((40 + d_rand(30)) << 8); D.roses[i].life = 1000; D.roses[i].c = (uint8_t)c; k++;
        }
}
static void d_finale(void)                      /* the song's end: they turn together, fireworks, ZORIONAK! */
{
    int i;
    D.cheer = 4000;
    for (i = 0; i < 3; i++) { D.spin[i] = 1000; D.dance[i] = 0; }
    for (i = 0; i < D_NF; i++) {
        D.fw[i].used = 1; D.fw[i].x = 40 + d_rand(160); D.fw[i].y = 30 + d_rand(40); D.fw[i].r = 18 + d_rand(14);
        D.fw[i].age = -i * 350; D.fw[i].c = (uint8_t)d_rand(12);
    }
}
static void d_dance(int k, int ms)              /* note k: she jumps it, the flute sings it */
{
    int n = D_TUNE[k][0], who = k % 3, i;
    d_note(n, ms);
    d_pad(n);
    d_hit(0, 30);                               /* a soft step as she lands */
    D.dance[who] = 450;
    if (d_phrase_end(k)) for (i = 0; i < 3; i++) D.dance[i] = 450;   /* a line's long note: all three jump */
    d_rose(D_LX[who], n % 12);
    if (k == D_LEN - 1) d_finale();
}
static void d_wrong(int who)                    /* Key: a wrong key; once, she sits down hard; twice, the seagull */
{
    int i, k = 0;
    if (++D.misses == 1) {
        D.fall[who] = 1600; d_hit(0, 110);      /* plof */
        for (i = 0; i < D_ND && k < 10; i++)
            if (!D.dust[i].used) {
                D.dust[i].used = 1; D.dust[i].x = (D_LX[who] + d_rand(31) - 15) << 8; D.dust[i].y = (D_FLOOR - 4) << 8;
                D.dust[i].vx = (d_rand(41) - 20) << 8; D.dust[i].vy = -((10 + d_rand(14)) << 8); D.dust[i].life = 800; k++;
            }
        for (i = 0; i < 3; i++) if (i != who) D.giggle[i] = 1400;
    } else {
        D.misses = 0;
        D.gull.used = 1; D.gull.x = 260 << 8; D.gull.tx = D_LX[who]; D.gull.who = who; D.gull.dropped = 0;
        kit_sfx(KIT_SFX_GULL, 7);
    }
}
static void d_press_now(int note)
{
    int k = D.idx % D_LEN;
    if (D.listen) return;
    if (D.mode == 1 && note != D_TUNE[k][0]) { d_wrong(k % 3); return; }
    D.misses = 0;
    d_dance(k, D_TUNE[k][1] * 150 + 150);
    D.idx++;
}
static void d_press(int note) { if (D.npq < 8) { D.pq_n[D.npq] = (uint8_t)note; D.pq_q[D.npq] = kit_ms() / D_STEP; D.npq++; } }

static void d_step(void)
{
    const int32_t dt = D_STEP;
    int i;
    D.t += (uint32_t)dt;
    for (i = 0; i < 4; i++) if (D.off_at[i] && (int32_t)(D.t - D.off_at[i]) >= 0) { kit_track_note_off(D_MEL, D.off_n[i]); D.off_at[i] = 0; }
    if (D.listen) {                             /* PLAY: the song, in its own time (an eighth 300 ms), danced */
        D.lat -= dt;
        if (D.lat <= 0) {
            D.ln = D_TUNE[D.lk][0]; D.lu = D.t + (uint32_t)(D_TUNE[D.lk][1] * 300 - 40);
            d_dance(D.lk, D_TUNE[D.lk][1] * 300 - 40);
            D.lat += D_TUNE[D.lk][1] * 300; D.lk++; D.idx = D.lk;
            if (D.lk >= D_LEN) { D.listen = 0; D.idx = 0; }
        }
    }
    for (i = 0; i < 3; i++) {
        if (D.dance[i] > 0) D.dance[i] -= dt; if (D.spin[i] > 0) D.spin[i] -= dt; if (D.fall[i] > 0) D.fall[i] -= dt;
        if (D.giggle[i] > 0) D.giggle[i] -= dt; if (D.splat[i] > 0) D.splat[i] -= dt; if (D.shake[i] > 0) D.shake[i] -= dt;
    }
    if (D.cheer > 0) D.cheer -= dt;
    for (i = 0; i < D_NR; i++)
        if (D.roses[i].used) {
            d_bit_t *r = &D.roses[i];
            r->x += r->vx * dt / 1000; r->y += r->vy * dt / 1000; r->vy += (90 << 8) * dt / 1000; r->life -= (int16_t)(dt * 8 / 10);
            if (r->life <= 0 || r->y > (240 << 8)) r->used = 0;
        }
    for (i = 0; i < D_ND; i++)
        if (D.dust[i].used) {
            d_bit_t *r = &D.dust[i];
            r->x += r->vx * dt / 1000; r->y += r->vy * dt / 1000; r->life -= (int16_t)dt;
            if (r->life <= 0) r->used = 0;
        }
    for (i = 0; i < D_NF; i++)
        if (D.fw[i].used) {
            if (D.fw[i].age < 0 && D.fw[i].age + dt >= 0) d_hit(19, 90);   /* a boom, far over the bay */
            D.fw[i].age += dt; if (D.fw[i].age > 1400) D.fw[i].used = 0;
        }
    if (D.gull.used) {                          /* in from the sea, low over her head, away */
        D.gull.x -= (70 << 8) * dt / 1000;
        if (!D.gull.dropped && (D.gull.x >> 8) <= D.gull.tx + 6) {
            D.gull.dropped = 1; D.poop.used = 1; D.poop.who = D.gull.who; D.poop.x = D.gull.x;
            D.poop.y = (40 + 60) << 8;
        }
        if ((D.gull.x >> 8) < -20) D.gull.used = 0;
    }
    if (D.poop.used) {
        D.poop.y += (160 << 8) * dt / 1000;
        if ((D.poop.y >> 8) >= D_FLOOR - 66) {
            kit_sfx(KIT_SFX_PLOP, 8);           /* splat */
            D.splat[D.poop.who] = 4000; D.shake[D.poop.who] = 800;
            for (i = 0; i < 3; i++) if (i != D.poop.who) D.giggle[i] = 1400;
            D.poop.used = 0;
        }
    }
    for (i = 0; i < 3; i++) { D.cloud[i] += (2 << 8) * dt / 1000 * (i + 2) / 3; if (D.cloud[i] > (280 << 8)) D.cloud[i] = -(60 << 8); }
    if (D.trainera) { D.trainera += (16 << 8) * dt / 1000; if (D.trainera > (270 << 8)) { D.trainera = 0; D.next_trainera = 18000 + d_rand(12000); } }
    else if ((D.next_trainera -= dt) <= 0) D.trainera = -(30 << 8);
}

/* ---- drawing ---- */
#define DC(r, g, b) KIT_RGB(r, g, b)
static const kit_color_t D_OUT = DC(34, 22, 46), D_SKIN = DC(241, 199, 163), D_WHITE = DC(246, 242, 234), D_BLACK = DC(30, 26, 34),
    D_RED = DC(216, 38, 42), D_REDD = DC(168, 28, 34), D_ABARKA = DC(176, 90, 42);
static const kit_color_t D_HAIR[3] = { DC(42, 26, 20), DC(122, 68, 36), DC(58, 36, 24) };
typedef struct { int16_t x, y, w, h; kit_color_t c; } d_part_t;
static d_part_t d_parts[48]; static int d_np;
static void d_add(int x, int y, int w, int h, kit_color_t c) { if (d_np < 48) { d_parts[d_np].x = (int16_t)x; d_parts[d_np].y = (int16_t)y; d_parts[d_np].w = (int16_t)w; d_parts[d_np].h = (int16_t)h; d_parts[d_np].c = c; d_np++; } }
static void d_flush(int cx, int sx)             /* every outline, then every fill; squeezed about cx by sx/256 as she turns */
{
    int i, pass;
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i < d_np; i++) {
            d_part_t *p = &d_parts[i];
            int l = cx + (p->x - cx) * sx / 256, r = cx + (p->x + p->w - cx) * sx / 256, w = r - l;
            if (w < 1) w = 1;
            if (pass == 0) kit_rect(l - 1, p->y - 1, w + 2, p->h + 2, D_OUT); else kit_rect(l, p->y, w, p->h, p->c);
        }
    d_np = 0;
}
static int d_sin(int32_t a1024) { return U_SIN[(a1024 >> 4) & 63]; }   /* sin, Q10, a turn = 1024 (the unicorn's table) */

static void d_lady(int i)
{
    int x = D_LX[i] + d_shift, d = D.dance[i] > 0, spin = D.spin[i] > 0, fall = D.fall[i] > 0, giggle = D.giggle[i] > 0 && !fall;
    int32_t p = spin ? 1024 - D.spin[i] * 1024 / 1000 : d ? 1024 - D.dance[i] * 1024 / 450 : 0;   /* 0..1024 */
    int half = d_sin(p / 2), hop = fall ? 0 : spin ? half * 4 / 1024 : d ? half * 9 / 1024 : 0;
    int flare = spin ? half * 8 / 1024 : d ? half * 3 / 1024 : 0;
    int turn = spin ? d_sin(p + 256) : 1024, back = turn < 0, sx = (turn < 0 ? -turn : turn) / 4;   /* sx: /256 */
    int sit = fall ? 20 : 0, top = D_FLOOR - 66 - hop + sit, rows = fall ? 16 : 26, r;
    int armsup = d && p > 200 && p < 820;
    if (sx < 52) sx = 52;
    for (r = 0; r < rows; r++) {                /* the skirt: red, wider to the hem, three black bands near it */
        int w = 16 + (fall ? r * 13 / 10 : r * (55 + flare * 100 / 30) / 100) * 2;
        int band = r >= rows - 7 && (rows - r) % 2 == 0;
        d_add(x - w / 2, top + 30 + r, w, 1, band ? D_BLACK : r > rows - 3 ? D_REDD : D_RED);
    }
    d_add(x - 7, top + 17, 14, 14, D_BLACK);    /* the bodice */
    if (giggle) { d_add(x - 11, top + 18, 4, 12, D_WHITE); d_add(x + 7, top + 18, 4, 8, D_WHITE); d_add(x + 2, top + 11, 4, 4, D_SKIN); }   /* a hand to her mouth */
    else if (fall) { d_add(x - 13, top + 22, 4, 12, D_WHITE); d_add(x + 9, top + 22, 4, 12, D_WHITE); d_add(x - 14, top + 33, 4, 3, D_SKIN); d_add(x + 10, top + 33, 4, 3, D_SKIN); }
    else if (armsup || spin) { d_add(x - 14, top + 16, 5, 12, D_WHITE); d_add(x + 9, top + 16, 5, 12, D_WHITE); d_add(x - 15, top + 27, 4, 3, D_SKIN); d_add(x + 11, top + 27, 4, 3, D_SKIN); }
    else { d_add(x - 11, top + 18, 4, 14, D_WHITE); d_add(x + 7, top + 18, 4, 14, D_WHITE); d_add(x - 11, top + 31, 4, 3, D_SKIN); d_add(x + 7, top + 31, 4, 3, D_SKIN); }
    d_add(x - 8, top + 16, 16, 4, D_WHITE);     /* the blouse's shoulders */
    d_add(x - 5, top + 4, 10, 12, back ? D_HAIR[i] : D_SKIN);
    if (i == 2) { d_add(x - 6, top, 12, 6, D_WHITE); d_add(x - 2, top + 5, 4, 4, D_WHITE); }   /* a white headscarf */
    else d_add(x - 6, top, 12, 5, D_HAIR[i]);
    if (i == 0) d_add(x + 5, top + 4, 3, 14, D_HAIR[i]);   /* a braid */
    if (i == 1) d_add(x - 8, top + 3, 3, 10, D_HAIR[i]);   /* a ponytail */
    if (fall) { d_add(x - 16, D_FLOOR - 7, 10, 4, D_WHITE); d_add(x + 6, D_FLOOR - 7, 10, 4, D_WHITE); d_add(x - 20, D_FLOOR - 8, 5, 5, D_ABARKA); d_add(x + 15, D_FLOOR - 8, 5, 5, D_ABARKA); }
    else {
        int lt = top + 30 + rows, ll = D_FLOOR - 3 - hop - lt, pointed = d && p > 150 && p < 870;
        d_add(x - 5, lt, 4, ll, D_WHITE); d_add(x + 1, lt, 4, ll, D_WHITE);
        d_add(x - 6, D_FLOOR - 4 - hop, 5, pointed ? 4 : 3, D_ABARKA); d_add(x + 1, D_FLOOR - 4 - hop, 5, pointed ? 4 : 3, D_ABARKA);
    }
    d_flush(x, sx);
    if (!fall && sx > 150) {                    /* the red laces crossing up the stockings */
        int y, lt = top + 30 + rows;
        for (y = lt + 1; y < D_FLOOR - 4 - hop; y += 2) { kit_pixel(x - 5 + ((y >> 1) % 2) * 3, y, DC(200, 32, 42)); kit_pixel(x + 1 + ((y >> 1) % 2) * 3, y, DC(200, 32, 42)); }
    }
    if (back || sx < 150) return;
    kit_rect(x - 3, top + 15, 6, 2, D_WHITE);   /* the collar, the bodice's lacing, the hair line, the face */
    kit_rect(x - 1, top + 20, 2, 1, DC(200, 58, 64)); kit_rect(x - 1, top + 23, 2, 1, DC(200, 58, 64)); kit_rect(x - 1, top + 26, 2, 1, DC(200, 58, 64));
    kit_rect(x - 5, top + 4, 10, 2, i == 2 ? DC(90, 58, 36) : D_HAIR[i]);
    if (fall) { kit_rect(x - 3, top + 8, 2, 2, D_OUT); kit_rect(x + 1, top + 8, 2, 2, D_OUT); kit_rect(x - 1, top + 12, 2, 2, D_OUT); }
    else if (giggle) { kit_rect(x - 3, top + 9, 2, 1, D_OUT); kit_rect(x + 1, top + 9, 2, 1, D_OUT); }
    else { kit_rect(x - 3, top + 9, 2, 2, D_OUT); kit_rect(x + 1, top + 9, 2, 2, D_OUT); kit_rect(x - 1, top + 13, 2, 1, D_OUT); }
    kit_rect(x - 4, top + 12, 2, 1, DC(232, 138, 138)); kit_rect(x + 2, top + 12, 2, 1, DC(232, 138, 138));
    if (D.splat[i] > 0) {                       /* the seagull's present, on her head */
        kit_rect(x - 6, top - 3, 13, 6, D_OUT); kit_rect(x - 2, top - 4, 5, 1, D_OUT);
        kit_rect(x - 5, top - 2, 11, 4, DC(255, 255, 255)); kit_rect(x - 1, top - 3, 3, 1, DC(255, 255, 255));
        kit_rect(x + 3, top + 2, 2, 4, DC(255, 255, 255)); kit_rect(x - 4, top + 2, 1, 3, DC(255, 255, 255));
    }
}

static void d_cloud(int x, int y, int big)
{
    static const int8_t P[4][4] = { { 0, 0, 14, 5 }, { 10, -4, 12, 6 }, { 22, -1, 12, 5 }, { -8, 2, 10, 3 } };
    int k, s = big ? 13 : 10;
    for (k = 0; k < 4; k++) kit_rect(x + P[k][0] * s / 10, y + P[k][1] * s / 10, P[k][2] * s / 10, P[k][3] * s / 10, DC(106, 74, 126));
    for (k = 0; k < 4; k++) kit_rect(x + P[k][0] * s / 10, y + (P[k][1] + P[k][3] - 2) * s / 10, P[k][2] * s / 10, 2, DC(232, 136, 138));
    for (k = 0; k < 4; k++) kit_rect(x + P[k][0] * s / 10 + 1, y + (P[k][1] + P[k][3]) * s / 10 - 1, P[k][2] * s / 10 - 2, 1, DC(255, 200, 144));
}
static int d_igeldo(int x)                      /* Igeldo's long ridge, down into the sea */
{
    int y = 64 - (x < 40 ? x * 6 / 40 : 6);
    if (x > 40) { int q = (x - 40) * 100 / 78; y += q * q / 100 * 38 / 100; }   /* (an approximate 1.6 power: a square) */
    return y;
}
static void d_scene(void)
{
    static const kit_color_t SKY[7] = { DC(43, 33, 84), DC(61, 42, 99), DC(90, 50, 110), DC(129, 61, 114), DC(178, 79, 110), DC(224, 122, 107), DC(242, 169, 112) };
    int i, x, y;
    for (i = 0; i < 7; i++) kit_rect(0, i * 14, KIT_W, 14, SKY[i]);
    for (i = 0; i < 30; i++) kit_pixel((i * 53) % 240, (i * 29) % 40 + 4, ((D.t / 600 + (uint32_t)i * 3) % 7) ? DC(255, 243, 214) : DC(125, 106, 160));
    for (i = 0; i < 3; i++) d_cloud(D.cloud[i] >> 8, i == 1 ? 18 : i ? 34 : 26, i == 1);
    kit_circle(160, 96, 13, DC(236, 160, 120)); kit_circle(160, 96, 9, DC(255, 226, 168));   /* the sun, going down into the sea */
    for (x = 0; x < 70; x++) { y = 70 + x / 10; kit_rect(x, y, 1, 100 - y, DC(106, 90, 134)); }   /* far hills, hazy */
    for (x = 0; x < 120; x++) { y = d_igeldo(x); kit_rect(x, y, 1, 101 - y, DC(52, 70, 62)); kit_pixel(x, y, DC(201, 138, 106)); }
    for (i = 0; i < 46; i++) { x = (i * 37 + i * i * 11) % 116; y = d_igeldo(x) + 3 + (i * i * 13 + i * 5) % 24; if (y < 100) kit_rect(x, y, 3, 2, DC(42, 58, 51)); }
    kit_rect(72, d_igeldo(72) + 6, 4, 3, DC(232, 224, 208)); kit_rect(71, d_igeldo(72) + 5, 6, 1, DC(168, 64, 42));
    y = d_igeldo(36);                           /* Igeldo's old lighthouse tower */
    kit_rect(33, y - 13, 8, 14, D_OUT); kit_rect(34, y - 12, 6, 12, DC(184, 171, 152));
    kit_rect(33, y - 14, 2, 2, D_OUT); kit_rect(36, y - 14, 2, 2, D_OUT); kit_rect(39, y - 14, 2, 2, D_OUT);
    if ((D.t / 1250) % 3 == 0) kit_rect(36, y - 10, 2, 2, DC(255, 230, 150));
    for (x = 172; x < 240; x++) {               /* Urgull: a dark rounded profile, the castle's outline on top */
        int d = x - 222, v = 2500 - d * d;
        y = 96 - d_isqrt(v < 0 ? 0 : v) * 40 / 50;
        kit_rect(x, y, 1, 101 - y, DC(71, 48, 94)); kit_pixel(x, y, DC(168, 96, 122));
    }
    kit_rect(207, 52, 24, 8, DC(71, 48, 94)); for (x = 207; x < 231; x += 3) kit_rect(x, 50, 2, 2, DC(71, 48, 94)); kit_rect(216, 45, 7, 8, DC(71, 48, 94));
    for (i = 0; i < 18; i++) kit_rect(0, 100 + i * 3, KIT_W, 3, i % 4 == 0 ? DC(111, 143, 208) : i < 6 ? DC(62, 95, 168) : DC(51, 80, 143));   /* the bay */
    for (i = 0; i < 14; i++) kit_rect((int)((i * 37 + D.t * 6 / 1000) % 240), 104 + (i * 11) % 36, 4 + i % 3, 1, DC(255, 217, 160));
    for (x = 113; x <= 151; x++) {              /* Santa Clara, its lighthouse blinking */
        int dx = x - 132, v = 361 - dx * dx, h = v > 0 ? d_isqrt(v) * 11 / 19 : 0;
        if (h > 0) { kit_rect(x, 102 - h, 1, h, DC(62, 90, 62)); kit_pixel(x, 102 - h, D_OUT); }
    }
    kit_rect(122, 93, 12, 1, DC(201, 138, 106));
    kit_rect(138, 89, 3, 5, DC(244, 239, 230)); kit_rect(138, 88, 3, 1, (D.t / 830) % 2 ? DC(255, 230, 128) : DC(138, 122, 96));
    {                                           /* one old boat at anchor, bobbing */
        int by = 112 + (int)((D.t / 300) % 4 < 2 ? 0 : 1), bx = 218;
        kit_rect(bx - 5, by, 11, 3, D_OUT); kit_rect(bx - 4, by, 9, 2, DC(42, 74, 106));
        kit_rect(bx, by - 11, 1, 11, DC(58, 42, 32));
        for (i = 0; i < 9; i++) kit_rect(bx + 1, by - 10 + i, i * 6 / 10 + 1, 1, DC(200, 120, 58));
    }
    if (D.trainera) {                           /* a trainera, the rowers pulling together */
        int tx = D.trainera >> 8, k, st = (D.t / 160) % 2;
        kit_rect(tx - 14, 132, 30, 3, D_OUT); kit_rect(tx - 13, 132, 28, 2, DC(200, 48, 42));
        for (k = 0; k < 6; k++) { int rx = tx - 10 + k * 4; kit_rect(rx, 129, 2, 3, D_WHITE); kit_rect(rx + (st ? -2 : 1), 134, 1, 2, DC(200, 160, 112)); kit_pixel(rx + (st ? -3 : 2), 136, DC(200, 160, 112)); }
    }
    for (i = 0; i < 2; i++) {                   /* gulls circling over the bay */
        int a = (int)(D.t * 512 / 6283 + i * 600), gx = (i ? 170 : 40) + 30 * d_sin(a + 256) / 1024, gy = (i ? 60 : 40) + 8 * d_sin(a) / 1024;
        int up = (D.t / 200 + (uint32_t)i) % 3 == 0;
        kit_pixel(gx, gy, D_WHITE); kit_pixel(gx - 1, gy - up, D_WHITE); kit_pixel(gx + 1, gy - up, D_WHITE); kit_pixel(gx - 2, gy - 2 * up, D_WHITE); kit_pixel(gx + 2, gy - 2 * up, D_WHITE);
    }
    kit_rect(0, 154, KIT_W, 12, DC(231, 201, 143));   /* the beach, the waves washing up it and back */
    for (x = 0; x < 240; x += 7) kit_rect(x, 160 + x % 3, 3, 1, DC(212, 180, 122));
    {
        int reach = 2 + 3 * d_sin((int)(D.t * 1024 / 7000)) / 1024;
        kit_rect(0, 154, KIT_W, reach + 2, DC(200, 172, 120));
        for (x = 0; x < 240; x += 4) kit_rect(x, 154 + reach + ((x / 4 + (int)(D.t / 400)) % 3 == 0), 4, 1, DC(246, 248, 255));
    }
    kit_rect(0, 166, KIT_W, 4, DC(154, 143, 132));   /* the wall, the white railing and its rings */
    kit_rect(0, 170, KIT_W, 20, DC(28, 22, 44));
    kit_rect(0, 170, KIT_W, 3, D_WHITE); kit_rect(0, 188, KIT_W, 2, D_WHITE);
    for (x = 0; x < 240; x += 12) { kit_rect(x, 170, 2, 20, D_WHITE); kit_circle(x + 7, 180, 4, D_WHITE); kit_circle(x + 7, 180, 2, DC(28, 22, 44)); }
    for (i = 0; i < 2; i++) {                   /* the lamps */
        int lx = i ? 210 : 30;
        kit_circle(lx + 1, 125, 11, (D.t / 700 + (uint32_t)i) % 2 ? DC(120, 104, 112) : DC(110, 96, 108));
        kit_rect(lx, 128, 3, 62, DC(44, 42, 58)); kit_rect(lx - 4, 128, 11, 2, DC(44, 42, 58));
        kit_rect(lx - 4, 122, 4, 6, DC(255, 230, 160)); kit_rect(lx + 3, 122, 4, 6, DC(255, 230, 160));
    }
    kit_rect(0, D_FLOOR - 10, KIT_W, 50, DC(185, 165, 138));   /* the promenade's paving */
    for (y = D_FLOOR - 6; y < 240; y += 8) for (x = (y % 16) ? 0 : 8; x < 240; x += 16) kit_rect(x, y, 14, 1, DC(168, 149, 122));
}

static void d_draw(void)
{
    int i, k = D.idx % D_LEN;
    char s[8];
    d_scene();
    for (i = 0; i < D_NF; i++)                  /* fireworks */
        if (D.fw[i].used && D.fw[i].age >= 0) {
            int a, r = D.fw[i].r * D.fw[i].age / 1400;
            if (D.fw[i].age < 1100)
                for (a = 0; a < 16; a++) kit_rect(D.fw[i].x + r * d_sin(a * 64 + 256) / 1024, D.fw[i].y + r * d_sin(a * 64) / 1024, 2, 2, U_COL[D.fw[i].c]);
        }
    for (i = 0; i < 3; i++) {                   /* the three; a head shaken under the gull's present */
        int jit = D.shake[i] > 0 ? ((D.t / 50) % 2 ? 1 : -1) : 0;
        d_shift = jit; d_lady(i); d_shift = 0;
    }
    for (i = 0; i < D_ND; i++) if (D.dust[i].used) kit_rect(D.dust[i].x >> 8, D.dust[i].y >> 8, 3, 2, DC(220, 206, 180));
    for (i = 0; i < D_NR; i++) if (D.roses[i].used) { kit_rect(D.roses[i].x >> 8, D.roses[i].y >> 8, 3, 3, U_COL[D.roses[i].c]); kit_rect((D.roses[i].x >> 8) + 1, (D.roses[i].y >> 8) + 3, 1, 2, DC(63, 138, 74)); }
    if (D.poop.used) { kit_rect((D.poop.x >> 8) - 1, (D.poop.y >> 8) - 1, 5, 6, D_OUT); kit_rect(D.poop.x >> 8, D.poop.y >> 8, 3, 4, DC(255, 255, 255)); }
    if (D.gull.used) {                          /* the seagull, drawn at 2x */
        int gx = D.gull.x >> 8, gy = 40 + (D.gull.tx + 70 > gx && gx > D.gull.tx - 70 ? (70 - (gx > D.gull.tx ? gx - D.gull.tx : D.gull.tx - gx)) * 60 / 70 : 0);
        int up = (D.t / 70) % 2;
#define DG(dx, dy, w, h, c) kit_rect(gx + (dx) * 2, gy + (dy) * 2, (w) * 2, (h) * 2, c)
        DG(-5, -1, 11, 5, D_OUT); DG(-4, 0, 9, 3, DC(244, 244, 240)); DG(-7, 0, 3, 2, DC(244, 244, 240)); DG(-7, -1, 3, 1, D_OUT);
        DG(-10, 1, 3, 1, DC(255, 208, 48)); DG(-9, 2, 1, 1, DC(216, 64, 32)); DG(4, 0, 3, 2, D_OUT); DG(5, 1, 2, 1, DC(244, 244, 240));
        if (up) { DG(-2, -5, 3, 5, DC(154, 164, 176)); DG(1, -7, 3, 7, DC(154, 164, 176)); DG(3, -8, 3, 2, D_OUT); }
        else { DG(-2, 3, 3, 4, DC(154, 164, 176)); DG(1, 3, 3, 5, DC(154, 164, 176)); DG(2, 7, 2, 1, D_OUT); }
        DG(-6, 0, 1, 1, DC(30, 26, 34));
#undef DG
    }
    if (!D.listen) {                            /* who dances next: a rose over her; in Key, the note's letter */
        int lx = D_LX[k % 3], ly = D_FLOOR - 84 + d_sin((int)(D.t * 1024 / 1570)) * 2 / 1024, n = D_TUNE[k][0];
        if (D.mode == 1) {
            static const int8_t RING[8][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } };
            const char *nm = U_NAMES[U_WHITE[n % 12]];
            char flat[3];
            int j;
            if (n % 12 == 10 || n % 12 == 3) { flat[0] = n % 12 == 10 ? 'B' : 'E'; flat[1] = 'b'; flat[2] = 0; nm = flat; }   /* B flat, E flat */
            for (j = 0; j < 8; j++) kit_text(lx + RING[j][0], ly - 12 + RING[j][1], nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, (D.t / 250) % 2 ? D_WHITE : DC(255, 233, 194));
            for (j = 0; j < 4; j++) kit_text(lx + RING[j][0] / 2, ly - 12 + RING[j][1] / 2, nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, D_OUT);
            kit_text(lx, ly - 12, nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, U_COL[n % 12]);
        } else { kit_circle(lx, ly - 4, 4, DC(255, 90, 110)); kit_rect(lx, ly, 1, 6, DC(63, 138, 74)); }
    }
    kit_rect(0, 0, KIT_W, 16, DC(27, 20, 51)); kit_rect(0, 0, KIT_W, 2, MODE_COL);   /* the top line */
    {
        int w = kit_text(4, 4, "HIRU DAMATXO", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, DC(255, 154, 166));
        kit_text(12 + w, 4, D.listen ? "LISTEN" : D.mode ? "KEY" : "EASY", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, DC(201, 191, 224));
    }
    kit_rect(160, 7, 74, 4, DC(58, 47, 94)); kit_rect(160, 7, 74 * k / D_LEN, 4, DC(255, 122, 138));
    if (D.cheer > 0) ctext(120, 40, "ZORIONAK!", KIT_FONT_MEDIUM, U_COL[(D.t / 120) % 12]);
    (void)s;
}

static void d_lights(void)                      /* Key: the next note's key; Easy: every key breathing */
{
    int k, want = D.mode == 1 && !D.listen ? D_TUNE[D.idx % D_LEN][0] : -1;
    if (D.listen) want = (int32_t)(D.t - D.lu) < 0 ? D.ln : -1;   /* PLAY: the key of the note sounding */
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, D.mode == 0 && !D.listen ? KIT_LIGHT_BREATH : KIT_DRUM_LO + k == want ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_PLAY, D.listen ? KIT_LIGHT_ON : KIT_LIGHT_DIM);
}

static int d_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_KEY_DOWN) { d_press(KIT_DRUM_LO + e->id); return 1; }
    if (e->type == KIT_EV_MIDI_NOTE_ON) { d_press(e->id); return 1; }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_PLAY) {   /* listen first: the song played and danced, or stop */
        D.listen = !D.listen; D.lk = 0; D.lat = 0; D.idx = 0; D.npq = 0;
        return 1;
    }
    if (e->type == KIT_EV_ENC) {
        if (e->id == KIT_ENC_KNOB1) D.mode = e->delta > 0 ? 1 : 0;
        if (e->id == KIT_ENC_KNOB4) {
            int b = e->delta > 0;
            if (b == D.quiet) { D.quiet = !b; kit_reverb(b ? 40 : 0); if (!b) d_pad_off(); }
        }
    }
    return 1;
}

static void d_frame(uint32_t now)
{
    int moved = 0;
    if (!D.on) { d_enter(); moved = 1; }
    while (D.q < now / D_STEP) {                /* step by step to now: the same states on every FM-1 */
        int j, k = 0;
        for (j = 0; j < D.npq; j++)
            if (D.pq_q[j] <= D.q) d_press_now(D.pq_n[j]);
            else { D.pq_n[k] = D.pq_n[j]; D.pq_q[k] = D.pq_q[j]; k++; }
        D.npq = k;
        D.q++; d_step(); moved = 1;
    }
    if (!moved) return;
    d_draw();
    d_lights();
}
