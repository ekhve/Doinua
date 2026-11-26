/* SPDX-License-Identifier: GPL-3.0-only */
/* Preset catalog payload appended to the existing Companion settings record.
 * This codec operates on bytes. The existing Store owns the flash transaction. */
#include <stdint.h>

enum { PRESET_NAME_MAX = 12, PRESET_DATA_MAX = 40, PRESET_HEADER = 6 };
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

static uint8_t *preset_record_write(uint8_t *out, const preset_record_t *item)
{
    unsigned i;
    *out++ = item->slot;
    *out++ = item->engine;
    *out++ = item->base;
    *out++ = item->name_len;
    *out++ = item->data_len;
    for (i = 0; i < item->name_len; ++i) *out++ = item->name[i];
    for (i = 0; i < item->data_len; ++i) *out++ = item->data[i];
    return out;
}

/* The settings prefix is already in out. Failure leaves the entire buffer intact. */
static inline int preset_catalog_write(uint8_t *out, unsigned capacity, unsigned settings_size,
                                const preset_record_t *items, unsigned count)
{
    uint8_t seen[32] = {0};
    unsigned end = settings_size, i, mask;
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
    for (i = 0; i < count; ++i) out = preset_record_write(out, &items[i]);
    return (int)end;
}

enum { PRESET_EDIT_INVALID = -1, PRESET_EDIT_SLOT = -2, PRESET_EDIT_FULL = -3 };

/* Return the new record size, or leave all bytes unchanged on error. The slot
 * must be empty for Save As and must already exist for confirmed overwrite. */
static inline int preset_catalog_edit(uint8_t *record, unsigned size, unsigned capacity,
                               unsigned settings_size, const preset_record_t *item,
                               int overwrite)
{
    preset_reader_t reader;
    preset_record_t current;
    unsigned count = 0, offset = size, old_bytes = 0;
    unsigned new_bytes = 5u + item->name_len + item->data_len;
    unsigned i, tail;
    int state;
    if (size > capacity || !preset_record_valid(item)) return PRESET_EDIT_INVALID;
    state = preset_catalog_open(&reader, record, size, settings_size);
    if (state == PRESET_CORRUPT) return PRESET_EDIT_INVALID;
    if (state == PRESET_VALID) {
        count = reader.remaining;
        while (reader.remaining) {
            unsigned start = (unsigned)(reader.next - record);
            if (preset_catalog_next(&reader, &current) != PRESET_VALID)
                return PRESET_EDIT_INVALID;
            if (current.slot == item->slot) {
                offset = start;
                old_bytes = (unsigned)(reader.next - record) - start;
                break;
            }
        }
    }
    if (!!old_bytes != !!overwrite) return PRESET_EDIT_SLOT;
    if (state == PRESET_ABSENT) {
        if (capacity - size < PRESET_HEADER + new_bytes) return PRESET_EDIT_FULL;
        for (i = 0; i < sizeof preset_magic; ++i) record[size + i] = preset_magic[i];
        size += PRESET_HEADER;
        offset = size;
    } else if ((!overwrite && count == 255) || capacity - (size - old_bytes) < new_bytes) {
        return PRESET_EDIT_FULL;
    }
    tail = size - offset - old_bytes;
    if (new_bytes > old_bytes) {
        for (i = tail; i > 0; --i)
            record[offset + new_bytes + i - 1] = record[offset + old_bytes + i - 1];
    } else {
        for (i = 0; i < tail; ++i)
            record[offset + new_bytes + i] = record[offset + old_bytes + i];
    }
    preset_record_write(record + offset, item);
    record[settings_size + 5] = count + !overwrite;
    return (int)(size - old_bytes + new_bytes);
}
