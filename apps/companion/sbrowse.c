/* SPDX-License-Identifier: GPL-3.0-only */
/* The Sound Browser (ST-68, D-087), as the Sound Browser Lab the owner approved on 2026-11-21 (D-084: the lab is the
 * screen's contract). PRESETS on a track page opens the list over the page, on the track's sound (or its template's
 * group: CHORDS Keys, BASS Bass, MELODY Lead); PRESETS walks the sounds, on into the next group at the end; K1 jumps a
 * whole group, back to where it was in it, only while the list is open ("K1 GROUP" says so); the keys play the sound
 * meanwhile. It closes 2 s after the last turn or key, and while a key is held it stays open; HOME closes it, and so
 * does ALGORITHM (another track). The groups down the left with how many sounds each has, the sounds of the group on
 * the right, twelve at a time, the one chosen lit, "3 / 32" at the top.
 * The sounds are the owner's picks of the engines the firmware has (sb_data.h, tools/gen_browser.py), found by name at
 * start-up. The DRUMS track keeps its own PRESETS (kits, and My kit's pads, D-053). Differences from the lab: no green
 * dots (the lab marked the first sounds to be built; they are), DRUMS dim on the synth tracks (it is the drums track's), the
 * groups' names in the kit's smallest font (TINY, 10 px; the lab's are 8 px). */
#include "sb_data.h"

#define SB_OPEN_MS 2000u
static int16_t sb_snd[SB_NALL];                 /* each list sound as the app's sound index (sound_index), -1: missing */
static int sb_pos[SB_NGROUPS + 1];              /* where each group was left, including USER */
static int sb_cur;                              /* the sound chosen in the list (an SB_ALL index); sb_open: it shows */
static uint32_t sb_at;                          /* the last turn or key */
#define SB_DRUMS (SB_NGROUPS - 1)
#define SB_USER SB_NGROUPS
static uint8_t sb_user_slots[255];
static int sb_user_count;

static inline int preset_workflow_recall(int t, unsigned slot);
static int preset_workflow_available(const preset_record_t *item);

static void sb_collect_users(void)
{
    preset_reader_t reader;
    preset_record_t item;
    sb_user_count = 0;
    if (companion_preset_status == COMPANION_PRESET_PENDING ||
        preset_catalog_open(&reader, companion_settings_record,
                            companion_settings_record_size, sizeof set) != PRESET_VALID) return;
    while (preset_catalog_next(&reader, &item) == PRESET_VALID)
        if (preset_workflow_available(&item)) sb_user_slots[sb_user_count++] = item.slot;
}

static int sb_groups(void) { return SB_NGROUPS + (sb_user_count != 0); }
static int sb_total(void) { return SB_NALL + sb_user_count; }
static int sb_group_of(int i) { return i >= SB_NALL ? SB_USER : SB_ALL[i].group; }

static int sb_eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

static void sb_init(void)                       /* each picked sound's engine and preset, by name */
{
    int i, e, p;
    for (i = 0; i < SB_NALL; i++) {
        sb_snd[i] = -1;
        for (e = 0; e < kit_engine_count() && sb_snd[i] < 0; e++)
            if (sb_eq(kit_engine_name(e), SB_ENGINE[SB_ALL[i].engine]))
                for (p = 0; p < kit_preset_count(e); p++)
                    if (sb_eq(kit_preset_name(e, p), SB_ALL[i].preset)) { sb_snd[i] = (int16_t)sound_index(e, p); break; }
    }
    for (i = 0; i <= SB_NGROUPS; i++) sb_pos[i] = -1;
}

static int sb_usable(int i)
{
    if (i < 0 || i >= sb_total()) return 0;
    return i >= SB_NALL || (sb_snd[i] >= 0 && SB_ALL[i].group != SB_DRUMS);
}
static int sb_count(int g)
{
    int i, n = 0;
    if (g == SB_USER) return sb_user_count;
    for (i = 0; i < SB_NALL; i++) n += sb_usable(i) && SB_ALL[i].group == g;
    return n;
}
static int sb_first(int g)                      /* the group's sound where it was left, else its first; -1: none */
{
    int i;
    if (sb_usable(sb_pos[g]) && sb_group_of(sb_pos[g]) == g) return sb_pos[g];
    if (g == SB_USER) return sb_user_count ? SB_NALL : -1;
    for (i = 0; i < SB_NALL; i++) if (sb_usable(i) && SB_ALL[i].group == g) return i;
    return -1;
}
static int sb_home_group(void)                  /* the template's group (the lab: CHORDS Keys, BASS Bass, MELODY Lead) */
{
    int r = set.role[track];
    return r == ROLE_BASS ? 0 : r == ROLE_MELODY ? 1 : 2;
}
static int sb_find(void)                        /* the track's sound in the list, or -1 */
{
    int i;
    if (preset_track_slot[track])
        for (i = 0; i < sb_user_count; ++i)
            if (sb_user_slots[i] == preset_track_slot[track]) return SB_NALL + i;
    for (i = 0; i < SB_NALL; i++) if (sb_usable(i) && sb_snd[i] == snd[track]) return i;
    return -1;
}

static void sb_choose(int i)
{
    if (i >= SB_NALL) {
        unsigned slot = sb_user_slots[i - SB_NALL];
        if (slot != preset_track_slot[track] && preset_workflow_recall(track, slot) != PRESET_VALID)
            return;
    } else if (snd[track] != sb_snd[i] || preset_track_slot[track]) {
        snd[track] = sb_snd[i];
        pick_sound(track);
    }
    sb_cur = i;
    sb_pos[sb_group_of(i)] = i;
}

static void sb_show(void) { sb_open = 1; sb_at = kit_ms(); dirty = 1; }

static void sb_turn(int d)                      /* PRESETS: a sound at a time, on into the next group */
{
    int i, n, total;
    if (!sb_open) sb_collect_users();
    i = sb_find();
    total = sb_total();
    if (i < 0) {                                /* not a list sound (a template's own): the template's group */
        int g = sb_home_group(), k;
        for (k = 0; k < sb_groups() && (i = sb_first((g + k) % sb_groups())) < 0; k++) ;
        if (i < 0) return;
        sb_choose(i);
        sb_show();
        return;
    }
    while (d) {                                 /* each click a sound (a fast turn brings several) */
        int s = d > 0 ? 1 : -1;
        for (n = 0; n < total; n++) {
            i = (i + s + total) % total;
            if (sb_usable(i)) break;
        }
        d -= s;
    }
    sb_choose(i);
    sb_show();
}

static void sb_jump(int d)                      /* K1 while the list shows: a whole group */
{
    int g = sb_group_of(sb_cur), k, i = -1, groups = sb_groups();
    while (d) {                                 /* each click a group */
        int s = d > 0 ? 1 : -1;
        for (k = 0, i = -1; k < groups && i < 0; k++) {
            g = (g + s + groups) % groups;
            i = sb_first(g);
        }
        d -= s;
    }
    if (i >= 0) sb_choose(i);
    sb_show();
}

static int sb_event(const kit_event_t *e)
{
    if (mode != MODE_PRODUCER || picker || help || song_page || is_drums())
        return 0;
    if (e->type == KIT_EV_ENC && e->id == KIT_ENC_PRESETS) { sb_turn(e->delta); return 1; }
    if (!sb_open)
        return 0;
    if (e->type == KIT_EV_ENC && e->id == KIT_ENC_KNOB1) { sb_jump(e->delta); return 1; }
    if (e->type == KIT_EV_KEY_DOWN) sb_at = kit_ms();                  /* playing keeps it open; the key still plays */
    if ((e->type == KIT_EV_BTN_DOWN && e->id == KIT_BTN_HOME) || (e->type == KIT_EV_ENC && e->id == KIT_ENC_ALGORITHM)) {
        sb_open = 0;
        dirty = 1;
        return e->id == KIT_BTN_HOME && e->type == KIT_EV_BTN_DOWN;     /* HOME only closes it; ALGORITHM goes on */
    }
    return 0;
}

static void sb_frame(uint32_t now)
{
    int k;
    if (!sb_open) return;
    if (mode != MODE_PRODUCER || picker || help || song_page || is_drums()) { sb_open = 0; dirty = 1; return; }
    for (k = 0; k < KIT_NKEYS; k++) if (kit_key_held(k)) { sb_at = now; break; }   /* a key held keeps it open */
    if ((int32_t)(now - sb_at) >= (int32_t)SB_OPEN_MS) { sb_open = 0; dirty = 1; }
}

static void sb_fit(char *d, const char *s, int w, int font)   /* s, cut to w pixels */
{
    int n = 0;
    while (s[n] && n < 31) { d[n] = s[n]; n++; }
    d[n] = 0;
    while (n > 1 && kit_text_width(d, font) > w) d[--n] = 0;
}

#define SB_BG KIT_RGB(14, 18, 26)
#define SB_SEL_D KIT_RGB(29, 58, 110)
static void sb_draw(void)                       /* the lab's list, over the page under the top line */
{
    int g = sb_group_of(sb_cur), k, r, i, n = 0, at = 0, first, y, rows[SB_NALL], nr = 0;
    int group_step = sb_user_count ? 16 : 17;
    char s[40], *d;
    kit_rect(0, 22, 240, 218, KIT_RGB(10, 13, 19));
    kit_round_rect(3, 25, 234, 212, 8, ACC);
    kit_round_rect(4, 26, 232, 210, 7, SB_BG);
    for (k = 0; k < sb_groups(); k++) {         /* the groups down the left, how many each has (DRUMS: the kits, for the
                                                 * drums track; dim here, as a synth track does not play them) */
        int on = k == g, c = 0;
        if (k == SB_DRUMS) { for (i = 0; i < SB_NALL; i++) c += sb_snd[i] >= 0 && SB_ALL[i].group == SB_DRUMS; }
        else c = sb_count(k);
        if (k == SB_DRUMS) { y = 35 + k * group_step; ch_text(10, y, SB_SHORT[k], KIT_FONT_TINY, KIT_ALIGN_LEFT, LINE); cat_int(s, c);
                             ch_text(66, y, s, KIT_FONT_TINY, KIT_ALIGN_RIGHT, LINE); continue; }
        y = 35 + k * group_step;
        if (on) kit_round_rect(6, y - 8, 64, 16, 4, SB_SEL_D);
        ch_text(10, y, k == SB_USER ? "USER" : SB_SHORT[k], KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? FG : c ? MUTED : LINE);
        cat_int(s, c);
        ch_text(66, y, s, KIT_FONT_TINY, KIT_ALIGN_RIGHT, on ? KIT_RGB(207, 224, 255) : KIT_RGB(74, 82, 99));
    }
    kit_rect(73, 30, 1, 202, KIT_RGB(34, 41, 54));
    kit_round_rect(6, 223, 64, 11, 3, CARD);
    ch_small(38, 228, "K1 GROUP", ACC);
    if (g == SB_USER) { n = nr = sb_user_count; at = sb_cur - SB_NALL; }
    else for (i = 0; i < SB_NALL; i++) if (sb_usable(i) && SB_ALL[i].group == g) { if (i == sb_cur) at = n; rows[nr++] = i; n++; }
    d = s; { const char *t = g == SB_USER ? "USER" : SB_GROUP[g]; while (*t) { *d++ = (char)(*t >= 'a' && *t <= 'z' ? *t - 32 : *t); t++; } *d = 0; }
    ch_text(80, 36, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, ACC);
    d = cat_int(s, at + 1); d = cat(d, " / "); cat_int(d, n);
    ch_text(230, 36, s, KIT_FONT_TINY, KIT_ALIGN_RIGHT, MUTED);
    first = at - 5;                             /* twelve rows, the one chosen near the middle */
    if (first > nr - 12) first = nr - 12;
    if (first < 0) first = 0;
    for (r = 0; r < 12 && first + r < nr; r++) {
        int on = first + r == at;
        y = 52 + r * 15;
        if (on) kit_round_rect(77, y - 7, 156, 14, 3, ACC);
        if (g == SB_USER) {
            preset_record_t item;
            char name[PRESET_NAME_MAX + 1];
            if (companion_preset_find(sizeof set, sb_user_slots[first + r], &item) != PRESET_VALID) continue;
            for (i = 0; i < item.name_len; ++i) name[i] = (char)item.name[i];
            name[i] = 0;
            sb_fit(s, name, 136, KIT_FONT_TINY);
        } else sb_fit(s, SB_ALL[rows[first + r]].name, 136, KIT_FONT_TINY);
        ch_text(89, y, s, KIT_FONT_TINY, KIT_ALIGN_LEFT, on ? KIT_WHITE : FG);
    }
    for (k = 0; k < 3; k++) {                   /* more above, more below */
        if (first > 0) kit_rect(155 - k, 42 + k, 2 * k + 1, 1, MUTED);
        if (first + 12 < nr) kit_rect(155 - k, 235 - k, 2 * k + 1, 1, MUTED);
    }
}
