#pragma once

#include <cstdint>

namespace controller {

// All timing uses unsigned subtraction, including across millis() rollover.
class DebouncedButton {
 public:
  void begin(bool pressed, uint32_t now) {
    observed_ = pressed;
    pressed_ = false;  // A held-at-startup button still gets a debounced press.
    changedAt_ = now;
  }

  bool update(bool pressed, uint32_t now, uint32_t debounceMs) {
    if (pressed != observed_) {
      observed_ = pressed;
      changedAt_ = now;
    }
    if (now - changedAt_ >= debounceMs) {
      pressed_ = observed_;
    }
    return pressed_;
  }

 private:
  bool observed_ = false;
  bool pressed_ = false;
  uint32_t changedAt_ = 0;
};

class AnalogFilter {
 public:
  void begin(uint16_t reading) { scaled_ = clamp(reading) * kScale; }

  void update(uint16_t reading) {
    scaled_ += (clamp(reading) * kScale - scaled_) / kDivisor;
  }

  uint8_t midiValue() const {
    return static_cast<uint8_t>(reading() / 8);
  }

  uint16_t reading() const {
    return static_cast<uint16_t>((scaled_ + kScale / 2) / kScale);
  }

 private:
  static int32_t clamp(uint16_t reading) { return reading > 1023 ? 1023 : reading; }
  static constexpr int32_t kScale = 16;
  static constexpr int32_t kDivisor = 8;
  int32_t scaled_ = 0;
};

enum class Voicing : uint8_t { Ninth, Major, Seventh };

constexpr Voicing voicingForReading(uint16_t reading) {
  return reading < 359 ? Voicing::Ninth
                      : (reading > 664 ? Voicing::Seventh : Voicing::Major);
}

class VoicingSelector {
 public:
  void begin(uint16_t reading, uint32_t now) {
    active_ = pending_ = voicingForReading(reading);
    changedAt_ = now;
  }

  Voicing update(uint16_t reading, uint32_t now, uint32_t settleMs) {
    const Voicing observed = voicingForReading(reading);
    if (observed != pending_) {
      pending_ = observed;
      changedAt_ = now;
    }
    if (now - changedAt_ >= settleMs) {
      active_ = pending_;
    }
    return active_;
  }

 private:
  Voicing active_ = Voicing::Seventh;
  Voicing pending_ = Voicing::Seventh;
  uint32_t changedAt_ = 0;
};

}  // namespace controller

