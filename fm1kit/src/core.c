/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit core: turns the backend's raw input state into app events, keeps the lights' state,
 * and the small services (random, held state). Portable. */
#include "kit_internal.h"

static uint32_t shown_notes, shown_buttons;
static uint32_t rng = 0x2545F491u;

void kit__core_poll(uint32_t notes, uint32_t buttons, const int8_t enc_steps[KIT_NENCS], const uint8_t vel[KIT_NKEYS])
{
    uint32_t d, i;
    kit_event_t e;
    d = notes ^ shown_notes;
    for (i = 0; d && i < KIT_NKEYS; i++) {
        if (!(d & (1u << i)))
            continue;
        e.type = (notes >> i) & 1u ? KIT_EV_KEY_DOWN : KIT_EV_KEY_UP;
        e.id = (uint8_t)i;
        e.vel = vel ? vel[i] : 100;
        e.delta = 0;
        app_event(&e);
    }
    shown_notes = notes;
    d = buttons ^ shown_buttons;
    for (i = 0; d && i < KIT_NBTNS; i++) {
        if (!(d & (1u << i)))
            continue;
        e.type = (buttons >> i) & 1u ? KIT_EV_BTN_DOWN : KIT_EV_BTN_UP;
        e.id = (uint8_t)i;
        e.vel = 0;
        e.delta = 0;
        app_event(&e);
    }
    shown_buttons = buttons;
    for (i = 0; i < KIT_NENCS; i++) {
        if (!enc_steps[i])
            continue;
        e.type = KIT_EV_ENC;
        e.id = (uint8_t)i;
        e.vel = 0;
        e.delta = enc_steps[i];
        app_event(&e);
    }
}

uint32_t kit__core_buttons(void) { return shown_buttons; }
int kit__core_busy(void) { return shown_notes != 0 || shown_buttons != 0; }

int kit_key_held(int key) { return key >= 0 && key < KIT_NKEYS && ((shown_notes >> key) & 1u); }
int kit_btn_held(int btn) { return btn >= 0 && btn < KIT_NBTNS && ((shown_buttons >> btn) & 1u); }
int kit_pot(void) { return kit__pot(); }

uint32_t kit_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

void kit_key_light(int key, int level)
{
    if (key >= 0 && key < KIT_NKEYS)
        kit__light_set(14 + key, level);
}

void kit_btn_light(int btn, int level)
{
    if (btn >= 0 && btn < KIT_NBTNS)
        kit__light_set(btn, level);
}

void kit_lights_off(void)
{
    int i;
    for (i = 0; i < 41; i++)
        kit__light_set(i, KIT_LIGHT_OFF);
}

static int clamp7(int v) { return v < 0 ? 0 : v > 127 ? 127 : v; }
static uint8_t chan(int ch) { return (uint8_t)((ch < 1 ? 1 : ch > 16 ? 16 : ch) - 1); }
void kit_midi_note_on(int ch, int note, int vel) { kit__midi_send((uint8_t)(0x90u | chan(ch)), (uint8_t)clamp7(note), (uint8_t)(vel < 1 ? 1 : clamp7(vel))); }
void kit_midi_note_off(int ch, int note) { kit__midi_send((uint8_t)(0x80u | chan(ch)), (uint8_t)clamp7(note), 0); }
void kit_midi_realtime(int b) { if (b >= 0xF8 && b <= 0xFF) kit__midi_send_rt((uint8_t)b); }
void kit_midi_cc(int ch, int cc, int val) { kit__midi_send((uint8_t)(0xB0u | chan(ch)), (uint8_t)clamp7(cc), (uint8_t)clamp7(val)); }
