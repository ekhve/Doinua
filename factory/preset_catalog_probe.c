/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define KIT_STORE_MAX 1024
#define KIT_STORE_ERR_SIZE (-1)
#define KIT_STORE_ERR_FULL (-2)
#define KIT_STORE_ERR_FLASH (-3)
#define KIT_BIG
static uint8_t stored[KIT_STORE_MAX], queued[KIT_STORE_MAX];
static unsigned stored_size, queued_size;
static int pending, last_status;

static int kit_store_load(void *out, int capacity)
{
    unsigned bytes = stored_size < (unsigned)capacity ? stored_size : (unsigned)capacity;
    memcpy(out, stored, bytes);
    return (int)bytes;
}

static int kit_store_save(const void *data, int bytes)
{
    assert(bytes >= 0 && bytes <= KIT_STORE_MAX);
    memcpy(queued, data, (unsigned)bytes);
    queued_size = (unsigned)bytes;
    pending = 1;
    return 0;
}

static int kit_store_pending(void) { return pending; }
static int kit_store_status(void) { return last_status; }

static void finish_write(int result)
{
    assert(pending);
    last_status = result;
    if (result == KIT_STORE_ERR_FLASH) return;  /* physical Store retries */
    pending = 0;
    if (!result) {
        memcpy(stored, queued, queued_size);
        stored_size = queued_size;
    }
}

#include "../apps/companion/settingsrecord.c"

enum { SETTINGS_VERSION = 6, KIT_NTRACKS = 4 };
static uint8_t set[505];
static int snd[KIT_NTRACKS];
static uint8_t edited[KIT_NTRACKS];

static int sound_index(int engine, int base) { return engine * 10 + base; }
static int preset_patch_capture(int t, preset_record_t *item)
{
    item->engine = 8;
    item->base = 1;
    item->data_len = 1;
    item->data[0] = edited[t];
    return 1;
}
static void preset_workflow_forget(int t);
static int preset_patch_recall(int t, const preset_record_t *item)
{
    preset_workflow_forget(t);
    edited[t] = item->data[0];
    return 1;
}

#include "../apps/companion/presetworkflow.c"

static void check_settings_record(void)
{
    uint8_t prefix[505] = {0}, original_tail[128];
    preset_record_t p = {1, 8, 0, 4, 1, {'B', 'a', 's', 's'}, {42}}, decoded;
    preset_reader_t reader;
    unsigned tail_size;

    assert(companion_settings_load(prefix, sizeof prefix, 6) == 0);
    prefix[0] = 6;
    assert(companion_settings_save(prefix, sizeof prefix) == 0);
    assert(queued_size == sizeof prefix);

    stored[0] = 5;
    stored[4] = 19;
    stored_size = 502;
    assert(companion_settings_load(prefix, sizeof prefix, 6) == 502);
    assert(prefix[0] == 5 && prefix[4] == 19);
    prefix[0] = 6;
    assert(companion_settings_save(prefix, sizeof prefix) == 0);
    assert(queued_size == sizeof prefix);

    memcpy(stored, prefix, sizeof prefix);
    stored_size = (unsigned)preset_catalog_write(stored, sizeof stored, sizeof prefix, &p, 1);
    tail_size = stored_size - sizeof prefix;
    assert(tail_size > 0 && tail_size <= sizeof original_tail);
    memcpy(original_tail, stored + sizeof prefix, tail_size);
    assert(companion_settings_load(prefix, sizeof prefix, 6) == (int)sizeof prefix);
    prefix[4] = 27;
    assert(companion_settings_save(prefix, sizeof prefix) == 0);
    assert(queued_size == stored_size && queued[4] == 27 && stored[4] == 19);
    assert(!memcmp(queued + sizeof prefix, original_tail, tail_size));
    memcpy(stored, queued, queued_size);
    assert(companion_settings_load(prefix, sizeof prefix, 6) == (int)sizeof prefix);
    assert(prefix[4] == 27);
    assert(preset_catalog_open(&reader, stored, stored_size, sizeof prefix) == PRESET_VALID);
    assert(preset_catalog_next(&reader, &decoded) == PRESET_VALID && decoded.data[0] == 42);

    /* A future catalog version stays intact during a routine settings save. */
    stored[sizeof prefix + 4] = 3;
    assert(companion_settings_load(prefix, sizeof prefix, 6) == (int)sizeof prefix);
    assert(companion_settings_save(prefix, sizeof prefix) == 0);
    assert(queued_size == stored_size && !memcmp(queued, stored, stored_size));
}

static void check_catalog_edits(void)
{
    enum { PREFIX = 505 };
    uint8_t bytes[1024], before[1024];
    preset_record_t first = {1, 8, 2, 4, 1, {'B', 'a', 's', 's'}, {42}};
    preset_record_t second = {2, 8, 5, 3, 2, {'S', 'u', 'b'}, {12, 15}};
    preset_record_t edited = first, decoded;
    preset_reader_t reader;
    int first_size, size, grown;

    memset(bytes, 0xa5, sizeof bytes);
    memcpy(before, bytes, sizeof before);
    assert(preset_catalog_edit(bytes, PREFIX, sizeof bytes, PREFIX, &first, 1) == PRESET_EDIT_SLOT);
    assert(preset_catalog_edit(bytes, PREFIX, PREFIX + PRESET_HEADER, PREFIX, &first, 0) == PRESET_EDIT_FULL);
    assert(!memcmp(bytes, before, sizeof bytes));
    first_size = preset_catalog_edit(bytes, PREFIX, sizeof bytes, PREFIX, &first, 0);
    assert(first_size == PREFIX + PRESET_HEADER + 10);
    size = preset_catalog_edit(bytes, first_size, sizeof bytes, PREFIX, &second, 0);
    assert(size == first_size + 10);
    assert(!memcmp(bytes, before, PREFIX));
    memcpy(before, bytes, sizeof before);
    edited.data_len = PRESET_DATA_MAX;
    edited.data[PRESET_DATA_MAX - 1] = 17;
    assert(preset_catalog_edit(bytes, size, sizeof bytes, PREFIX, &edited, 0) == PRESET_EDIT_SLOT);
    assert(preset_catalog_edit(bytes, size, size, PREFIX, &edited, 1) == PRESET_EDIT_FULL);
    assert(!memcmp(bytes, before, sizeof bytes));
    grown = preset_catalog_edit(bytes, size, sizeof bytes, PREFIX, &edited, 1);
    assert(grown == size + PRESET_DATA_MAX - 1);
    assert(preset_catalog_open(&reader, bytes, grown, PREFIX) == PRESET_VALID);
    assert(preset_catalog_next(&reader, &decoded) == PRESET_VALID);
    assert(!memcmp(&edited, &decoded, sizeof edited));
    assert(preset_catalog_next(&reader, &decoded) == PRESET_VALID);
    assert(!memcmp(&second, &decoded, sizeof second));
    assert(preset_catalog_next(&reader, &decoded) == PRESET_ABSENT);
    assert(preset_catalog_edit(bytes, grown, sizeof bytes, PREFIX, &first, 1) == size);
    assert(!memcmp(bytes, before, size));

    bytes[PREFIX + 4] = 3;
    memcpy(before, bytes, sizeof before);
    assert(preset_catalog_edit(bytes, size, sizeof bytes, PREFIX, &first, 1) == PRESET_EDIT_INVALID);
    assert(!memcmp(bytes, before, sizeof bytes));
}

static void check_preset_save(void)
{
    uint8_t prefix[505] = {6};
    preset_record_t item = {1, 8, 0, 4, 1, {'B', 'a', 's', 's'}, {42}}, found;

    stored_size = 0;
    pending = 0;
    assert(companion_settings_load(prefix, sizeof prefix, 6) == 0);
    assert(companion_preset_save(prefix, sizeof prefix, &item, 0) == COMPANION_PRESET_PENDING);
    assert(companion_preset_save(prefix, sizeof prefix, &item, 1) == COMPANION_PRESET_BUSY);
    prefix[4] = 29;
    assert(companion_settings_save(prefix, sizeof prefix) == 0);
    assert(pending && !stored_size && queued[4] == 29);
    finish_write(KIT_STORE_ERR_FLASH);
    companion_preset_poll(sizeof prefix, 6);
    assert(companion_preset_status == COMPANION_PRESET_PENDING);
    finish_write(0);
    companion_preset_poll(sizeof prefix, 6);
    assert(companion_preset_status == COMPANION_PRESET_SAVED);
    assert(companion_settings_load(prefix, sizeof prefix, 6) == (int)sizeof prefix);
    assert(prefix[4] == 29);
    assert(companion_preset_find(sizeof prefix, 1, &found) == PRESET_VALID);
    assert(found.data[0] == 42);
    assert(companion_preset_save(prefix, sizeof prefix, &item, 1) == COMPANION_PRESET_SAVED);
    assert(!pending);

    item.data[0] = 81;
    assert(companion_preset_save(prefix, sizeof prefix, &item, 1) == COMPANION_PRESET_PENDING);
    finish_write(KIT_STORE_ERR_FULL);
    prefix[4] = 31;
    assert(companion_settings_save(prefix, sizeof prefix) == 0);
    assert(companion_preset_status == COMPANION_PRESET_WRITE_FAILED);
    finish_write(0);
    assert(companion_settings_load(prefix, sizeof prefix, 6) == (int)sizeof prefix);
    assert(prefix[4] == 31);
    assert(companion_preset_find(sizeof prefix, 1, &found) == PRESET_VALID);
    assert(found.data[0] == 42);

    assert(companion_preset_save(prefix, sizeof prefix, &item, 1) == COMPANION_PRESET_PENDING);
    pending = 0;  /* power loss before commit */
    assert(companion_settings_load(prefix, sizeof prefix, 6) == (int)sizeof prefix);
    assert(companion_preset_find(sizeof prefix, 1, &found) == PRESET_VALID);
    assert(found.data[0] == 42);
}

static void check_preset_workflow(void)
{
    preset_record_t item;
    stored_size = 0;
    pending = 0;
    set[0] = SETTINGS_VERSION;
    assert(companion_settings_load(set, sizeof set, SETTINGS_VERSION) == 0);
    edited[0] = 24;
    assert(preset_workflow_save(0, 0, "First", 5) == COMPANION_PRESET_PENDING);
    assert(preset_track_slot[0] == 0);
    assert(preset_workflow_recall(1, 1) == COMPANION_PRESET_BUSY);
    finish_write(0);
    preset_workflow_poll();
    assert(preset_track_slot[0] == 1 && companion_preset_next_slot(sizeof set) == 2);
    edited[0] = 48;
    assert(preset_workflow_save(0, 2, 0, 0) == PRESET_EDIT_SLOT);
    assert(preset_workflow_save(0, 1, 0, 0) == COMPANION_PRESET_PENDING);
    finish_write(KIT_STORE_ERR_FULL);
    assert(preset_workflow_recall(1, 1) == PRESET_VALID);
    assert(edited[1] == 24 && snd[1] == sound_index(8, 1) && preset_track_slot[1] == 1);

    assert(preset_workflow_save(0, 1, 0, 0) == COMPANION_PRESET_PENDING);
    finish_write(0);
    assert(preset_workflow_recall(1, 1) == PRESET_VALID && edited[1] == 48);
    assert(companion_preset_find(sizeof set, 1, &item) == PRESET_VALID);
    assert(item.name_len == 5 && !memcmp(item.name, "First", 5));
    assert(preset_workflow_save(0, 0, "Second", 6) == COMPANION_PRESET_PENDING);
    preset_workflow_forget(0);  /* track selected a different factory patch while Store finished */
    finish_write(0);
    preset_workflow_poll();
    assert(preset_track_slot[0] == 0 && preset_track_slot[1] == 1);
    assert(companion_preset_next_slot(sizeof set) == 3);
    assert(preset_workflow_recall(0, 2) == PRESET_VALID && preset_track_slot[0] == 2);
}

int main(void)
{
    uint8_t bytes[1024], before[1024];
    preset_record_t records[10] = {0}, actual;
    preset_reader_t reader;
    int size, i;
    unsigned k;
    memset(bytes, 0xa5, sizeof bytes);
    memcpy(before, bytes, sizeof bytes);
    assert(preset_catalog_open(&reader, bytes, 505, 505) == PRESET_ABSENT);
    assert(preset_catalog_write(bytes, sizeof bytes, 505, records, 0) == 505);
    for (i = 0; i < 10; ++i) {
        records[i].slot = (uint8_t)(i + 1);
        records[i].engine = 8;
        records[i].base = (uint8_t)i;
        records[i].name_len = PRESET_NAME_MAX;
        records[i].data_len = PRESET_DATA_MAX;
        memcpy(records[i].name, "Factory Bass", PRESET_NAME_MAX);
        memset(records[i].data, i, PRESET_DATA_MAX);
    }
    assert(!preset_catalog_write(bytes, sizeof bytes, 505, records, 10));
    assert(!memcmp(bytes, before, sizeof bytes));
    size = preset_catalog_write(bytes, sizeof bytes, 505, records, 9);
    assert(size == 505 + PRESET_HEADER + 9 * (5 + PRESET_NAME_MAX + PRESET_DATA_MAX) &&
           !memcmp(bytes, before, 505));
    assert(preset_catalog_open(&reader, bytes, size, 505) == PRESET_VALID);
    for (i = 0; i < 9; ++i) {
        assert(preset_catalog_next(&reader, &actual) == PRESET_VALID);
        assert(!memcmp(&actual, &records[i], sizeof actual));
    }
    assert(preset_catalog_next(&reader, &actual) == PRESET_ABSENT);
    for (k = 506; k < (unsigned)size; ++k)
        assert(preset_catalog_open(&reader, bytes, k, 505) == PRESET_CORRUPT);
    bytes[509] = 2;
    assert(preset_catalog_open(&reader, bytes, size, 505) == PRESET_CORRUPT);
    bytes[509] = 1;
    bytes[505 + PRESET_HEADER + 5 + PRESET_NAME_MAX + PRESET_DATA_MAX] = bytes[505 + PRESET_HEADER];
    assert(preset_catalog_open(&reader, bytes, size, 505) == PRESET_CORRUPT);
    records[1].slot = records[0].slot;
    memcpy(before, bytes, sizeof bytes);
    assert(!preset_catalog_write(bytes, sizeof bytes, 505, records, 2));
    assert(!memcmp(bytes, before, sizeof bytes));
    check_settings_record();
    check_catalog_edits();
    check_preset_save();
    check_preset_workflow();
    puts("PASS: settings migration and tail preservation.");
    puts("PASS: Save As, confirmed overwrite with variable length, and unchanged bytes on rejected edits.");
    puts("PASS: queued preset save, retry, durable recall, failed write rollback, and interruption.");
    puts("PASS: Save As and overwrite identity, rollback, slot recall, and track changes during save.");
    puts("PASS: legacy settings, 9 sample slots, round-trip, truncation, unsupported version, duplicate slots, atomic failure.");
}
