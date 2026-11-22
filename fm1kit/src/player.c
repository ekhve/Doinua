/* SPDX-License-Identifier: GPL-3.0-only */
/* fm1kit player: plays a sequence by itself (D-025). Portable; the backends call kit__player_poll from their main loop,
 * never from an interrupt (it calls the app). Note k starts at start + k * step, computed from the start each time,
 * so the tempo does not drift however late a poll comes; it sounds until 92 % of its step. */
#include "kit_internal.h"

static uint8_t seq[KIT_PLAY_MAX];
static int seq_n, seq_vel, seq_loop, playing, sounding = -1;
static uint32_t start_ms, step, next_k;

void kit_play_start(const uint8_t *notes, int n, uint32_t step_ms, int vel, int loop)
{
    int i;
    kit_play_stop();
    if (n <= 0 || step_ms == 0)
        return;
    if (n > KIT_PLAY_MAX)
        n = KIT_PLAY_MAX;
    for (i = 0; i < n; i++)
        seq[i] = notes[i];
    seq_n = n;
    seq_vel = vel < 1 ? 1 : vel > 127 ? 127 : vel;
    seq_loop = loop;
    step = step_ms;
    start_ms = kit_ms();
    next_k = 0;
    playing = 1;
    kit__player_poll(start_ms);             /* the first note now */
}

void kit_play_stop(void)
{
    if (sounding >= 0)
        kit_note_off(sounding);
    sounding = -1;
    playing = 0;
}

int kit_playing(void) { return playing; }

void kit__player_poll(uint32_t now)
{
    kit_event_t e;
    uint32_t k_at;
    if (!playing)
        return;
    /* the sounding note's end: 92 % of its step */
    if (sounding >= 0 && next_k > 0) {
        uint32_t end = start_ms + (next_k - 1u) * step + step * 92u / 100u;
        if ((int32_t)(now - end) >= 0) {
            kit_note_off(sounding);
            sounding = -1;
        }
    }
    k_at = start_ms + next_k * step;
    if ((int32_t)(now - k_at) < 0)
        return;
    if (!seq_loop && next_k >= (uint32_t)seq_n) {
        if (sounding < 0) {
            playing = 0;
            e.type = KIT_EV_PLAY_END;
            e.id = 0; e.vel = 0; e.delta = 0;
            app_event(&e);
        }
        return;
    }
    if (sounding >= 0)
        kit_note_off(sounding);
    sounding = seq[next_k % (uint32_t)seq_n];
    kit_note_on(sounding, seq_vel);
    e.type = KIT_EV_PLAY_NOTE;
    e.id = (uint8_t)sounding;
    e.vel = (uint8_t)seq_vel;
    e.delta = (int8_t)(next_k % (uint32_t)seq_n);
    next_k++;
    app_event(&e);
}
