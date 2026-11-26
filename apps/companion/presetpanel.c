/* SPDX-License-Identifier: GPL-3.0-only */
/* SAVE panel follows docs/labs/preset-save-lab.html. */
enum { PS_OFF, PS_MENU, PS_NAME, PS_WAIT, PS_NOTICE };
static const char ps_glyphs[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";
static int ps_page, ps_home, ps_choice, ps_cursor, ps_result;
static unsigned ps_slot;
static char ps_title[32], ps_name[PRESET_NAME_MAX + 1];

static const char *preset_track_name(int t, char *out)
{
    preset_record_t item;
    unsigned i, slot = preset_track_slot[t];
    if (slot && companion_preset_status != COMPANION_PRESET_PENDING &&
        companion_preset_find(sizeof set, slot, &item) == PRESET_VALID) {
        for (i = 0; i < item.name_len; ++i) out[i] = (char)item.name[i];
        out[i] = 0;
        return out;
    }
    return kit_preset_name(kit_track_engine(t), kit_track_preset(t));
}

static int ps_glyph_index(char c)
{
    int i;
    for (i = 0; ps_glyphs[i]; ++i)
        if (ps_glyphs[i] == c) return i;
    return 0;
}

static void preset_panel_begin(void)
{
    char user[PRESET_NAME_MAX + 1];
    const char *name = preset_track_name(track, user);
    unsigned i;
    ps_slot = preset_track_slot[track];
    sb_fit(ps_title, name, 143, KIT_FONT_SMALL);
    for (i = 0; i < PRESET_NAME_MAX; ++i) {
        char c = *name ? *name++ : ' ';
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        ps_name[i] = ps_glyphs[ps_glyph_index(c)];
    }
    ps_name[PRESET_NAME_MAX] = 0;
    ps_choice = ps_cursor = ps_result = 0;
    ps_page = PS_MENU;
}

static void ps_save(int overwrite)
{
    int left = 0, right = PRESET_NAME_MAX;
    if (!overwrite) {
        while (left < right && ps_name[left] == ' ') ++left;
        while (right > left && ps_name[right - 1] == ' ') --right;
        if (left == right) { ps_result = PRESET_EDIT_INVALID; return; }
    }
    ps_result = preset_workflow_save(track, overwrite ? ps_slot : 0, ps_name + left, right - left);
    ps_page = ps_result == COMPANION_PRESET_PENDING ? PS_WAIT : PS_NOTICE;
}

static int preset_panel_event(const kit_event_t *e)
{
    if (e->id == KIT_BTN_HOME && e->type == KIT_EV_BTN_UP && ps_home) {
        ps_home = 0;
        return 1;
    }
    if (ps_page == PS_NOTICE && panel_event(e)) {
        ps_page = PS_OFF;
        dirty = 1;
    }
    if (!ps_page) {
        if (e->type != KIT_EV_BTN_DOWN || e->id != KIT_BTN_SAVE || !sound_edit_open() ||
            mode != MODE_PRODUCER || picker || help || song_page || sb_open) return 0;
        preset_panel_begin();
        dirty = 1;
        return 1;
    }
    if (e->type == KIT_EV_KEY_UP ||
        ((e->type == KIT_EV_BTN_DOWN || e->type == KIT_EV_BTN_UP) && e->id == KIT_BTN_PLAY))
        return 0;  /* let notes release and allow stopping transport so Store can write */
    if (ps_page == PS_WAIT)
        return e->type == KIT_EV_KEY_DOWN || e->type == KIT_EV_ENC ||
               e->type == KIT_EV_BTN_DOWN || e->type == KIT_EV_BTN_UP;
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_HOME) {
        ps_home = home_long = 1;
        ps_page = PS_OFF;
    } else if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_SAVE) {
        if (ps_page == PS_NAME) ps_save(0);
        else if (!ps_choice) ps_page = PS_OFF;
        else if (ps_slot && ps_choice == 1) ps_save(1);
        else ps_page = PS_NAME;
    } else if (e->type == KIT_EV_ENC) {
        if (e->id == KIT_ENC_SELECT) {
            if (ps_page == PS_MENU) wrap(&ps_choice, e->delta, ps_slot ? 3 : 2);
            else wrap(&ps_cursor, e->delta, PRESET_NAME_MAX);
        } else if (ps_page == PS_NAME && e->id == KIT_ENC_KNOB1) {
            int glyph = ps_glyph_index(ps_name[ps_cursor]);
            wrap(&glyph, e->delta, sizeof ps_glyphs - 1);
            ps_name[ps_cursor] = ps_glyphs[glyph];
            ps_result = 0;
        }
    } else return e->type == KIT_EV_KEY_DOWN || e->type == KIT_EV_BTN_DOWN || e->type == KIT_EV_BTN_UP;
    dirty = 1;
    return 1;
}

static void preset_panel_frame(void)
{
    if (ps_page != PS_WAIT || companion_preset_status == COMPANION_PRESET_PENDING) return;
    ps_result = companion_preset_status;
    ps_page = PS_NOTICE;
    dirty = 1;
}

static void preset_panel_draw(void)
{
    const kit_color_t amber = KIT_RGB(255, 179, 71);
    char text[48], *end;
    int i, step;
    if (!ps_page) return;
    kit_clear(KIT_RGB(11, 20, 22));
    kit_text(8, 8, ps_title, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_INK);
    kit_text(232, 11, "SAVE PRESET", KIT_FONT_TINY, KIT_ALIGN_RIGHT, amber);
    kit_rect(8, 27, 224, 2, FX_GRID);
    if (ps_page == PS_MENU) {
        if (ps_slot) {
            end = cat(text, "Overwrite ");
            end = cat(end, ps_title);
            cat(end, "?");
        } else cat(text, "Save a new preset?");
        kit_text(8, 41, text, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_INK);
        for (i = 0; i < (ps_slot ? 3 : 2); ++i) {
            const char *item = !i ? "Cancel" : ps_slot && i == 1 ? "Overwrite" : "Save As";
            int y = 74 + 24 * i;
            if (i == ps_choice) kit_rect(8, y - 3, 224, 20, KIT_RGB(70, 53, 30));
            kit_text(14, y, item, KIT_FONT_SMALL, KIT_ALIGN_LEFT, i == ps_choice ? amber : FX_INK);
        }
        kit_text(8, 216, "SELECT chooses / SAVE confirms", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
        kit_text(8, 228, "HOME cancels", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    } else if (ps_page == PS_NAME) {
        kit_text(8, 41, "Name the new preset", KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_INK);
        step = 18;
        for (i = 0; i < PRESET_NAME_MAX; ++i) {
            char glyph[] = {ps_name[i] == ' ' ? '.' : ps_name[i], 0};
            int x = 14 + i * step;
            kit_text(x, 82, glyph, KIT_FONT_SMALL, KIT_ALIGN_CENTER, i == ps_cursor ? amber : FX_INK);
            if (i == ps_cursor) kit_rect(x - 5, 100, 10, 2, amber);
        }
        kit_text(8, 216, ps_result == PRESET_EDIT_INVALID ? "Enter a name" : "SELECT moves / K1 changes character",
                 KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
        kit_text(8, 228, "SAVE confirms / HOME cancels", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    } else {
        const char *message = ps_page == PS_WAIT ? "Saving preset" :
            ps_result == COMPANION_PRESET_SAVED ? "Preset saved" :
            ps_result == PRESET_EDIT_FULL ? "User preset storage is full" : "Save failed";
        kit_text(8, 41, message, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_INK);
        if (ps_page == PS_WAIT)
            kit_text(8, 216, "Waiting for storage. Stop playback if needed.", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    }
}
