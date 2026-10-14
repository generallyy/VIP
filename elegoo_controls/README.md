# Elegoo Uno MIDI chord controller

The two buttons play MIDI chords whose voicing is selected by the potentiometer.
Notes remain on while a button is held and receive MIDI Note Off messages when
the button is released.

| A0 range | Pin 2 | Pin 3 | Pin 13 light |
| --- | --- | --- | --- |
| `0-358` | C major ninth: C4 E4 G4 B4 D5 | F major ninth: F4 A4 C5 E5 G5 | Off |
| `359-664` | C major: C4 E4 G4 C5 | F major: F4 A4 C5 F5 | On |
| `665-1023` | C major seventh: C4 E4 G4 B4 | F major seventh: F4 A4 C5 E5 | Off |

Moving the potentiometer into another range while holding a button changes only
the notes that differ. Shared notes keep sounding without receiving another MIDI
Note On message. The sketch waits 40 milliseconds before accepting a range
change to prevent analog noise near a boundary from rapidly retriggering notes.
The highest note of each chord is accented at MIDI velocity 120; the remaining
notes use velocity 100.

## Wiring

The sketch uses the Uno's internal pull-up resistors. Connect each button between
its input pin and GND. No external button resistor is needed.

Connect the potentiometer's center (wiper) pin to A0. Connect its two outside
pins to 5V and GND. Pin 13 uses the Uno's built-in LED; an external LED needs a
current-limiting resistor.

## Upload the sketch

1. Open `elegoo_controls.ino` in the Arduino IDE.
2. Select **Arduino Uno** and the board's COM port.
3. Upload the sketch.
4. Close the Arduino IDE Serial Monitor before starting the MIDI helper.

## Start MIDI output on Windows

A standard Uno R3 presents itself as a USB serial device rather than a native
USB-MIDI device. `run_midi.ps1` reads the sketch's messages and converts them to
real Windows MIDI Note On and Note Off messages.

First list the Windows MIDI output devices:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_midi.ps1 -ListDevices
```

Then start the controller, replacing `COM3` and `0` with the Uno COM port and
the desired MIDI device ID:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_midi.ps1 -Port COM3 -MidiDeviceId 0
```

The default output is MIDI channel 1. Add `-Channel 2`, for example, to use
another channel. To feed a DAW, select a virtual MIDI port as the output device
and choose the matching port as the DAW's MIDI input. Press Ctrl+C to stop the
helper; it resets the MIDI output so notes do not remain stuck.
