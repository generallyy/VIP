#pragma once

#include <Arduino.h>
#include "ChordMap.h"

namespace config {
constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kAnalogSampleMs = 5;
constexpr uint32_t kVoicingSettleMs = 40;
constexpr bool kSerialDebug = true;
constexpr uint8_t kLedPin = LED_BUILTIN;
constexpr uint8_t kLedButtonPin = 2;
constexpr uint8_t kVoicingPin = A1;  // A1 is digital pin 15 on Teensy 4.0.

constexpr bool validConfiguration() {
  bool pins[40] = {};
  pins[kLedPin] = true;
  if (kVoicingPin < A0 || kVoicingPin > A13 || pins[kVoicingPin]) return false;
  pins[kVoicingPin] = true;
  for (const auto& button : kButtons) {
    if (button.pin >= 40 || pins[button.pin] || !controller::validButton(button)) {
      return false;
    }
    pins[button.pin] = true;
  }
  return true;
}
static_assert(validConfiguration(), "Check pins, MIDI values, and chords");
}  // namespace config
