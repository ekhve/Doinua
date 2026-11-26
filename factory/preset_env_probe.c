/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the preset envelope payload with FMini-style inherited release and shared Juno ADSR. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../fm1kit/include/fm1kit.h"
#include "../apps/companion/presetenv.c"

enum { FOLLOW_DECAY = 255 };
static int engine;
static uint8_t env[2][4];
static const uint8_t factory_env[2][4] = {{11, 20, 32, FOLLOW_DECAY}, {10, 15, 40, FOLLOW_DECAY}};

int kit_track_engine(int track) { return track == 0 ? engine : -1; }

int kit_track_envelope_of(int track, int kind, uint8_t values[4])
{
    if (track != 0 || (engine != KIT_ENGINE_FMINI && engine != KIT_ENGINE_JUNO)) return 0;
    memcpy(values, env[engine == KIT_ENGINE_JUNO ? 0 : kind], 4);
    if (values[KIT_ENV_RELEASE] == FOLLOW_DECAY) values[KIT_ENV_RELEASE] = values[KIT_ENV_DECAY];
    return 1;
}

int kit_track_envelope(int track, int kind, int control, int value)
{
    assert(track == 0);
    env[engine == KIT_ENGINE_JUNO ? 0 : kind][control] = (uint8_t)value;
    return 1;
}

int main(void)
{
    uint8_t bytes[PRESET_ENV_BYTES], values[4], before[sizeof env];
    unsigned mask, length;
    engine = KIT_ENGINE_FMINI;
    memcpy(env, factory_env, sizeof env);
    kit_track_envelope(0, KIT_ENV_FILTER, KIT_ENV_DECAY, 50);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_RELEASE, 42);
    mask = preset_env_bit(engine, KIT_ENV_FILTER, KIT_ENV_DECAY) |
           preset_env_bit(engine, KIT_ENV_AMP, KIT_ENV_RELEASE);
    length = preset_env_capture(0, mask, bytes);
    assert(length == 3 && bytes[1] == 50 && bytes[2] == 42);

    memcpy(env, factory_env, sizeof env);
    memcpy(before, env, sizeof before);
    assert(!preset_env_restore(0, bytes, length - 1));
    assert(!preset_env_restore(0, bytes, length + 1));
    bytes[2] = 128;
    assert(!preset_env_restore(0, bytes, length));
    assert(!memcmp(env, before, sizeof env));
    bytes[2] = 42;
    assert(preset_env_restore(0, bytes, length));
    kit_track_envelope(0, KIT_ENV_FILTER, KIT_ENV_DECAY, 53);
    assert(kit_track_envelope_of(0, KIT_ENV_FILTER, values) && values[KIT_ENV_RELEASE] == 53);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_DECAY, 61);
    assert(kit_track_envelope_of(0, KIT_ENV_AMP, values) && values[KIT_ENV_RELEASE] == 42);

    engine = KIT_ENGINE_JUNO;
    assert(!preset_env_restore(0, bytes, length));
    mask = preset_env_bit(engine, KIT_ENV_FILTER, KIT_ENV_ATTACK) |
           preset_env_bit(engine, KIT_ENV_AMP, KIT_ENV_ATTACK);
    assert(mask == 1);
    kit_track_envelope(0, KIT_ENV_AMP, KIT_ENV_ATTACK, 24);
    length = preset_env_capture(0, mask, bytes);
    assert(length == 2 && bytes[1] == 24);
    memcpy(env, factory_env, sizeof env);
    assert(preset_env_restore(0, bytes, length));
    assert(kit_track_envelope_of(0, KIT_ENV_FILTER, values) && values[KIT_ENV_ATTACK] == 24);
    assert(kit_track_envelope_of(0, KIT_ENV_AMP, values) && values[KIT_ENV_ATTACK] == 24);
    engine = KIT_ENGINE_BASIC;
    assert(!preset_env_restore(0, bytes, length));
    assert(!preset_env_capture(0, mask, bytes));
    puts("PASS: inherited release, independent release, shared ADSR, and rejected payloads.");
    return 0;
}
