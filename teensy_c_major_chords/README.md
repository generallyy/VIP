# Teensy 4.0 C major chord controller

A separate copy of `teensy_midi_controller` for seven chord buttons and one
tag potentiometer. Sends native USB MIDI on channel 1, with USB serial for
debugging. No MIDI bridge is needed.

"The seven chords in C major" means the diatonic chords: **C, Dm, Em, F, G,
Am, Bdim**. Their qualities follow the key; not every chord is major. Normal
mode plays a three-note triad. The 7 and 9 tags stack additional notes from
the C major scale; 9 mode includes the seventh.

## Wiring

Disconnect USB before rewiring. Buttons connect their signal pin to GND when
pressed and use `INPUT_PULLUP`; no external pull-up resistors are required.
All controls share Teensy GND.

| Pin | Normal | 7 tag | 9 tag |
| --- | --- | --- | --- |
| 2 | C | Cmaj7 | Cmaj9 |
| 3 | Dm | Dm7 | Dm9 |
| 4 | Em | Em7 | Em7(b9) |
| 5 | F | Fmaj7 | Fmaj9 |
| 6 | G | G7 | G9 |
| 7 | Am | Am7 | Am9 |
| 8 | Bdim | Bm7b5 | Bm7b5(b9) |

Connect the potentiometer **wiper to A1 (digital pin 15)** and its outside
terminals to **3.3V and GND**. A 10k linear pot is suitable. Teensy 4.0 inputs
are not 5V tolerant ([PJRC specifications](https://www.pjrc.com/store/teensy40.html)).
Swap the outside wires if the knob direction is reversed.

This copy uses A1 as requested. In the previous project A1 was optional
reverb, A2 was optional voicing, and A3 was pan. Only A1 is read in this
project; pan and reverb controls are absent.

The selector keeps the previous project's thresholds and order:

| A1 reading (10-bit) | Tag | Notes per chord |
| --- | --- | --- |
| 0-358 (low) | 9 | 5 |
| 359-664 (middle) | Normal | 3 |
| 665-1023 (high) | 7 | 4 |

The initial knob position is used at startup. Changes settle for 40ms, with
ADC averaging and 5ms sampling. Buttons are debounced for 25ms. Hold to play,
release to stop. Multiple buttons can sound together; shared pitches remain
on until their last owner releases them. Turning the knob changes held
chords without retriggering common notes. The highest note of a newly played
chord uses velocity 120, other notes 100. The built-in LED follows pin 2.

## Exact MIDI notes

MIDI note numbers are definitive; DAWs may display different octave names.
Root C is note 60. Each row's normal chord is its first three notes; the 7
tag uses the first four; the 9 tag uses all five.

| Button | Full five-note bank |
| --- | --- |
| C | 60, 64, 67, 71, 74 |
| Dm | 62, 65, 69, 72, 76 |
| Em | 64, 67, 71, 74, 77 |
| F | 65, 69, 72, 76, 79 |
| G | 67, 71, 74, 77, 81 |
| Am | 69, 72, 76, 79, 83 |
| Bdim | 71, 74, 77, 81, 84 |

E and B have flat ninths so every pitch stays in C major. B's seventh is
half-diminished, not a fully diminished seventh. Edit `include/ChordMap.h`
to change pitches, button assignments, or MIDI channels.

## Build and upload

Open **this folder** in VS Code with PlatformIO, then use **Project Tasks >
teensy40 > General > Build** or **Upload**. The project retains Teensy platform
6.0.0, C++17, and `USB_MIDI_SERIAL` from the previous controller.

From this folder in PlatformIO Core CLI:

```sh
pio run -e teensy40
pio run -e teensy40 --target upload
```

Use a USB data cable. If prompted, press Teensy's physical Program button.
Uploading replaces the firmware currently on that board.

Select Teensy's MIDI input (usually **Teensy MIDI**) on a software-instrument
track in your DAW, enable channel 1 and monitoring, and hold a button.
In Studio One, add it as a New Keyboard under External Devices. In
GarageBand, select a Software Instrument track; reopen the app if it does
not detect a newly connected controller. The DAW produces the audio.
The transport uses [PJRC's USB MIDI API](https://www.pjrc.com/teensy/td_midi.html).

## Debug and verify

```sh
pio device monitor --baud 115200
```

Serial prints debounced button changes and settled tag changes, for example
`TAG pin=15 raw=500 mode=NORMAL`. Send **s** for all button states and the
current tag; send **p** for panic (sustain off, all notes off, all sound off
on all channels). Release and repress held buttons after panic. Close the
monitor before uploading. The loop does not wait for a serial monitor.

If a button does not respond, send **s** while holding it: LOW means pressed.
Check switch contacts and common ground if it stays HIGH. If tags jump,
check the A1 wiper and 3.3V/GND connections. If notes stick after unplugging,
use your DAW's panic command; disconnected hardware cannot send Note Offs.

Run desktop tests on Windows with Visual Studio C++ Build Tools:

```powershell
powershell -ExecutionPolicy Bypass -File .\test\run_tests.ps1
```

Or with a C++17 compiler on macOS/Linux:

```sh
mkdir -p .pio/core-tests
c++ -std=c++17 -Wall -Wextra -pedantic -Ilib/ControllerCore/src -Iinclude test/test_core.cpp -o .pio/core-tests/core_tests
./.pio/core-tests/core_tests
```

Tests exercise all seven actual button mappings in all three modes, changes
with all seven held, shared-note releases, panic, startup, disconnect reset,
debounce, clock rollover, and selector thresholds/settling. The copied core
also retains its CC/program and analog-filter tests. Compilation and desktop
tests cannot verify wiring or MIDI reception on physical hardware.

| File | Purpose |
| --- | --- |
| `include/ChordMap.h` | Seven buttons and three chord banks per button |
| `include/ControllerConfig.h` | A1 selector, timing, LED, pin validation |
| `src/main.cpp` | GPIO/ADC scan, USB MIDI, serial diagnostics |
| `lib/ControllerCore/src/` | Copied input/core behavior and note ownership |
| `test/` | Desktop behavior tests and Windows runner |
