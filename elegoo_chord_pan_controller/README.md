# Elegoo chord and pan MIDI controller

This project turns an Elegoo Uno R3, two pushbuttons, and a potentiometer into
a physical controller for digital music software. It combines the chord and
effects-control prototypes in this repository, bringing chord playback and
continuous pan control into one PlatformIO project. Embedded C++ firmware and
Python/PowerShell MIDI bridges connect the hardware to desktop instruments,
demonstrating input debouncing, analog signal filtering, and communication
across hardware and software.

| Control | Function | MIDI data |
| --- | --- | --- |
| Potentiometer on A0 | Pan: left (0), center (64), right (127) | CC10 |
| Button on pin 2 | Hold to play Fmaj7: F4, A4, C5, E5 | Notes 65, 69, 72, 76 |
| Button on pin 3 | Hold to play Cmaj7: C4, E4, G4, B4 | Notes 60, 64, 67, 71 |

Releasing a button stops its chord. Both buttons can be held together. The
highest note in each chord uses velocity 120; the other notes use velocity 100.
The knob is filtered to reduce analog noise, and the buttons are debounced.
Some DAWs label octaves differently; the MIDI note numbers above are definitive.

## How the pieces connect

The Uno sends messages over USB serial. A bridge converts them into MIDI, and
the DAW's software instrument produces the sound.

| Setup | Bridge | MIDI connection into the DAW |
| --- | --- | --- |
| Windows + PreSonus Studio One | `run_midi.ps1` | A virtual MIDI port, such as loopMIDI |
| macOS + GarageBand | `midi_bridge.py` | A CoreMIDI port created by the bridge |

Both setups use the same firmware. The Uno R3 appears as a serial device, so
uploading the firmware is followed by running the appropriate bridge.

## What you need

- Elegoo Uno R3, a USB data cable, a breadboard, and jumper wires.
- One potentiometer (a 10k linear potentiometer is suitable).
- Two momentary pushbuttons.
- [Visual Studio Code](https://code.visualstudio.com/) with the
  [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide).
- Windows: Studio One and a compatible virtual MIDI cable application.
- macOS: GarageBand and Python 3 for the bridge.

## Wiring

Disconnect USB power before changing the wiring.

| Part | Wiring |
| --- | --- |
| Potentiometer | Center/wiper pin to A0; outside pins to 5V and GND |
| Fmaj7 button | Connect pin 2 to GND when pressed |
| Cmaj7 button | Connect pin 3 to GND when pressed |

Both buttons use `INPUT_PULLUP`, so they need no external pull-up resistors.
All ground connections must connect to Uno GND. For four-legged pushbuttons,
use contacts that connect only when pressed; the two legs on one internally
connected side will always read as connected.

If the knob works backwards, exchange its two outside wires. Never connect 5V
directly to GND.

## Flash the Uno with PlatformIO

Flashing means compiling the firmware and uploading it to the board.

1. Install PlatformIO IDE in VS Code and let its setup finish.
2. Choose **File > Open Folder** and open this `elegoo_chord_pan_controller`
   folder. You should see `platformio.ini` and `src/main.cpp` in Explorer.
3. Connect the Uno with its USB cable.
4. Stop any running MIDI bridge and close any Serial Monitor using the board.
5. Click the PlatformIO icon in the left sidebar.
6. Expand **Project Tasks > uno > General** and select **Upload**.
7. Wait for the terminal to report **SUCCESS**.

Upload builds the project first. The first build can take longer while
PlatformIO downloads its tools. The right-arrow Upload button on the bottom
toolbar is another way to start it. See the
[PlatformIO VS Code guide](https://docs.platformio.org/en/latest/integration/ide/vscode.html).

For terminal use, open **PlatformIO Core CLI** from PlatformIO's Quick Access
panel and run these commands from this folder:

```bash
pio run
pio run --target upload
```

The first command builds only; the second builds and uploads. Firmware stays
on the board after power is disconnected. Upload again when changing firmware.

### If the upload port is not detected

Run `pio device list` in the PlatformIO terminal. On Windows, you can also check
Device Manager under **Ports (COM & LPT)**. Use the Uno's port, for example:

```bash
pio run --target upload --upload-port COM3
```

Alternatively, add `upload_port = COM3` under `[env:uno]` in `platformio.ini`.
On macOS, use the board's `/dev/cu.usbmodem...` or `/dev/cu.usbserial...` path.
All port names in this guide are examples; substitute your actual port.

## Windows + PreSonus Studio One

Here, PreSonus refers to Studio One. A PreSonus audio interface can remain your
normal audio output; MIDI input and audio output are configured separately.

### One-time MIDI setup

1. Install and open [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)
   or another compatible virtual MIDI cable.
2. In loopMIDI, enter `Elegoo Chords and Pan` as the new port name and click **+**.
3. Leave loopMIDI running while using the controller.

The PowerShell bridge uses Windows MIDI directly, so it requires no Python
packages. Use the virtual port to route its output into Studio One.

### Start the Windows bridge

Open PowerShell in this project folder. List the available MIDI outputs:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_midi.ps1 -ListDevices
```

Find the number beside your virtual port. Start the bridge with that number
and the Uno's COM port:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_midi.ps1 -Port COM3 -MidiDeviceId 2
```

`2` is an example. Use the ID printed for your virtual port, rather than an
unrelated synthesizer or physical MIDI output. Keep this PowerShell window
running while playing.

### Configure Studio One

1. Open **Studio One > Options > External Devices** and click **Add**.
2. Select **New Keyboard** and name it `Elegoo Chords and Pan`.
3. Set **Receive From** to your virtual MIDI port; leave **Send To** unset.
4. Enable channel 1 or all channels and save the device.
5. Create an Instrument Track and load a software instrument.
6. Select the Elegoo device as the track input and enable monitoring.
7. Press a chord button to play. To record notes, record-enable the track and
   press Record.

Menu labels may vary by Studio One version. The device is added as a keyboard
because it sends notes; see
[PreSonus's MIDI device setup guide](https://support.presonus.com/hc/en-us/articles/360045189351-Basic-Midi-Setup-New-Keyboard-Control-Surface-Instrument-Faderport).

### Make the knob control pan

The knob sends CC10. Some instruments respond directly; controlling Studio
One's mixer pan may require a Control Link assignment:

1. Open the Console's **External Devices** panel and edit the Elegoo device.
2. Enable **MIDI Learn** and turn only the potentiometer to learn its CC10 knob.
3. Disable MIDI Learn.
4. Move the track's pan control and the physical knob, then use the Control
   Link arrow to associate the hardware control with pan.

Track pan uses a global mapping, so that assignment targets the linked pan
control. It does not automatically follow every track you select. For details,
see [PreSonus's MIDI Learn guide](https://support.presonus.com/hc/en-us/articles/210040463-MIDI-External-Devices-Control-Surfaces-Setup)
and [global versus focus mapping](https://support.presonus.com/hc/en-us/articles/210050323-Studio-One-3-Global-and-Focus-MIDI-Mapping).

To record linked mixer-pan movement, enable the track's pan automation and use
an automation-writing mode such as Touch during playback. MIDI note recording
and mixer automation recording are separate operations.

## macOS + GarageBand

### One-time Python setup

With Python 3 installed, open Terminal in this project folder and run:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

The dependencies are `pyserial`, `mido`, and `python-rtmidi`.

### Start the Mac bridge

For each new Terminal session, change to the project folder and activate the
environment. List the serial ports, then start with the Uno's port:

```bash
source .venv/bin/activate
python midi_bridge.py --list-ports
python midi_bridge.py --port /dev/cu.usbmodem1101 --verbose
```

Replace `/dev/cu.usbmodem1101` with the actual port. The bridge creates a
CoreMIDI port named **Elegoo Chords and Pan**. No separate IAC or loopMIDI setup
is needed for this bridge. Keep Terminal running while playing; `--verbose`
prints the forwarded notes and controller values and can be omitted later.

### Play and record in GarageBand

1. Start the bridge, then open GarageBand. Reopen GarageBand if the input is
   not detected.
2. Create or select a **Software Instrument** track and choose a patch.
3. Hold either button to play its chord; release it to stop.
4. Turn the knob and listen for pan changes using stereo speakers or headphones.
5. Record on the software-instrument track to capture the performance and
   supported MIDI controller data.

The knob sends MIDI pan data; audible behavior can depend on the selected
instrument or patch. GarageBand supports editing MIDI pan data, as documented
in [Apple's release notes](https://support.apple.com/en-gb/109515). This setup
targets software instruments, rather than mixer automation of an unrelated
prerecorded audio track.

## Starting and stopping a session

- Windows: connect the Uno, open loopMIDI, start `run_midi.ps1`, then open
  Studio One and monitor your instrument track.
- macOS: connect the Uno, activate `.venv`, start `midi_bridge.py`, then open
  GarageBand and select your software-instrument track.
- Stop: release both chord buttons, then press **Ctrl+C** in the bridge terminal.
- To flash new firmware: stop the bridge first, upload, then restart the bridge.

Both bridges default to MIDI channel 1. Add `-Channel 2` to the Windows command
or `--channel 2` to the Mac command to change it, and match the DAW's input.

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| No serial port appears | Use a USB data cable; try another USB port. Check the board's USB serial driver if it remains unrecognized. |
| Upload or bridge says the port is busy | Stop other bridges and close Serial Monitors before trying again. |
| `pio` is not recognized | Use PlatformIO Core CLI inside VS Code or use the Upload task. |
| Studio One has no Elegoo MIDI input | Confirm loopMIDI is running and the bridge targets its device ID; add that port as a New Keyboard. |
| GarageBand receives no notes | Start the Mac bridge first, reopen GarageBand, and select a Software Instrument track. |
| Notes work but pan does not | Check CC10 output, stereo playback, and the patch's MIDI response. In Studio One, link CC10 to mixer pan if needed. |
| Button is always on or never responds | Check pin 2/3 to GND wiring and the pushbutton's internally connected legs. |
| Knob values jump while untouched | Check the wiper on A0, common GND, and secure connections. |
| Notes remain held after a disconnect | Use the DAW's panic/all-notes-off function if available, or reload the software instrument. |

For a firmware-only check, stop the bridge and open PlatformIO Serial Monitor
at **115200 baud**. Turning the knob should print `CC 10 ...`. Pressing a button
prints four `ON` lines; releasing it prints four `OFF` lines. Close the monitor
before restarting the bridge.

## Project files and developer checks

| File | Purpose |
| --- | --- |
| `platformio.ini` | Uno board, Arduino framework, and monitor baud rate |
| `src/main.cpp` | Button chords, debounce, and filtered pan control |
| `run_midi.ps1` | Windows serial-to-MIDI bridge |
| `midi_bridge.py` | macOS serial-to-CoreMIDI bridge |
| `requirements.txt` | Python bridge dependencies |
| `test_midi_bridge.py` | Python serial-message parser tests |

From this folder, run the parser tests with Python (no third-party packages
are needed for the tests):

```bash
python -m unittest -v
```

The serial protocol is `ON note velocity`, `OFF note 0`, or
`CC controller value`, one message per line at 115200 baud. The bridges validate
MIDI data values in the range 0-127.
