/* SPDX-License-Identifier: GPL-3.0-only */
/* ST-73: the selected bank and its note lifecycle through fm1kit's engine dispatch. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"
#include "../../apps/companion/sb_data.h"

uint32_t kit__lock(void) { return 0; }
void kit__unlock(uint32_t key) { (void)key; }
int kit__track_pitch(int track) { (void)track; return 0; }

int main(void)
{
    const int engine = KIT_ENGINE_CF_VA - 2;
    const uint8_t sends[][4] = {
        {0, 0, 10, 10}, {0, 50, 20, 65}, {0, 60, 20, 70},
        {0, 55, 15, 70}, {0, 70, 30, 80}, {0, 65, 20, 75},
        {0, 55, 25, 75}, {0, 70, 15, 65}, {0, 25, 20, 40},
    };
    _Static_assert(KIT_ENGINE_CF_VA == 12, "saved CoralFlower VA engine ID");
    assert(!strcmp(kit__eng_name(engine), "CoralFlower VA"));
    assert(kit__eng_npresets(engine) == sizeof sends / sizeof sends[0]);
    assert(kit__eng_preset_mono(engine, 0));
    unsigned displayed = 0;
    for (unsigned i = 0; i < SB_NALL; ++i) {
        const sb_entry_t *entry = &SB_ALL[i];
        if (strcmp(SB_ENGINE[entry->engine], kit__eng_name(engine))) continue;
        assert(!strcmp(entry->preset, kit__eng_preset_name(engine, displayed)));
        ++displayed;
    }
    assert(displayed == sizeof sends / sizeof sends[0]);

    for (unsigned preset = 0; preset < displayed; ++preset) {
        assert(!!kit__eng_preset_mono(engine, preset) == (preset == 0));
        kit__eng_select(0, engine, preset);
        uint8_t actual_sends[4], bus[12];
        kit__eng_fx(0, actual_sends, bus);
        assert(!memcmp(sends[preset], actual_sends, sizeof actual_sends));
        assert(!kit__eng_nactive(0));
        kit__eng_note_on(0, 60, 100);
        int32_t peak = 0;
        for (unsigned i = 0; i < 384; ++i) {
            int32_t mix[256] = {0};
            kit__eng_render_part(0, mix, 256);
            for (unsigned frame = 0; frame < 256; ++frame) {
                int32_t value = mix[frame] < 0 ? -mix[frame] : mix[frame];
                if (value > peak) peak = value;
            }
        }
        assert(peak > 100 && peak < 60000);
        kit__eng_note_off(0, 60);
        assert(kit__eng_nreleasing(0) == 1);
        for (unsigned i = 0; i < 3500 && kit__eng_nactive(0); ++i) {
            int32_t mix[256] = {0};
            kit__eng_render_part(0, mix, 256);
        }
        assert(!kit__eng_nactive(0));
    }

    kit__eng_select(0, engine, 1);
    kit__eng_select(1, engine, 2);
    kit__eng_note_on(0, 60, 100);
    kit__eng_note_on(0, 67, 100);
    kit__eng_note_on(1, 48, 90);
    assert(kit__eng_nactive(0) == 2 && kit__eng_nactive(1) == 1);
    assert(kit__eng_steal(0));
    assert(kit__eng_has(0, 67) && kit__eng_has(1, 48));
    puts("PASS: CoralFlower VA bank, factory effects, independent tracks and release");
}
