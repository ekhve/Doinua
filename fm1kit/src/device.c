/* SPDX-License-Identifier: MPL-2.0
 * Derived from felucca-bsp examples/hwtest/hwtest.c,
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments. Changes for fm1kit. */
/* fm1kit device backend: the FM-1, on felucca-bsp. One translation unit with the BSP headers.
 *
 *   boot     watchdog, boot-loop guard (two boots that never reach 2 s healthy -> UBOOT),
 *            fault screen; then LCD, keys (TIMER5 10 kHz), ADC, audio (ALNK0 I2S), USB
 *   safety   kept from hwtest and never left to the app: the USB-MIDI update entry (the installer
 *            replaces this firmware), the SysEx UBOOT soft key, and OCT- + OCT+ held 5 s -> UBOOT
 *   screen   a 240x240 RGB565 framebuffer in the pool; the dirty rows go out after each app_frame
 *   sound    kit__synth_render fills each half of the I2S buffer from the audio interrupt
 *   lights   off / dim / on / breath through the scan's LED arrays */
#include <stdint.h>
#define FELUCCA_CDC 1
#define FELUCCA_OTA 1
#ifndef FM1KIT_PRODUCT
#define FM1KIT_PRODUCT "FM-1_1301"
#endif
#define FELUCCA_ID FM1KIT_PRODUCT       /* the identity the installer sees, == the package's --product */
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
/* the only flash this firmware may write: the update's staging area (ota.c) and fm1kit's store (kit_store_*,
 * store.c: four candidate 4 KiB sectors in the BSP's extended data area XDATA0, 0x93000..0x96FFF, D-022). Never
 * the stock firmware's VM, BTIF and USR areas (0xE7000..0xFBFFF) or key_mac, as Felucca and Optimist keep them;
 * every package (ours too) writes the app area only up to 0x93000, so a reinstall leaves the store alone.
 * Everything else is read-only to us (felucca-bsp fm1_flash_policy.h). */
#define KIT_STORE_LO 0x00093000u
#define KIT_STORE_HI 0x00097000u
/* and the song (kit_bank_*, bank.c, API 31, D-085): Felucca's project area 0x97000..0x9EFFF, the owner's choice B in
 * Decision #115; the bank, as the store, writes only sectors that are blank or its own */
#define KIT_BANK_LO 0x00097000u
#define KIT_BANK_HI 0x0009F000u           /* KIT_BANK_SECTORS (8) sectors: checked below */
#define FM1_FLASH_WRITE_REGIONS(X) X(0x000E0000u, 0x000E5000u) X(KIT_STORE_LO, KIT_STORE_HI) X(KIT_BANK_LO, KIT_BANK_HI)
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#include "fm1_timer.h"
/* velocity: the keys' carbon pills, read by the scan (felucca-bsp fm1_input.h "Analog rows", as fm1-velocity, CS-1
 * and fm1-ctl read them). A held key on rows PA7 / PA8 (keys 0..19) is read with its row driven: lower = pressed
 * harder; the top row (keys 20..26) through the PA10 probe: higher = harder. Per key since its press: the extreme
 * reading and how many there were; the main loop turns them into the key's velocity (key_velocity). */
#define KIT_ROWKEYS 20u
static volatile uint16_t touch_ext[27];        /* rows: the lowest driven reading; top row: the highest probe reading */
static volatile uint8_t touch_n[27];
static inline void kit__touch_row(int32_t k, uint32_t v, uint32_t driven)
{
    if (driven && k >= 0 && k < (int32_t)KIT_ROWKEYS) {
        if (!touch_n[k] || v < touch_ext[k])
            touch_ext[k] = (uint16_t)v;
        if (touch_n[k] < 255u)
            touch_n[k]++;
    }
}
static inline void kit__touch_top(int32_t k, uint32_t v, uint32_t pressed)
{
    if (pressed && k >= (int32_t)KIT_ROWKEYS && k < 27) {
        if (!touch_n[k] || v > touch_ext[k])
            touch_ext[k] = (uint16_t)v;
        if (touch_n[k] < 255u)
            touch_n[k]++;
    }
}
#define KIT_HELD(col, rowbit) (FM1_KEYMAP[rowbit][col] >= 14 && ((fm1_in.notes >> (FM1_KEYMAP[rowbit][col] - 14)) & 1u))
#define FM1_INPUT_DRIVE(col, rowbit) KIT_HELD(col, rowbit)
#define FM1_INPUT_LED_OK(col, rowbit) (!KIT_HELD(col, rowbit))
#define FM1_INPUT_ANALOG(col, rowbit, v, driven) kit__touch_row(FM1_KEYMAP[rowbit][col] - 14, v, driven)
#define FM1_INPUT_PROBE(col, rowbit, v, pressed) kit__touch_top(FM1_KEYMAP[rowbit][col] - 14, v, pressed)
#include "fm1_input.h"
#include "fm1_adc.h"
#include "fm1_lcd.h"
#include "fm1_flash.h"
#include "fm1_audio.h"
#include "fm1_uart.h"
#include "libc.c"
/* USB audio (API 48, D-130, ST-98): felucca-bsp's class-compliant UAC1 at 44.1 kHz, as Felucca ships it: the FM-1's
 * output to the computer (EP4 IN) and the computer's audio in (EP4 OUT). The update loader stays MIDI-only (its own
 * build); the SysEx soft key and the installer's session go over the same USB-MIDI interface as before. */
#define HALF_FRAMES 128u                       /* the audio's half buffer, frames (usb.c sizes its rings by it) */
#define FELUCCA_UAC 1
#define FELUCCA_UAC_48K 0
#define FELUCCA_UAC_OUT 1
#include "usb.c"
#include "ota.c"
#include "kit_internal.h"

extern uint32_t _data_start[], _data_end[], _data_load[], _bss_start[], _bss_end[];
extern uint32_t _pool_start[], _pool_end[], _rt_start[], _rt_end[], _rt_load[];

static volatile uint32_t ms;
static uint32_t boots_ok;
/* volatile (felucca-bsp#17): a compiler that sees the record only ever given the magic can drop the compare,
 * and the first boot after an install lands in UBOOT */
static volatile struct { uint32_t magic, pending, failed; } guard __attribute__((section(".noinit")));
static volatile int pot_value = 1023;
volatile int kit__audio_rc = 1;                /* fm1_audio_init_checked's answer (0 = running): diagnostics */

/* ---- backend services ---- */
uint32_t kit__lock(void) { return irq_save(); }
void kit__unlock(uint32_t k) { irq_restore(k); }
int kit__pot(void) { return pot_value; }
int kit_usb_audio_connected(void) { return usb.config && (uac.alt || uo.alt); }   /* API 48 */
uint32_t kit_ms(void) { return ms; }

/* ---- timer: 10 kHz key scan, ms, USB at 2 kHz ---- */
void fm1_timer5_irq(void)
{
    static uint32_t sub, last, acc;
    uint32_t t = fm1_ticks();
    fm1_timer5_ack();
    fm1_input_tick();
    acc += t - last;
    last = t;
    while (acc >= 1000u * FM1_TICKS_PER_US) {
        acc -= 1000u * FM1_TICKS_PER_US;
        ms++;
    }
    if (++sub == 5u) {
        sub = 0;
        usb_poll();
    }
}
extern void isr_timer5(void);

/* ---- audio: ALNK0 I2S, two halves of HALF_FRAMES stereo frames, 24-bit right-justified in int32.
 * Q15 << 7: a -6 dBFS ceiling, as Felucca (its audio.c OUT_SHIFT: the codec ran clean at -18 dBFS) ---- */
#define OUT_SHIFT 7
#define HALF_WORDS (HALF_FRAMES * 2u)
static int32_t abuf[2u * HALF_WORDS];
static int16_t mono[HALF_FRAMES];

void fm1_alnk0_irq(void)
{
    uint8_t p = fm1_audio_pending();
    fm1_audio_ack_aux(p);
    if (p & FM1_AUDIO_HALF) {
        uint32_t half = fm1_audio_free_half(), i;
        int32_t *o = &abuf[half * HALF_WORDS];
        uac_render_start();                     /* USB audio (API 48): the computer's half of the exchange */
        kit__synth_render(mono, (int)HALF_FRAMES);
        for (i = 0; i < HALF_FRAMES; i++)
            o[2u * i] = o[2u * i + 1u] = (int32_t)mono[i] * (1 << OUT_SHIFT);
        uac_tap(o, HALF_FRAMES);                /* the FM-1's output to the computer, before the input joins it */
        uac_play(o, HALF_FRAMES, kit__usb_in_gain());   /* the computer's audio, heard at the input's level (0: not) */
        fm1_audio_ack_half();
    }
}
extern void isr_alnk0(void);

/* ---- screen: ST7789 240x240, RGB565 sent high byte first ---- */
static uint16_t fb[KIT_W * KIT_H] __attribute__((section(".pool")));
uint16_t *kit__fb = fb;
static void lcd_flush(void)                     /* the dirty rows, full width */
{
    int y0 = kit__dirty_y0, y1 = kit__dirty_y1;
    if (y1 < y0)
        return;
    kit__dirty_y0 = KIT_H;
    kit__dirty_y1 = -1;
    fm1_lcd_blit(0, (uint32_t)y0, KIT_W, (uint32_t)(y1 - y0 + 1), fb + y0 * KIT_W,
                 (uint32_t)((KIT_H - y0) * KIT_W));
    fm1_wdt_feed();
}

static void lcd_fill_now(uint16_t c)            /* outside the framebuffer: fault and UBOOT screens */
{
    int i;
    for (i = 0; i < KIT_W * KIT_H; i++)
        fb[i] = c;
    kit__dirty(0, KIT_H - 1);
    lcd_flush();
}

/* ---- lights ---- */
static void led_bits(uint8_t *arr, uint32_t id, int on)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on)
                    arr[p] |= (uint8_t)(1u << r);
                else
                    arr[p] &= (uint8_t)~(1u << r);
            }
}

void kit__light_set(int id, int level)
{
    uint32_t k;
    if (id < 0 || id >= (int)FM1_NKEY)
        return;
    k = irq_save();
    led_bits(fm1_led, (uint32_t)id, level == KIT_LIGHT_ON);
    led_bits(fm1_led_dim, (uint32_t)id, level == KIT_LIGHT_DIM || level == KIT_LIGHT_HALF);   /* HALF: the mid level over the glow */
    led_bits(fm1_led_mid, (uint32_t)id, level == KIT_LIGHT_HALF);
    led_bits(fm1_led_breath, (uint32_t)id, level == KIT_LIGHT_BREATH);
    irq_restore(k);
}

/* ---- USB serial: kit_log ---- */
static void put(char c)
{
    if (!cdc.dtr || co_w - co_r >= CO_N)
        return;
    cdc_out[co_w % CO_N] = (uint8_t)c;
    RING_PUBLISH();
    co_w++;
}

void kit_log(const char *s)
{
    while (*s)
        put(*s++);
    put('\r');
    put('\n');
}

/* ---- MIDI: in from the TRS jack (UART1 RX DMA ring, felucca-bsp fm1_uart.h) and USB-MIDI (usb.c's ring),
 * fed to the parser from the main loop; out over USB-MIDI ---- */
#define UART_RING 256u
static volatile uint8_t uart_ring[UART_RING] __attribute__((aligned(16)));
static uint32_t uart_pos;

static void midi_poll(void)
{
    uint32_t n = fm1_uart1_rx_take();
    while (n--)                                 /* the TRS jack */
        kit__midi_byte(1, uart_ring[uart_pos++ & (UART_RING - 1u)]);
    if (midi_in_overflow) {                     /* usb.c dropped a broken stream: start clean */
        mi_r = mi_w;
        midi_in_overflow = 0;
    }
    while (mi_r != mi_w) {                      /* USB: 4-byte event packets, [CIN][status][d1][d2] */
        uint32_t pkt = midi_in_q[mi_r % MQ], cin = pkt & 15u, nb, k;
        mi_r++;
        nb = cin == 0xCu || cin == 0xDu ? 2u : cin >= 8u ? 3u : 1u;
        for (k = 0; k < nb; k++)
            kit__midi_byte(0, (uint8_t)(pkt >> (8u * (k + 1u))));
    }
}

void kit__midi_send(uint8_t st, uint8_t d1, uint8_t d2)
{
    uint32_t k = irq_save();
    (void)midi_out_try((uint32_t)(st >> 4) | (uint32_t)st << 8 | (uint32_t)d1 << 16 | (uint32_t)d2 << 24);
    irq_restore(k);
}

void kit__midi_send_rt(uint8_t b)               /* a single-byte USB-MIDI event: CIN F */
{
    uint32_t k = irq_save();
    (void)midi_out_try(0x0Fu | (uint32_t)b << 8);
    irq_restore(k);
}

int kit_midi_connected(void) { return usb_midi_out_ready(); }

/* ---- store: KIT_STORE_SECTORS sectors from KIT_STORE_LO (the only app data region FM1_FLASH_WRITE_REGIONS allows) ---- */
static void audio_silence(void)                 /* both halves zeroed: while the flash is busy no interrupt runs, the
                                                 * DMA repeats the buffer, and it is silence instead of a buzz */
{
    uint32_t i;
    for (i = 0; i < 2u * HALF_WORDS; i++)
        abuf[i] = 0;
}
int kit__flash_read(int sector, int off, uint8_t *dst, int n)
{
    return fl_read(KIT_STORE_LO + (uint32_t)sector * 0x1000u + (uint32_t)off, dst, (uint32_t)n) ? -1 : 0;
}
int kit__flash_erase(int sector)
{
    uint32_t took;
    audio_silence();
    return fl_erase4k(KIT_STORE_LO + (uint32_t)sector * 0x1000u, &took) ? -1 : 0;
}
int kit__flash_write(int sector, int off, const uint8_t *src, int n)
{
    audio_silence();
    return fl_write(KIT_STORE_LO + (uint32_t)sector * 0x1000u + (uint32_t)off, src, (uint32_t)n) ? -1 : 0;
}

/* ---- bank: KIT_BANK_SECTORS sectors from KIT_BANK_LO (API 31) ---- */
typedef char kit_bank_fits[(KIT_BANK_HI - KIT_BANK_LO == KIT_BANK_SECTORS * 0x1000u) ? 1 : -1];
int kit__bank_read(int sector, int off, uint8_t *dst, int n)
{
    if (sector < 0 || sector >= KIT_BANK_SECTORS) return -1;
    return fl_read(KIT_BANK_LO + (uint32_t)sector * 0x1000u + (uint32_t)off, dst, (uint32_t)n) ? -1 : 0;
}
int kit__bank_erase(int sector)
{
    uint32_t took;
    if (sector < 0 || sector >= KIT_BANK_SECTORS) return -1;
    audio_silence();
    return fl_erase4k(KIT_BANK_LO + (uint32_t)sector * 0x1000u, &took) ? -1 : 0;
}
int kit__bank_write_at(int sector, int off, const uint8_t *src, int n)
{
    if (sector < 0 || sector >= KIT_BANK_SECTORS) return -1;
    audio_silence();
    return fl_write(KIT_BANK_LO + (uint32_t)sector * 0x1000u + (uint32_t)off, src, (uint32_t)n) ? -1 : 0;
}

/* ---- the update (ota.c): the installer's M-UPGRADE session over USB-MIDI ---- */
static uint8_t flash_ok;                        /* a known flash part: the update may write it */
static uint32_t ota_now_ms(void) { return ms; }
static void ota_idle(void) { fm1_wdt_feed(); }
static int ota_in_area(uint32_t off, uint32_t n) { return FL_IN(off, n, OTA_AREA, OTA_AREA + OTA_AREA_LEN); }
static int ota_erase(uint32_t off)
{
    uint32_t took;
    if (!ota_in_area(off, 0x1000u) || (off & 0xFFFu))
        return -8;
    return fl_erase4k(off, &took);
}
static int ota_prog(uint32_t off, const void *p, uint32_t n)
{
    if (!ota_in_area(off, n))
        return -8;
    return fl_write(off, p, n);
}
static int ota_fread(uint32_t off, void *p, uint32_t n) { return fl_read(off, p, n); }
static void ota_show(uint32_t step, int32_t code)   /* the steps as a bar across the bottom */
{
    kit_rect(0, 200, 240, 24, KIT_BLACK);
    if (step < 9u)
        kit_rect(0, 200, (int)(step > 6u ? 6u : step) * 40, 24, KIT_RGB(0, 200, 255));
    else
        kit_rect(0, 200, 240, 24, code == 1 ? KIT_RGB(0, 220, 0) : KIT_RGB(255, 0, 0));
    lcd_flush();
}
static void ota_commit(const uint8_t *parm)
{
    guard.pending = 0;                          /* an intentional reset, not a failed boot */
    guard.failed = 0;
    fm1_audio_stop();
    usb_detach();
    fm1_delay_ms(30);
    fm1_enter_update(parm);
}

static void enter_uboot(void)
{
    fm1_audio_stop();
    lcd_fill_now(KIT_RGB(0, 0, 120));
    kit_text(120, 108, "UPDATE MODE", KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, KIT_WHITE);
    lcd_flush();
    fm1_delay_ms(20);
    usb_detach();
    fm1_delay_ms(30);
    guard.pending = 0;
    guard.failed = 0;
    fm1_enter_uboot();
}

/* a fault: red screen, then reset; the record in fm1_crash survives to the next boot */
static void fm1_fault(const fm1_crash_t *c)
{
    (void)c;
    fm1_audio_stop();
    lcd_fill_now(KIT_RGB(255, 0, 0));
    fm1_delay_ms(1500);
    fm1_reboot();
}

/* ---- velocity: a key's pill reading -> 1..127. Rows (keys 0..19): the driven reading at each velocity, measured on
 * fm1-emu's pill model (CS-1's measurements of a real FM-1; factory/scenarios/kit_velocity.json checks it), and
 * interpolated between. The top row (keys 20..26) reads through the PA10 probe, which the emulator does not model:
 * it gets 100 until it is calibrated on a real unit (docs/PLATFORM.md D-017). ---- */
static const int16_t VEL_READ[6] = { 985, 954, 908, 790, 659, 546 };   /* driven reading ... */
static const int16_t VEL_AT[6] = { 1, 16, 32, 64, 96, 127 };           /* ... at this velocity */
#define VEL_WAIT_MS 12u                         /* a press waits at most this long for its readings */

static int key_velocity(uint32_t k)
{
    int32_t v = touch_ext[k];
    uint32_t i;
    if (!touch_n[k] || k >= KIT_ROWKEYS)
        return 100;                             /* no reading in time, or the top row: a middle velocity */
    if (v >= VEL_READ[0])
        return 1;
    for (i = 1; i < 6u; i++)
        if (v >= VEL_READ[i])
            return VEL_AT[i - 1] + (VEL_READ[i - 1] - v) * (VEL_AT[i] - VEL_AT[i - 1]) / (VEL_READ[i - 1] - VEL_READ[i]);
    return 127;
}

static void app_main(void)
{
    uint32_t last_frame = 0, oct_held_since = 0, oct_held = 0, i;
    uint32_t shown = 0, pressing = 0, press_at[27];   /* keys reported to the app; presses waiting; since when */
    uint8_t vel[27];
    int8_t steps[KIT_NENCS];
    {
        uint32_t id = fl_jedec();
        flash_ok = id == 0x856014u || id == 0xB36014u;   /* the two 1 MiB parts FM-1s ship with */
        if (flash_ok)
            ota_boot_cleanup();                 /* what an interrupted update left behind */
    }
    fm1_lcd_init();
    kit_clear(KIT_BLACK);
    lcd_flush();
    fm1_input_init();
    fm1_adc_init();
    pot_value = fm1_adc_read(FM1_ADC_MASTER);   /* (from here on the scan owns the converter: fm1_an.master) */
    fm1_an.drive = 1;                           /* the pills: velocity (felucca-bsp fm1_input.h "Analog rows") */
    fm1_an.probe = 1;
    fm1_an.con = (uint16_t)FM1_ADC_CON(7, 2);
    for (i = 0; i < 2u * HALF_WORDS; i++)
        abuf[i] = 0;
    kit__synth_init();
    kit__audio_rc = fm1_audio_init_checked(abuf, 2u * HALF_WORDS, HALF_WORDS, FM1_AUDIO_RATE, isr_alnk0, 3);
    usb_start();
    fm1_uart1_midi_init(uart_ring, UART_RING);  /* (before TIMER5: it writes port H) */
    fm1_timer5_start(isr_timer5, 4);            /* above ALNK0 (3): the scan nests into the render */
    fm1_guard_lock_top();
    fm1_irq_enable_all();
    {
        uint32_t f = irq_save();                /* the scan writes port A too */
        fm1_lcd_backlight(1);
        irq_restore(f);
    }
    if (kit__audio_rc)                          /* refused (an unknown audio clock, ...): silent, and say so */
        kit_log("fm1kit: audio did not start");
    app_init();
    lcd_flush();
    for (;;) {
        uint32_t now = ms;
        fm1_wdt_feed();
        usb_retry(now);
        if (usb.uboot_req)                      /* the SysEx soft key */
            enter_uboot();
        ota_service();                          /* the installer's handshake */
        if (usb.ota_req) {                      /* and its update: commits and resets, or comes back */
            usb.ota_req = 0;
            kit_all_notes_off();
            if (flash_ok)
                ota_session();
            kit_clear(KIT_BLACK);
            app_init();
        }
        if (!boots_ok && now > 2000u) {         /* it runs: the next boot is not a loop */
            boots_ok = 1;
            guard.pending = 0;
            guard.failed = 0;
        }
        if ((fm1_in.buttons & 3u) == 3u) {      /* OCT- + OCT+ held 5 s: update mode, whatever the app does */
            if (!oct_held) {                    /* (a flag, not a time of 0: ms starts at 0) */
                oct_held = 1;
                oct_held_since = now;
            } else if (now - oct_held_since >= 5000u) {
                enter_uboot();
            }
        } else {
            oct_held = 0;
        }
        if (fm1_an.master)
            pot_value = fm1_an.master;
        while (ci_r != ci_w) {                  /* serial: u = UBOOT, r = reboot */
            char c = (char)cdc_in[ci_r % CI_N];
            ci_r++;
            if (c == 'u')
                enter_uboot();
            if (c == 'r') {
                usb_detach();
                fm1_delay_ms(30);
                fm1_reboot();
            }
        }
        {                                       /* input, every pass: a press goes to the app with its velocity */
            uint32_t notes = fm1_in.notes, ready = shown & notes;
            int moved = 0;
            for (i = 0; i < 27u; i++) {
                uint32_t b = 1u << i;
                if ((notes & b) && !(shown & b)) {
                    if (!(pressing & b)) {      /* a new press: its readings start now */
                        uint32_t f = irq_save();
                        touch_n[i] = 0;
                        irq_restore(f);
                        press_at[i] = now;
                        pressing |= b;
                    }
                    if (touch_n[i] >= 2u || now - press_at[i] >= VEL_WAIT_MS) {
                        vel[i] = (uint8_t)key_velocity(i);
                        ready |= b;
                        pressing &= ~b;
                    }
                } else if (!(notes & b)) {
                    pressing &= ~b;
                }
            }
            for (i = 0; i < KIT_NENCS; i++) {
                steps[i] = (int8_t)fm1_enc_take(i);
                moved |= steps[i];
            }
            if (ready != shown || moved || fm1_in.buttons != kit__core_buttons()) {
                kit__core_poll(ready, fm1_in.buttons, steps, vel);
                shown = ready;
            }
        }
        midi_poll();
        kit__player_poll(kit_ms());             /* the player's next note (player.c) */
        kit__seq_poll(kit_ms());                /* the song player (seq.c) */
        kit__store_poll(now);                   /* a queued record, written when it is due (store.c) */
        kit__bank_poll(now);                    /* and the song (bank.c) */
#ifdef FM1KIT_TEST_FAULT_MS
        /* factory/safety.py only (tools/build.py --test-fault MS, never a release package): a write to the NULL page
         * MS after boot, so the boot guard can be seen catching a firmware that crashes at boot (R-036, #9) */
        if (now >= FM1KIT_TEST_FAULT_MS)
            *(volatile uint32_t *)0 = 1;
#endif
        if (now - last_frame >= 10u) {          /* ~100 frames a second at most */
            last_frame = now;
            app_frame(now);
            lcd_flush();
        }
    }
}

void fm1_cstart(void)
{
    uint32_t *s, *d;
    fm1_time_init();
    fm1_wdt_arm(0x0D);
    if (guard.magic != 0x464B4954u || guard.pending > 1u || guard.failed > 1u) {   /* "FKIT"; they only hold 0, 1 */
        guard.magic = 0x464B4954u;
        guard.failed = 0;
        guard.pending = 0;
    }
    if (guard.pending)
        guard.failed++;
    guard.pending = 1;
    if (guard.failed >= 2u) {                   /* two boots that never got healthy: UBOOT */
        guard.failed = 0;
        guard.pending = 0;
        fm1_enter_uboot();
    }
    fm1_irq_init();
    for (d = _bss_start; d < _bss_end; d++)
        *d = 0;
    for (d = _pool_start; d < _pool_end; d++)
        *d = 0;
    for (s = _data_load, d = _data_start; d < _data_end; s++, d++)
        *d = *s;
    for (s = _rt_load, d = _rt_start; d < _rt_end; s++, d++)    /* the flash driver, run from RAM */
        *d = *s;
    fm1_guard_enable(FM1_GUARD_STACK | FM1_GUARD_WRITE | FM1_GUARD_BUS | FM1_GUARD_PC);
    app_main();
    for (;;)
        ;
}
