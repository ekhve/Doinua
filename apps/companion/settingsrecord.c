/* SPDX-License-Identifier: GPL-3.0-only */
/* Companion's settings prefix and catalog tail share one Store transaction. */
static uint8_t companion_settings_record[KIT_STORE_MAX] KIT_BIG;
static unsigned companion_settings_record_size;

static int companion_settings_load(void *settings, unsigned size, unsigned version)
{
    uint8_t *prefix = settings;
    int read = kit_store_load(companion_settings_record, sizeof companion_settings_record);
    unsigned n = read > 0 ? (unsigned)read : 0;
    unsigned i, copied = n < size ? n : size;

    /* Recognize the current prefix before retaining a catalog. Older versions
     * use the existing migration and start without any catalog. */
    companion_settings_record_size = n >= size && companion_settings_record[0] == version ? n : size;
    for (i = 0; i < copied; ++i) prefix[i] = companion_settings_record[i];
    return (int)copied;
}

static int companion_settings_save(const void *settings, unsigned size)
{
    const uint8_t *prefix = settings;
    unsigned i, bytes = companion_settings_record_size > size ? companion_settings_record_size : size;
    if (bytes > sizeof companion_settings_record) return KIT_STORE_ERR_SIZE;
    for (i = 0; i < size; ++i) companion_settings_record[i] = prefix[i];
    return kit_store_save(companion_settings_record, (int)bytes);
}
