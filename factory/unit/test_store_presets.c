/* SPDX-License-Identifier: GPL-3.0-only */
/* Companion preset persistence through the production Store and a NOR model. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kit_internal.h"
#include "../../apps/companion/settingsrecord.c"

enum { SECTOR_BYTES = 4096, SETTINGS_BYTES = 505, SETTINGS_VERSION = 6 };
static uint8_t flash[KIT_STORE_SECTORS][SECTOR_BYTES];
static uint32_t now;
static int playing;

uint32_t kit_ms(void) { return now; }
int kit__core_busy(void) { return 0; }
int kit__synth_active(void) { return 0; }
int kit_playing(void) { return 0; }
int kit_seq_playing(void) { return playing; }

int kit__flash_read(int s, int off, uint8_t *out, int n)
{
    memcpy(out, flash[s] + off, n);
    return 0;
}

int kit__flash_erase(int s)
{
    memset(flash[s], 255, SECTOR_BYTES);
    return 0;
}

int kit__flash_write(int s, int off, const uint8_t *in, int n)
{
    int i;
    for (i = 0; i < n; ++i) flash[s][off + i] &= in[i];
    return 0;
}

int main(void)
{
    uint8_t settings[SETTINGS_BYTES] = {SETTINGS_VERSION}, restarted[SETTINGS_BYTES] = {0};
    const preset_record_t item = {1, 8, 1, 9, 2, {'T','I','N','E',' ','S','O','L','O'}, {40,10}};
    preset_record_t recalled;
    memset(flash, 255, sizeof flash);
    assert(companion_settings_load(settings, sizeof settings, SETTINGS_VERSION) == 0);
    settings[4] = 29;
    assert(companion_preset_save(settings, sizeof settings, &item, 0) == COMPANION_PRESET_PENDING);
    playing = 1;
    now = KIT_STORE_SETTLE_MS;
    kit__store_poll(now);
    assert(kit_store_pending());
    assert(kit_store_load(restarted, sizeof restarted) == 0);
    playing = 0;
    kit__store_poll(++now);
    companion_preset_poll(sizeof settings, SETTINGS_VERSION);
    assert(!kit_store_pending() && companion_preset_status == COMPANION_PRESET_SAVED);

    /* Drop the app's active record before loading again from the NOR model. */
    memset(companion_settings_record, 0, sizeof companion_settings_record);
    companion_settings_record_size = 0;
    assert(companion_settings_load(restarted, sizeof restarted, SETTINGS_VERSION) == SETTINGS_BYTES);
    assert(restarted[4] == 29);
    assert(companion_preset_find(sizeof restarted, item.slot, &recalled) == PRESET_VALID);
    assert(companion_preset_matches(&item, &recalled));
    puts("PASS: Companion Store defers during playback and reloads the committed preset and settings.");
    return 0;
}
