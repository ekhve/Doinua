/* SPDX-License-Identifier: GPL-3.0-only */
/* ST-92: exercise BS2 through the same engine dispatch used by both apps. */
#include <assert.h>
#include <string.h>
#include "kit_internal.h"
#include "../../apps/companion/sb_data.h"
uint32_t kit__lock(void) { return 0; }
void kit__unlock(uint32_t key) { (void)key; }
int kit__track_pitch(int t) { (void)t; return 0; }

int main(void)
{
    const int engine = KIT_ENGINE_BS2 - 2;
    enum { UPSTREAM_PRESETS = 11, PRESETS = UPSTREAM_PRESETS + 1 };
    int32_t block[256];
    _Static_assert(KIT_ENGINE_BS2 == 10, "saved Bass Something 2 engine ID");
    assert(engine < kit__eng_count());
    assert(!strcmp(kit__eng_name(engine), "Bass Something 2"));
    assert(kit__eng_npresets(engine) == PRESETS);
    assert(!strcmp(kit__eng_preset_name(engine, UPSTREAM_PRESETS), "Clean Punch"));
    int matches = 0;
    for (int i = 0; i < SB_NALL; ++i) {
        const sb_entry_t *entry = &SB_ALL[i];
        if (strcmp(SB_ENGINE[entry->engine], kit__eng_name(engine))) continue;
        assert(!strcmp(entry->preset, kit__eng_preset_name(engine, matches)));
        ++matches;
    }
    assert(matches == PRESETS);
    for (int p = 0; p < PRESETS; ++p) {
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
