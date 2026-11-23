/* SPDX-License-Identifier: GPL-3.0-only */
/* ST-92: exercise BS2 through the same engine dispatch used by both apps. */
#include <assert.h>
#include <string.h>
#include "kit_internal.h"
uint32_t kit__lock(void) { return 0; }
void kit__unlock(uint32_t key) { (void)key; }
int kit__track_pitch(int t) { (void)t; return 0; }

int main(void)
{
    const int engine = KIT_ENGINE_BS2 - 2;
    int32_t block[256];
    assert(kit__eng_count() == engine + 1);
    assert(!strcmp(kit__eng_name(engine), "Bass Something 2"));
    assert(kit__eng_npresets(engine) == 11);
    for (int p = 0; p < 11; ++p) {
        int peak = 0;
        kit__eng_select(0, engine, p);
        kit__eng_note_on(0, 57, 100);
        for (int i = 0; i < 256; ++i) {
            memset(block, 0, sizeof block);
            kit__eng_render_part(0, block, 256);
            for (int j = 0; j < 256; ++j) {
                int magnitude = block[j] < 0 ? -block[j] : block[j];
                if (magnitude > peak) peak = magnitude;
            }
        }
        assert(peak > 100 && peak < 60000);
        kit__eng_note_off(0, 57);
        for (int i = 0; i < 10000 && kit__eng_nactive(0); ++i) {
            memset(block, 0, sizeof block);
            kit__eng_render_part(0, block, 256);
        }
        assert(!kit__eng_nactive(0));
    }
    kit__eng_select(1, engine, 1);
    kit__eng_note_on(1, 45, 100);
    kit__eng_note_on(0, 57, 100);
    kit__eng_note_on(0, 69, 100);
    kit__eng_note_off(0, 57);
    assert(kit__eng_nactive(0) == 1 && kit__eng_has(0, 69));
    assert(kit__eng_steal(0) && !kit__eng_nactive(0));
    assert(kit__eng_has(1, 45));
}
