/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit's effects bus (ST-69, D-093): each track's DRIVE, then its sends into three shared buses, CHORUS, DELAY and
 * REVERB (synth.c's room, D-060), mono. After Felucca's and fmini-1's fx.c (https://github.com/ruppert/fmini-1,
 * GPL-3.0-only; Felucca by Leo Kuroshita, Hugelton Instruments): the same buses and the same sends, so a preset's
 * FX(DIST, CHORUS, DELAY, REVERB) and an FMini patch's twelve FX settings sound as they were made to. Mono: the
 * FM-1 plays one channel; the stereo chorus and ping-pong delay are summed. synth.c calls it, under the kit's lock
 * or in the audio interrupt; the buses' memory is in the pool. */
#include <stdint.h>
#include "kit_internal.h"

#define DLY_LEN 32768u                          /* 0.74 s (fmini-1: 1.49 s; the pool's room, D-093) */
#define CHO_LEN 2048u
static int16_t dly_buf[DLY_LEN] __attribute__((section(".pool")));
static int16_t cho_buf[CHO_LEN] __attribute__((section(".pool")));

static uint8_t snd[KIT_NTRACKS][4];             /* DRIVE, CHORUS, DELAY, REVERB sends, 0..127 */
static struct {
    uint8_t crush, darkness, dry;               /* zero is an open tone and full wet, the original drive */
    int32_t lp;
} dirt[KIT_NTRACKS];
static kit_fx_t bus = { 0, 60, 40, 0, 40, 60, 90 };   /* fmini-1's defaults: STEREO, DEPTH 60, RATE 40; ANALOG, TIME,
                                                       * FEEDBACK 60, MIX 90 */
static uint8_t delay_tone = 127;               /* retain the delay type's original response until edited */
static struct {
    int track;
    uint8_t drive, crush, darkness, delay, reverb, dub, time;
    int32_t wah_frequency, wah_damping, wah_low, wah_band;
} perform = {.track = -1};
static struct {
    uint32_t cho_w, cho_ph, dly_w, dly_start, dly_ph;
    int32_t cho_lp, dly_lp, dly_hp, dl_q;
    uint32_t cho_left, dly_left;                /* samples to run on after the last sound in or out (a line's length) */
} fx;
static int32_t cho_in[KIT_BLOCK], dly_in[KIT_BLOCK], rev_in[KIT_BLOCK];

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static int32_t louder(int32_t a, int32_t b) { return a > b ? a : b; }
/* a * b / 32768, rounded (b <= 32768: 32-bit, rule 8). Rounded, not floored: a loop's last -1 dies out instead of
 * going round for ever (a floor rounds every small negative down) */
static int32_t mulq15(int32_t a, int32_t b) { return (a >> 15) * b + (((a & 32767) * b + 16384) >> 15); }
static int32_t tri(uint32_t ph) { int32_t t = (int32_t)(ph >> 16) - 32768; return (t < 0 ? -t : t) * 2 - 32768; }   /* -32768..32767 */

void kit_track_sends(int t, int drive, int chorus, int delay, int reverb)
{
    uint32_t k;
    if (t < 0 || t >= KIT_NTRACKS) return;
    k = kit__lock();
    snd[t][0] = (uint8_t)clamp32(drive, 0, 127);
    snd[t][1] = (uint8_t)clamp32(chorus, 0, 127);
    snd[t][2] = (uint8_t)clamp32(delay, 0, 127);
    snd[t][3] = (uint8_t)clamp32(reverb, 0, 127);
    kit__unlock(k);
}
int kit_track_send_of(int t, int which) { return t >= 0 && t < KIT_NTRACKS && which >= 0 && which < 4 ? snd[t][which] : 0; }

void kit_fx_bus(const kit_fx_t *f)
{
    uint32_t k;
    if (!f) return;
    k = kit__lock();
    bus = *f;
    bus.cho_type %= 3u;
    bus.dly_type %= 4u;
    kit__unlock(k);
}
void kit_fx_bus_of(kit_fx_t *f) { if (f) *f = bus; }

void kit_delay_tone(int tone)
{
    uint32_t k = kit__lock();
    delay_tone = (uint8_t)clamp32(tone, 0, 127);
    kit__unlock(k);
}

int kit_delay_tone_of(void) { return delay_tone; }

static int32_t wah_frequency(int x)
{
    /* 2 * sin(pi * Hz / 44100), Q15, across the lab's 250 Hz to 5.7 kHz sweep. */
    static const uint16_t coeff[] = {1167, 2178, 4062, 7567, 14043, 25701};
    int interval = x * 5 / 127, fraction = x * 5 % 127;
    return fraction ? coeff[interval] + (coeff[interval + 1] - coeff[interval]) * fraction / 127 : coeff[interval];
}

void kit_perform_fx(int t, int pair, int x, int y)
{
    uint32_t k = kit__lock();
    int keep_wah = perform.wah_frequency && perform.track == t && pair == KIT_PERFORM_WAH && x >= 2;
    x = (int)clamp32(x, 0, 127);
    y = (int)clamp32(y, 0, 127);
    perform.track = -1;
    perform.drive = perform.crush = perform.darkness = perform.delay = perform.reverb = perform.dub = 0;
    perform.wah_frequency = 0;
    if (!keep_wah) perform.wah_low = perform.wah_band = 0;
    if (t >= 0 && t < KIT_NTRACKS && pair >= 0 && pair < KIT_NPERFORM_FX) {
        if (pair == KIT_PERFORM_DIRT && (x || y)) {
            perform.drive = (uint8_t)x;
            perform.crush = (uint8_t)y;
            perform.track = t;
        } else if (x >= 2 && pair == KIT_PERFORM_DUB) {
            perform.delay = (uint8_t)(x * 4 / 5);
            perform.dub = 1;
            perform.time = (uint8_t)y;
            perform.track = t;
        } else if (x >= 2 && pair == KIT_PERFORM_SPACE) {
            perform.reverb = (uint8_t)x;
            perform.darkness = (uint8_t)(y * 85 / 100);
            perform.track = t;
        } else if (x >= 2 && pair == KIT_PERFORM_WAH) {
            int32_t q = 256 + y * 3507 / 127;
            perform.wah_frequency = wah_frequency(x);
            perform.wah_damping = (32768 * 256) / q;
            perform.track = t;
        }
    }
    kit__unlock(k);
}

static int32_t delay_samples(int time)
{
    uint32_t ms = 10u + (uint32_t)time * time * 1390u / 16129u;
    if (bus.dly_type == 3u) ms = 31u + ms / 10u;
    return clamp32((int32_t)(ms * 441u / 10u), 2, (int32_t)DLY_LEN - 4);
}

int kit_delay_ms_of(void) { return (delay_samples(bus.dly_time) * 10 + 220) / 441; }

static int32_t tone_coefficient(int tone)
{
    /* Q15 one-pole coefficients at 400, 800, 1600, 3200, 6400 and 12800 Hz. */
    static const int16_t coefficients[] = { 1815, 3530, 6680, 11998, 19602, 27478 };
    int octave = tone * 5 / 126;
    int fraction = tone * 5 % 126;
    int32_t lower = coefficients[octave];
    return fraction ? lower + (coefficients[octave + 1] - lower) * fraction / 126 : lower;
}

void kit_track_dirt(int t, const kit_dirt_t *f)
{
    uint32_t k;
    if (!f || t < 0 || t >= KIT_NTRACKS) return;
    k = kit__lock();
    if (!dirt[t].darkness || dirt[t].dry == 127) dirt[t].lp = 0;
    dirt[t].crush = (uint8_t)clamp32(f->crush, 0, 127);
    dirt[t].darkness = (uint8_t)(127 - clamp32(f->tone, 0, 127));
    dirt[t].dry = (uint8_t)(127 - clamp32(f->mix, 0, 127));
    kit__unlock(k);
}

void kit_track_dirt_of(int t, kit_dirt_t *f)
{
    if (!f || t < 0 || t >= KIT_NTRACKS) return;
    *f = (kit_dirt_t){dirt[t].crush, (uint8_t)(127 - dirt[t].darkness), (uint8_t)(127 - dirt[t].dry)};
}

int kit__fx_sends(int t)
{
    return t >= 0 && t < KIT_NTRACKS &&
           (snd[t][0] | snd[t][1] | snd[t][2] | snd[t][3] | dirt[t].crush | dirt[t].darkness | (perform.track == t));
}

void kit__fx_begin(void)                        /* a block: the buses' inputs empty */
{
    int i;
    for (i = 0; i < KIT_BLOCK; i++) cho_in[i] = dly_in[i] = rev_in[i] = 0;
}

static inline int32_t fx_drive(int32_t x, int32_t gain)
{
    int32_t u = clamp32((x >> 4) * gain >> 4, -49152, 49152);
    int32_t u2 = ((u >> 1) * (u >> 1)) >> 13;
    int32_t u3 = ((u2 >> 1) * (u >> 1)) >> 13;
    return clamp32(u - u3 * 4 / 27, -32767, 32767);
}

int kit_dirt_bits(int crush) { return 16 - (clamp32(crush, 0, 127) * 13 + 63) / 127; }

static inline int32_t dirt_shape(int32_t in, int32_t gain, int32_t mask)
{
    int32_t out = gain ? fx_drive(in, gain) : in;
    if (mask) {
        int32_t magnitude = out < 0 ? -out : out;
        magnitude = (magnitude + ((mask + 1) >> 1)) & ~mask;
        out = clamp32(out < 0 ? -magnitude : magnitude, -32767, 32767);
    }
    return out;
}

int32_t kit_track_dirt_curve(int t, int32_t sample)
{
    int32_t wet, gain, mask, shaped;
    sample = clamp32(sample, -32767, 32767);
    if (t < 0 || t >= KIT_NTRACKS) return sample;
    wet = (127 - dirt[t].dry) * 32768 / 127;
    gain = snd[t][0] ? 256 + snd[t][0] * 8 : 0;
    mask = (1 << (16 - kit_dirt_bits(dirt[t].crush))) - 1;
    shaped = dirt_shape(sample, gain, mask);
    return sample + mulq15(shaped - sample, wet);
}

static void fx_dirt_block(int t, int32_t *x, int32_t drive, int32_t crush, int32_t darkness)
{
    int32_t wet = (127 - dirt[t].dry) * 32768 / 127;
    int32_t gain = drive ? 256 + drive * 8 : 0;
    int32_t mask = (1 << (16 - kit_dirt_bits(crush))) - 1;
    int32_t tone = darkness ? tone_coefficient(127 - darkness) : 0;
    int i;
    if (!wet) return;
    for (i = 0; i < KIT_BLOCK; i++) {
        int32_t dry = x[i], shaped = dirt_shape(dry, gain, mask);
        if (tone) {
            dirt[t].lp += mulq15(shaped - dirt[t].lp, tone);
            if (!shaped && dirt[t].lp >= -9 && dirt[t].lp <= 9) dirt[t].lp = 0;
            shaped = dirt[t].lp;
        }
        x[i] = dry + mulq15(shaped - dry, wet);
    }
}

static void fx_wah_block(int32_t *x)
{
    int32_t low = perform.wah_low, band = perform.wah_band;
    int32_t coefficient = perform.wah_frequency, damping = perform.wah_damping;
    int i;
    for (i = 0; i < KIT_BLOCK; i++) {
        int32_t high;
        low = clamp32(low + mulq15(band, coefficient), -65535, 65535);
        high = clamp32(x[i], -65535, 65535) - low - mulq15(band, damping);
        band = clamp32(band + mulq15(high, coefficient), -65535, 65535);
        if (!x[i] && low >= -16 && low <= 16 && band >= -16 && band <= 16) low = band = 0;
        x[i] = mulq15(band, damping);
    }
    perform.wah_low = low;
    perform.wah_band = band;
}

/* track t's block x (Q15, KIT_BLOCK samples): its DRIVE (fmini-1's DIST: a soft clip, its tone darker as it drives
 * harder), into the mix, and its sends into the buses */
void kit__fx_track(int t, int32_t *x, int32_t *mix)
{
    int i;
    int32_t d = snd[t][0], c = snd[t][1] * 258, l = snd[t][2], r = snd[t][3];
    int32_t crush = dirt[t].crush, darkness = dirt[t].darkness;
    if (perform.track == t) {
        if (perform.wah_frequency) fx_wah_block(x);
        d = louder(d, perform.drive);
        l = louder(l, perform.delay);
        r = louder(r, perform.reverb);
        crush = louder(crush, perform.crush);
        darkness = louder(darkness, perform.darkness);
    }
    l *= 258;
    r *= 258;
    if (crush || darkness || dirt[t].dry) {
        fx_dirt_block(t, x, d, crush, darkness);
    } else if (d) {
        int32_t g = 256 + d * 8;                /* Q8: 1x .. 5x into the clip */
        for (i = 0; i < KIT_BLOCK; i++) x[i] = fx_drive(x[i], g);
    }
    for (i = 0; i < KIT_BLOCK; i++) {
        mix[i] += x[i];
        if (c) cho_in[i] += mulq15(x[i], c);
        if (l) dly_in[i] += mulq15(x[i], l);
        if (r) rev_in[i] += mulq15(x[i], r);
    }
}

int32_t *kit__fx_rev_in(void) { return rev_in; }

static int any(const int32_t *b) { int i; for (i = 0; i < KIT_BLOCK; i++) if (b[i]) return 1; return 0; }

static int32_t cho_tap(uint32_t wp, int32_t r)  /* Q8 samples back, between two */
{
    uint32_t ri = (uint32_t)r >> 8;
    int32_t f = r & 255, c0 = cho_buf[(wp - ri) & (CHO_LEN - 1u)], c1 = cho_buf[(wp - ri - 1u) & (CHO_LEN - 1u)];
    return (c0 + (((c1 - c0) * f) >> 8)) << 1;
}
static int32_t dly_at(uint32_t w, uint32_t d)  /* d samples back, or silence from before the delay started */
{
    return d <= w - fx.dly_start ? dly_buf[(w - d) & (DLY_LEN - 1u)] : 0;
}

/* the chorus and the delay into mix (the reverb is synth.c's, on kit__fx_rev_in) */
void kit__fx_buses(int32_t *mix)
{
    int i;
    int cho = any(cho_in), dly = any(dly_in);
    if (cho || fx.cho_left) {                     /* CHORUS: fmini-1's three types, summed to one side */
        uint32_t ty = bus.cho_type, rate = bus.cho_rate > 127 ? 127u : bus.cho_rate;
        uint32_t cinc = 1460u + rate * rate * 9u;        /* ~0.03 .. 3.3 Hz */
        int32_t sw = (int32_t)clamp32((int32_t)(6400000u / (cinc / 32u + 1u)), 2, 137) * 3 * bus.cho_depth / 60;
        int32_t d = ty == 0u ? (sw * 3) >> 2 : ty == 1u ? sw >> 1 : (sw * 5) >> 3, kc = ty == 0u ? 14500 : ty == 1u ? 21500 : 26000;
        int loud = 0;
        for (i = 0; i < KIT_BLOCK; i++) {
            uint32_t wp = fx.cho_w;
            int32_t y;
            cho_buf[wp & (CHO_LEN - 1u)] = (int16_t)clamp32(cho_in[i] >> 1, -32768, 32767);
            fx.cho_ph += cinc;
            if (ty == 0u)                       /* STEREO (Boss CE-2): one line ~6.8 ms, the right a quarter later */
                y = (cho_tap(wp, (300 << 8) + (((tri(fx.cho_ph) + 32768) * d) >> 8))
                     + cho_tap(wp, (300 << 8) + (((tri(fx.cho_ph + 0x40000000u) + 32768) * d) >> 8))) >> 1;
            else if (ty == 1u) {                /* JUNO: two sides in opposite phase about 3.4 ms */
                int32_t a = tri(fx.cho_ph) + 32768;
                y = (cho_tap(wp, (150 << 8) + ((a * d) >> 8)) + cho_tap(wp, (150 << 8) + (((65535 - a) * d) >> 8))) >> 1;
            } else                              /* MULTI: four voices 9.5 .. 17 ms in quadrature */
                y = (cho_tap(wp, (500 << 8) + (((tri(fx.cho_ph) + 32768) * d) >> 8))
                     + cho_tap(wp, (640 << 8) + (((tri(fx.cho_ph + 0x40000000u) + 32768) * d) >> 8))
                     + cho_tap(wp, (420 << 8) + (((tri(fx.cho_ph + 0x80000000u) + 32768) * d) >> 8))
                     + cho_tap(wp, (760 << 8) + (((tri(fx.cho_ph + 0xC0000000u) + 32768) * d) >> 8))) >> 2;
            fx.cho_lp += mulq15(y - fx.cho_lp, kc);
            mix[i] += fx.cho_lp;
            loud |= fx.cho_lp > 8 || fx.cho_lp < -8 || cho_in[i];
            fx.cho_w++;
        }
        fx.cho_left = loud ? CHO_LEN : fx.cho_left > KIT_BLOCK ? fx.cho_left - KIT_BLOCK : 0;
        if (!fx.cho_left) fx.cho_lp = 0;
    }
    if (dly || fx.dly_left) {                     /* DELAY: ANALOG, DIGITAL, TAPE, SLAP (fmini-1's, mono) */
        uint32_t ty = bus.dly_type;
        int32_t tgt, fb = (bus.dly_fb > 120 ? 120 : bus.dly_fb) * 230, dmix = bus.dly_mix * 258, kc, loud = 0;
        int time = bus.dly_time;
        int edited_tone = delay_tone < 127;
        if (perform.dub) {
            fb = louder(fb, 70 * 230);
            time = ty == 3u ? perform.time : perform.time * 93 / 127;
        }
        if (ty == 3u) fb /= 3;                    /* SLAP: one short echo, a repeat or two */
        tgt = delay_samples(time) << 8;
        if (!fx.dly_left) { fx.dly_start = fx.dly_w; fx.dly_lp = fx.dly_hp = 0; fx.dl_q = tgt; }
        fx.dl_q += clamp32(tgt - fx.dl_q, -2048, 2048);   /* a time change bends, as tape */
        kc = ty == 1u ? 26000 : ty == 2u ? 14000 : 11500;  /* the loop's low-pass: DIGITAL ~11 kHz, TAPE ~3.4, ANALOG ~2.6 */
        if (edited_tone) kc = tone_coefficient(delay_tone);
        for (i = 0; i < KIT_BLOCK; i++) {
            uint32_t w = fx.dly_w, dq = (uint32_t)fx.dl_q, dd = dq >> 8;
            int32_t a = dly_at(w, dd), b = dly_at(w, dd + 1u), x, in;
            if (ty == 0u || ty == 2u) {         /* ANALOG drifts, TAPE wows */
                fx.dly_ph += ty == 2u ? 6400u : 10700u;
                dq += (uint32_t)((tri(fx.dly_ph) * (ty == 2u ? 24 : 12)) >> 7);
                dd = dq >> 8;
                a = dly_at(w, dd); b = dly_at(w, dd + 1u);
            }
            x = a + (((b - a) * (int32_t)(dq & 255u)) >> 8);
            fx.dly_lp += mulq15(x - fx.dly_lp, kc);
            in = (dly_in[i] >> 1) + mulq15(fx.dly_lp, fb);
            if (ty == 2u) { fx.dly_hp += (in - fx.dly_hp) >> 6; in -= fx.dly_hp; }   /* TAPE's low cut */
            if (ty != 1u) in = in > 24000 ? 24000 + ((in - 24000) >> 2) : in < -24000 ? -24000 + ((in + 24000) >> 2) : in;
            dly_buf[w & (DLY_LEN - 1u)] = (int16_t)clamp32(in, -32768, 32767);
            fx.dly_w++;
            mix[i] += mulq15((edited_tone ? fx.dly_lp : x) * 2, dmix);
            loud |= x > 8 || x < -8 || dly_in[i];
        }
        /* on while there is sound, and one line's length after it (an echo may still be on its way) */
        fx.dly_left = loud ? (uint32_t)(fx.dl_q >> 8) + 2u * KIT_BLOCK : fx.dly_left > KIT_BLOCK ? fx.dly_left - KIT_BLOCK : 0;
    }
}
