/* SPDX-License-Identifier: GPL-3.0-only */
/* The Game mode's games (D-062): Unicorn Jump (ST-51), Hiru Damatxo (ST-53), Aldapeko Sagarra (ST-54), the Night Train
 * (ST-55, D-064). SELECT turns
 * from one to the next; each starts at its own title, its sounds set as it opens and put back as it closes. */
#define G_N 4
static int g_cur;
static int game_on(void) { return U.on || D.on || A.on || T.on; }
static void game_leave(void) { if (U.on) u_leave(); if (D.on) d_leave(); if (A.on) a_leave(); if (T.on) t_leave(); }
static int game_event(const kit_event_t *e)
{
    if (e->type == KIT_EV_ENC && e->id == KIT_ENC_SELECT && !(g_cur == 0 && U.teach)) {   /* the next game (not while teaching) */
        game_leave();
        g_cur = (g_cur + (e->delta > 0 ? 1 : G_N - 1)) % G_N;
        return 1;
    }
    return g_cur == 0 ? u_event(e) : g_cur == 1 ? d_event(e) : g_cur == 2 ? a_event(e) : t_event(e);
}
static void game_frame(uint32_t now)
{
    if (g_cur == 0) u_frame(now); else if (g_cur == 1) d_frame(now); else if (g_cur == 2) a_frame(now); else t_frame(now);
}
