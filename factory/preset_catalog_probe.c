/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../apps/companion/presetcatalog.c"

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
    puts("PASS: legacy settings, 9 sample slots, round-trip, truncation, unsupported version, duplicate slots, atomic failure.");
}
