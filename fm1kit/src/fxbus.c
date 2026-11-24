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
static kit_fx_t bus = { 0, 60, 40, 0, 40, 60, 90 };   /* fmini-1's defaults: STEREO, DEPTH 60, RATE 40; ANALOG, TIME,
                                                       * FEEDBACK 60, MIX 90 */
static uint8_t delay_tone = 127;               /* retain the delay type's original response until edited */
static struct {
    uint32_t cho_w, cho_ph, dly_w, dly_start, dly_ph;
    int32_t cho_lp, dly_lp, dly_hp, dl_q;
    uint32_t cho_left, dly_left;                /* samples to run on after the last sound in or out (a line's length) */
} fx;
static int32_t cho_in[KIT_BLOCK], dly_in[KIT_BLOCK], rev_in[KIT_BLOCK];

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
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

static int32_t delay_tone_coefficient(int tone)
{
    /* Q15 one-pole coefficients at 400, 800, 1600, 3200, 6400 and 12800 Hz. */
    static const int16_t coefficients[] = { 1815, 3530, 6680, 11998, 19602, 27478 };
    int octave = tone * 5 / 126;
    int fraction = tone * 5 % 126;
    int32_t lower = coefficients[octave];
    return fraction ? lower + (coefficients[octave + 1] - lower) * fraction / 126 : lower;
}

int kit__fx_sends(int t) { return t >= 0 && t < KIT_NTRACKS && (snd[t][0] | snd[t][1] | snd[t][2] | snd[t][3]); }

void kit__fx_begin(void)                        /* a block: the buses' inputs empty */
{
    int i;
    for (i = 0; i < KIT_BLOCK; i++) cho_in[i] = dly_in[i] = rev_in[i] = 0;
}

/* track t's block x (Q15, KIT_BLOCK samples): its DRIVE (fmini-1's DIST: a soft clip, its tone darker as it drives
 * harder), into the mix, and its sends into the buses */
void kit__fx_track(int t, int32_t *x, int32_t *mix)
{
    int i;
    int32_t d = snd[t][0], c = snd[t][1] * 258, l = snd[t][2] * 258, r = snd[t][3] * 258;
    if (d) {
        int32_t g = 256 + d * 8;                /* Q8: 1x .. 5x into the clip */
        for (i = 0; i < KIT_BLOCK; i++) {
            int32_t u = clamp32((x[i] >> 4) * g >> 4, -49152, 49152), u2, u3, v;   /* Q15, clipped at +-1.5 */
            u2 = ((u >> 1) * (u >> 1)) >> 13;   /* u^2, Q15 */
            u3 = ((u2 >> 1) * (u >> 1)) >> 13;  /* u^3, Q15 */
            v = u - u3 * 4 / 27;                /* a cubic soft clip: flat at +-1.5, where it reaches 1 (odd: no DC) */
            x[i] = clamp32(v, -32767, 32767);
        }
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
        uint32_t ty = bus.dly_type, ms = 10u + (uint32_t)bus.dly_time * bus.dly_time * 1390u / 16129u;   /* fmini-1's dly_ms */
        int32_t tgt, fb = (bus.dly_fb > 120 ? 120 : bus.dly_fb) * 230, dmix = bus.dly_mix * 258, kc, loud = 0;
        int edited_tone = delay_tone < 127;
        if (ty == 3u) { ms = 31u + ms / 10u; fb /= 3; }  /* SLAP: one short echo, a repeat or two */
        tgt = (int32_t)clamp32((int32_t)(ms * 441u / 10u), 2, (int32_t)DLY_LEN - 4) << 8;
        if (!fx.dly_left) { fx.dly_start = fx.dly_w; fx.dly_lp = fx.dly_hp = 0; fx.dl_q = tgt; }
        fx.dl_q += clamp32(tgt - fx.dl_q, -2048, 2048);   /* a time change bends, as tape */
        kc = ty == 1u ? 26000 : ty == 2u ? 14000 : 11500;  /* the loop's low-pass: DIGITAL ~11 kHz, TAPE ~3.4, ANALOG ~2.6 */
        if (edited_tone) kc = delay_tone_coefficient(delay_tone);
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
