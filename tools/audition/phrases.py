"""The audition phrases, one per kind of sound, at 96 BPM (a bar 2.5 s, a slot 5 bars = 12.5 s, the last bar a tail).
Notes as (start_beat, length_beats, note, velocity). Used for every firmware, ours included (via a C table)."""
BPM = 96
BEAT_MS = 60000 / BPM
SLOT_BEATS = 20
AM7, FM7 = (57, 60, 64, 67), (53, 57, 60, 64)

BASSLINE = [(0, 1.25, 45, 100), (1.5, .5, 45, 90), (2, 1, 48, 100), (3, .5, 43, 90), (3.5, .5, 45, 100)]
MELODY = [(0, .5, 76, 100), (.5, .5, 74, 100), (1, 1, 72, 100), (2, .5, 69, 100), (2.5, .5, 72, 100), (3, 1, 74, 100)]


def shift(notes, beats):
    return [(s + beats, l, n, v) for s, l, n, v in notes]


def std(poly=True):
    """keys, pads, brass, plucks: chords Am7 and Fmaj7 (broken into eighths when the synth plays one note), the bass
    line, the melody hook"""
    out = []
    for bar, ch in ((0, AM7), (1, FM7)):
        if poly:
            out += [(bar * 4, 3.85, n, 90) for n in ch]
        else:
            out += [(bar * 4 + i * .5, .45, ch[i % 4] + (12 if i >= 4 else 0), 95) for i in range(8)]
    return out + shift(BASSLINE, 8) + shift(MELODY, 12)


def bass():
    """a bass part in the bass register: a low A groove with octave jumps and ghost notes, then a run with a slide
    (overlapping notes: a mono synth glides), twice through, the second time ending on a held low A"""
    g = [(0, .75, 33, 115), (.75, .25, 33, 70), (1, .5, 45, 100), (1.5, .5, 33, 90), (2, .5, 36, 105), (2.5, .25, 36, 70),
         (2.75, .5, 38, 100), (3.25, .3, 40, 95), (3.5, .6, 43, 100)]                       # the last overlaps: a slide
    r = [(4, .75, 41, 115), (4.75, .25, 41, 70), (5, .5, 53, 100), (5.5, .5, 41, 90), (6, .5, 43, 105), (6.5, .5, 45, 100),
         (7, .55, 40, 100), (7.5, .5, 43, 95)]
    end = [(8, .75, 33, 115), (8.75, .25, 33, 70), (9, .5, 45, 100), (9.5, .5, 33, 90), (10, .5, 36, 105), (10.5, .5, 38, 100),
           (11, .55, 40, 100), (11.5, .6, 43, 100), (12, 3.5, 33, 110)]
    return g + r + end


def lead(poly=True):
    """a lead: one bar of the chord underneath (when it can), then a slow legato line high up with long held notes
    (overlapping: glide, vibrato and the filter can be heard), then the hook"""
    out = [(0, 3.85, n, 85) for n in AM7] if poly else [(i * .5, .45, AM7[i % 4] + 12, 90) for i in range(8)]
    line = [(4, 1.6, 76, 105), (5.5, .6, 79, 100), (6, 1.6, 81, 105), (7.5, .6, 79, 100), (8, 1.1, 76, 100),
            (9, .6, 74, 100), (9.5, 2.6, 72, 105), (12, .6, 69, 100), (12.5, .6, 72, 100), (13, .6, 74, 100),
            (13.5, 2.5, 76, 110)]
    return out + line


def fx():
    """effects, drones, noises: one held note, then a short low one"""
    return [(0, 8, 60, 110), (10, 4, 48, 110)]


def drums(kick, snare, hat):
    out = []
    for b in range(4):
        o = b * 4
        out += [(o, .25, kick, 110), (o + 2.5, .25, kick, 95), (o + 1, .25, snare, 105), (o + 3, .25, snare, 105)]
        if b & 1:
            out.append((o + 2.75, .25, kick, 90))
        out += [(o + s * .5, .25, hat, 60 if s & 1 else 85) for s in range(8)]
    return out


KINDS = {"std": std, "bass": bass, "lead": lead, "fx": fx}


def notes(kind, poly=True):
    f = KINDS[kind]
    return f(poly) if kind in ("std", "lead") else f()


BASS_W = ("BASS", "SUB", "808", "REESE", "RUBBER", "WOBBLE", "LOW END")
LEAD_W = ("LEAD", "SOLO", "G-FUNK", "HOOK", "WHISTLE", "FLUTE", "SAX", "VIOLIN", "OBOE", "CLAR", "TRUMPET", "WHIN",
          "SIREN", "MOSQUITO", "MELLOW MAN", "HONEY", "NUMAN", "EMERSON", "HERO", "SYNC", "TIN WHISTLE", "RECORDER",
          "HARMONICA", "TRP")
FX_W = ("DRONE", "UFO", "THUNDER", "WIND", "SIREN FX", "TELEPHONE", "POWER UP", "LASER", "HELICOPTER", "ROBOT", "SEA",
        "JET", "HORROR", "RISER", "IMPACT", "RAYGUN", "STATIC", "SPACESHIP", "BEEP", "GLITCH", "TAPE STOP", "SUB DROP",
        "OCEAN", "VOID", "TIDAL", "MACHINE HUM", "STARFIELD", "MOTHER", "FOG", "RAIN", "ARCADE", "METAL", "TYPHOON",
        "MOTORCYCLE", "SWEEP", "GONG", "MORSE", "DOG", "NOISE", "SFX", "CARPET", "STAR DUST", "PULSAR", "GHOST SEQ",
        "SINGING BOWL", "CALM", "NORTHERN", "ALIEN", "IRON", "SPACE")


def kind_of(name):
    u = name.upper()
    if any(w in u for w in FX_W) and "PAD" not in u:
        return "fx"
    if any(w in u for w in BASS_W):
        return "bass"
    if any(w in u for w in LEAD_W):
        return "lead"
    return "std"
