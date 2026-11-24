/* SPDX-License-Identifier: GPL-3.0-only */
/* ST-72: the OberVave bank, per-track voices and factory ARP metadata. */
#include "fm1kit.h"
#include "../../fm1kit/src/obervave_hooks.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_voice(int preset)
{
    int heard = 0;
    kit__obv_select(0, preset);
    kit__obv_note_on(0, 60, 100);
    kit__obv_note_on(0, 67, 96);
    assert(kit__obv_has(0, 67));

    /* Make the requested samples straddle the engine's control interval. */
    for (int block = 0; block < 256; ++block) {
        int32_t mix[99] = {0};
        if (block == 200) kit__obv_note_off(0, 67);
        kit__obv_render(0, mix, 99);
        for (int i = 0; i < 99; ++i) heard |= mix[i] != 0;
    }
    assert(heard);
    assert(kit__obv_has(0, 60));
    kit__obv_all_off(0);
    assert(kit__obv_nactive(0) == kit__obv_nreleasing(0));
    kit__obv_select(0, -1);
    assert(!kit__obv_on(0) && !kit__obv_nactive(0));
}

int main(void)
{
    const struct {
        const char *name;
        uint8_t mode, ticks, octaves, gate;
    } expected[] = {
        {"NIGHTDRIVE", 0, 6, 2, 56}, {"SHADOW ARP", 2, 8, 2, 51},
        {"GLASS ARP", 0, 6, 3, 36}, {"DREAM ARP", 3, 12, 2, 86},
        {"ARP CHORD", 3, 12, 1, 81}, {"DARK ARP", 0, 6, 1, 76},
    };
    size_t next = 0;
    assert(kit__obv_npresets() == 19);
    for (int preset = 0; preset < kit__obv_npresets(); ++preset) {
        kit_preset_arp_t arp;
        if (kit_preset_arp(KIT_ENGINE_OBERVAVE, preset, &arp)) {
            assert(next < sizeof expected / sizeof expected[0]);
            assert(strcmp(kit__obv_preset_name(preset), expected[next].name) == 0);
            assert(arp.mode == expected[next].mode && arp.ticks == expected[next].ticks);
            assert(arp.octaves == expected[next].octaves && arp.gate == expected[next].gate);
            ++next;
        }
        check_voice(preset);
    }
    assert(next == sizeof expected / sizeof expected[0]);
    puts("PASS: OberVave presets, note lifecycle and six factory arpeggios");
    return 0;
}
