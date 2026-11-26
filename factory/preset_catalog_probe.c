/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../apps/companion/presetcatalog.c"

#define KIT_STORE_MAX 1024
#define KIT_STORE_ERR_SIZE (-1)
#define KIT_BIG
static uint8_t stored[KIT_STORE_MAX], queued[KIT_STORE_MAX];
static unsigned stored_size, queued_size;

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
    return 0;
}

#include "../apps/companion/settingsrecord.c"

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
        records[i].name_len = 12;
        records[i].data_len = 37;
        memcpy(records[i].name, "Factory Bass", 12);
        memset(records[i].data, i, 37);
    }
    assert(!preset_catalog_write(bytes, sizeof bytes, 505, records, 10));
    assert(!memcmp(bytes, before, sizeof bytes));
    size = preset_catalog_write(bytes, sizeof bytes, 505, records, 9);
    assert(size == 997 && !memcmp(bytes, before, 505));
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
    bytes[565] = bytes[511];
    assert(preset_catalog_open(&reader, bytes, size, 505) == PRESET_CORRUPT);
    records[1].slot = records[0].slot;
    memcpy(before, bytes, sizeof bytes);
    assert(!preset_catalog_write(bytes, sizeof bytes, 505, records, 2));
    assert(!memcmp(bytes, before, sizeof bytes));
    check_settings_record();
    check_catalog_edits();
    puts("PASS: settings migration and tail preservation.");
    puts("PASS: Save As, confirmed overwrite with variable length, and unchanged bytes on rejected edits.");
    puts("PASS: legacy settings, 9 sample slots, round-trip, truncation, unsupported version, duplicate slots, atomic failure.");
}
