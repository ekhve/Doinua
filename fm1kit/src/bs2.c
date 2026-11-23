/* SPDX-License-Identifier: GPL-3.0-only */
/* ST-92: the kit holds its lock while changing a track's mono synth. */
#include "kit_internal.h"
#include "bs2/control.h"
#include "bs2/bs2_bank.h"
#include "bs2_hooks.h"

static bs2_track_t tracks[KIT_NTRACKS] KIT_BIG;
static uint8_t enabled[KIT_NTRACKS];

int kit__bs2_npresets(void) { return BS2_N; }
const char *kit__bs2_preset_name(int p)
{
    return p >= 0 && p < BS2_N ? BS2_NAMES[p] : "?";
}

void kit__bs2_select(int t, int p)
{
    patch_t patch = {0};
    char name[17];
    enabled[t] = 0;
    if (p < 0 || p >= BS2_N) return;
    if (!bsx_import(BS2_PATCHES[p], sizeof BS2_PATCHES[p], &patch, name)) return;
    bs2_prepare(&tracks[t], &patch);
    enabled[t] = 1;
}

int kit__bs2_on(int t) { return enabled[t]; }
void kit__bs2_note_on(int t, int note, int vel) { bs2_key_down(&tracks[t], note, vel); }
void kit__bs2_note_off(int t, int note) { bs2_key_up(&tracks[t], note); }
void kit__bs2_all_off(int t) { bs2_key_up(&tracks[t], tracks[t].held); }
int kit__bs2_has(int t, int note) { return tracks[t].held == note; }
int kit__bs2_nactive(int t) { return tracks[t].voice.amp.stage != ENV_IDLE; }
int kit__bs2_nreleasing(int t) { return tracks[t].voice.amp.stage == ENV_RELEASE; }
int kit__bs2_steal(int t)
{
    if (!kit__bs2_nactive(t)) return 0;
    tracks[t].held = -1;
    tracks[t].voice.amp = (env_t){0};
    tracks[t].voice.menv = (env_t){0};
    return 1;
}

void kit__bs2_render(int t, int32_t *mix, int n)
{
    float block[CTRL_N];
    if (!kit__bs2_nactive(t)) return;
    while (n > 0) {
        int count = n < (int)CTRL_N ? n : (int)CTRL_N;
        bs2_samples(&tracks[t], block, (unsigned)count, 120.f);
        for (int i = 0; i < count; ++i) {
            int32_t sample = (int32_t)(block[i] * 32768.f);
            if (sample > 40000) sample = 40000 + (sample - 40000) / 4;
            if (sample < -40000) sample = -40000 + (sample + 40000) / 4;
            *mix++ += sample;
        }
        n -= count;
    }
}
