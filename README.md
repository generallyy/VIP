# Physical MIDI Controllers

This repository explores physical interfaces for digital music production using
an Elegoo Uno R3 or Teensy 4.0, pushbuttons, and potentiometers. The projects
progress from chord generation and continuous effects control to a combined
controller. The Uno projects pair embedded C++ firmware with Python and
PowerShell bridges that translate USB serial messages into MIDI. The work covers input debouncing, analog signal
filtering, MIDI communication, and integration with desktop music software.
The combined Uno and Teensy projects use PlatformIO in VS Code and include
setup instructions for Windows with Studio One and macOS with GarageBand.
The Teensy project sends USB MIDI directly and has configurable controls for
expanding the hardware.

## Projects

| Project | Focus |
| --- | --- |
| [Teensy 4.0 MIDI controller](teensy_midi_controller/README.md) | Native USB MIDI, the current chord/pan controls, optional reverb/voicings, and expandable button/knob mappings. |
| [Chord controller](elegoo_controls/README.md) | Two chord buttons with potentiometer-selected voicings and a Windows MIDI bridge. |
| [Effects controller](garageband_fx_controls/README.md) | Continuous pan and reverb controls with a Python bridge for GarageBand. |
| [Combined chord and pan controller](elegoo_chord_pan_controller/README.md) | Fmaj7/Cmaj7 buttons and continuous pan control, combining both prototypes in a PlatformIO project. |

Start with the **Teensy 4.0 MIDI controller** for the new hardware. For the Uno,
use the **combined chord and pan controller**. Each project includes wiring,
uploading, and DAW setup instructions. Teensy potentiometers use **3.3V**,
replacing the Uno's 5V wiring.
