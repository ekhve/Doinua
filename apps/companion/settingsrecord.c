/* SPDX-License-Identifier: GPL-3.0-only */
/* Companion's settings prefix and catalog tail share one Store transaction. */
#include "presetcatalog.c"

enum {
    COMPANION_PRESET_IDLE, COMPANION_PRESET_PENDING, COMPANION_PRESET_SAVED,
    COMPANION_PRESET_WRITE_FAILED = -4, COMPANION_PRESET_BUSY = -5
};
static uint8_t companion_settings_record[KIT_STORE_MAX] KIT_BIG;
static unsigned companion_settings_record_size;
static int companion_preset_status;

static unsigned companion_settings_read(unsigned size, unsigned version)
{
    int read = kit_store_load(companion_settings_record, sizeof companion_settings_record);
    unsigned n = read > 0 ? (unsigned)read : 0;
    /* Recognize the current prefix before retaining a catalog. Older versions
     * use the existing migration and start without any catalog. */
    companion_settings_record_size = n >= size && companion_settings_record[0] == version ? n : size;
    return n;
}

static int companion_settings_load(void *settings, unsigned size, unsigned version)
{
    uint8_t *prefix = settings;
    unsigned n = companion_settings_read(size, version);
    unsigned i, copied = n < size ? n : size;

    companion_preset_status = COMPANION_PRESET_IDLE;
    for (i = 0; i < copied; ++i) prefix[i] = companion_settings_record[i];
    return (int)copied;
}

static void companion_preset_poll(unsigned size, unsigned version)
{
    if (companion_preset_status != COMPANION_PRESET_PENDING || kit_store_pending()) return;
    if (kit_store_status() == 0) {
        companion_preset_status = COMPANION_PRESET_SAVED;
    } else {
        /* Do not let a routine settings write recommit a rejected catalog. */
        companion_settings_read(size, version);
        companion_preset_status = COMPANION_PRESET_WRITE_FAILED;
    }
}

static int companion_settings_save(const void *settings, unsigned size)
{
    const uint8_t *prefix = settings;
    unsigned i, bytes;
    if (!size || size > sizeof companion_settings_record) return KIT_STORE_ERR_SIZE;
    companion_preset_poll(size, prefix[0]);
    bytes = companion_settings_record_size > size ? companion_settings_record_size : size;
    if (bytes > sizeof companion_settings_record) return KIT_STORE_ERR_SIZE;
    for (i = 0; i < size; ++i) companion_settings_record[i] = prefix[i];
    return kit_store_save(companion_settings_record, (int)bytes);
}

/* Compare the encoded parts only. Bytes beyond the name and payload are workspace. */
static inline int companion_preset_matches(const preset_record_t *a, const preset_record_t *b)
{
    unsigned i;
    if (a->slot != b->slot || a->engine != b->engine || a->base != b->base ||
        a->name_len != b->name_len || a->data_len != b->data_len) return 0;
    for (i = 0; i < a->name_len; ++i) if (a->name[i] != b->name[i]) return 0;
    for (i = 0; i < a->data_len; ++i) if (a->data[i] != b->data[i]) return 0;
    return 1;
}

static inline int companion_preset_find(unsigned size, unsigned slot, preset_record_t *item)
{
    preset_reader_t reader;
    int state = preset_catalog_open(&reader, companion_settings_record,
                                    companion_settings_record_size, size);
    if (state != PRESET_VALID) return state;
    while (preset_catalog_next(&reader, item) == PRESET_VALID)
        if (item->slot == slot) return PRESET_VALID;
    return PRESET_ABSENT;
}

/* Scan once and select the first unused nonzero slot. Slot identity is
 * independent of browser order and does not change on overwrite. */
static inline int companion_preset_next_slot(unsigned size)
{
    uint8_t used[32] = {0};
    preset_reader_t reader;
    preset_record_t item;
    unsigned slot;
    int state = preset_catalog_open(&reader, companion_settings_record,
                                    companion_settings_record_size, size);
    if (state == PRESET_ABSENT) return 1;
    if (state != PRESET_VALID) return PRESET_EDIT_INVALID;
    while (preset_catalog_next(&reader, &item) == PRESET_VALID)
        used[item.slot >> 3] |= 1u << (item.slot & 7);
    for (slot = 1; slot <= 255; ++slot)
        if (!(used[slot >> 3] & (1u << (slot & 7)))) return (int)slot;
    return PRESET_EDIT_FULL;
}

static inline int companion_preset_save(const void *settings, unsigned size,
                                        const preset_record_t *item, int overwrite)
{
    const uint8_t *prefix = settings;
    preset_record_t current;
    int bytes;
    if (!size || size > sizeof companion_settings_record) return PRESET_EDIT_INVALID;
    companion_preset_poll(size, prefix[0]);
    if (companion_preset_status == COMPANION_PRESET_PENDING) return COMPANION_PRESET_BUSY;
    if (overwrite && companion_preset_find(size, item->slot, &current) == PRESET_VALID &&
        companion_preset_matches(&current, item))
        return companion_preset_status = COMPANION_PRESET_SAVED;

    bytes = preset_catalog_edit(companion_settings_record, companion_settings_record_size,
                               sizeof companion_settings_record, size, item, overwrite);
    if (bytes < 0) return bytes;
    companion_settings_record_size = (unsigned)bytes;
    if (companion_settings_save(settings, size) == 0)
        return companion_preset_status = COMPANION_PRESET_PENDING;
    companion_settings_read(size, prefix[0]);
    return companion_preset_status = COMPANION_PRESET_WRITE_FAILED;
}
