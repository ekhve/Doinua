/* SPDX-License-Identifier: GPL-3.0-only */
/* player.c (D-025, R-043..R-045): on a simulated clock with late polls,
 *   - note k starts at start + k * step, within 2 ms, however late the polls come (no drift over 200 notes)
 *   - each note sounds for 92 % of a step
 *   - a looping sequence repeats; one that doesn't ends with KIT_EV_PLAY_END after its last note
 *   - stop silences the sounding note at once
 *   - each started note arrives as KIT_EV_PLAY_NOTE with its index */
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"

static uint32_t now;
static struct { uint32_t t; int note, on; } log_[2048];
static int nlog, ends, last_index = -1, sounding[128];
static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

uint32_t kit_ms(void) { return now; }
void kit_note_on(int note, int vel) { (void)vel; if (nlog < 2048) { log_[nlog].t = now; log_[nlog].note = note; log_[nlog++].on = 1; } sounding[note] = 1; }
void kit_note_off(int note) { if (nlog < 2048) { log_[nlog].t = now; log_[nlog].note = note; log_[nlog++].on = 0; } sounding[note] = 0; }
void app_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_PLAY_END) ends++;
    if (e->type == KIT_EV_PLAY_NOTE) last_index = e->delta;
}

static void run(uint32_t ms, uint32_t poll_every)
{
    uint32_t end = now + ms;
    while (now < end) { now += poll_every; kit__player_poll(now); }
}

int main(void)
{
    const uint8_t scale[8] = { 60, 62, 64, 65, 67, 69, 71, 72 };
    int i, on = 0, worst = 0, n92 = 0;
    now = 1000;
    kit_play_start(scale, 8, 667, 100, 1);
    run(667 * 200, 1);                               /* 200 steps, polled every ms */
    for (i = 0; i < nlog; i++)
        if (log_[i].on) {
            int want = 1000 + on * 667, d = (int)log_[i].t - want;
            if (d < 0) d = -d;
            if (d > worst) worst = d;
            CHECK(log_[i].note == scale[on % 8], "note %d is %d, not %d", on, log_[i].note, scale[on % 8]);
            on++;
        } else {
            int len = (int)log_[i].t - (int)log_[i - 1].t;
            if (len >= 613 - 2 && len <= 614 + 2) n92++;
        }
    CHECK(on == 201, "201 notes started in 200 steps (%d)", on);
    CHECK(worst <= 2, "every note within 2 ms of start + k * step (worst %d ms)", worst);
    CHECK(n92 >= 199, "notes sound for 92 %% of a step (%d of %d)", n92, on - 1);
    CHECK(last_index == 200 % 8, "the event carries the index in the sequence (%d)", last_index);

    /* late polls (every 7 ms) do not make it drift */
    nlog = 0; now = 50000;
    kit_play_start(scale, 8, 500, 100, 1);
    run(500 * 100, 7);
    for (i = 0, on = 0, worst = 0; i < nlog; i++)
        if (log_[i].on) {
            int d = (int)log_[i].t - (50000 + on * 500);
            if (d < 0) d = -d;
            if (d > worst) worst = d;
            on++;
        }
    CHECK(worst <= 7, "polled every 7 ms: each note within one poll of its time, no drift (worst %d ms)", worst);

    /* stop */
    kit_play_stop();
    for (i = 0; i < 128; i++) CHECK(!sounding[i], "stop silences note %d", i);
    CHECK(!kit_playing(), "stopped");

    /* no loop: the end */
    nlog = 0; now = 200000; ends = 0;
    kit_play_start(scale, 3, 400, 100, 0);
    run(2000, 1);
    for (i = 0, on = 0; i < nlog; i++) on += log_[i].on;
    CHECK(on == 3 && ends == 1 && !kit_playing(), "a sequence that doesn't loop plays 3 notes and ends once (%d notes, %d ends)", on, ends);

    printf("%s: player, %d failed\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
