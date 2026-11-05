#pragma once

#include <Arduino.h>
#include <Controller.h>

// Keep the existing two-button / one-knob wiring by default. Enable a feature
// only after wiring its extra pot; a floating analog input sends random values.
#ifndef MIDI_ENABLE_REVERB
#define MIDI_ENABLE_REVERB 0
#endif
#ifndef MIDI_ENABLE_VOICING
#define MIDI_ENABLE_VOICING 0
#endif

namespace config {
using namespace controller;

constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kAnalogSampleMs = 5;
constexpr uint32_t kVoicingSettleMs = 40;
constexpr bool kSerialDebug = true;
constexpr uint32_t kPotDebugIntervalMs = 100;
constexpr uint8_t kLedPin = LED_BUILTIN;
constexpr uint8_t kLedButtonPin = 2;  // Light follows this button while held.
constexpr bool kEnableVoicing = MIDI_ENABLE_VOICING != 0;
constexpr uint8_t kVoicingPin = A2;

constexpr ChordBank kFMajor = {{
    {{65, 69, 72, 76, 79}, 5},  // F major ninth
    {{65, 69, 72, 77}, 4},      // F major with octave root
    {{65, 69, 72, 76}, 4},      // F major seventh (default)
}};
constexpr ChordBank kCMajor = {{
    {{60, 64, 67, 71, 74}, 5},
    {{60, 64, 67, 72}, 4},
    {{60, 64, 67, 71}, 4},
}};

// Add entries here. Counts and runtime storage follow the array automatically.
// All buttons connect the named pin to GND and use INPUT_PULLUP.
constexpr ButtonConfig kButtons[] = {
    chordButton(2, kFMajor),
    chordButton(4, kCMajor),
    // ccButton(5, 64),          // Hold for sustain; release sends 0.
    // ccButton(6, 91, true),    // Toggle reverb send between 127 and 0.
    // programButton(7, 0),     // Program numbers are 0-127.
    // panicButton(8),
};
constexpr size_t kButtonCount = sizeof(kButtons) / sizeof(kButtons[0]);

struct AnalogConfig {
  uint8_t pin;
  uint8_t controller;
  uint8_t channel;
  bool enabled;
};
constexpr AnalogConfig kAnalogs[] = {
    {A3, 10, 1, true},                     // Pan wiper on digital pin 17.
    {A1, 91, 1, MIDI_ENABLE_REVERB != 0},   // Optional reverb send.
};
constexpr size_t kAnalogCount = sizeof(kAnalogs) / sizeof(kAnalogs[0]);

constexpr bool validConfiguration() {
  bool pins[40] = {};
  pins[kLedPin] = true;
  if (kEnableVoicing) {
    if (kVoicingPin < A0 || kVoicingPin > A13 || pins[kVoicingPin]) return false;
    pins[kVoicingPin] = true;
  }
  for (const auto& button : kButtons) {
    if (button.pin >= 40 || pins[button.pin] || !validButton(button)) return false;
    pins[button.pin] = true;
  }
  for (const auto& analog : kAnalogs) {
    if (!analog.enabled) continue;
    if (analog.pin < A0 || analog.pin > A13 || pins[analog.pin] ||
        analog.controller > 127 || analog.channel < 1 || analog.channel > 16) {
      return false;
    }
    pins[analog.pin] = true;
  }
  return true;
}
static_assert(validConfiguration(),
              "Check duplicate/reserved pins, analog pins, MIDI values, and chords");
}  // namespace config

