# Teensy 4.0 MIDI controller

A standalone PlatformIO project for the Teensy 4.0. The default configuration
preserves the current combined controller: two held chord buttons and a pan
knob. It also includes the earlier reverb and chord-voicing features as optional
controls, plus mappings for additional chord, CC, program-change, and panic
buttons.

The Teensy sends USB MIDI directly to the computer. Select its MIDI input in
Studio One, GarageBand, or another DAW; no Python/PowerShell MIDI bridge or
virtual MIDI cable is needed. USB serial remains available for commands.

## Wiring

Disconnect power before rewiring. **Use Teensy's 3.3V pin for every
potentiometer, replacing the Uno's 5V connection. Teensy 4.0 signal pins are
not 5V tolerant.** Use the main micro-USB device port and a USB data cable.

| Control | Connection | Default behavior |
| --- | --- | --- |
| F chord button | Pin 2 to GND when pressed | Fmaj7: 65, 69, 72, 76 |
| C chord button | Pin 4 to GND when pressed | Cmaj7: 60, 64, 67, 71 |
| Pan potentiometer | Wiper to A3 (digital pin 17); outside pins to 3.3V/GND | CC10: 0 left, 64 center, 127 right |
| Optional reverb potentiometer | Wiper to A1 (pin 15); outside pins to 3.3V/GND | CC91: 0-127 when enabled |
| Optional voicing potentiometer | Wiper to A2 (pin 16); outside pins to 3.3V/GND | Ninth / major / seventh when enabled |
| Built-in LED | Onboard pin 13; no wiring | On while the pin 2 button is held; off when released |
| Optional external LED | Pin 13 through a 1k ohm resistor to LED anode (+); cathode (-) to GND | Follows the built-in LED |

Buttons use `INPUT_PULLUP`; no external button resistor is needed. A 10k linear
potentiometer is suitable. All controls share Teensy GND. Exchange a knob's
outside wires if its direction is reversed. A3 is numbered pin **17**, not pin 3.

For a standard two-legged external LED, the longer leg is normally the anode
(+) and the shorter leg / flat side is the cathode (-). Connect
`13 -> 1k ohm resistor -> LED (+)` and `LED (-) -> GND`. Keep the resistor in
series with the LED. The light follows the debounced pin 2 button in both
firmware profiles, including when MIDI is not connected; the voicing selector
does not change the light.

The F chord button uses pin 2 and the C chord button uses pin 4. The earlier
Uno projects use different button pin assignments.
Both chords can sound together. Releasing a button stops only notes no longer
held by another button on that MIDI channel. The highest pitch uses velocity
120; other notes use 100. Default channel is 1.

## Build and upload with PlatformIO

1. Install VS Code and the PlatformIO IDE extension.
2. Open this **teensy_midi_controller** folder, containing `platformio.ini`.
3. Wire the controls as above and connect the Teensy by USB.
4. Use **Project Tasks > teensy40 > General > Build**, then **Upload**.
5. If the uploader asks, press Teensy's physical **Program** button once.

Equivalent commands in PlatformIO Core CLI, from this folder:

```sh
pio run -e teensy40
pio run -e teensy40 --target upload
```

The first build downloads the compiler and Teensy Arduino framework. The
project pins Teensy platform 6.0.0 and selects `USB_MIDI_SERIAL` so both MIDI
and a serial port are present. PlatformIO supplies the Teensy upload tools.
The Program button is for uploading, not a musical input.

## Connect the DAW

Connect and upload before opening the DAW. The MIDI input normally appears as
**Teensy MIDI**; the exact display name can vary by operating system.

- **Windows / Studio One:** add a New Keyboard under Options > External
  Devices, select Teensy's MIDI input as Receive From, and enable channel 1.
  Select that device on a software-instrument track and enable monitoring.
  Assign CC10 through Control Link if the instrument or mixer pan does not
  respond directly.
- **macOS / GarageBand:** select a Software Instrument track and play a chord
  button. Reopen GarageBand if it was running before the controller connected.
  Pan and reverb response depends on the patch's supported controller mappings.

The controller sends MIDI, not audio. Audio comes from the DAW's instrument.
Mixer automation and recorded MIDI controller data depend on the DAW and its
mapping. The older projects retain their serial bridges for Uno use; do not
run those bridges for this Teensy firmware.

Knobs send their current values when USB configures and whenever their filtered
MIDI values change. If the DAW starts listening later, turn the knob or open
PlatformIO Serial Monitor and send **r** to resend current knob values.

## Enable the earlier features

The default build reads only A3 (pin 17), so the one-knob setup works without
unconnected inputs producing random messages. Wire each extra knob before
enabling it.

To enable **both** reverb on A1 and the voicing selector on A2:

```sh
pio run -e teensy40_all_features --target upload
```

For only one feature, change the corresponding default macro from `0` to `1`
in `include/ControllerConfig.h` and upload the `teensy40` environment:

```cpp
#define MIDI_ENABLE_REVERB 1
#define MIDI_ENABLE_VOICING 1
```

The selector has the same 10-bit boundaries and 40ms settling time as the
original chord-only sketch. It changes voicings while buttons are held,
retaining shared notes without retriggering them.

| A2 reading | Pin 2: F chord | Pin 4: C chord |
| --- | --- | --- |
| 0-358 | 65, 69, 72, 76, 79 (major ninth) | 60, 64, 67, 71, 74 (major ninth) |
| 359-664 | 65, 69, 72, 77 (major + octave) | 60, 64, 67, 72 (major + octave) |
| 665-1023 | 65, 69, 72, 76 (major seventh) | 60, 64, 67, 71 (major seventh) |

CC91 is reverb send; an audible effect requires a compatible patch or a DAW
assignment. With the selector disabled, chords stay in major-seventh mode.

## Add buttons and knobs

Edit **include/ControllerConfig.h**, then rebuild and upload. Input counts and
runtime storage follow the configuration arrays automatically.

For example, add these entries inside `kButtons`, wiring each free pin to GND
through a separate button:

```cpp
chordButton(10, kCMajor, 2), // Another chord button, on MIDI channel 2.
ccButton(5, 64),            // Hold for sustain; release sends 0.
ccButton(6, 91, true),      // Toggle CC91 between 127 and 0.
programButton(7, 0),        // Send program 0 once per press.
panicButton(8),            // Stop sound and release sustain on all channels.
```

Use distinct MIDI controls when a knob and a button should act independently.
Channels are 1-16; controller, program, and data values are 0-127. Program 0
may be displayed as preset 1 by a DAW.

For a new chord, define a bank before `kButtons`, then add a button entry:

```cpp
constexpr ChordBank kDMinor = {{
    {{62, 65, 69, 72, 76}, 5}, // Minor ninth
    {{62, 65, 69, 74}, 4},     // Minor + octave
    {{62, 65, 69, 72}, 4},     // Minor seventh (default mode)
}};
// Inside kButtons:
chordButton(9, kDMinor),
```

Each chord supports up to eight distinct MIDI notes. The three bank entries
follow the global selector's ninth / major / seventh positions; their actual
notes are freely configurable. Repeat a voicing in all three entries for a
button that should ignore selector changes.

For a new analog control, add an enabled entry to `kAnalogs`:

```cpp
{A4, 11, 1, true},  // Expression CC11 on MIDI channel 1, pin 18.
```

Wire the new potentiometer to A4 (pin 18), 3.3V, and GND. Every enabled knob gets its
own filter, a 5ms sample interval, and change-only MIDI output. The ADC is
explicitly set to 10 bits; filtering starts at each knob's actual position.
Buttons are debounced for 25ms. There are no `delay()` calls in the scan loop.

Configuration checks reject duplicate pins (including analog/digital aliases),
pin 13 conflicts, invalid MIDI values, and empty or duplicate-note chords at
compile time. Pins 24-39 need access to underside pads; check the Teensy 4.0
pinout before expanding beyond the breadboard pins.

## Extend the firmware

| File | Responsibility |
| --- | --- |
| `platformio.ini` | Board, USB type, and build profiles |
| `include/ControllerConfig.h` | Wiring, chord banks, mappings, and feature switches |
| `lib/ControllerCore/src/Inputs.h` | Debounce, analog smoothing, and settled selector |
| `lib/ControllerCore/src/Controller.h` | Button actions, shared-note ownership, voicing changes, and panic |
| `src/main.cpp` | Teensy GPIO/ADC scanning, USB MIDI adapter, and serial commands |
| `test/test_core.cpp` | Desktop behavior tests with captured MIDI messages |

To add another kind of button action, extend `ButtonAction` and its dispatch
in `Controller.h`. To add an encoder, matrix, or GPIO expander, implement its
scanner in the hardware layer and feed debounced events to
`controls.setPressed(index, pressed)` or send CCs through the MIDI adapter.
Input expansion hardware is a future addition; the current scanner uses
direct GPIO. Musical behavior and note ownership do not depend on the input
hardware. The firmware core uses fixed storage sized from configuration.

## Test and troubleshoot

Build both hardware profiles:

```sh
pio run -e teensy40 -e teensy40_all_features
```

Run desktop behavior tests on Windows with Visual Studio C++ Build Tools:

```powershell
powershell -ExecutionPolicy Bypass -File .\test\run_tests.ps1
```

The runner discovers the compiler and puts generated files under `.pio`.
On macOS/Linux with a C++ compiler, use:

```sh
mkdir -p .pio/core-tests
c++ -std=c++17 -Wall -Wextra -pedantic -Ilib/ControllerCore/src test/test_core.cpp -o .pio/core-tests/core_tests
./.pio/core-tests/core_tests
```

Tests cover the default notes and velocities, simultaneous chords, shared
notes, independent channels, changes while held, CC/program buttons, panic,
disconnect-state reset, debounce and clock rollover, filter endpoints, and
selector boundaries/settling. Builds and desktop tests do not verify wiring
or DAW behavior; check those on the connected board after upload.

For a hardware check, use a MIDI monitor: a default chord press sends four
Note Ons and its release sends four Note Offs. Pan spans CC10 values 0-127.
Two overlapping custom chords keep a common note on until the last owner
releases it. Incoming USB MIDI is drained without echoing it back.

### Monitor buttons and potentiometers

Upload the updated `teensy40` firmware, then select **Project Tasks > teensy40 >
Monitor** in PlatformIO, or run:

```sh
pio device monitor --baud 115200
```

Select Teensy's serial port if prompted. With multiple serial devices, use
`pio device monitor --port COM3 --baud 115200`, replacing COM3 with the actual
port. Close the monitor before uploading again, then reopen it afterward.
USB serial debugging can run alongside the DAW's USB MIDI input.

The monitor prints only input changes: debounced button presses/releases,
changes to a pot's MIDI value, and optional settled voicing changes. There is
no startup banner or automatic status report. Example output (timestamps and
analog readings will vary):

```text
[1500 ms] BUTTON pin=4 raw=LOW state=PRESSED ch=1
[1800 ms] BUTTON pin=4 raw=HIGH state=RELEASED ch=1
[2000 ms] POT pin=17 cc=10 ch=1 raw=520 filtered=520 midi=65
```

- **Buttons:** press/release events print immediately after the 25ms debounce.
  `raw=HIGH` means the input is not grounded; `raw=LOW` means grounded.
  `state` is the debounced interpretation of that reading.
- **Pot:** `raw` is the ADC sample (0-1023), `filtered` is the smoothed reading,
  and `midi` is the resulting value (0-127). The logger checks at most ten
  times per second and prints only when the MIDI value differs from the last
  one shown. Small raw/filtered fluctuations within the same MIDI value do
  not print. Opening the monitor silently sets the starting knob positions.
- **Voicing:** when enabled, a line prints only after the selector settles
  into a different mode. MIDI Note On/Off messages are not printed separately.

If a button does not respond: hold it and send
**s** for a status snapshot. If its raw level stays HIGH when pressed, check
the switch contacts, ground, signal wire, and connection to its configured pin. If it stays
LOW when released, check for a permanent connection to ground. If press and
release events appear but there is no sound, inspect USB MIDI with a MIDI
monitor and check the DAW input and patch.

Send **s** (or S) in the monitor to explicitly request a full status report. Optional
reverb/voicing controls also appear when enabled. Debugging defaults to on;
set `kSerialDebug = false` in `include/ControllerConfig.h` and rebuild to
disable it. The logger does not wait for a monitor or free buffer space; it
skips lines if the serial buffer is full so scanning and MIDI can continue.

Send **p** through PlatformIO Serial Monitor (`pio device monitor`, 115200
baud) to stop active notes, clear configured CC latches, and send sustain-off,
all-notes-off, and all-sound-off on all channels. Held buttons must be released
before they can play again. Send **r** to resend knobs. Debug output is for
inspection, not the Uno's old serial-to-MIDI bridge protocol. Use a MIDI
monitor to verify reception on the computer.

If no MIDI device appears, verify the USB data cable and upload the correct
environment. If uploading cannot restart Teensy, press its Program button
when prompted. If a knob behaves randomly, check 3.3V/GND/wiper connections
and disable any optional control that is not wired. If notes remain sounding
after unplugging, use the DAW's panic function: disconnected hardware cannot
send Note Offs. On a USB-powered restart, the firmware starts with current
button/knob states when the computer configures the MIDI device.

## Hardware references

- [PJRC Teensy 4.0 specifications and voltage limits](https://www.pjrc.com/store/teensy40.html)
- [PJRC USB MIDI API](https://www.pjrc.com/teensy/td_midi.html)
- [PlatformIO Teensy USB build flags](https://docs.platformio.org/en/latest/platforms/teensy.html)
- [PlatformIO Teensy 4.0 board and upload configuration](https://docs.platformio.org/en/stable/boards/teensy/teensy40.html)
