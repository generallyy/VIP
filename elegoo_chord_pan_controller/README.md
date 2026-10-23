# Elegoo chord and pan MIDI controller

This project combines the chord buttons from `elegoo_controls` with the
continuous GarageBand control from `garageband_fx_controls` in one controller.

- Turning the A0 knob sends MIDI **CC10 (pan)** from 0 (left), through 64
  (center), to 127 (right).
- Holding the pin 2 button plays **Fmaj7**: F4, A4, C5, E5.
- Holding the pin 3 button plays **Cmaj7**: C4, E4, G4, B4.
- Releasing a button sends Note Off for every note in its chord.

The knob is low-pass filtered to reduce analog jitter. Both buttons use the
Uno's internal pull-up resistors and software debouncing.

## Wiring

Disconnect USB power before changing the wiring.

| Control | Connection |
| --- | --- |
| Pan potentiometer | Center pin to A0; outside pins to 5V and GND |
| Fmaj7 button | One side to pin 2; other side to GND |
| Cmaj7 button | One side to pin 3; other side to GND |

No external button resistors are required because the sketch uses
`INPUT_PULLUP`. If the knob pans in the opposite direction from the one you
want, exchange its two outside wires. Never connect 5V directly to GND.

## Build and upload with PlatformIO in VS Code

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the
   **PlatformIO IDE** extension.
2. In VS Code, choose **File > Open Folder** and open this entire
   `elegoo_chord_pan_controller` folder. Do not open `src/main.cpp` by itself.
3. Connect the Elegoo Uno R3 by USB.
4. Use the PlatformIO checkmark in the VS Code status bar to build.
5. Use the PlatformIO arrow in the status bar to upload. PlatformIO normally
   detects the serial port automatically.

The PlatformIO environment is defined in `platformio.ini` for an Arduino Uno,
and the controller firmware is in `src/main.cpp`. Equivalent terminal commands
from this folder are:

```bash
pio run
pio run --target upload
```

Close the PlatformIO Serial Monitor before running a MIDI bridge; only one
application can use the serial port at a time. If upload auto-detection fails,
add the detected port to the Uno environment in `platformio.ini`, for example:

```ini
upload_port = COM3
```

The Uno R3 sends serial data rather than native USB MIDI, so after uploading,
use the bridge for your operating system.

## macOS and GarageBand

In Terminal, change to this folder and install the bridge dependencies:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

Find the Uno's port, then start the bridge with the port it reports:

```bash
python midi_bridge.py --list-ports
python midi_bridge.py --port /dev/cu.usbmodem1101
```

Start the bridge before opening GarageBand. If GarageBand was already open,
quit and reopen it so it detects **Elegoo Chords and Pan**. Select a software
instrument track, hold either chord button to play it, and turn the knob to pan.
Add `--verbose` to the bridge command to see each forwarded MIDI event.

GarageBand records the knob as MIDI region automation on software-instrument
tracks; it does not treat generic incoming CC data as mixer automation for an
unrelated prerecorded audio track.

## Windows

List MIDI output devices:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_midi.ps1 -ListDevices
```

Then replace `COM3` and `0` with the Uno port and desired MIDI device ID:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_midi.ps1 -Port COM3 -MidiDeviceId 0
```

To feed a DAW, choose a virtual MIDI port as the output device and select its
matching input inside the DAW. Press Ctrl+C to stop; the bridge resets its MIDI
output so held notes do not become stuck.

Both bridges default to MIDI channel 1. Use `--channel 2` on macOS or
`-Channel 2` on Windows to choose a different channel.

## Test the bridge parser

The parser tests use only Python's standard library:

```bash
python -m unittest -v
```

Expected serial messages are `ON note velocity`, `OFF note 0`, and
`CC controller value`. The Arduino sketch and both bridges use 115200 baud.
