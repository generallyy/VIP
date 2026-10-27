# Physical MIDI Controllers

This repository explores physical interfaces for digital music production using
an Elegoo Uno R3, pushbuttons, and potentiometers. The projects progress from
chord generation and continuous effects control to a combined controller,
pairing embedded C++ firmware with Python and PowerShell bridges that translate
USB serial messages into MIDI. The work covers input debouncing, analog signal
filtering, MIDI communication, and integration with desktop music software.
The combined project uses PlatformIO in VS Code and includes setup instructions
for Windows with Studio One and macOS with GarageBand.

## Projects

| Project | Focus |
| --- | --- |
| [Chord controller](elegoo_controls/README.md) | Two chord buttons with potentiometer-selected voicings and a Windows MIDI bridge. |
| [Effects controller](garageband_fx_controls/README.md) | Continuous pan and reverb controls with a Python bridge for GarageBand. |
| [Combined chord and pan controller](elegoo_chord_pan_controller/README.md) | Fmaj7/Cmaj7 buttons and continuous pan control, combining both prototypes in a PlatformIO project. |

Start with the **combined chord and pan controller** for the current firmware,
wiring, flashing instructions, and DAW setup.
