/* SPDX-License-Identifier: GPL-3.0-only */
/* Preset catalog payload appended to the existing Companion settings record.
 * This codec operates on bytes. The existing Store owns the flash transaction. */
#include <stdint.h>

enum { PRESET_NAME_MAX = 12, PRESET_DATA_MAX = 37, PRESET_HEADER = 6 };
enum { PRESET_CORRUPT = -1, PRESET_ABSENT = 0, PRESET_VALID = 1 };
static const uint8_t preset_magic[5] = {'D', 'P', 'S', 'T', 1};

typedef struct {
    uint8_t slot, engine, base, name_len, data_len;
    uint8_t name[PRESET_NAME_MAX];
    uint8_t data[PRESET_DATA_MAX];
} preset_record_t;

typedef struct {
    const uint8_t *next;
    const uint8_t *end;
    unsigned remaining;
} preset_reader_t;

static int preset_record_valid(const preset_record_t *p)
{
    unsigned i;
    int named = 0;
    if (!p->slot || !p->name_len || p->name_len > PRESET_NAME_MAX ||
        !p->data_len || p->data_len > PRESET_DATA_MAX)
        return 0;
    for (i = 0; i < p->name_len; ++i) {
        if (p->name[i] < 32 || p->name[i] > 126) return 0;
        if (p->name[i] != ' ') named = 1;
    }
    return named;
}

static int preset_catalog_next(preset_reader_t *reader, preset_record_t *record)
{
    preset_record_t value = {0};
    const uint8_t *at = reader->next;
    unsigned i;
    if (!reader->remaining) return PRESET_ABSENT;
    if (reader->end - at < 5) return PRESET_CORRUPT;
    value.slot = *at++;
    value.engine = *at++;
    value.base = *at++;
    value.name_len = *at++;
    value.data_len = *at++;
    if (value.name_len > PRESET_NAME_MAX || value.data_len > PRESET_DATA_MAX ||
        reader->end - at < value.name_len + value.data_len)
        return PRESET_CORRUPT;
    for (i = 0; i < value.name_len; ++i) value.name[i] = *at++;
    for (i = 0; i < value.data_len; ++i) value.data[i] = *at++;
    if (!preset_record_valid(&value)) return PRESET_CORRUPT;
    *record = value;
    reader->next = at;
    --reader->remaining;
    return PRESET_VALID;
}

/* Open only an entirely valid tail. Old settings records have no tail. */
static int preset_catalog_open(preset_reader_t *reader, const uint8_t *record,
                               unsigned size, unsigned settings_size)
{
    uint8_t seen[32] = {0};
    preset_reader_t scan;
    preset_record_t item;
    unsigned i, mask;
    if (size == settings_size) return PRESET_ABSENT;
    if (size < settings_size || size - settings_size < PRESET_HEADER) return PRESET_CORRUPT;
    record += settings_size;
    for (i = 0; i < sizeof preset_magic; ++i)
        if (record[i] != preset_magic[i]) return PRESET_CORRUPT;
    scan.next = record + PRESET_HEADER;
    scan.end = record + (size - settings_size);
    scan.remaining = record[5];
    while (scan.remaining) {
        if (preset_catalog_next(&scan, &item) != PRESET_VALID) return PRESET_CORRUPT;
        mask = 1u << (item.slot & 7u);
        if (seen[item.slot >> 3] & mask) return PRESET_CORRUPT;
        seen[item.slot >> 3] |= mask;
    }
    if (scan.next != scan.end) return PRESET_CORRUPT;
    *reader = (preset_reader_t){ record + PRESET_HEADER, scan.end, record[5] };
    return PRESET_VALID;
}

/* The settings prefix is already in out. Failure leaves the entire buffer intact. */
static int preset_catalog_write(uint8_t *out, unsigned capacity, unsigned settings_size,
                                const preset_record_t *items, unsigned count)
{
    uint8_t seen[32] = {0};
    unsigned end = settings_size, i, k, mask;
    if (end > capacity || count > 255) return 0;
    if (!count) return (int)end;
    if (capacity - end < PRESET_HEADER) return 0;
    end += PRESET_HEADER;
    for (i = 0; i < count; ++i) {
        const preset_record_t *item = &items[i];
        unsigned bytes = 5u + item->name_len + item->data_len;
        mask = 1u << (item->slot & 7u);
        if (!preset_record_valid(item) || seen[item->slot >> 3] & mask || capacity - end < bytes)
            return 0;
        seen[item->slot >> 3] |= mask;
        end += bytes;
    }
    out += settings_size;
    for (i = 0; i < sizeof preset_magic; ++i) *out++ = preset_magic[i];
    *out++ = count;
    for (i = 0; i < count; ++i) {
        const preset_record_t *item = &items[i];
        *out++ = item->slot;
        *out++ = item->engine;
        *out++ = item->base;
        *out++ = item->name_len;
        *out++ = item->data_len;
        for (k = 0; k < item->name_len; ++k) *out++ = item->name[k];
        for (k = 0; k < item->data_len; ++k) *out++ = item->data[k];
    }
    return (int)end;
}
