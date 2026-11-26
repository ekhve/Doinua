/* SPDX-License-Identifier: GPL-3.0-only */
/* Only edited fields are replayed on the base patch. The two mixer levels
 * preserve changes to overall volume that the rounded balance cannot express. */
enum { PRESET_OSC_BYTES = 7 };

static inline unsigned preset_osc_capture(int track, uint8_t mask, uint8_t data[PRESET_OSC_BYTES])
{
    kit_sound_osc_t osc;
    unsigned pos = 1;
    if (mask & ~15u) return 0;
    data[0] = mask;
    if (!mask) return pos;
    if (!kit_track_osc_of(track, &osc)) return 0;
    if (mask & (1u << KIT_OSC_WAVE1)) data[pos++] = osc.wave1;
    if (mask & (1u << KIT_OSC_WAVE2)) data[pos++] = osc.wave2;
    if (mask & (1u << KIT_OSC_DETUNE)) {
        unsigned cents = (unsigned)(osc.detune + 700);
        if (cents > 1400) return 0;
        data[pos++] = (uint8_t)cents;
        data[pos++] = (uint8_t)(cents >> 8);
    }
    if (mask & (1u << KIT_OSC_MIX)) {
        if (!kit_track_osc_levels_of(track, data + pos)) return 0;
        pos += 2;
    }
    return pos;
}

static inline int preset_osc_restore(int track, const uint8_t *data, unsigned length)
{
    kit_sound_osc_t osc;
    unsigned pos = 1, control;
    uint8_t mask;
    int detune = 0, wave[2] = {0};
    const uint8_t *levels = 0;
    if (!length || length > PRESET_OSC_BYTES) return 0;
    mask = data[0];
    if (mask & ~15u || (mask && !kit_track_osc_of(track, &osc))) return 0;
    for (control = KIT_OSC_WAVE1; control <= KIT_OSC_WAVE2; ++control) {
        if (!(mask & (1u << control))) continue;
        if (pos >= length || data[pos] >= KIT_OSC_NWAVES) return 0;
        wave[control] = data[pos++];
    }
    if (mask & (1u << KIT_OSC_DETUNE)) {
        if (length - pos < 2) return 0;
        detune = data[pos] + 256 * data[pos + 1] - 700;
        if (detune > 700) return 0;
        pos += 2;
    }
    if (mask & (1u << KIT_OSC_MIX)) {
        if (length - pos < 2 || data[pos] > 127 || data[pos + 1] > 127) return 0;
        levels = data + pos;
        pos += 2;
    }
    if (pos != length) return 0;
    for (control = KIT_OSC_WAVE1; control <= KIT_OSC_WAVE2; ++control)
        if (mask & (1u << control)) kit_track_osc(track, control, wave[control]);
    if (mask & (1u << KIT_OSC_DETUNE)) kit_track_osc(track, KIT_OSC_DETUNE, detune);
    if (levels) kit_track_osc_levels(track, levels);
    return 1;
}
