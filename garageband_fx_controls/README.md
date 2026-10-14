# GarageBand pan and reverb controller

This project turns two potentiometers connected to an Elegoo Uno R3 into a
physical effects controller for a GarageBand software-instrument track.

- The **pan** knob sends MIDI Control Change 10 from `0` (left) through `64`
  (center) to `127` (right).
- The **reverb** knob sends MIDI Control Change 91 from `0` (dry) to `127`
  (maximum send).
- The sketch filters normal analog noise before sending values, which produces
  smoother automation and avoids unnecessary MIDI events.

## Parts and wiring

You need the Elegoo Uno R3, two 10k linear potentiometers, a breadboard, jumper
wires, and the Uno USB cable. Wire the board while its USB cable is disconnected.

| Control | Potentiometer center pin | Potentiometer outside pins |
| --- | --- | --- |
| Pan | A0 | 5V and GND |
| Reverb | A1 | 5V and GND |

If a knob works backwards, exchange that potentiometer's two outside wires. Do
not connect 5V directly to GND.

## 1. Upload the Arduino sketch

1. Open `garageband_fx_controls.ino` in the Arduino IDE.
2. Select **Arduino Uno** as the board and select the Uno's serial port.
3. Upload the sketch.
4. Close the Arduino IDE Serial Monitor. Only one program can use the serial
   port at a time.

The Uno R3 normally identifies itself as a USB serial device, not as a USB MIDI
controller. The included bridge turns the sketch's serial messages into a
virtual CoreMIDI port that GarageBand can receive.

## 2. Set up the MIDI bridge on the Mac

Open Terminal in this directory and create a Python virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

List the Mac's serial ports:

```bash
python midi_bridge.py --list-ports
```

The Uno is commonly named `/dev/cu.usbmodem...`; some compatible boards use
`/dev/cu.usbserial...`. Start the bridge with the name shown on your Mac:

```bash
python midi_bridge.py --port /dev/cu.usbmodem1101
```

Leave that Terminal window running. Add `--verbose` if you want to see every
control value while testing. Press Control-C to stop the bridge.

## 3. Record the controls in GarageBand

1. Start the bridge before opening GarageBand. If GarageBand was already open,
   quit and reopen it so it sees **Elegoo GarageBand Controls** as a MIDI input.
2. Select the software-instrument track you want to control.
3. Press Record, then move the knobs during the song. GarageBand records live
   MIDI controller movement as region automation.
4. To inspect the result, select the recorded MIDI region, open the Piano Roll,
   show Automation, and select **Pan** or the relevant reverb/controller lane.

CC10 is the standard MIDI pan controller. CC91 is the standard reverb-send
controller, but GarageBand patches do not all expose reverb in the same way. If
the pan knob records but the reverb knob has no audible effect, choose a patch
whose Smart Controls include Reverb, or use a MIDI-controllable reverb Audio
Unit. The bridge is still sending CC91 correctly; `--verbose` will confirm it.

The values are recorded into a MIDI region, so this setup is intended for a
software-instrument track. GarageBand does not treat generic incoming MIDI CC
messages as mixer automation for an arbitrary prerecorded audio track.

## Change the MIDI channel or controller numbers

The bridge uses MIDI channel 1 by default. To use another channel:

```bash
python midi_bridge.py --port /dev/cu.usbmodem1101 --channel 2
```

To change the controller numbers, edit the two entries in `controls[]` near the
top of `garageband_fx_controls.ino`, then upload the sketch again.

## Troubleshooting

- **Port busy:** close Arduino Serial Monitor and any other serial terminal.
- **No serial port:** try another data-capable USB cable or USB adapter, then
  run `python midi_bridge.py --list-ports` again.
- **GarageBand does not see the controller:** start the bridge first and reopen
  GarageBand. Check GarageBand > Settings > Audio/MIDI for detected MIDI inputs.
- **Values move when untouched:** shorten the analog wires and make sure both
  potentiometer ground pins have a solid connection to Uno GND.
- **A knob never reaches an endpoint:** the potentiometer may have normal end
  tolerance. This can be calibrated by changing `readingToMidiValue()` in the
  sketch.

## Run the bridge parser tests

The message parser tests use only Python's standard library:

```bash
python -m unittest -v
```
