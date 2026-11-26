/* SPDX-License-Identifier: GPL-3.0-only */
/* Record only edited envelope controls. This keeps FMini's inherited release
 * tied to decay unless RELEASE itself was edited, and Juno uses one ADSR. */
enum { PRESET_ENV_BYTES = 9 };

static unsigned preset_env_bit(int engine, int kind, int control)
{
    return 1u << (control + (kind == KIT_ENV_AMP && engine != KIT_ENGINE_JUNO ? 4 : 0));
}

static inline unsigned preset_env_capture(int track, uint8_t mask, uint8_t data[PRESET_ENV_BYTES])
{
    unsigned length = 1;
    int kind, control;
    data[0] = mask;
    for (kind = KIT_ENV_FILTER; kind <= KIT_ENV_AMP; ++kind) {
        uint8_t values[4];
        if (!(mask & (15u << (4 * kind)))) continue;
        if (!kit_track_envelope_of(track, kind, values)) return 0;
        for (control = 0; control < 4; ++control)
            if (mask & (1u << (control + 4 * kind))) data[length++] = values[control];
    }
    return length;
}

static inline int preset_env_restore(int track, const uint8_t *data, unsigned length)
{
    unsigned position = 1;
    int engine = kit_track_engine(track), kind, control;
    uint8_t mask;
    if (!length || length > PRESET_ENV_BYTES) return 0;
    mask = data[0];
    if (engine == KIT_ENGINE_JUNO && (mask & 0xf0u)) return 0;
    for (kind = KIT_ENV_FILTER; kind <= KIT_ENV_AMP; ++kind) {
        uint8_t current[4];
        if (!(mask & (15u << (4 * kind)))) continue;
        if (!kit_track_envelope_of(track, kind, current)) return 0;
        for (control = 0; control < 4; ++control)
            if (mask & (1u << (control + 4 * kind))) {
                if (position >= length || data[position] > 127) return 0;
                ++position;
            }
    }
    if (position != length) return 0;
    position = 1;
    for (kind = KIT_ENV_FILTER; kind <= KIT_ENV_AMP; ++kind)
        for (control = 0; control < 4; ++control)
            if (mask & (1u << (control + 4 * kind)))
                kit_track_envelope(track, kind, control, data[position++]);
    return 1;
}
