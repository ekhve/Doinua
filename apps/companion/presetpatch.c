/* SPDX-License-Identifier: GPL-3.0-only */
/* Payload v1: track effects, native controls for the base engine, then only
 * the edited oscillator/envelope fields. Shared song effects are never read. */
enum { PRESET_PATCH_VERSION = 1, PRESET_TRACK_FX_BYTES = 7 };
typedef struct {
    const uint8_t *fx, *filter, *voice, *lfo, *mod, *osc, *env;
    unsigned osc_len, env_len;
} preset_patch_view_t;

static int preset_voice_supported(int engine)
{
    return engine >= 0 && engine <= KIT_ENGINE_FMINI && engine != KIT_ENGINE_DRUMS;
}

static int preset_native_filter(int engine)
{
    return engine == KIT_ENGINE_FMINI || engine == KIT_ENGINE_JUNO;
}

static unsigned preset_mask_bytes(unsigned mask)
{
    unsigned n = 0;
    for (; mask; mask >>= 1) n += mask & 1u;
    return n;
}

static const uint8_t *preset_patch_take(const uint8_t **at, const uint8_t *end, unsigned n)
{
    const uint8_t *part = *at;
    if ((unsigned)(end - part) < n) return 0;
    *at += n;
    return part;
}

static int preset_patch_range(const uint8_t *at, unsigned n, unsigned max)
{
    unsigned i;
    if (!at) return 0;
    for (i = 0; i < n; ++i) if (at[i] > max) return 0;
    return 1;
}

static int preset_patch_open(const preset_record_t *item, preset_patch_view_t *view)
{
    const uint8_t *p = item->data, *end;
    int engine = item->engine;
    if (!item->data_len || item->data_len > PRESET_DATA_MAX ||
        engine >= kit_engine_count() || engine == KIT_ENGINE_DRUMS ||
        item->base >= kit_preset_count(engine) || *p++ != PRESET_PATCH_VERSION) return 0;
    end = item->data + item->data_len;
    *view = (preset_patch_view_t){0};
    view->fx = preset_patch_take(&p, end, PRESET_TRACK_FX_BYTES);
    if (!preset_patch_range(view->fx, PRESET_TRACK_FX_BYTES, 127)) return 0;
    if (preset_native_filter(engine)) {
        view->filter = preset_patch_take(&p, end, 4);
        if (!preset_patch_range(view->filter, 4, 127)) return 0;
    }
    if (preset_voice_supported(engine)) {
        view->voice = preset_patch_take(&p, end, 4);
        if (!view->voice || (view->voice[0] & 3) > KIT_VMODE_LEGATO || view->voice[0] > 6 ||
            view->voice[1] + 256 * view->voice[2] > 2000 ||
            view->voice[3] < 1 || view->voice[3] > 12) return 0;
    }
    if (engine == KIT_ENGINE_FMINI) {
        view->lfo = preset_patch_take(&p, end, 4);
        if (!preset_patch_range(view->lfo, 2, 127) || view->lfo[2] >= KIT_LFO_NTARGETS ||
            view->lfo[3] >= KIT_LFO_NWAVES) return 0;
        view->mod = preset_patch_take(&p, end, 4);
        if (!view->mod || view->mod[0] >= SED_NMOD || view->mod[1] >= SED_NMOD ||
            view->mod[2] == 255 || view->mod[3] == 255 || p == end) return 0;
        view->osc_len = 1 + preset_mask_bytes(*p & 15u) + preset_mask_bytes(*p & 12u);
        view->osc = preset_patch_take(&p, end, view->osc_len);
        if (!view->osc || !preset_osc_walk(-1, view->osc, view->osc_len)) return 0;
    }
    if (preset_native_filter(engine)) {
        if (p == end) return 0;
        view->env_len = 1 + preset_mask_bytes(*p);
        view->env = preset_patch_take(&p, end, view->env_len);
        if (!view->env || !preset_env_validate(engine, view->env, view->env_len)) return 0;
    }
    return p == end;
}

static inline int preset_patch_capture(int t, preset_record_t *item)
{
    uint8_t *p = item->data;
    kit_dirt_t dirt;
    preset_patch_view_t check;
    unsigned n;
    int engine = kit_track_engine(t), base = kit_track_preset(t), i;
    if (engine < 0 || engine == KIT_ENGINE_DRUMS || base < 0 || base > 255) return 0;
    item->engine = engine;
    item->base = base;
    *p++ = PRESET_PATCH_VERSION;
    for (i = 0; i < 4; ++i) *p++ = kit_track_send_of(t, i);
    kit_track_dirt_of(t, &dirt);
    *p++ = dirt.crush; *p++ = dirt.tone; *p++ = dirt.mix;
    if (preset_native_filter(engine)) {
        kit_sound_filter_t filter;
        if (!kit_track_filter_of(t, &filter)) return 0;
        *p++ = filter.cutoff; *p++ = filter.resonance; *p++ = filter.amount; *p++ = filter.keytrack;
    }
    if (preset_voice_supported(engine)) {
        unsigned glide = kit_track_glide_ms(t);
        *p++ = kit_track_voice_mode(t) | (kit_track_slide(t) << 2);
        *p++ = (uint8_t)glide; *p++ = (uint8_t)(glide >> 8);
        *p++ = kit_track_bend_range_of(t);
    }
    if (engine == KIT_ENGINE_FMINI) {
        kit_sound_lfo_t lfo;
        if (!kit_track_lfo_of(t, &lfo)) return 0;
        *p++ = lfo.rate; *p++ = lfo.depth; *p++ = lfo.target; *p++ = lfo.wave;
        *p++ = sed_mod[t].target[0]; *p++ = sed_mod[t].target[1];
        *p++ = sed_mod[t].amount[0] + 127; *p++ = sed_mod[t].amount[1] + 127;
        n = preset_osc_capture(t, sed_osc_edits[t], p);
        if (!n) return 0;
        p += n;
    }
    if (preset_native_filter(engine)) {
        n = preset_env_capture(t, sed_env_edits[t], p);
        if (!n) return 0;
        p += n;
    }
    item->data_len = p - item->data;
    return preset_patch_open(item, &check);
}

static inline int preset_patch_recall(int t, const preset_record_t *item)
{
    preset_patch_view_t view;
    const uint8_t *p;
    if (t < 0 || t >= KIT_NTRACKS || !preset_patch_open(item, &view)) return 0;
    kit_track_sound_local(t, item->engine, item->base);
    sound_mod_reset(t);
    p = view.fx;
    kit_track_sends(t, p[0], p[1], p[2], p[3]);
    kit_track_dirt(t, &(kit_dirt_t){p[4], p[5], p[6]});
    if ((p = view.filter))
        kit_track_filter(t, &(kit_sound_filter_t){p[0], p[1], p[2], p[3]});
    if ((p = view.voice)) {
        kit_track_voice(t, p[0] & 3, p[1] + 256 * p[2], p[0] >> 2);
        kit_track_bend_range(t, p[3]);
    }
    if ((p = view.lfo)) kit_track_lfo(t, &(kit_sound_lfo_t){p[0], p[1], p[2], p[3]});
    if ((p = view.mod)) {
        sed_mod[t] = (sed_mod_t){{p[0], p[1]}, {(int)p[2] - 127, (int)p[3] - 127}};
        sound_mod_apply(t, fx_mod[t]);
    }
    if (view.osc) {
        preset_osc_restore(t, view.osc, view.osc_len);
        sed_osc_edits[t] = view.osc[0];
    }
    if (view.env) {
        preset_env_restore(t, view.env, view.env_len);
        sed_env_edits[t] = view.env[0];
    }
    return 1;
}
