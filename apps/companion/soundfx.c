/* SPDX-License-Identifier: GPL-3.0-only */
/* Sound Edit Lab v3 (D-072): live FX controls over the track's playing page. */
enum { FXP_NONE, FXP_PERFORM, FXP_DELAY, FXP_REVERB, FXP_DIRT, FXP_COUNT };
static int fx_page, fx_pressed, fx_long, fx_bend_track = -1;
static uint32_t fx_press_at, fx_bend_at;
static int fx_bend[KIT_NTRACKS], fx_mod[KIT_NTRACKS];
static int fx_pair = KIT_PERFORM_DUB, fx_pair_track = -1, fx_xy[2] = {0, 64};
static const char *const fx_pair_text[][3] = {
    {"FILTER", "X CUTOFF", "Y RESONANCE"},
    {"WAH", "X SWEEP", "Y SHARPNESS"},
    {"DUB", "X ECHO", "Y TIME"},
    {"SPACE", "X REVERB", "Y DARK"},
    {"DIRT", "X DRIVE", "Y CRUSH"},
    {"WOBBLE", "X SPEED", "Y DEPTH"}
};

#define FX_INK KIT_RGB(233, 236, 242)
#define FX_BG KIT_RGB(14, 16, 23)
#define FX_DIM KIT_RGB(108, 115, 134)
#define FX_GRID KIT_RGB(34, 39, 58)
#define FX_PINK KIT_RGB(255, 107, 154)
#define FX_CYAN KIT_RGB(111, 214, 255)
#define FX_ROOM KIT_RGB(154, 208, 255)
#define FX_ORANGE KIT_RGB(255, 122, 89)

static int sound_fx_open(void) { return fx_page != FXP_NONE; }

static void sound_fx_bend(int t, int value)
{
    if (fx_bend_track == t) fx_bend_track = -1;     /* a MIDI wheel takes over from the panel */
    fx_bend[t] = value;
    kit_track_bend(t, value);
    if (sound_fx_open() && t == track) dirty = 1;
}

static void sound_fx_mod(int t, int value)
{
    fx_mod[t] = value;
    kit_track_mod(t, value);
    if (sound_fx_open() && t == track) dirty = 1;
}

static void sound_fx_close(void)
{
    if (!sound_fx_open()) return;
    fx_page = FXP_NONE;
    if (fx_bend_track >= 0) sound_fx_bend(fx_bend_track, 0);
    dirty = 1;
}

static int sound_fx_available(void)
{
    return mode == MODE_PRODUCER && !picker && !help && !song_page && !rl_open && !rl_ask && sq_view < 0 && !is_drums();
}

static int fx_time_limit(const kit_fx_t *bus) { return bus->dly_type == 3 ? 127 : 93; }

static void fx_reset_pair(void)
{
    fx_xy[0] = fx_pair == KIT_PERFORM_WOBBLE ? 64 : 0;
    fx_xy[1] = fx_pair == KIT_PERFORM_DIRT || fx_pair == KIT_PERFORM_WOBBLE ? 0 : 64;
    kit_perform_fx(-1, fx_pair, 0, 0);
}

static void fx_show_page(int page)
{
    fx_page = page;
    if (page == FXP_PERFORM && fx_pair_track != track) {
        fx_pair_track = track;
        fx_reset_pair();
    }
    dirty = 1;
}

static void fx_adjust_send(int which, int delta)
{
    int sends[4], i;
    for (i = 0; i < 4; i++) sends[i] = kit_track_send_of(track, i);
    clampi(&sends[which], delta, 0, 127);
    kit_track_sends(track, sends[0], sends[1], sends[2], sends[3]);
}

static void fx_adjust_delay(int knob, int delta)
{
    kit_fx_t bus;
    int value;
    kit_fx_bus_of(&bus);
    if (knob == 0) {
        value = bus.dly_time;
        clampi(&value, delta, 0, fx_time_limit(&bus));
        bus.dly_time = (uint8_t)value;
        kit_fx_bus(&bus);
    } else if (knob == 1) {
        value = bus.dly_fb;
        clampi(&value, delta, 0, 120);
        bus.dly_fb = (uint8_t)value;
        kit_fx_bus(&bus);
    } else if (knob == 2) {
        fx_adjust_send(KIT_SEND_DELAY, delta);
    } else {
        kit_delay_tone(kit_delay_tone_of() + delta);
    }
}

static void fx_adjust_reverb(int knob, int delta)
{
    kit_reverb_shape_t shape;
    int value;
    if (knob == 1) {
        fx_adjust_send(KIT_SEND_REVERB, delta);
        return;
    }
    kit_reverb_shape_of(&shape);
    if (knob == 0) {
        value = shape.size;
        clampi(&value, delta, 0, 127);
        shape.size = (uint8_t)value;
    } else if (knob == 2) {
        value = shape.damp;
        clampi(&value, delta, 0, 100);
        shape.damp = (uint8_t)value;
    } else {
        value = shape.pre_ms;
        clampi(&value, delta, 0, 120);
        shape.pre_ms = (uint8_t)value;
    }
    kit_reverb_shape(&shape);
}

static void fx_adjust_dirt(int knob, int delta)
{
    kit_dirt_t setting;
    int value;
    uint8_t *control;
    if (knob == 0) {
        fx_adjust_send(KIT_SEND_DRIVE, delta);
        return;
    }
    kit_track_dirt_of(track, &setting);
    control = knob == 1 ? &setting.crush : knob == 2 ? &setting.tone : &setting.mix;
    value = *control;
    clampi(&value, delta, 0, 127);
    *control = (uint8_t)value;
    kit_track_dirt(track, &setting);
}

static int sound_fx_event(const kit_event_t *e)
{
    int knob, delta, value;
    if (e->id == KIT_BTN_FX && e->type == KIT_EV_BTN_DOWN && sound_fx_available()) {
        fx_pressed = 1;
        fx_long = 0;
        fx_press_at = kit_ms();
        sb_open = 0;
        dirty = 1;
        return 1;
    }
    if (e->id == KIT_BTN_FX && e->type == KIT_EV_BTN_UP && fx_pressed) {
        fx_pressed = 0;
        if (sound_fx_available() && !fx_long) {
            fx_show_page(fx_page + 1 == FXP_COUNT ? FXP_PERFORM : fx_page + 1);
        }
        return 1;
    }
    if (!sound_fx_open() || e->type != KIT_EV_ENC) return 0;
    if (!sound_fx_available()) {
        sound_fx_close();
        return 0;
    }
    if (e->id == KIT_ENC_ALGORITHM) {
        sound_fx_close();
        return 0;
    }
    if (e->id == KIT_ENC_SELECT) {
        if (fx_page == FXP_PERFORM) {
            wrap(&fx_pair, e->delta, KIT_NPERFORM_FX);
            fx_reset_pair();
        } else {
            int page = fx_page - FXP_PERFORM;
            wrap(&page, e->delta, FXP_COUNT - FXP_PERFORM);
            fx_show_page(page + FXP_PERFORM);
        }
        dirty = 1;
        return 1;
    }
    if (e->id < KIT_ENC_KNOB1 || e->id > KIT_ENC_KNOB4) return 0;
    knob = e->id - KIT_ENC_KNOB1;
    delta = e->delta * 4;
    if (fx_page == FXP_DELAY) {
        fx_adjust_delay(knob, delta);
    } else if (fx_page == FXP_REVERB) {
        fx_adjust_reverb(knob, delta);
    } else if (fx_page == FXP_DIRT) {
        fx_adjust_dirt(knob, delta);
    } else if (knob == 0) {
        value = fx_bend[track];
        clampi(&value, e->delta * 512, -8192, 8191);
        sound_fx_bend(track, value);
        fx_bend_track = track;
        fx_bend_at = kit_ms();
    } else if (knob == 1) {
        value = fx_mod[track];
        clampi(&value, delta, 0, 127);
        sound_fx_mod(track, value);
    } else if (fx_pair == KIT_PERFORM_FILTER) {
        kit_sound_filter_t filter;
        if (kit_track_filter_of(track, &filter)) {
            uint8_t *param = knob == 2 ? &filter.cutoff : &filter.resonance;
            value = *param;
            clampi(&value, delta, 0, 127);
            *param = (uint8_t)value;
            kit_track_filter(track, &filter);
        }
    } else {
        if (fx_pair_track != track) {
            fx_pair_track = track;
            fx_reset_pair();
        }
        clampi(&fx_xy[knob - 2], delta, 0, 127);
        kit_perform_fx(track, fx_pair, fx_xy[0], fx_xy[1]);
    }
    dirty = 1;
    return 1;
}

static void sound_fx_frame(uint32_t now)
{
    if (mode != MODE_PRODUCER && fx_pair_track >= 0) {
        fx_pair_track = -1;
        fx_reset_pair();
    }
    if (!sound_fx_available()) {
        sound_fx_close();
        fx_pressed = 0;
        return;
    }
    if (fx_pressed && !fx_long && now - fx_press_at >= HOLD_MS) {
        fx_long = 1;
        fx_show_page(FXP_PERFORM);
    }
    if (fx_bend_track >= 0 && now - fx_bend_at >= 180u) {
        int t = fx_bend_track;
        int v = fx_bend[t];
        v -= v / 4 + (v > 0 ? 1 : v < 0 ? -1 : 0);
        sound_fx_bend(t, v);
        if (v) fx_bend_track = t;
    }
}

static int fx_percent(int value, int limit) { return (value * 100 + limit / 2) / limit; }

static void fx_dial(int cx, const char *label, const char *value, int percent, kit_color_t col)
{
    static const int8_t arc[][2] = {
        {-14, 14}, {-17, 11}, {-18, 8}, {-20, 4}, {-20, 0}, {-20, -4}, {-18, -8}, {-17, -11},
        {-14, -14}, {-11, -17}, {-8, -18}, {-4, -20}, {0, -20}, {4, -20}, {8, -18}, {11, -17},
        {14, -14}, {17, -11}, {18, -8}, {20, -4}, {20, 0}, {20, 4}, {18, 8}, {17, 11}, {14, 14}
    };
    int i, end = (percent * 24 + 50) / 100;
    int cy = 196;
    for (i = 0; i <= 24; i++) {
        int x = cx + arc[i][0], y = cy + arc[i][1];
        kit_circle(x, y, 2, i <= end ? col : FX_GRID);
    }
    kit_line(cx, cy, cx + arc[end][0] * 13 / 20, cy + arc[end][1] * 13 / 20, FX_INK);
    kit_text(cx, 163, label, KIT_FONT_TINY, KIT_ALIGN_CENTER, FX_DIM);
    kit_text(cx, 222, value, KIT_FONT_XSMALL, KIT_ALIGN_CENTER, FX_INK);
}

static void fx_draw_delay(int delay_ms, int feedback, int send, int tone)
{
    int i, level = 100;
    kit_rect_outline(4, 34, 232, 120, 1, FX_GRID);
    kit_text(10, 38, "Note and echoes over 3 s", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    for (i = 0; i < 30 && i * delay_ms < 3000 && level > 0; i++) {
        kit_rect(10 + 220 * i * delay_ms / 3000, 150 - level, 4, level,
                 i == 0 ? FX_INK : tone < 64 ? KIT_RGB(54, 132, 156) : FX_CYAN);
        level = i == 0 ? send : level * feedback * 9 / 1000;
    }
}

static void fx_draw_perform(int bend, int mod, int x, int y)
{
    int dx = (bend - 50) * 90 / 100;
    int px = 116 + x * 114 / 100, py = 152 - y * 114 / 100;
    kit_text(8, 40, "BEND", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    kit_rect(8, 54, 90, 8, FX_GRID);
    if (dx) kit_rect(dx < 0 ? 53 + dx : 53, 54, dx < 0 ? -dx : dx, 8, FX_PINK);
    kit_rect(52, 51, 2, 14, FX_INK);
    kit_text(8, 72, "MOD", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    kit_rect(8, 86, 90, 8, FX_GRID);
    if (mod) kit_rect(8, 86, mod * 90 / 100, 8, KIT_RGB(255, 209, 102));
    kit_text(8, 106, fx_pair_text[fx_pair][0], KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_PINK);
    kit_text(8, 124, fx_pair_text[fx_pair][1], KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_CYAN);
    kit_text(8, 136, fx_pair_text[fx_pair][2], KIT_FONT_TINY, KIT_ALIGN_LEFT, KIT_RGB(199, 155, 255));
    kit_text(8, 148, "SELECT pair", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    kit_rect_outline(112, 34, 122, 122, 1, FX_GRID);
    kit_line(173, 35, 173, 154, FX_GRID);
    kit_line(113, 95, 232, 95, FX_GRID);
    if (x >= 0 && y >= 0) {
        kit_circle(px, py, 12, KIT_RGB(61, 32, 52));
        kit_circle(px, py, 5, FX_PINK);
    }
}

static void fx_draw_reverb(int tail_ms, int send, int damp, int pre_ms)
{
    int x, w = tail_ms * 220 / 5000;
    int level, offset = pre_ms * 220 / 5000;
    kit_color_t color = KIT_RGB(154 - damp, 208 - damp, 255 - damp);
    kit_rect_outline(4, 34, 232, 120, 1, FX_GRID);
    kit_text(10, 38, "Room tail over 5 s", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    for (x = 0; x < w; x++) {
        int remaining = (w - x) * 100 / w;
        level = send * remaining * remaining * remaining / 1000000;
        if (level) kit_rect(10 + offset + x, 150 - level, 1, level, color);
    }
}

static void fx_draw_dirt(void)
{
    int i, prev = 148;
    kit_rect_outline(4, 34, 232, 120, 1, FX_GRID);
    kit_line(10, 148, 230, 40, FX_GRID);
    for (i = 0; i <= 110; i++) {
        int32_t sample = i * 65534 / 110 - 32767;
        int y = 94 - kit_track_dirt_curve(track, sample) * 54 / 32767;
        if (i) kit_line(8 + 2 * i, prev, 10 + 2 * i, y, FX_ORANGE);
        prev = y;
    }
    kit_text(10, 38, "INPUT TO OUTPUT", KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
}

static void sound_fx_draw(void)
{
    static const kit_color_t knob_col[] = { KIT_RGB(255, 107, 107), KIT_RGB(255, 209, 102), FX_CYAN, KIT_RGB(199, 155, 255) };
    static const char *const delay_labels[] = { "TIME", "FEEDBACK", "SEND", "TONE" };
    static const char *const perform_labels[] = { "BEND", "MOD", "X", "Y" };
    static const char *const reverb_labels[] = { "SIZE", "SEND", "DAMP", "PRE-DELAY" };
    static const char *const dirt_labels[] = { "DRIVE", "CRUSH", "TONE", "MIX" };
    static const char *const pages[] = { "PERFORM", "DELAY", "REVERB", "DIRT" };
    static const kit_color_t page_colors[] = { FX_PINK, FX_CYAN, FX_ROOM, FX_ORANGE };
    static const char *const *const page_labels[] = { perform_labels, delay_labels, reverb_labels, dirt_labels };
    kit_fx_t bus;
    kit_reverb_shape_t room;
    kit_dirt_t dirt;
    int i, v[4], ms = kit_delay_ms_of(), tail = kit_reverb_tail_ms_of();
    int perform = fx_page == FXP_PERFORM, reverb = fx_page == FXP_REVERB, dirt_page = fx_page == FXP_DIRT;
    int tone = kit_delay_tone_of();
    const char *const *labels = page_labels[fx_page - 1];
    const char *page = pages[fx_page - 1];
    kit_color_t color = page_colors[fx_page - 1];
    char title[32], value[16];
    const char *name = kit_preset_name(kit_track_engine(track), kit_track_preset(track));
    kit_fx_bus_of(&bus);
    kit_reverb_shape_of(&room);
    kit_track_dirt_of(track, &dirt);
    clampi(&ms, 0, 1, 743);
    v[2] = fx_percent(kit_track_send_of(track, KIT_SEND_DELAY), 127);
    if (perform) {
        kit_sound_filter_t filter;
        v[0] = fx_percent(fx_bend[track] + 8192, 16383);
        v[1] = fx_percent(fx_mod[track], 127);
        if (fx_pair == KIT_PERFORM_FILTER) {
            int supported = kit_track_filter_of(track, &filter);
            v[2] = supported ? fx_percent(filter.cutoff, 127) : -1;
            v[3] = supported ? fx_percent(filter.resonance, 127) : -1;
        } else {
            v[2] = fx_percent(fx_xy[0], 127);
            v[3] = fx_percent(fx_xy[1], 127);
        }
    } else if (reverb) {
        v[0] = fx_percent(room.size, 127);
        v[1] = fx_percent(kit_track_send_of(track, KIT_SEND_REVERB), 127);
        v[2] = room.damp;
        v[3] = fx_percent(room.pre_ms, 120);
    } else if (dirt_page) {
        v[0] = fx_percent(kit_track_send_of(track, KIT_SEND_DRIVE), 127);
        v[1] = fx_percent(dirt.crush, 127);
        v[2] = fx_percent(dirt.tone, 127);
        v[3] = fx_percent(dirt.mix, 127);
    } else {
        v[0] = fx_percent(ms, 743);
        v[1] = fx_percent(bus.dly_fb > 120 ? 120 : bus.dly_fb, 120);
        v[3] = fx_percent(tone, 127);
    }
    kit_clear(FX_BG);
    sb_fit(title, name, 220 - kit_text_width(page, KIT_FONT_XSMALL), KIT_FONT_SMALL);
    kit_text(6, 4, title, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_INK);
    kit_text(234, 7, page, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, color);
    for (i = 0; i < 4; i++)
        kit_rect(6 + i * 57, 25, 53, 3, i == fx_page - 1 ? color : FX_GRID);
    if (perform) fx_draw_perform(v[0], v[1], v[2], v[3]);
    else if (reverb) fx_draw_reverb(tail, v[1], room.damp, room.pre_ms);
    else if (dirt_page) fx_draw_dirt();
    else fx_draw_delay(ms, v[1], v[2], v[3]);
    for (i = 0; i < 4; i++) {
        if (reverb && i == 0) {
            int tenths = (tail + 50) / 100;
            char *out = cat_int(value, tenths / 10);
            out = cat(out, ".");
            out = cat_int(out, tenths % 10);
            cat(out, " s");
        } else if (reverb && i == 3) cat(cat_int(value, room.pre_ms), " ms");
        else if (dirt_page && i == 1) {
            int bits = kit_dirt_bits(dirt.crush);
            if (bits == 16) cat(value, "OFF");
            else cat(cat_int(value, bits), " bit");
        } else if (fx_page == FXP_DELAY && i == 0) cat(cat_int(value, ms), " ms");
        else if (fx_page == FXP_DELAY && i == 3 && tone == 127) cat(value, "TYPE");
        else if (perform && i == 0) {
            int raw = fx_bend[track];
            int tenth = ((raw < 0 ? -raw : raw) * 20 + 4096) / 8192;
            char *out = value;
            if (tenth && raw < 0) *out++ = '-';
            if (tenth && raw > 0) *out++ = '+';
            cat(cat_int(cat(cat_int(out, tenth / 10), "."), tenth % 10), " st");
        }
        else if (v[i] < 0) cat(value, "--");
        else cat(cat_int(value, v[i]), "%");
        fx_dial(30 + i * 60, labels[i], value, v[i] < 0 ? 0 : v[i], v[i] < 0 ? FX_DIM : knob_col[i]);
    }
}
