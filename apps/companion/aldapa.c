/* SPDX-License-Identifier: GPL-3.0-only */
/* Aldapeko Sagarra, the Game mode's third game (ST-54, D-063), included by app.c after damatxo.c. A golden late
 * afternoon just before autumn on a Basque hillside: on the highest branch of the old apple tree the txantxangorri sings
 * the traditional tune (Aldapeko sagarraren, herrikoa: the melody only), a note for each key played, an octave above
 * the keys: Easy any key, Key the next note's key (its letter over the robin). It waits for her. Down the slope the
 * baserri's windows come and go and its chimney smokes, two latxa sheep graze; rays turn round the setting sun, birds
 * cross the sky, dust glints. "Nork dantzatuko ote du soinutxo hori?" (who will dance to that little tune?): the sheep
 * do, a hop a note; the song's end: TXIRULIRULI!, both hop, the windows light. A wrong key in Key: an apple drops on a
 * sheep's head (baa!); two in a row: that sheep poops and the robin hides its eyes. PLAY: listen first. K1 the mode,
 * K2 the voice (flute, music box, bird whistle), K4 the music. The picture: tools/aldapa/ (our own pixel art). */

#define A_STEP 50
#define A_NN 10
#define A_NB 8
#define A_NP 12
/* the tune: note, length in eighths; then who sings it (0 the robin; 1 the robin while a sheep dances) */
static const uint8_t A_PART_A[][2] = { {62,1},{62,1},{64,1},{65,1},{62,1},{62,1},{64,1},{65,1},{62,1},{62,1},{64,1},{65,1},{62,2},{57,2},
    {62,2},{64,1},{65,1},{62,2},{57,2},{65,2},{65,1},{67,1},{69,2},{70,1},{69,1},{67,2},{64,2},{62,4} };
static const uint8_t A_PART_B[][2] = { {65,1},{65,1},{67,1},{65,1},{64,4},{65,1},{65,1},{67,1},{67,1},{69,4},
    {74,1},{74,1},{74,1},{69,1},{72,1},{70,1},{69,1},{67,1},{69,1},{67,1},{65,1},{64,1},{62,4} };
#define A_LA ((int)(sizeof A_PART_A / sizeof A_PART_A[0]))
#define A_LB ((int)(sizeof A_PART_B / sizeof A_PART_B[0]))
#define A_LEN (2 * A_LA + 2 * A_LB)
static int a_note(int k) { return k < 2 * A_LA ? A_PART_A[k % A_LA][0] : A_PART_B[(k - 2 * A_LA) % A_LB][0]; }
static int a_len8(int k) { return k < 2 * A_LA ? A_PART_A[k % A_LA][1] : A_PART_B[(k - 2 * A_LA) % A_LB][1]; }
static int a_dance(int k) { return k >= 2 * A_LA && (k - 2 * A_LA) % A_LB >= 10; }   /* "nork dantzatuko ...": the sheep dance */

/* where things are in the picture (tools/aldapa/layers.py, ald_layers.json) */
static const int16_t A_SHEEP[2][2][2] = { { { 138, 190 }, { 134, 182 } }, { { 184, 181 }, { 180, 173 } } };   /* [sheep][graze, up]: x, y */
static const int16_t A_WIN[2][2] = { { 102, 184 }, { 126, 184 } };
#define A_CHIM_X 147
#define A_CHIM_Y 156
#define A_ROBIN_X 177
#define A_ROBIN_Y 64
#define A_SUN_X 64
#define A_SUN_Y 140

typedef struct { int32_t x, y, vx, vy; int16_t life; uint8_t c, used; } a_bit_t;
static struct {
    int on, mode, voice, quiet, misses, idx, victim, danceturn;
    uint32_t q, t;
    int32_t sing, tilt, shy, next_tilt, cheer, next_light, next_flock, next_smoke;
    int lights[2];
    struct { int up, dir, stars; int32_t next, walk, hop; } sheep[2];   /* walk: px x 256 */
    a_bit_t notes[A_NN], birds[A_NB], smoke[A_NP];
    struct { int32_t x, y, vx, vy; int tx, ty, used, i; } apple;
    uint8_t pel_x[12], pel_y[12]; int npel; int32_t pel_at[4]; int pel_n;
    int32_t wisp[4], mote_ph;
    int listen, lk; int32_t lat;
    int ln; uint32_t lu;                        /* PLAY: the key lit for the note sounding, until */
    int32_t fin_at; int fin_n;
    uint8_t off_n[4]; uint32_t off_at[4];
    int chord, chord_n[4];
    int saved_e[KIT_NTRACKS], saved_p[KIT_NTRACKS], saved_rv;
    uint8_t pq_n[8]; uint32_t pq_q[8]; int npq;
    uint32_t rnd;
} A;
static int a_rand(int n) { A.rnd = A.rnd * 1664525u + 1013904223u; return (int)((A.rnd >> 8) % (uint32_t)n); }

/* ---- sound ---- */
#define A_MEL KIT_TRACK_MELODY
static const uint8_t A_VOICE[3][2] = { { 5, 3 }, { KIT_ENGINE_BASIC, KIT_INST_MUSICBOX }, { KIT_ENGINE_BASIC, KIT_INST_SINE } };   /* flute, music box, bird whistle */
static void a_sing(int n, int ms)               /* the robin's note: an octave above the key */
{
    int i, k = 0;
    n += 12;
    for (i = 0; i < 4; i++) if (A.off_at[i] && A.off_n[i] == n) { kit_track_note_off(A_MEL, n); A.off_at[i] = 0; }
    kit_track_note_on(A_MEL, n, 100);
    for (i = 0; i < 4; i++) if (!A.off_at[i]) { k = i; break; } else if (A.off_at[i] < A.off_at[k]) k = i;
    if (A.off_at[k]) kit_track_note_off(A_MEL, A.off_n[k]);
    A.off_n[k] = (uint8_t)n; A.off_at[k] = A.t + (uint32_t)ms; if (!A.off_at[k]) A.off_at[k] = 1;
}
/* D minor: Dm, Gm, A, F; the one holding the note (kept while the note is in it) */
static const uint8_t A_CH[4][4] = { { 57, 62, 65, 38 }, { 55, 58, 62, 43 }, { 57, 61, 64, 45 }, { 57, 60, 65, 41 } };
static const int8_t A_CHORD_OF[12] = { 4, 3, 1, 2, 3, 1, 1, 2, 3, 1, 2, 2 };
static void a_pad_off(void)
{
    int i;
    if (A.chord) { for (i = 0; i < 3; i++) kit_track_note_off(KIT_TRACK_CHORDS, A.chord_n[i]); kit_track_note_off(KIT_TRACK_BASS, A.chord_n[3]); }
    A.chord = 0;
}
static void a_pad(int n)
{
    int c = A_CHORD_OF[n % 12], i;
    if (A.quiet) return;
    if (A.chord) for (i = 0; i < 3; i++) if (A_CH[A.chord - 1][i] % 12 == n % 12) c = A.chord;
    if (c == A.chord) return;
    a_pad_off();
    A.chord = c;
    for (i = 0; i < 4; i++) A.chord_n[i] = A_CH[c - 1][i];
    for (i = 0; i < 3; i++) kit_track_note_on(KIT_TRACK_CHORDS, A.chord_n[i], 22);
    kit_track_note_on(KIT_TRACK_BASS, A.chord_n[3], 12);
}

static void a_enter(void)
{
    int t, i;
    for (t = 0; t < KIT_NTRACKS; t++) { A.saved_e[t] = kit_track_engine(t); A.saved_p[t] = kit_track_preset(t); }
    kit_seq_stop();
    kit_track_sound(A_MEL, A_VOICE[A.voice][0], A_VOICE[A.voice][1]);
    kit_track_sound(KIT_TRACK_DRUMS, KIT_ENGINE_DRUMS, 0);
    kit_track_sound(KIT_TRACK_CHORDS, 1, 1);    /* Analog: SOFT PAD */
    kit_track_sound(KIT_TRACK_BASS, 1, 7);      /* Analog: SUB BASS */
    A.saved_rv = kit_reverb_of(); kit_reverb(A.quiet ? 0 : 40);
    A.on = 1; A.q = kit_ms() / A_STEP; A.t = A.q * A_STEP; A.rnd = 4242;
    A.idx = 0; A.misses = 0; A.chord = 0; A.listen = 0; A.cheer = 0; A.npq = 0; A.npel = 0; A.pel_n = 0; A.fin_n = 0;
    A.sing = A.tilt = A.shy = 0; A.next_tilt = 3000; A.next_light = 2000; A.next_flock = 4000; A.next_smoke = 0;
    A.lights[0] = A.lights[1] = 0; A.apple.used = 0;
    for (i = 0; i < 2; i++) { A.sheep[i].up = i; A.sheep[i].next = 3000 + i * 2000; A.sheep[i].walk = 0; A.sheep[i].dir = i ? -1 : 1; A.sheep[i].hop = 0; A.sheep[i].stars = 0; }
    for (i = 0; i < A_NN; i++) A.notes[i].used = 0;
    for (i = 0; i < A_NB; i++) A.birds[i].used = 0;
    for (i = 0; i < A_NP; i++) A.smoke[i].used = 0;
    for (i = 0; i < 4; i++) A.wisp[i] = (i * 70 - 20) << 8;
}
static void a_leave(void)
{
    int t;
    for (t = 0; t < KIT_NTRACKS; t++) kit_track_sound(t, A.saved_e[t], A.saved_p[t]);
    kit_reverb(A.saved_rv);
    A.chord = 0;
    kit_lights_off();
    A.on = 0;
}

/* ---- playing ---- */
static void a_note_up(int x, int y, int c, int vx)
{
    int i;
    for (i = 0; i < A_NN; i++)
        if (!A.notes[i].used) {
            A.notes[i].used = 1; A.notes[i].x = x << 8; A.notes[i].y = y << 8; A.notes[i].vx = vx << 8;
            A.notes[i].vy = -((22 + a_rand(12)) << 8); A.notes[i].life = 1200; A.notes[i].c = (uint8_t)c; return;
        }
}
static void a_sing_note(int k, int ms)
{
    int n = a_note(k);
    a_sing(n, ms);
    a_pad(n);
    A.sing = 300; A.tilt = 0;
    if (a_dance(k)) { int s = A.danceturn++ % 2; A.sheep[s].hop = 450; A.sheep[s].up = 1; A.sheep[s].next = 1500; }
    a_note_up(A_ROBIN_X - 2, A_ROBIN_Y + 4, n % 12, -10 - a_rand(18));
    a_note_up(A_ROBIN_X + 2, A_ROBIN_Y + 2, n % 12, -10 - a_rand(18));
    if (k == A_LEN - 1) { A.cheer = 4000; A.lights[0] = A.lights[1] = 1; A.fin_n = 4; A.fin_at = 0; }   /* TXIRULIRULI! */
}
static void a_wrong(void)                       /* Key: once, an apple on a sheep's head; twice, that sheep poops */
{
    if (++A.misses == 1) {
        int i = A.victim = a_rand(2);
        A.tilt = 600;
        A.apple.used = 1; A.apple.i = i; A.apple.x = (150 + a_rand(30)) << 8; A.apple.y = 100 << 8;
        A.apple.tx = A_SHEEP[i][0][0] + 3 + (A.sheep[i].walk >> 8); A.apple.ty = A_SHEEP[i][0][1] + 2 - (A.sheep[i].walk >> 8) * 28 / 100;
        A.apple.vx = (A.apple.tx - (A.apple.x >> 8)) * 256 * 10 / 7; A.apple.vy = -(10 << 8);
    } else {
        int i = A.victim;
        A.misses = 0; A.shy = 1800;
        A.pel_n = 4; A.pel_at[0] = 250;
        A.sheep[i].up = 1; A.sheep[i].next = 2000;
    }
}
static void a_press_now(int note)
{
    int k = A.idx % A_LEN;
    if (A.listen) return;
    if (A.mode == 1 && note != a_note(k)) { a_wrong(); return; }
    A.misses = 0;
    a_sing_note(k, a_len8(k) * 160 + 120);
    A.idx++;
}
static void a_press(int note) { if (A.npq < 8) { A.pq_n[A.npq] = (uint8_t)note; A.pq_q[A.npq] = kit_ms() / A_STEP; A.npq++; } }

static void a_step(void)
{
    const int32_t dt = A_STEP;
    int i;
    A.t += (uint32_t)dt;
    for (i = 0; i < 4; i++) if (A.off_at[i] && (int32_t)(A.t - A.off_at[i]) >= 0) { kit_track_note_off(A_MEL, A.off_n[i]); A.off_at[i] = 0; }
    if (A.listen) {                             /* PLAY: the song in its own time (an eighth 250 ms) */
        A.lat -= dt;
        if (A.lat <= 0) {
            A.ln = a_note(A.lk); A.lu = A.t + (uint32_t)(a_len8(A.lk) * 250 - 30);
            a_sing_note(A.lk, a_len8(A.lk) * 250 - 30);
            A.lat += a_len8(A.lk) * 250; A.lk++; A.idx = A.lk;
            if (A.lk >= A_LEN) { A.listen = 0; A.idx = 0; }
        }
    }
    if (A.fin_n > 0 && (A.fin_at -= dt) <= 0) { /* the finale: the sheep hop, the robin trills, four times */
        static const uint8_t TRILL[3] = { 62, 65, 69 };
        A.fin_n--; A.fin_at = 500;
        A.sheep[0].hop = A.sheep[1].hop = 450; A.sheep[0].up = A.sheep[1].up = 1;
        A.sing = 300; a_sing(TRILL[A.fin_n % 3] + 12, 120);
        { int dx = a_rand(30) - 15, c = a_rand(12), x = 150 + a_rand(60); a_note_up(x, 70, c, dx); }   /* drawn one by one (in the order the PC build took them): C leaves a call's argument order to the compiler */
    }
    if (A.pel_n > 0 && (A.pel_at[0] -= dt) <= 0) {   /* plop, plop, plop, plop: behind the sheep */
        int i2 = A.victim, k = 4 - A.pel_n;
        int rx = A_SHEEP[i2][0][0] + 27 + (A.sheep[i2].walk >> 8), ry = A_SHEEP[i2][0][1] + 21 - (A.sheep[i2].walk >> 8) * 28 / 100;
        if (A.npel >= 12) { int j; for (j = 1; j < 12; j++) { A.pel_x[j - 1] = A.pel_x[j]; A.pel_y[j - 1] = A.pel_y[j]; } A.npel = 11; }
        A.pel_x[A.npel] = (uint8_t)(rx + k * 2 - k % 2); A.pel_y[A.npel] = (uint8_t)(ry + k % 2); A.npel++;
        kit_sfx(KIT_SFX_PLOP, 6);
        A.pel_n--; A.pel_at[0] = 180;
    }
    if (A.apple.used) {
        A.apple.vy += (220 << 8) * dt / 1000; A.apple.x += A.apple.vx * dt / 1000; A.apple.y += A.apple.vy * dt / 1000;
        if ((A.apple.y >> 8) >= A.apple.ty) {
            int i2 = A.apple.i;
            kit_track_note_on(KIT_TRACK_DRUMS, KIT_DRUM_LO + kit_drum_key_of(20), 90); kit_track_note_off(KIT_TRACK_DRUMS, KIT_DRUM_LO + kit_drum_key_of(20));   /* bonk */
            kit_sfx(KIT_SFX_BAA, 8);
            A.sheep[i2].stars = 1400; A.sheep[i2].up = 1; A.sheep[i2].next = 1600; A.apple.used = 0;
        }
    }
    for (i = 0; i < 2; i++) {                   /* the sheep: grazing, looking up, a few steps along the slope */
        if (A.sheep[i].stars > 0) A.sheep[i].stars -= dt;
        if (A.sheep[i].hop > 0) A.sheep[i].hop -= dt;
        if ((A.sheep[i].next -= dt) <= 0) {
            A.sheep[i].up = !A.sheep[i].up; A.sheep[i].next = A.sheep[i].up ? 1500 + a_rand(2000) : 3000 + a_rand(5000);
            if (!A.sheep[i].up && a_rand(2)) A.sheep[i].dir = -A.sheep[i].dir;
        }
        if (!A.sheep[i].up && A.sheep[i].hop <= 0) {
            A.sheep[i].walk += A.sheep[i].dir * 205 * dt / 1000;
            if (A.sheep[i].walk > (4 << 8)) A.sheep[i].dir = -1; else if (A.sheep[i].walk < -(4 << 8)) A.sheep[i].dir = 1;
        }
    }
    if (A.sing > 0) A.sing -= dt;
    if (A.tilt > 0) A.tilt -= dt;
    if (A.shy > 0) A.shy -= dt;
    if (A.cheer > 0) A.cheer -= dt;
    if ((A.next_tilt -= dt) <= 0 && A.sing <= 0) {   /* the robin looks round; now and then a bird far off */
        A.tilt = 800; A.next_tilt = 2500 + a_rand(3000);
        if (a_rand(5) < 2) kit_sfx(KIT_SFX_CHIRP, 3);
    }
    if ((A.next_light -= dt) <= 0) { A.lights[a_rand(2)] ^= 1; A.next_light = 3000 + a_rand(5000); }
    if ((A.next_smoke -= dt) <= 0) {            /* the chimney's smoke, a puff at a time */
        A.next_smoke = 400;
        for (i = 0; i < A_NP; i++)
            if (!A.smoke[i].used) {
                A.smoke[i].used = 1; A.smoke[i].x = A_CHIM_X << 8; A.smoke[i].y = A_CHIM_Y << 8;
                A.smoke[i].vx = (3 + a_rand(3)) << 8; A.smoke[i].vy = -((7 + a_rand(3)) << 8); A.smoke[i].life = 3600; break;
            }
    }
    for (i = 0; i < A_NP; i++)
        if (A.smoke[i].used) {
            A.smoke[i].x += A.smoke[i].vx * dt / 1000; A.smoke[i].y += A.smoke[i].vy * dt / 1000;
            if ((A.smoke[i].life -= (int16_t)dt) <= 0) A.smoke[i].used = 0;
        }
    if ((A.next_flock -= dt) <= 0) {            /* a small flock, crossing the sky towards home */
        int n = 3 + a_rand(4), y0 = 16 + a_rand(54), dir = a_rand(5) < 3 ? -1 : 1, k = 0;
        A.next_flock = 12000 + a_rand(10000);
        for (i = 0; i < A_NB && k < n; i++)
            if (!A.birds[i].used) {
                A.birds[i].used = 1; A.birds[i].x = ((dir < 0 ? 250 : -10) - dir * (k * 9 + a_rand(4))) << 8;
                A.birds[i].y = (y0 + (k > n / 2 ? k - n / 2 : n / 2 - k) * 4) << 8; A.birds[i].vx = dir * ((14 + a_rand(2)) << 8); A.birds[i].c = (uint8_t)a_rand(4); k++;
            }
    }
    for (i = 0; i < A_NB; i++)
        if (A.birds[i].used) { A.birds[i].x += A.birds[i].vx * dt / 1000; if ((A.birds[i].x >> 8) < -20 || (A.birds[i].x >> 8) > 260) A.birds[i].used = 0; }
    for (i = 0; i < A_NN; i++)
        if (A.notes[i].used) {
            A.notes[i].x += A.notes[i].vx * dt / 1000; A.notes[i].y += A.notes[i].vy * dt / 1000;
            if ((A.notes[i].life -= (int16_t)(dt * 7 / 10)) <= 0) A.notes[i].used = 0;
        }
    for (i = 0; i < 4; i++) { A.wisp[i] += (2 + i % 2) * 256 * dt / 1000; if (A.wisp[i] > (250 << 8)) A.wisp[i] = -(70 << 8); }
    A.mote_ph += dt;
}

/* ---- drawing ---- */
static int a_slope(int x) { return 226 - x * 28 / 100; }   /* the hill's line: it hides the baserri's lower half */
static void a_rays(void)
{
    /* light shafts fanning up from the sun, swaying and breathing: sparse light pixels (the screen has no see-through),
       the denser the brighter; only over the sky */
    int k;
    for (k = 0; k < 11; k++) {
        int a = -502 + 300 * (2 * k + 1) / 22 + d_sin((int)(A.t / 12) + k * 210) * 8 / 1024;   /* angle: a turn 1024 */
        int bright = 1 + (d_sin((int)(A.t / 2) + k * 310) + 1024) * 2 / 2048, r;
        int ca = d_sin(a + 256), sa = d_sin(a);
        for (r = 8; r < 230; r += 4) {
            int w = r / 22 + 1, j;
            for (j = -w; j <= w; j += 3) {
                int x, y;
                if ((r * 7 + j * 13 + k * 5 + (int)(A.t / 400)) % 7 >= bright) continue;
                x = A_SUN_X + (ca * r - sa * j) / 1024; y = A_SUN_Y + (sa * r + ca * j) / 1024;
                if (y < 146 && y > 14 && x >= 0 && x < 240 && (x < 160 || y < 40)) kit_pixel(x, y, KIT_RGB(255, 236, 196));
            }
        }
    }
}
static void a_draw(void)
{
    int i, k = A.idx % A_LEN;
    kit_image(0, 0, &IMG_ALD_BG, 1);
    a_rays();
    for (i = 0; i < 4; i++) {                   /* thin cloud wisps, gold underneath, drifting */
        int x0 = A.wisp[i] >> 8, y0 = 26 + i * 23, len = 30 + i * 9, x;
        for (x = 0; x < len; x += 2) { int h = x < len / 4 || x > len * 3 / 4 ? 1 : 2; kit_rect(x0 + x, y0 - h, 2, h, KIT_RGB(240, 190, 170)); kit_rect(x0 + x, y0 + 1, 2, 1, KIT_RGB(255, 214, 150)); }
    }
    for (i = 0; i < 2; i++) {                   /* the baserri's windows, lit, only what the hill leaves showing */
        int x, y;
        if (!A.lights[i]) continue;
        for (x = A_WIN[i][0] + 1; x < A_WIN[i][0] + 11; x++) {
            int vis = a_slope(x) < A_WIN[i][1] + 12 ? a_slope(x) : A_WIN[i][1] + 12;
            for (y = A_WIN[i][1] + 1; y < vis; y++) kit_pixel(x, y, (x == A_WIN[i][0] + 5 || y == A_WIN[i][1] + 6) ? KIT_RGB(122, 74, 32) : y < A_WIN[i][1] + 4 ? KIT_RGB(255, 230, 160) : KIT_RGB(255, 200, 96));
        }
    }
    for (i = 0; i < A_NP; i++)                  /* the smoke, curling off on the breeze, thinning */
        if (A.smoke[i].used) {
            int r = 2 + (3600 - A.smoke[i].life) / 900, x = A.smoke[i].x >> 8, y = A.smoke[i].y >> 8;
            x += d_sin((int)(A.t / 6) + y * 20) * 2 / 1024;
            if (A.smoke[i].life > 1200) kit_circle(x, y, r, A.smoke[i].life > 2400 ? KIT_RGB(236, 232, 242) : KIT_RGB(220, 210, 224));
            else { kit_pixel(x, y, KIT_RGB(220, 210, 224)); kit_pixel(x + 2, y - 1, KIT_RGB(220, 210, 224)); }
        }
    for (i = 0; i < A.npel; i++) { kit_rect(A.pel_x[i], A.pel_y[i], 2, 2, KIT_RGB(58, 36, 20)); kit_pixel(A.pel_x[i], A.pel_y[i], KIT_RGB(106, 68, 40)); }
    for (i = 0; i < 2; i++) {                   /* the sheep */
        int up = A.sheep[i].up, hop = A.sheep[i].hop > 0 ? d_sin((450 - A.sheep[i].hop) * 512 / 450) * 6 / 1024 : 0;
        int dx = A.sheep[i].walk >> 8, dy = -dx * 28 / 100 - hop;
        static const kit_image_t *const IM[2][2] = { { &IMG_ALD_SHEEP0_GRAZE, &IMG_ALD_SHEEP0_UP }, { &IMG_ALD_SHEEP1_GRAZE, &IMG_ALD_SHEEP1_UP } };
        kit_image(A_SHEEP[i][up][0] + dx, A_SHEEP[i][up][1] + dy, IM[i][up], 1);
        if (A.sheep[i].stars > 0) {             /* stars round the bonked head */
            int j, cx = A_SHEEP[i][0][0] + 4 + dx, cy = A_SHEEP[i][0][1] - 4 + dy;
            for (j = 0; j < 3; j++) {
                int a = (int)(A.t * 1024 / 1050) + j * 341, x = cx + 6 * d_sin(a + 256) / 1024, y = cy + 2 * d_sin(a) / 1024;
                kit_rect(x, y - 1, 1, 3, KIT_RGB(255, 225, 77)); kit_rect(x - 1, y, 3, 1, KIT_RGB(255, 225, 77));
            }
        }
    }
    if (A.apple.used) { kit_circle(A.apple.x >> 8, A.apple.y >> 8, 2, KIT_RGB(196, 42, 30)); kit_rect(A.apple.x >> 8, (A.apple.y >> 8) - 4, 1, 2, KIT_RGB(58, 36, 20)); }
    kit_image(A_ROBIN_X, A_ROBIN_Y, A.shy > 0 ? &IMG_ALD_ROBIN_SHY : A.sing > 0 ? &IMG_ALD_ROBIN_SING : A.tilt > 0 ? &IMG_ALD_ROBIN_TILT : &IMG_ALD_ROBIN_REST, 1);
    for (i = 0; i < A_NB; i++)                  /* the birds going home: wings up, wings down, two pixels a dot */
        if (A.birds[i].used) {
            int x = A.birds[i].x >> 8, y = A.birds[i].y >> 8, up = (A.t / 110 + i) % 2;
            kit_rect(x, y, 2, 2, KIT_RGB(58, 42, 62));
            if (up) { kit_rect(x - 2, y - 2, 2, 2, KIT_RGB(58, 42, 62)); kit_rect(x - 4, y - 4, 2, 2, KIT_RGB(58, 42, 62)); kit_rect(x + 2, y - 2, 2, 2, KIT_RGB(58, 42, 62)); kit_rect(x + 4, y - 4, 2, 2, KIT_RGB(58, 42, 62)); }
            else { kit_rect(x - 2, y, 2, 2, KIT_RGB(58, 42, 62)); kit_rect(x - 4, y + 2, 2, 2, KIT_RGB(58, 42, 62)); kit_rect(x + 2, y, 2, 2, KIT_RGB(58, 42, 62)); kit_rect(x + 4, y + 2, 2, 2, KIT_RGB(58, 42, 62)); }
        }
    for (i = 0; i < 18; i++) {                  /* dust glinting in the light */
        int x = 10 + (i * 53) % 130, y = 96 + (i * 37 + (int)(A.mote_ph / 300)) % 94;
        if ((A.mote_ph / 250 + (uint32_t)i * 3) % 5 < 2) kit_pixel(x + d_sin((int)(A.mote_ph / 8) + i * 90) * 3 / 1024, y, KIT_RGB(255, 230, 160));
    }
    for (i = 0; i < A_NN; i++)
        if (A.notes[i].used) {
            int x = A.notes[i].x >> 8, y = A.notes[i].y >> 8;
            kit_rect(x, y, 4, 3, U_COL[A.notes[i].c]); kit_rect(x + 3, y - 7, 1, 8, U_COL[A.notes[i].c]); kit_rect(x + 3, y - 7, 3, 1, U_COL[A.notes[i].c]);
        }
    if (!A.listen && A.mode == 1) {             /* Key: the next note's letter, over the robin */
        static const int8_t RING[8][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } };
        int n = a_note(k), j, y = 40 + d_sin((int)(A.t * 1024 / 1570)) * 2 / 1024;
        const char *nm = U_NAMES[U_WHITE[n % 12]];
        char flat[3];
        if (n % 12 == 10) { flat[0] = 'B'; flat[1] = 'b'; flat[2] = 0; nm = flat; }
        for (j = 0; j < 8; j++) kit_text(188 + RING[j][0], y + RING[j][1], nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, (A.t / 250) % 2 ? KIT_RGB(255, 255, 255) : KIT_RGB(255, 243, 200));
        for (j = 0; j < 4; j++) kit_text(188 + RING[j][0] / 2, y + RING[j][1] / 2, nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, KIT_RGB(42, 29, 20));
        kit_text(188, y, nm, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, U_COL[n % 12]);
    }
    kit_rect(0, 0, KIT_W, 16, KIT_RGB(30, 22, 44)); kit_rect(0, 0, KIT_W, 2, MODE_COL);   /* the top line */
    {
        int w = kit_text(4, 4, "ALDAPEKO SAGARRA", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, KIT_RGB(255, 200, 144));
        kit_text(10 + w, 4, A.listen ? "LISTEN" : A.mode ? "KEY" : "EASY", KIT_FONT_XSMALL, KIT_ALIGN_LEFT, KIT_RGB(240, 224, 208));
    }
    kit_rect(192, 7, 44, 4, KIT_RGB(80, 64, 80)); kit_rect(192, 7, 44 * k / A_LEN, 4, KIT_RGB(255, 176, 96));
    if (A.cheer > 0) ctext(120, 26, "TXIRULIRULI!", KIT_FONT_MEDIUM, U_COL[(A.t / 120) % 12]);
}

static void a_lights(void)
{
    int k, want = A.mode == 1 && !A.listen ? a_note(A.idx % A_LEN) : -1;
    if (A.listen) want = (int32_t)(A.t - A.lu) < 0 ? A.ln : -1;   /* PLAY: the key of the note sounding */
    for (k = 0; k < KIT_NKEYS; k++) kit_key_light(k, A.mode == 0 && !A.listen ? KIT_LIGHT_BREATH : KIT_DRUM_LO + k == want ? KIT_LIGHT_ON : KIT_LIGHT_OFF);
    kit_btn_light(KIT_BTN_PLAY, A.listen ? KIT_LIGHT_ON : KIT_LIGHT_DIM);
}

static int a_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_KEY_DOWN) { a_press(KIT_DRUM_LO + e->id); return 1; }
    if (e->type == KIT_EV_MIDI_NOTE_ON) { a_press(e->id); return 1; }
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_PLAY) { A.listen = !A.listen; A.lk = 0; A.lat = 0; A.idx = 0; A.npq = 0; return 1; }
    if (e->type == KIT_EV_ENC) {
        if (e->id == KIT_ENC_KNOB1) A.mode = e->delta > 0 ? 1 : 0;
        if (e->id == KIT_ENC_KNOB2) {           /* the robin's voice: flute, music box, bird whistle */
            A.voice = (A.voice + (e->delta > 0 ? 1 : 2)) % 3;
            kit_track_sound(A_MEL, A_VOICE[A.voice][0], A_VOICE[A.voice][1]);
        }
        if (e->id == KIT_ENC_KNOB4) {
            int b = e->delta > 0;
            if (b == A.quiet) { A.quiet = !b; kit_reverb(b ? 40 : 0); if (!b) a_pad_off(); }
        }
    }
    return 1;
}

static void a_frame(uint32_t now)
{
    int moved = 0;
    if (!A.on) { a_enter(); moved = 1; }
    while (A.q < now / A_STEP) {
        int j, k = 0;
        for (j = 0; j < A.npq; j++)
            if (A.pq_q[j] <= A.q) a_press_now(A.pq_n[j]);
            else { A.pq_n[k] = A.pq_n[j]; A.pq_q[k] = A.pq_q[j]; k++; }
        A.npq = k;
        A.q++; a_step(); moved = 1;
    }
    if (!moved) return;
    a_draw();
    a_lights();
}
