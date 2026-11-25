# fm1-scales — Architecture

How the firmware and the factory are built, and why. Interfaces in detail are in `INTERFACES.md`, decisions in
`PLATFORM.md` (D-nnn), requirements in `REQUIREMENTS.md` (R-nnn), hazards in `HAZARDS.md` (HAZ-nn). Diagrams are
Mermaid; GitHub renders them.

## 1. Context

```mermaid
flowchart LR
    child([Child / learner]) -- keys, knobs, buttons --> fm1
    fm1 -- sound, screen, key lights --> child
    owner([Owner]) -- installs, play-tests, decides --> fm1
    subgraph fm1 [M-VAVE FM-1]
        fw[fm1-scales firmware]
    end
    kbd[MIDI keyboard] -- TRS MIDI IN --> fm1
    pc[Computer or tablet] <-- USB-MIDI: notes, installer, UBOOT key --> fm1
    pc -- USB audio --- fm1
    repo[(GitHub: ekhve/fm1-scales)] -- .fwsc package --> pc
    factory[Factory: agents + gates + CI] -- PRs --> repo
    owner -- stories, decisions, approvals --> repo
```

The firmware is the only thing on the FM-1 we change. Everything else on the device (the ROM bootloader, the update
loader we ship unchanged from felucca-bsp, the stock firmware's flash areas) is treated as outside the boundary and
is never written (D-006, D-011, D-022).

## 2. Layers

```mermaid
flowchart TB
    subgraph apps [apps/ — one per firmware]
        hello[hello]
        future[the learning app, to come]
    end
    subgraph api [fm1kit.h — the fixed API, FM1KIT_API 15]
    end
    subgraph portable [fm1kit portable core — runs on the FM-1 and on a PC]
        core[core.c: events, lights, held state]
        gfx[gfx.c: framebuffer, shapes, text]
        synth[synth.c: four tracks, eight voices, Basic tones, drums, volume]
        engines[engines.c: Felucca-Engines host, four parts]
        music[music.c: names, keys, scales, run orders, tempo, chords, key analysis]
        store[store.c: record format, sector choice]
        midi[midi.c: MIDI parser]
        player[player.c: sequence player]
        image[image.c: images from app assets]
        seq[seq.c: song, settings, recorder, song player]
    end
    subgraph backends [backends — kit_internal.h]
        device[device.c: the FM-1]
        host[host.c: PC simulator]
    end
    subgraph third [third_party, pinned]
        bsp[felucca-bsp: chip, board, loader, tools — MPL-2.0]
        fe[Felucca-Engines: 6 synth engines — MIT]
        emu[fm1-emu: the FM-1 emulator — MIT, factory patch]
    end
    apps --> api --> portable --> backends
    engines --> fe
    device --> bsp
    host -. same scripts as .- emu
```

**Rules that keep it this way.** Apps include `fm1kit.h` only (R-018, `safety.py`). The portable core never touches
hardware; a backend implements `kit_internal.h` (`INTERFACES.md` IF-02). The same portable code is compiled into
both backends, which is why the device's screen must equal the host's pixel for pixel (R-002).

## 3. Runtime on the FM-1

Three contexts, by priority:

| Context | Rate | Does | Must not |
|---|---|---|---|
| TIMER5 interrupt (priority 4) | 10 kHz | Key and encoder scan, LED multiplexing, pill readings (velocity), ms clock, USB poll every 5th tick | Call app code |
| ALNK0 audio interrupt (priority 3) | every 128 frames (2.9 ms) | `kit__synth_render` into the free half of the I2S buffer: Basic voices or the selected engine | Take longer than 2.9 ms (R-015 keeps it under 80 %) |
| Main loop | as fast as it runs | Watchdog, update service and UBOOT checks, OCT-combo timer, MIDI in (TRS ring, USB ring), the player's next note, the song player's notes, the store's paced write, key presses with velocity, app events, `app_frame` every ≥ 10 ms, LCD flush of dirty rows | Block for long: a flash erase stops interrupts for tens of ms, so the store zeroes the audio buffer first |

### A key press

```mermaid
sequenceDiagram
    participant K as Key (carbon pill)
    participant T as TIMER5 scan
    participant M as Main loop
    participant A as App
    participant S as Synth (audio IRQ)
    participant U as USB-MIDI
    K->>T: contact closes, pill pressed
    T->>T: debounce, then read the pill with the row driven (every 2.2 ms)
    M->>M: new press, wait for 2 readings (≤ 12 ms), the strongest gives the velocity
    M->>A: KIT_EV_KEY_DOWN (key, velocity)
    A->>S: kit_note_on(note, velocity)
    A->>U: kit_midi_note_on(1, note, velocity)
    A->>M: draw (app_frame), lights
    S-->>K: sound in the next buffer half (≤ 2.9 ms)
    M-->>M: LCD flush of the rows that changed
```

### Modes of the FM-1 with the firmware

```mermaid
stateDiagram-v2
    [*] --> Boot
    Boot --> UpdateMode: 2 boots in a row never reached 2 s healthy (boot guard)
    Boot --> Running: hardware up, app_init
    Running --> UpdateMode: installer's upgrade command (staged loader, commit)
    Running --> UpdateMode: UBOOT soft key over USB-MIDI (R-031)
    Running --> UpdateMode: OCT- + OCT+ held 5 s (R-012)
    Running --> Fault: CPU fault (guard)
    Fault --> Boot: red screen 1.5 s, reboot
    Running --> Saving: kit_store_save
    Saving --> Running: record written (sound silent meanwhile)
    UpdateMode --> [*]: installer writes a firmware
```

`Running` is where an app's own states live. Each app documents its states here when it has them; the `While`
clauses of its requirements name them (playbook §9).

### The companion's states (apps/companion)

What the screen and the keys are doing. The song (four tracks, the loop) runs underneath every state and is never
changed by moving between them.

```mermaid
stateDiagram-v2
    [*] --> Producer
    state Producer {
        [*] --> Manual
        state "CHORDS · MANUAL (start)" as Manual
        state "CHORDS · PAD" as Pad
        state "BASS / MELODY / DRUMS (keyboards)" as Keys
        state "Song page" as Song
        Manual --> Pad: SCL
        Pad --> Manual: SCL
        Manual --> Keys: ALGORITHM
        Pad --> Keys: ALGORITHM
        Keys --> Manual: ALGORITHM (to CHORDS)
        Manual --> Song: GLO tapped
        Pad --> Song: GLO tapped
        Keys --> Song: GLO tapped
        Song --> Manual: GLO tapped (back to the page it came from)
    }
    state "Help (overlay)" as Help
    state "Mode picker" as Picker
    state "Confirm" as Confirm
    state "Guided (placeholder)" as Guided
    state "Game (placeholder)" as Game
    Producer --> Help: GLO held
    Help --> Producer: GLO released
    Producer --> Picker: HOME held
    Guided --> Picker: HOME held
    Game --> Picker: HOME held
    Picker --> Confirm: HOME on another mode
    Picker --> Producer: HOME on the current mode, or any other button
    Confirm --> Guided: HOME (Guided chosen)
    Confirm --> Game: HOME (Game chosen)
    Confirm --> Producer: HOME (Producer chosen)
    Confirm --> Picker: any other button
```

Orthogonal to these, the transport: **stopped**, **playing**, and **armed** (REC: the current track records while it
plays). The song's key is read again when the song stops (D-034), so pages never move under the hands.

### Producer sound panels

On a pitched Producer track, Sound Edit and Sound FX are alternate panels over the current track page.
The track's playing mode and transport stay active while either panel is open. Held-key releases still reach the
instrument. The panel closes when the player switches tracks or enters a view that takes over the controls.

```mermaid
stateDiagram-v2
    state "Track page" as Track
    state "Sound Edit" as Edit
    state "Sound FX" as FX
    Track --> Edit: EDIT, ENV, or LFO
    Track --> FX: FX
    Edit --> FX: FX
    FX --> Edit: EDIT, ENV, or LFO
    Edit --> Track: HOME
    FX --> Track: HOME
```

| Panel | Page order and controls |
| --- | --- |
| Sound Edit | EDIT steps through OSC, FILTER, F.ENV, A.ENV, VOICE, LFO and MOD. SELECT steps in either direction. Hold EDIT to return to OSC. ENV toggles the two envelope pages and LFO opens LFO directly. |
| Sound FX | FX steps through PERFORM, DELAY, REVERB and DIRT. Hold FX to return to PERFORM. SELECT chooses the X/Y pair on PERFORM and changes pages elsewhere. |

Plain EDIT enters the panel when the player releases it. When EDIT is used as a chord for recording or sequencing,
the relevant sequencer interaction takes precedence. On Sound Edit FILTER, the meter reads the selected FMini or
Juno-106 track before the shared effects. Both Juno envelope pages access its shared ADSR. In Sound FX, DELAY and
REVERB expose shared effects plus the current track's send. DIRT acts on the current track.

### An interaction: one finger plays a chord while recording

```mermaid
sequenceDiagram
    participant P as Player
    participant A as companion (pad.c, app.c)
    participant T as fm1kit theory (music.c)
    participant Q as sequencer (seq.c)
    participant S as synth (synth.c)
    P->>A: F5 NEXT down (velocity)
    A->>A: pad_next_degree: the progression's next step (ii, colour 9)
    A->>T: kit_triads, kit_colour_chord (ii, minor, 9, voicing)
    T-->>A: Dm9: D3 F3 A3 C4 E4
    loop each note (block, strum 30 ms apart, or arp at eighths)
        A->>Q: kit_seq_note_on(CHORDS, note, vel)
        Q->>S: kit_track_note_on (sounds now)
        Q-->>Q: armed and playing: remember the start tick
    end
    A->>A: lights: ii full, V breathing, screen shows Dm9 and next G13
    P->>A: F5 up
    A->>Q: kit_seq_note_off for each note
    Q-->>Q: adds each note with its start, length and velocity, as played
```

## 4. Flash

| Range | Owner | We write? |
|---|---|---|
| 0x00000–0x03FFF | Boot area: header, SPL, config | Never |
| 0x04000–0x92FFF | App area: the package's app (ours ~170 KB) | Only by an install |
| 0x93000–0x96FFF | XDATA0: **fm1kit store**, four candidate sectors | Yes: blank or our own sectors only (R-020, D-022) |
| 0x97000–0xDFFFF | Felucca-family data (projects, samples) | Never |
| 0xE0000–0xE4FFF | Update staging (the staged loader) | Only the update entry (ota.c) |
| 0xE5000–0xE6FFF | Felucca-family autosave, drum records | Never |
| 0xE7000–0xE9FFF | Stock SDK system: VM, BTIF | Never |
| 0xEA000–0xFBFFF | Stock firmware's USR area | Never |
| 0xFC000–0xFEFFF | Felucca-family settings | Never |
| 0xFF000–0xFFFFF | key_mac | Never |

The flash driver itself refuses writes outside our two regions (`FM1_FLASH_WRITE_REGIONS` in `device.c`, R-019).

## 5. Build

```mermaid
flowchart LR
    gen[tools/gen_font.py, gen_tables.py, Felucca-Engines gen_engine_tables.py] --> hdr[build/gen/*.h]
    hdr --> hostb[cc: portable + host.c + app] --> apphost[app-host]
    hdr --> devb[JieLi clang pi32v2: portable + device.c + app + BSP crt0/vec/isr] --> elf[app.elf] --> bin[app.bin + size checks]
    bsp[felucca-bsp loader/] --> ldr[loader build, LZ4 wrap] --> ota[ota.bin, hash pinned]
    bin --> pkg[felucca-bsp fm1pkg.py + 3 SDK files, hash checked] --> fwsc[app.fwsc]
    ota --> pkg
```

`tools/build.py` stops on: an image over the app area, RAM or pool over their regions, a loader whose hash differs
from `factory/loader.sha256`, SDK files that are not the tested ones.

## 6. The factory

```mermaid
flowchart TB
    story[Story issue: user story + acceptance criteria] --> reqs[REQUIREMENTS.md + feature scenarios]
    reqs --> code[Code + executable scenarios]
    code --> check{factory/check.sh}
    check -->|reqlint, trace| docs[(wording, links)]
    check -->|build, unit, oracle| kit[(platform)]
    check -->|safety, usb_midi, cpu| hw[(updatable, MIDI, CPU budget)]
    check -->|scenarios| both[(host simulator AND emulated firmware)]
    check --> ci[CI on every push]
    ci --> pr[PR]
    pr --> owner{Owner}
    owner -->|needs-you: decisions, screens, hardware, play tests| story
```

| Gate | Checks | Fails on |
|---|---|---|
| reqlint | Requirement wording (playbook §6) | A rule violation without a waiver |
| trace | Requirements ↔ scenarios ↔ checks ↔ decisions ↔ hazards; writes `TRACEABILITY.md` | A broken link, an uncited scenario, an unrun automated scenario, a stale matrix |
| build | Both builds, sizes, the loader pin, SDK hashes | Any of them |
| unit | Portable code on the host (`factory/unit/`) | A failed check |
| oracle | music.c vs Sketchpad, 840 answers | One difference |
| safety | Over emulated USB: handshake, UBOOT key, OCT combo both ways; loader pin; apps only use fm1kit.h | Any of them |
| usb_midi | Notes out with velocity ≤ 20 ms, notes in reach the app | Any of them |
| cpu | Every sound, 8 voices, cycle-modelled: interrupts < 80 % (D-091) | One sound over |
| scenarios | Pitch, silence, level ratios, lights, screens; host vs device pixel parity; reboot persistence | One check |
