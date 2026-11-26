/* SPDX-License-Identifier: GPL-3.0-only */
/* The panel and browser use the same track identity and save transaction. */
static uint8_t preset_track_slot[KIT_NTRACKS], preset_pending_slot;
static int preset_pending_track = -1;

static void preset_workflow_forget(int t)
{
    preset_track_slot[t] = 0;
    if (preset_pending_track == t) preset_pending_track = -1;
}

static void preset_workflow_poll(void)
{
    companion_preset_poll(sizeof set, SETTINGS_VERSION);
    if (preset_pending_track < 0 || companion_preset_status == COMPANION_PRESET_PENDING) return;
    if (companion_preset_status == COMPANION_PRESET_SAVED)
        preset_track_slot[preset_pending_track] = preset_pending_slot;
    preset_pending_track = -1;
}

/* The overwrite action must supply the exact slot shown in the confirmation.
 * Save As passes zero and supplies a name. */
static inline int preset_workflow_save(int t, unsigned overwrite_slot, const char *name, unsigned name_len)
{
    preset_record_t item = {0};
    unsigned i;
    int slot, result;
    if (t < 0 || t >= KIT_NTRACKS) return PRESET_EDIT_INVALID;
    preset_workflow_poll();
    if (companion_preset_status == COMPANION_PRESET_PENDING) return COMPANION_PRESET_BUSY;
    if (overwrite_slot) {
        if (preset_track_slot[t] != overwrite_slot ||
            companion_preset_find(sizeof set, overwrite_slot, &item) != PRESET_VALID) return PRESET_EDIT_SLOT;
    } else {
        if (!name || !name_len || name_len > PRESET_NAME_MAX) return PRESET_EDIT_INVALID;
        slot = companion_preset_next_slot(sizeof set);
        if (slot < 0) return slot;
        item.slot = slot;
        item.name_len = name_len;
        for (i = 0; i < name_len; ++i) item.name[i] = (uint8_t)name[i];
    }
    if (!preset_patch_capture(t, &item)) return PRESET_EDIT_INVALID;
    result = companion_preset_save(&set, sizeof set, &item, overwrite_slot != 0);
    if (result == COMPANION_PRESET_PENDING) {
        preset_pending_track = t;
        preset_pending_slot = item.slot;
    } else if (result == COMPANION_PRESET_SAVED) {
        preset_track_slot[t] = item.slot;
    }
    return result;
}

static inline int preset_workflow_recall(int t, unsigned slot)
{
    preset_record_t item;
    int state;
    if (t < 0 || t >= KIT_NTRACKS || !slot) return PRESET_EDIT_INVALID;
    preset_workflow_poll();
    if (companion_preset_status == COMPANION_PRESET_PENDING) return COMPANION_PRESET_BUSY;
    state = companion_preset_find(sizeof set, slot, &item);
    if (state != PRESET_VALID) return state;
    if (!preset_patch_recall(t, &item)) return PRESET_EDIT_INVALID;
    snd[t] = sound_index(item.engine, item.base);
    preset_track_slot[t] = item.slot;
    return PRESET_VALID;
}
