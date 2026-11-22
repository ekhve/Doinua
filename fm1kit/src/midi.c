/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit MIDI in: a byte parser (running status, real-time bytes ignored in between, SysEx skipped) that turns
 * note on / off, control change, pitch bend and Start / Continue / Stop into app events; a clock (F8) moves the song
 * player when it follows (seq.c). One parser per source (USB, TRS), so two streams never mix.
 * Portable: the backend feeds bytes from the main loop (never from an interrupt: app_event runs app code). */
#include "kit_internal.h"

typedef struct {
    uint8_t status;             /* running status (0 = none) */
    uint8_t data[2];
    uint8_t n;                  /* data bytes so far */
    uint8_t sysex;
} parser_t;

static parser_t parsers[KIT_MIDI_SOURCES];

static void emit(uint8_t st, uint8_t d1, uint8_t d2)
{
    kit_event_t e;
    uint8_t kind = st & 0xF0u;
    e.delta = (int8_t)((st & 0x0Fu) + 1);     /* channel 1..16 */
    e.id = d1;
    e.vel = d2;
    if (kind == 0x90u && d2)
        e.type = KIT_EV_MIDI_NOTE_ON;
    else if (kind == 0x80u || kind == 0x90u)  /* note on at velocity 0 = note off */
        e.type = KIT_EV_MIDI_NOTE_OFF;
    else if (kind == 0xB0u)
        e.type = KIT_EV_MIDI_CC;
    else if (kind == 0xE0u)
        e.type = KIT_EV_MIDI_BEND;
    else
        return;
    app_event(&e);
}

static int data_bytes(uint8_t st)
{
    uint8_t k = st & 0xF0u;
    return k == 0xC0u || k == 0xD0u ? 1 : 2;
}

void kit__midi_byte(int source, uint8_t b)
{
    parser_t *p;
    if (source < 0 || source >= KIT_MIDI_SOURCES)
        return;
    p = &parsers[source];
    if (b >= 0xF8u) {                         /* real-time: may come anywhere, changes no running status */
        if (b == 0xF8u)
            kit__seq_clock(kit_ms());
        else if (b >= 0xFAu && b <= 0xFCu) {
            kit_event_t e;
            e.type = KIT_EV_MIDI_CLOCK;
            e.id = b;
            e.vel = 0;
            e.delta = 0;
            app_event(&e);
        }
        return;
    }
    if (b == 0xF0u) { p->sysex = 1; p->status = 0; return; }
    if (b >= 0x80u) {
        p->sysex = 0;
        if (b >= 0xF0u) { p->status = 0; return; }   /* system common (and F7): clears running status */
        p->status = b;
        p->n = 0;
        return;
    }
    if (p->sysex || !p->status)
        return;
    p->data[p->n++] = b;
    if (p->n >= data_bytes(p->status)) {
        emit(p->status, p->data[0], p->n > 1 ? p->data[1] : 0);
        p->n = 0;                             /* running status: the next data bytes reuse it */
    }
}
