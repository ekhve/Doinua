/* SPDX-License-Identifier: GPL-3.0-only */
/* Explanations (ST-43, D-052), included by app.c after card() and before app_frame. Turning K1..K4 shows, for under two
 * seconds, the card's name and new value and one line on what that value does; GLO, SYSTEM, K3 EXPLAIN switches them
 * off. The cards record what they show as they are drawn, so the explanation is always of what is on the screen. */

/* what each card's value does: the value's own line first, else the card's */
typedef struct { const char *label, *value, *text; } exp_line_t;
static const exp_line_t EXP_LINES[] = {
    { "HELP", "Free", "No help: play anything, nothing lights up but what you play." },
    { "HELP", "Key", "The key's scale lights dim, its home note breathes." },
    { "HELP", "Key+root", "While the song plays, the root of the chord lights full: the safe note." },
    { "HELP", "Chord", "The chord now lights full, the scale dim. A note you play: green in the chord, amber outside the scale." },
    { "HELP", "Whites", "The white keys play the scale, so nothing sounds wrong. The black keys rest." },
    { "HELP", 0, "How much the lights help you." },
    { "OUTSIDE", "play", "A note outside the scale plays like any other." },
    { "OUTSIDE", "soft", "A note outside the scale plays quietly." },
    { "OUTSIDE", "mute", "A note outside the scale makes no sound." },
    { "PLUS", "off", "Just the note you play." },
    { "PLUS", "3rd", "A second note with each one, a third above in the key." },
    { "PLUS", "6th", "A second note with each one, a sixth above in the key." },
    { "PLUS", "octave", "The same note an octave up, with each one." },
    { "PLUS", "echo", "Each note repeats twice, softer, a dotted eighth apart." },
    { "STYLE", 0, "The music the ideas and patterns come from." },
    { "IDEA", 0, "Which idea of the style. SELECT plays another version of it: the same rhythm, other notes." },
    { "SPEED", "100 %", "The song at its own tempo." },
    { "SPEED", 0, "Practise slower: the whole song plays at this share of its tempo. What you record lands in its place." },
    { "HEAR", "listen/play", "It plays the idea once round, then it is your turn." },
    { "HEAR", "on", "The idea plays every time round." },
    { "HEAR", "off", "Only what you play sounds." },
    { "LINE", 0, "Which bassline of the style." },
    { "PATTERN", 0, "Which pattern your held pad plays." },
    { "PHRASE", 0, "Which phrase your held pad plays: a call, then its answer." },
    { "FEEL", "Block", "The chord's notes together." },
    { "FEEL", "Strum", "The chord's notes one after the other, like a guitar." },
    { "FEEL", "Arp", "The chord's notes one at a time, an arpeggio." },
    { "FEEL", "straight", "Even eighths." },
    { "FEEL", "swing", "Swung: the off-beats a little late." },
    { "FEEL", "Straight", "On the grid." },
    { "FEEL", "Swing", "Swing: the off-sixteenths a little late." },
    { "FEEL", "MPC", "The MPC's heavy swing: almost triplets." },
    { "FEEL", "Dilla", "The kick swung hard, the snare late, the hats loose: it bounces." },
    { "FEEL", "Laid", "The snare and hats behind the beat: relaxed." },
    { "FEEL", "Pushed", "The hats ahead of the beat: urgent." },
    { "FEEL", "Dembow", "The dembow's snares lean a little late." },
    { "FEEL", "Shatta", "A strong bounce, the kick early." },
    { "FEEL", "Drunk", "Everything loose, on purpose." },
    { "FEEL", 0, "How the notes sit against the beat." },
    { "FOLLOW", 0, "On: with the song playing, it follows your recorded chords by itself." },
    { "GRID", "as played", "It plays back exactly as you played." },
    { "GRID", 0, "On playback each note moves to the nearest place on this grid. What you played is kept." },
    { "CLICK", "Off", "No click." },
    { "CLICK", "off", "No click." },
    { "CLICK", "Rec", "A click on each beat while you record." },
    { "CLICK", "rec", "A click on each beat while you record." },
    { "CLICK", "On", "A click on each beat whenever the song plays." },
    { "CLICK", "on", "A click on each beat whenever the song plays." },
    { "COUNT-IN", "Off", "REC records at once." },
    { "COUNT-IN", "off", "REC records at once." },
    { "COUNT-IN", 0, "REC while stopped counts 4, 3, 2, 1 first, then records." },
    { "LEVEL", 0, "How loud the click is." },
    { "KEY", 0, "The song's key. AUTO works it out from what you play." },
    { "TEMPO", 0, "Beats a minute, 40 to 240." },
    { "LENGTH", 0, "How many bars the loop has." },
    { "TIME", 0, "Four beats a bar." },
    { "TRACK", 0, "Which track's MIDI channel K2 sets." },
    { "CHANNEL", 0, "The MIDI channel this track listens and sends on." },
    { "IN", "Channels", "Each track plays the notes of its own channel." },
    { "IN", "This track", "Every channel plays the track on screen." },
    { "IN", "Follow", "Each channel its track, and the screen goes to the track played." },
    { "WAKE", 0, "Whether a MIDI note wakes the screen too." },
    { "SCREEN OFF", "Never", "The screen stays on." },
    { "SCREEN OFF", 0, "The screen goes dark after this long with nothing played. The lights stay." },
    { "TUNE", 0, "Tune the FM-1 to other instruments, in cents of a semitone." },
    { "EXPLAIN", "On", "A knob says what its value does, like this." },
    { "EXPLAIN", "Off", "No explanations." },
    { "VOICE", 0, "How the chord's notes are stacked: root position, an inversion, open or wide." },
    { "PROG", 0, "Which chord progression the pads follow." },
    { "ROOT", 0, "The chord's root, a step of the key." },
    { "TYPE", 0, "Major, minor, sus, diminished or augmented." },
    { "COLOR", 0, "Extra notes that colour the chord: a 7th, a 9th ..." },
    { "PLAY", 0, "What the lights show while you play." },
    /* the drums' cards (D-053): "D:" in front, so TUNE here is not the FM-1's tuning */
    { "D:REPEAT", "off", "The key plays once." },
    { "D:REPEAT", 0, "Held, the key plays again and again at this note value of the song's tempo." },
    { "D:TUNE", 0, "This key's sound, higher or lower in semitones. A kick or a tom names the note it sounds." },
    { "D:DECAY", 0, "How long this key's sound rings, against the kit's." },
    { "FX", "off", "The drums as they are." },
    { "FX", "Punch", "A compressor: the hits louder and tighter." },
    { "FX", "MPC", "Squashed and soft-clipped, as an MPC: they hit hard." },
    { "FX", "Lofi", "Fewer bits, a narrow band and crackle: dusty." },
    { "FX", "Tape", "Warm saturation and a gentle top: like tape." },
    { "BEAT", "main", "The style's beat." },
    { "BEAT", "busy", "A busier beat: more hats, more ghosts." },
    { "BEAT", "fill", "A fill: the snare and the toms in the second half." },
    { "BEAT", "break", "A break: the hats and one kick." },
    { "BEAT", "drop", "The drop: everything but the kick." },
    { "FILLS", "off", "The main pad plays its beat, bar after bar." },
    { "FILLS", 0, "The main pad plays a fill in the last bar of every 4 or 8." },
    { "GHOSTS", "off", "No ghost notes: the quiet snares between the beats." },
    { "GHOSTS", "soft", "The style's own ghost notes." },
    { "GHOSTS", "more", "More ghost notes, as a drummer adds them." },
};
#define EXP_NLINES ((int)(sizeof EXP_LINES / sizeof EXP_LINES[0]))

static char exp_lab[4][16], exp_val[4][16];     /* what each card shows */
static uint8_t exp_grey[4];
static int exp_pending;                         /* 1..4: a knob turned, explain its card once drawn */
static uint32_t exp_until;
static char exp_title[34];
static const char *exp_text;

static void exp_card(int i, const char *label, const char *value, int grey)
{
    int k;
    if (i < 0 || i > 3) return;
    for (k = 0; k < 15 && label[k]; k++) exp_lab[i][k] = label[k];
    exp_lab[i][k] = 0;
    for (k = 0; k < 15 && value[k]; k++) exp_val[i][k] = value[k];
    exp_val[i][k] = 0;
    exp_grey[i] = (uint8_t)grey;
}

static int exp_eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

static const char *exp_find(const char *label, const char *value)
{
    int i;
    for (i = 0; i < EXP_NLINES; i++)
        if (exp_eq(EXP_LINES[i].label, label) && EXP_LINES[i].value && exp_eq(EXP_LINES[i].value, value)) return EXP_LINES[i].text;
    for (i = 0; i < EXP_NLINES; i++)
        if (exp_eq(EXP_LINES[i].label, label) && !EXP_LINES[i].value) return EXP_LINES[i].text;
    return 0;
}

static void exp_say(const char *title, const char *text)   /* a hint in the same box (R-243) */
{
    exp_title[0] = 0;
    cat(exp_title, title);
    exp_text = text;
    exp_pending = 0;
    exp_until = kit_ms() + 2200;
    if (!exp_until) exp_until = 1;
    dirty = 1;
}

static void exp_knob(int id)                    /* a knob turned: explain its card when the page is drawn */
{
    if (id >= KIT_ENC_KNOB1 && id <= KIT_ENC_KNOB4) exp_pending = id - KIT_ENC_KNOB1 + 1;
}

static void exp_draw(uint32_t now)              /* last, over the page */
{
    char line[48];
    const char *p;
    int y, n;
    if (exp_pending) {
        int i = exp_pending - 1;
        exp_pending = 0;
        exp_text = 0;
        if (set.explain && !exp_grey[i] && exp_lab[i][0]) {
            if (is_drums() && !song_page && !rl_open) {   /* the drums' own lines first */
                char dl[20];
                cat(cat(dl, "D:"), exp_lab[i]);
                exp_text = exp_find(dl, exp_val[i]);
            }
            if (!exp_text) exp_text = exp_find(exp_lab[i], exp_val[i]);
        }
        if (exp_text) {
            char *d = cat(exp_title, exp_lab[i]);
            cat(cat(d, "  "), exp_val[i]);
            exp_until = now + 1800;             /* under two seconds (R-151) */
            if (!exp_until) exp_until = 1;
        } else {
            exp_until = 0;
        }
    }
    if (!exp_until) return;
    if (exp_text && !exp_text[0]) {             /* a one-word hint (D-072): the word, small, in the middle */
        kit_round_rect(76, 94, 88, 38, 8, CARD2);
        kit_text(120, 101, exp_title, KIT_FONT_MEDIUM, KIT_ALIGN_CENTER, FG);
        return;
    }
    kit_round_rect(6, 66, 228, 74, 8, CARD2);
    kit_text(14, 71, exp_title, KIT_FONT_SMALL, KIT_ALIGN_LEFT, FG);
    p = exp_text;
    y = 94;
    while (*p && y <= 124) {                    /* the words, wrapped to the box */
        int last = 0;
        n = 0;
        while (p[n] && n < 47) {
            line[n] = p[n];
            line[n + 1] = 0;
            if (kit_text_width(line, KIT_FONT_XSMALL) > 212) break;
            if (p[n] == ' ') last = n;
            n++;
        }
        if (p[n] && last) n = last;             /* break at the last space that fits */
        line[n] = 0;
        kit_text(14, y, line, KIT_FONT_XSMALL, KIT_ALIGN_LEFT, MUTED);
        p += n;
        while (*p == ' ') p++;
        y += 15;
    }
}

static void exp_frame(uint32_t now)
{
    if (exp_until && (int32_t)(now - exp_until) >= 0) { exp_until = 0; dirty = 1; }
}
