/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit host backend: runs an app on a PC with no window, from a scripted input, and writes
 * what the FM-1 would show and play. Same input script syntax as fm1-live --bench, so one
 * factory scenario runs here (milliseconds) and in the emulator (the real firmware).
 *
 *   app-host ... --store FILE        the store and the bank (kit_store_*, kit_bank_*) from FILE at the start, back to it at the end
 *   app-host --list-sounds          every sound: engine, preset, their names (tab-separated)
 *   app-host --ms 3000 --keys '500:C4@100,900:C4^,1200:PLAY,1300:PLAY^,1500:e2:+3,0:pot:900' \
 *            --out DIR --snap 400,1000
 * writes DIR/audio.wav (44.1 kHz mono s16), DIR/lcd_<ms>.ppm and DIR/leds_<ms>.json at each snap,
 * DIR/log.txt (kit_log lines) and DIR/midi_out.txt (what the app sent out: ms and the message in hex).
 * T:mHEX in the script puts those MIDI bytes into the TRS jack, as in the emulator. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kit_internal.h"

static uint16_t fb[KIT_W * KIT_H];
uint16_t *kit__fb = fb;

static uint32_t now_ms_;
static int pot = 900;                   /* where the emulator's MASTER pot rests (fm1-emu periph.rs) */
static uint8_t lights[41];
static uint32_t in_notes, in_buttons;
static int8_t in_enc[KIT_NENCS];
static uint8_t in_vel[KIT_NKEYS];
static FILE *logf, *midif;
static uint8_t flash[KIT_STORE_SECTORS + KIT_BANK_SECTORS][4096];  /* the store's sectors, then the bank's (--store FILE keeps them) */
static const char *store_path;

static int kit__flash_read_any(int sector, int off, uint8_t *dst, int n)
{
    if (off < 0 || off + n > 4096) return -1;
    memcpy(dst, &flash[sector][off], (size_t)n);
    return 0;
}
int kit__flash_read(int sector, int off, uint8_t *dst, int n)
{
    if (sector < 0 || sector >= KIT_STORE_SECTORS || off < 0 || off + n > 4096) return -1;
    memcpy(dst, &flash[sector][off], (size_t)n);
    return 0;
}
int kit__flash_erase(int sector)
{
    if (sector < 0 || sector >= KIT_STORE_SECTORS) return -1;
    memset(flash[sector], 0xFF, 4096);
    return 0;
}
int kit__bank_read(int sector, int off, uint8_t *dst, int n)
{
    return sector < 0 || sector >= KIT_BANK_SECTORS ? -1 : kit__flash_read_any(KIT_STORE_SECTORS + sector, off, dst, n);
}
int kit__bank_erase(int sector)
{
    if (sector < 0 || sector >= KIT_BANK_SECTORS) return -1;
    memset(flash[KIT_STORE_SECTORS + sector], 0xFF, 4096);
    return 0;
}
int kit__bank_write_at(int sector, int off, const uint8_t *src, int n)
{
    int i;
    if (sector < 0 || sector >= KIT_BANK_SECTORS || off < 0 || off + n > 4096) return -1;
    for (i = 0; i < n; i++)
        flash[KIT_STORE_SECTORS + sector][off + i] &= src[i];
    return 0;
}
int kit__flash_write(int sector, int off, const uint8_t *src, int n)
{
    int i;
    if (sector < 0 || sector >= KIT_STORE_SECTORS || off < 0 || off + n > 4096) return -1;
    for (i = 0; i < n; i++)                     /* NOR flash: programming only clears bits */
        flash[sector][off + i] &= src[i];
    return 0;
}

void kit__midi_send(uint8_t st, uint8_t d1, uint8_t d2)
{
    if (midif) fprintf(midif, "%u %02x%02x%02x\n", now_ms_, st, d1, d2);
}
void kit__midi_send_rt(uint8_t b)
{
    if (midif) fprintf(midif, "%u %02x\n", now_ms_, b);
}
int kit_midi_connected(void) { return 1; }
uint32_t kit__lock(void) { return 0; }
void kit__unlock(uint32_t k) { (void)k; }
int kit__pot(void) { return pot; }
int kit_usb_audio_connected(void) { return 0; }   /* API 48: no USB on the PC */
uint32_t kit_ms(void) { return now_ms_; }
void kit__light_set(int id, int level) { if (id >= 0 && id < 41) lights[id] = (uint8_t)level; }
void kit_log(const char *s)
{
    if (logf) fprintf(logf, "%u %s\n", now_ms_, s);
}

/* ---- the script ---- */
typedef struct { uint32_t t; int kind, id, down, vel, delta; uint8_t bytes[8]; } act_t;   /* kind 0 key/button, 1 enc, 2 pot, 3 MIDI */
static act_t acts[512];
static int nacts;

static const char *BTN[KIT_NBTNS] = { "OCTDN", "OCTUP", "FX", "SCL", "ENV", "LFO", "EDIT", "GLO",
                                      "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC" };

static int key_id(const char *n)      /* matrix id like the emulator: 0..13 buttons, 14..40 keys */
{
    char u[16];
    int i, base, acc = 0, oct, midi;
    const char *p;
    for (i = 0; n[i] && i < 15; i++) u[i] = (char)toupper((unsigned char)n[i]);
    u[i] = 0;
    for (i = 0; i < KIT_NBTNS; i++)
        if (!strcmp(u, BTN[i])) return i;
    if (u[0] == 'N' && isdigit((unsigned char)u[1])) { i = atoi(u + 1); return i < 27 ? 14 + i : -1; }
    if (u[0] == 'K' && isdigit((unsigned char)u[1])) { i = atoi(u + 1); return i < 41 ? i : -1; }
    switch (u[0]) {
    case 'C': base = 0; break; case 'D': base = 2; break; case 'E': base = 4; break;
    case 'F': base = 5; break; case 'G': base = 7; break; case 'A': base = 9; break;
    case 'B': base = 11; break; default: return -1;
    }
    p = u + 1;
    if (*p == '#') { acc = 1; p++; } else if (*p == 'B' && p[1]) { acc = -1; p++; }
    oct = atoi(p);
    midi = (oct + 1) * 12 + base + acc;
    return midi >= KIT_KEY0_NOTE && midi < KIT_KEY0_NOTE + 27 ? 14 + midi - KIT_KEY0_NOTE : -1;
}

static int enc_id(const char *n)
{
    if (!strcasecmp(n, "SELECT")) return 0;
    if (!strcasecmp(n, "ALGO") || !strcasecmp(n, "ALGORITHM")) return 1;
    if (!strcasecmp(n, "PRESET") || !strcasecmp(n, "PRESETS")) return 6;
    if ((n[0] == 'K' || n[0] == 'k') && n[1] >= '1' && n[1] <= '4' && !n[2]) return 2 + n[1] - '1';
    if ((n[0] == 'e' || n[0] == 'E') && isdigit((unsigned char)n[1])) return atoi(n + 1) % KIT_NENCS;
    return -1;
}

static void parse_script(const char *s)
{
    char buf[4096], *tok, *save;
    strncpy(buf, s, sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (tok = strtok_r(buf, ",", &save); tok && nacts < 512; tok = strtok_r(0, ",", &save)) {
        act_t a = { 0, 0, -1, 1, 100, 0 };
        char *c1 = strchr(tok, ':'), *name, *c2, *at;
        if (!c1) continue;
        *c1 = 0;
        a.t = (uint32_t)atoi(tok);
        name = c1 + 1;
        c2 = strchr(name, ':');
        if (c2) {                                   /* T:pot:V or T:eN:+K */
            *c2 = 0;
            if (!strcasecmp(name, "pot")) { a.kind = 2; a.delta = atoi(c2 + 1); acts[nacts++] = a; continue; }
            a.kind = 1;
            a.id = enc_id(name);
            a.delta = atoi(c2 + 1);
            if (a.id >= 0) acts[nacts++] = a;
            continue;
        }
        if ((name[0] == 'm' || name[0] == 'M') && isxdigit((unsigned char)name[1])) {   /* T:mHEX: MIDI into the jack */
            int k;
            for (k = 0; name[1 + 2 * k] && name[2 + 2 * k] && k < 8; k++) {
                char hx[3] = { name[1 + 2 * k], name[2 + 2 * k], 0 };
                a.bytes[k] = (uint8_t)strtol(hx, 0, 16);
            }
            a.kind = 3;
            a.delta = k;
            acts[nacts++] = a;
            continue;
        }
        if (name[strlen(name) - 1] == '^') { a.down = 0; name[strlen(name) - 1] = 0; }
        if ((at = strchr(name, '@'))) { *at = 0; a.vel = atoi(at + 1); }
        a.id = key_id(name);
        if (a.id >= 0) acts[nacts++] = a;
        else fprintf(stderr, "script: unknown key '%s'\n", name);
    }
}

static void apply(const act_t *a)
{
    if (a->kind == 2) { pot = a->delta; return; }
    if (a->kind == 3) {
        int k;
        for (k = 0; k < a->delta; k++) kit__midi_byte(1, a->bytes[k]);
        return;
    }
    if (a->kind == 1) { in_enc[a->id] = (int8_t)(in_enc[a->id] + a->delta); return; }
    if (a->id < 14) {
        if (a->down) in_buttons |= 1u << a->id; else in_buttons &= ~(1u << a->id);
    } else {
        if (a->down) {
            in_notes |= 1u << (a->id - 14);
            in_vel[a->id - 14] = (uint8_t)(a->vel < 1 ? 1 : a->vel > 127 ? 127 : a->vel);
        } else {
            in_notes &= ~(1u << (a->id - 14));
        }
    }
}

/* ---- outputs ---- */
static void write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", KIT_W, KIT_H);
    for (i = 0; i < KIT_W * KIT_H; i++) {
        uint16_t c = fb[i];
        unsigned char rgb[3] = { (unsigned char)((c >> 11) * 255 / 31), (unsigned char)(((c >> 5) & 63) * 255 / 63),
                                 (unsigned char)((c & 31) * 255 / 31) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

static void write_leds(const char *path)
{
    FILE *f = fopen(path, "w");
    int i;
    if (!f) { perror(path); exit(1); }
    fprintf(f, "{\"buttons\": [");
    for (i = 0; i < 14; i++) fprintf(f, "%s%d", i ? ", " : "", lights[i]);
    fprintf(f, "], \"keys\": [");
    for (i = 0; i < 27; i++) fprintf(f, "%s%d", i ? ", " : "", lights[14 + i]);
    fprintf(f, "]}\n");
    fclose(f);
}

static void put32(FILE *f, uint32_t v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); fputc((v >> 16) & 255, f); fputc(v >> 24, f); }
static void put16(FILE *f, uint32_t v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }

int main(int argc, char **argv)
{
    uint32_t total_ms = 3000, snaps[64], next_frame = 0, samples = 0;
    int nsnaps = 0, i, a = 0;
    const char *out = "work/host";
    char path[512];
    FILE *wav;
    int16_t blk[KIT_BLOCK];
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--ms") && i + 1 < argc) total_ms = (uint32_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--keys") && i + 1 < argc) parse_script(argv[++i]);
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else if (!strcmp(argv[i], "--store") && i + 1 < argc) store_path = argv[++i];
        else if (!strcmp(argv[i], "--list-sounds")) {   /* every engine's presets, in order: engine preset name */
            int e, p;
            for (e = 0; e < kit_engine_count(); e++)
                for (p = 0; p < kit_preset_count(e); p++)
                    printf("%d\t%d\t%s\t%s\n", e, p, kit_engine_name(e), kit_preset_name(e, p));
            return 0;
        }
        else if (!strcmp(argv[i], "--snap") && i + 1 < argc) {
            char *s = argv[++i], *tok, *save;
            for (tok = strtok_r(s, ",", &save); tok && nsnaps < 64; tok = strtok_r(0, ",", &save))
                snaps[nsnaps++] = (uint32_t)atoi(tok);
        } else { fprintf(stderr, "usage: %s [--ms N] [--keys SCRIPT] [--out DIR] [--snap MS,MS]\n", argv[0]); return 2; }
    }
    /* actions in time order (insertion sort: scripts are short) */
    for (i = 1; i < nacts; i++) {
        act_t k = acts[i];
        int j = i - 1;
        while (j >= 0 && acts[j].t > k.t) { acts[j + 1] = acts[j]; j--; }
        acts[j + 1] = k;
    }
    snprintf(path, sizeof path, "%s/log.txt", out);
    logf = fopen(path, "w");
    snprintf(path, sizeof path, "%s/midi_out.txt", out);
    midif = fopen(path, "w");
    snprintf(path, sizeof path, "%s/audio.wav", out);
    wav = fopen(path, "wb");
    if (!wav || !logf) { fprintf(stderr, "cannot write into %s (does it exist?)\n", out); return 1; }
    fwrite("RIFF\0\0\0\0WAVEfmt ", 1, 16, wav);
    put32(wav, 16); put16(wav, 1); put16(wav, 1); put32(wav, KIT_FS); put32(wav, KIT_FS * 2); put16(wav, 2); put16(wav, 16);
    fwrite("data\0\0\0\0", 1, 8, wav);

    memset(flash, 0xFF, sizeof flash);
    if (store_path) {                           /* the store as the last run left it */
        FILE *f = fopen(store_path, "rb");
        if (f) {
            size_t got = fread(flash, 1, sizeof flash, f);   /* a file from before the bank: the store alone */
            if (got != sizeof flash && got != (size_t)KIT_STORE_SECTORS * 4096u) memset(flash, 0xFF, sizeof flash);
            fclose(f);
        }
    }
    kit__synth_init();
    app_init();
    while (now_ms_ < total_ms) {
        while (a < nacts && acts[a].t <= now_ms_) apply(&acts[a++]);
        kit__player_poll(now_ms_);
        kit__seq_poll(now_ms_);
        kit__store_poll(now_ms_);
        kit__bank_poll(now_ms_);
        if (now_ms_ >= next_frame) {             /* the main loop: ~100 passes a second */
            kit__core_poll(in_notes, in_buttons, in_enc, in_vel);
            memset(in_enc, 0, sizeof in_enc);
            app_frame(now_ms_);
            kit__dirty_y0 = KIT_H; kit__dirty_y1 = -1;
            next_frame = now_ms_ + 10;
        }
        for (i = 0; i < nsnaps; i++)
            if (snaps[i] == now_ms_) {
                snprintf(path, sizeof path, "%s/lcd_%u.ppm", out, snaps[i]);
                write_ppm(path);
                snprintf(path, sizeof path, "%s/leds_%u.json", out, snaps[i]);
                write_leds(path);
            }
        while ((uint64_t)samples * 1000u < (uint64_t)(now_ms_ + 1) * KIT_FS) {
            kit__synth_render(blk, KIT_BLOCK);
            for (i = 0; i < KIT_BLOCK; i++)     /* the device's -6 dBFS ceiling (device.c OUT_SHIFT 7 of 8) */
                blk[i] = (int16_t)(blk[i] / 2);
            fwrite(blk, 2, KIT_BLOCK, wav);
            samples += KIT_BLOCK;
        }
        now_ms_++;
    }
    fflush(wav);
    fseek(wav, 4, SEEK_SET); put32(wav, 36 + samples * 2);
    fseek(wav, 40, SEEK_SET); put32(wav, samples * 2);
    fclose(wav);
    fclose(logf);
    if (midif) fclose(midif);
    if (store_path) {
        FILE *f = fopen(store_path, "wb");
        if (!f || fwrite(flash, 1, sizeof flash, f) != sizeof flash) { perror(store_path); return 1; }
        fclose(f);
    }
    return 0;
}
