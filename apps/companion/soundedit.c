/* SPDX-License-Identifier: GPL-3.0-only */
/* Sound Edit v3 (D-072): live sound controls in Producer. */
enum { SED_OSC, SED_FILTER, SED_FENV, SED_AENV, SED_VOICE, SED_NPAGES };
static int sed_page = -1, sed_press_track = -1;
static uint32_t sed_press_at;
static const char *const sed_waves[] = { "TRI", "SHARK", "SAW", "SQR", "PLS W", "PLS N", "SIN" };
static const char *const sed_modes[] = { "POLY", "MONO", "LEGATO" };
static const uint16_t sed_glide_times[] = {
    0, 5, 7, 10, 14, 20, 28, 40, 56, 80, 113, 160, 226, 320, 452, 640, 905, 1280, 1810, 2000
};

static int sed_voice_supported(void)
{
    /* These engines use the kit's voice allocation, glide and pitch path. */
    return kit_track_engine(track) <= KIT_ENGINE_FMINI && kit_track_engine(track) != KIT_ENGINE_DRUMS;
}

static int sed_glide_step(int ms)
{
    int i, last = (int)(sizeof sed_glide_times / sizeof sed_glide_times[0]) - 1;
    for (i = 0; i < last; i++)
        if (ms * 2 < sed_glide_times[i] + sed_glide_times[i + 1]) break;
    return i;
}

static int sound_edit_open(void) { return sed_page >= 0; }

static void sound_edit_close(void)
{
    sed_press_track = -1;
    if (!sound_edit_open()) return;
    sed_page = -1;
    dirty = 1;
}

static void sound_edit_press(void)
{
    sed_press_track = sound_fx_available() ? track : -1;
    sed_press_at = kit_ms();
}

static void sound_edit_release(void)
{
    if (sed_press_track != track || !sound_fx_available()) return;
    sed_press_track = -1;
    sound_fx_close();
    sb_open = 0;
    if (kit_ms() - sed_press_at >= HOLD_MS || !sound_edit_open()) sed_page = SED_OSC;
    else wrap(&sed_page, 1, SED_NPAGES);
    dirty = 1;
}

static void sed_adjust_filter(int knob, int delta)
{
    kit_sound_filter_t filter;
    int value;
    if (!kit_track_filter_of(track, &filter)) return;
    uint8_t *controls[] = { &filter.cutoff, &filter.resonance, &filter.amount, &filter.keytrack };
    value = *controls[knob];
    clampi(&value, delta * 4, 0, 127);
    *controls[knob] = (uint8_t)value;
    kit_track_filter(track, &filter);
}

static void sed_adjust_osc(int knob, int delta)
{
    kit_sound_osc_t osc;
    if (!kit_track_osc_of(track, &osc)) return;
    if (knob < 2) {
        int wave = knob ? osc.wave2 : osc.wave1;
        wrap(&wave, delta, KIT_OSC_NWAVES);
        kit_track_osc(track, knob == 0 ? KIT_OSC_WAVE1 : KIT_OSC_WAVE2, wave);
    } else if (knob == 2) {
        kit_track_osc(track, KIT_OSC_DETUNE, osc.detune + delta * 5);
    } else {
        kit_track_osc(track, KIT_OSC_MIX, osc.mix + delta * 4);
    }
}

static void sed_adjust_env(int knob, int delta)
{
    uint8_t env[4];
    int kind = sed_page == SED_FENV ? KIT_ENV_FILTER : KIT_ENV_AMP;
    if (kit_track_envelope_of(track, kind, env)) kit_track_envelope(track, kind, knob, env[knob] + delta * 4);
}

static void sed_adjust_voice(int knob, int delta)
{
    int voice_mode, glide, slide;
    if (!sed_voice_supported()) return;
    voice_mode = kit_track_voice_mode(track);
    glide = kit_track_glide_ms(track);
    slide = kit_track_slide(track);
    if (knob == 0) {
        int before = voice_mode;
        wrap(&voice_mode, delta, KIT_VMODE_LEGATO + 1);
        if (voice_mode != before) kit_track_notes_off(track);
    } else if (knob == 1) {
        int step = sed_glide_step(glide);
        clampi(&step, delta, 0, (int)(sizeof sed_glide_times / sizeof sed_glide_times[0]) - 1);
        glide = sed_glide_times[step];
    } else if (knob == 2) {
        wrap(&slide, delta, 2);
    } else {
        kit_track_bend_range(track, kit_track_bend_range_of(track) + delta);
        return;
    }
    kit_track_voice(track, voice_mode, glide, slide);
}

static int sound_edit_event(const kit_event_t *e)
{
    int knob;
    if (e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_ENV && sound_fx_available()) {
        sound_fx_close();
        sb_open = 0;
        sed_page = sed_page == SED_FENV ? SED_AENV : SED_FENV;
        dirty = 1;
        return 1;
    }
    if (!sound_edit_open() || e->type != KIT_EV_ENC) return 0;
    if (!sound_fx_available() || e->id == KIT_ENC_ALGORITHM) {
        sound_edit_close();
        return 0;
    }
    if (e->id == KIT_ENC_SELECT) wrap(&sed_page, e->delta, SED_NPAGES);
    else if (e->id >= KIT_ENC_KNOB1 && e->id <= KIT_ENC_KNOB4) {
        knob = e->id - KIT_ENC_KNOB1;
        if (sed_page == SED_FILTER) sed_adjust_filter(knob, e->delta);
        else if (sed_page == SED_OSC) sed_adjust_osc(knob, e->delta);
        else if (sed_page == SED_VOICE) sed_adjust_voice(knob, e->delta);
        else sed_adjust_env(knob, e->delta);
    } else return 0;
    dirty = 1;
    return 1;
}

static int sed_wave(int phase, int wave)
{
    int tri = phase < 50 ? 2 * phase : 2 * (100 - phase);
    switch (wave) {
    case KIT_OSC_TRI: return 2 * tri - 100;
    case KIT_OSC_SHARK: return phase < 75 ? 8 * phase / 3 - 100 : 700 - 8 * phase;
    case KIT_OSC_SAW: return 2 * phase - 100;
    case KIT_OSC_SQR: return phase < 50 ? 100 : -100;
    case KIT_OSC_WIDE: return phase < 70 ? 100 : -100;
    case KIT_OSC_NARROW: return phase < 20 ? 100 : -100;
    default: return (phase < 50 ? 1 : -1) * 4 * (phase % 50) * (50 - phase % 50) / 25;
    }
}

static void sed_draw_osc(const kit_sound_osc_t *osc)
{
    int x, voice;
    for (voice = 0; voice < 2; voice++) {
        int wave = voice ? osc->wave2 : osc->wave1;
        int level = voice ? osc->mix : 127 - osc->mix;
        int last = 94;
        kit_color_t color = voice ? KIT_RGB(199, 155, 255) : FX_CYAN;
        for (x = 0; x <= 220; x++) {
            int y = 94 - sed_wave(x * 200 / 220 % 100, wave) * (10 + level / 4) / 100;
            if (x) kit_line(9 + x, last, 10 + x, y, color);
            last = y;
        }
    }
}

/* A cutoff and resonance sketch. A live spectrum needs a separate audio tap. */
static void sed_draw_filter(const kit_sound_filter_t *filter, kit_color_t color)
{
    int x, last = 80, cutoff = (int)filter->cutoff * 216 / 127;
    for (x = 0; x <= 220; x++) {
        int distance = x - cutoff;
        int y = 80 + (distance > 0 ? distance * 2 : 0);
        int peak = distance < 0 ? -distance : distance;
        if (peak < 16) y -= filter->resonance * (16 - peak) / 64;
        if (y > 150) y = 150;
        if (x) kit_line(9 + x, last, 10 + x, y, color);
        last = y;
    }
}

static void sed_draw_env(const uint8_t env[4], const int times[4], kit_color_t color)
{
    static const char *const stage[] = { "A", "D", "S", "R" };
    static const uint8_t fall[] = { 127, 99, 77, 59, 46, 35, 27, 20, 15, 11, 8, 5, 3, 2, 1, 0 };
    const int bottom = 148, top = 59;
    int i, j, start = 11, width[4], total = 0, x = start, y = bottom;
    int levels[] = { 0, 127, env[KIT_ENV_SUSTAIN], env[KIT_ENV_SUSTAIN], 0 };
    for (i = 0; i < 4; i++) {
        int ms = i == KIT_ENV_SUSTAIN ? 600 : times[i];
        width[i] = 20 + ms * 90 / (ms + 500);
        total += width[i];
    }
    for (i = 0; i < 4; i++) {
        int span = width[i] * 218 / total, from = x;
        kit_text(from + span - 1, 39, stage[i], KIT_FONT_TINY, KIT_ALIGN_RIGHT, FX_DIM);
        for (j = 1; j <= 15; j++) {
            int level = levels[i + 1] + (levels[i] - levels[i + 1]) * fall[j] / 127;
            int px = from + j * span / 15, py = bottom - level * (bottom - top) / 127;
            kit_line(x, y, px, py, color);
            x = px; y = py;
        }
    }
}

static void sed_draw_voice(int voice_mode, int glide, int slide, kit_color_t color)
{
    static const uint8_t fall[] = { 100, 73, 51, 35, 23, 14, 7, 3, 0 };
    int i, n = voice_mode == KIT_VMODE_POLY ? 3 : 1;
    int span = glide ? 10 + glide * 150 / (glide + 250) : 0;
    kit_text(10, 40, sed_modes[voice_mode], KIT_FONT_SMALL, KIT_ALIGN_LEFT, color);
    kit_text(10, 57, !glide ? "No glide" : slide == KIT_SLIDE_LEGATO ? "While held" : "Always",
             KIT_FONT_TINY, KIT_ALIGN_LEFT, FX_DIM);
    for (i = 0; i < n; i++) {
        int from = 138 - i * 19, rise = n == 1 ? 40 : i == 1 ? 7 : 15;
        int j, x = 65, y = from;
        kit_line(10, from, x, from, color);
        for (j = 1; j <= 8; j++) {
            int px = 65 + span * j / 8, py = from - rise * (100 - fall[j]) / 100;
            kit_line(x, y, px, py, color);
            x = px; y = py;
        }
        kit_line(x, y, 230, y, color);
    }
}

static void sed_time_text(char *value, int ms)
{
    if (ms < 1000) cat(cat_int(value, ms), " ms");
    else {
        int hundredths = (ms + 5) / 10 % 100;
        char *out = cat(cat_int(value, (ms + 5) / 1000), ".");
        if (hundredths < 10) out = cat(out, "0");
        cat(cat_int(out, hundredths), " s");
    }
}

static void sound_edit_draw(void)
{
    static const char *const names[] = { "OSC", "FILTER", "F.ENV", "A.ENV", "VOICE" };
    static const char *const osc_labels[] = { "WAVE 1", "WAVE 2", "DETUNE", "MIX" };
    static const char *const filter_labels[] = { "CUTOFF", "RESO", "ENV AMT", "KEY TRK" };
    static const char *const env_labels[] = { "ATTACK", "DECAY", "SUSTAIN", "RELEASE" };
    static const char *const voice_labels[] = { "MODE", "GLIDE", "SLIDE", "BEND RANGE" };
    static const kit_color_t knob_colors[] = { KIT_RGB(255, 107, 107), KIT_RGB(255, 209, 102), FX_CYAN, KIT_RGB(199, 155, 255) };
    static const kit_color_t page_colors[] = { KIT_RGB(111, 183, 255), KIT_RGB(255, 179, 71), KIT_RGB(255, 209, 102), KIT_RGB(126, 224, 138), KIT_RGB(199, 155, 255) };
    kit_sound_osc_t osc;
    kit_sound_filter_t filter;
    uint8_t env[4];
    int i, values[4] = {0}, times[4] = {0}, supported;
    int envelope = sed_page == SED_FENV || sed_page == SED_AENV;
    int voice = sed_page == SED_VOICE, voice_mode = 0, glide = 0, slide = 0, bend = 2;
    char title[32], value[16];
    const char *const *labels = voice ? voice_labels : envelope ? env_labels : sed_page == SED_FILTER ? filter_labels : osc_labels;
    const char *page = names[sed_page];
    kit_color_t color = page_colors[sed_page];
    const char *preset = kit_preset_name(kit_track_engine(track), kit_track_preset(track));
    kit_clear(FX_BG);
    sb_fit(title, preset, 220 - kit_text_width(page, KIT_FONT_XSMALL), KIT_FONT_SMALL);
    kit_text(6, 4, title, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FX_INK);
    kit_text(234, 7, page, KIT_FONT_XSMALL, KIT_ALIGN_RIGHT, color);
    for (i = 0; i < 7; i++) {
        int start = 6 + (228 * i) / 7, end = 6 + (228 * (i + 1)) / 7;
        kit_rect(start, 25, end - start - 4, 3, i == sed_page ? color : FX_GRID);
    }
    kit_rect_outline(4, 34, 232, 120, 1, FX_GRID);
    if (voice) {
        supported = sed_voice_supported();
        if (supported) {
            voice_mode = kit_track_voice_mode(track);
            glide = kit_track_glide_ms(track);
            slide = kit_track_slide(track);
            bend = kit_track_bend_range_of(track);
            values[0] = fx_percent(voice_mode, KIT_VMODE_LEGATO);
            values[1] = fx_percent(sed_glide_step(glide), (int)(sizeof sed_glide_times / sizeof sed_glide_times[0]) - 1);
            values[2] = slide == KIT_SLIDE_LEGATO ? 100 : 0;
            values[3] = fx_percent(bend - 1, 11);
            sed_draw_voice(voice_mode, glide, slide, color);
        }
    } else if (envelope) {
        supported = kit_track_envelope_of(track, sed_page == SED_FENV ? KIT_ENV_FILTER : KIT_ENV_AMP, env);
        if (supported) {
            for (i = 0; i < 4; i++) {
                values[i] = fx_percent(env[i], 127);
                if (i != KIT_ENV_SUSTAIN) times[i] = kit_track_envelope_ms(track, env[i]);
            }
            sed_draw_env(env, times, color);
        }
    } else if (sed_page == SED_FILTER) {
        supported = kit_track_filter_of(track, &filter);
        if (supported) {
            values[0] = fx_percent(filter.cutoff, 127);
            values[1] = fx_percent(filter.resonance, 127);
            values[2] = fx_percent(filter.amount, 127);
            values[3] = fx_percent(filter.keytrack, 127);
            sed_draw_filter(&filter, color);
        }
    } else {
        supported = kit_track_osc_of(track, &osc);
        if (supported) {
            values[0] = fx_percent(osc.wave1, KIT_OSC_NWAVES - 1);
            values[1] = fx_percent(osc.wave2, KIT_OSC_NWAVES - 1);
            values[2] = fx_percent(osc.detune + 700, 1400);
            values[3] = fx_percent(osc.mix, 127);
            sed_draw_osc(&osc);
        }
    }
    for (i = 0; i < 4; i++) {
        if (!supported) cat(value, "--");
        else if (sed_page == SED_OSC && i < 2) cat(value, sed_waves[i ? osc.wave2 : osc.wave1]);
        else if (sed_page == SED_OSC && i == 2) cat(cat_int(value, osc.detune), " ct");
        else if (envelope && i != KIT_ENV_SUSTAIN) sed_time_text(value, times[i]);
        else if (voice) {
            if (i == 0) cat(value, sed_modes[voice_mode]);
            else if (i == 1 && glide) sed_time_text(value, glide);
            else if (i == 1) cat(value, "OFF");
            else if (i == 2) cat(value, slide == KIT_SLIDE_LEGATO ? "LEGATO" : "ALWAYS");
            else cat(cat_int(value, bend), " st");
        }
        else cat(cat_int(value, values[i]), "%");
        fx_dial(30 + i * 60, labels[i], value, values[i], supported ? knob_colors[i] : FX_DIM);
    }
}
