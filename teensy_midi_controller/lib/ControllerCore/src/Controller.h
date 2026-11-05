#pragma once

#include <cstddef>
#include <cstdint>

#include "Inputs.h"

namespace controller {

constexpr size_t kMaxChordNotes = 8;
constexpr uint8_t kNoteVelocity = 100;
constexpr uint8_t kTopNoteVelocity = 120;

struct Chord {
  uint8_t notes[kMaxChordNotes];
  uint8_t count;
};

struct ChordBank {
  // Order matches Voicing: ninth, major, seventh.
  Chord voicings[3];
};

enum class ButtonAction : uint8_t { Chord, MomentaryCC, ToggleCC, Program, Panic };

struct ButtonConfig {
  uint8_t pin;
  ButtonAction action;
  uint8_t channel;  // MIDI channels are 1-16, not zero-based.
  const ChordBank* bank;
  uint8_t number;   // Controller or program number, depending on action.
  uint8_t onValue;
  uint8_t offValue;
};

constexpr ButtonConfig chordButton(uint8_t pin, const ChordBank& bank,
                                   uint8_t channel = 1) {
  return {pin, ButtonAction::Chord, channel, &bank, 0, 0, 0};
}

constexpr ButtonConfig ccButton(uint8_t pin, uint8_t cc, bool toggle = false,
                                uint8_t channel = 1, uint8_t on = 127,
                                uint8_t off = 0) {
  return {pin, toggle ? ButtonAction::ToggleCC : ButtonAction::MomentaryCC,
          channel, nullptr, cc, on, off};
}

constexpr ButtonConfig programButton(uint8_t pin, uint8_t program,
                                     uint8_t channel = 1) {
  return {pin, ButtonAction::Program, channel, nullptr, program, 0, 0};
}

constexpr ButtonConfig panicButton(uint8_t pin) {
  return {pin, ButtonAction::Panic, 1, nullptr, 0, 0, 0};
}

constexpr bool validButton(const ButtonConfig& config) {
  if (config.channel < 1 || config.channel > 16 || config.number > 127 ||
      config.onValue > 127 || config.offValue > 127) {
    return false;
  }
  if (config.action != ButtonAction::Chord) {
    return true;
  }
  if (config.bank == nullptr) {
    return false;
  }
  for (const auto& chord : config.bank->voicings) {
    if (chord.count == 0 || chord.count > kMaxChordNotes) {
      return false;
    }
    for (size_t i = 0; i < chord.count; ++i) {
      if (chord.notes[i] > 127) {
        return false;
      }
      for (size_t j = 0; j < i; ++j) {
        if (chord.notes[i] == chord.notes[j]) {
          return false;
        }
      }
    }
  }
  return true;
}

// Output supplies noteOn, noteOff, controlChange, and programChange. It can
// be the Teensy USB adapter or a recording sink in desktop tests.
template <typename Output, size_t ButtonCount>
class Controller {
 public:
  static_assert(ButtonCount > 0 && ButtonCount < 256, "Use 1-255 button entries");

  Controller(Output& output, const ButtonConfig (&config)[ButtonCount])
      : output_(output), config_(config) {}

  void setPressed(size_t index, bool pressed) {
    if (index >= ButtonCount || pressed_[index] == pressed) {
      return;
    }
    pressed_[index] = pressed;
    const auto& config = config_[index];
    switch (config.action) {
      case ButtonAction::Chord:
        active_[index] = pressed;
        rebuildNotes();
        break;
      case ButtonAction::MomentaryCC:
        if (pressed || active_[index]) {
          output_.controlChange(config.number,
                                pressed ? config.onValue : config.offValue,
                                config.channel);
        }
        active_[index] = pressed;
        break;
      case ButtonAction::ToggleCC:
        if (pressed) {
          active_[index] = !active_[index];
          output_.controlChange(config.number,
                                active_[index] ? config.onValue : config.offValue,
                                config.channel);
        }
        break;
      case ButtonAction::Program:
        if (pressed) {
          output_.programChange(config.number, config.channel);
        }
        break;
      case ButtonAction::Panic:
        if (pressed) {
          panic();
        }
        break;
    }
  }

  void setVoicing(Voicing voicing) {
    if (mode_ != voicing) {
      mode_ = voicing;
      rebuildNotes();
    }
  }

  // Release sound without retriggering buttons that are still held. They must
  // be released and pressed again. CC latches return to their configured off.
  void panic() {
    for (size_t i = 0; i < ButtonCount; ++i) {
      if (active_[i] && (config_[i].action == ButtonAction::MomentaryCC ||
                         config_[i].action == ButtonAction::ToggleCC)) {
        output_.controlChange(config_[i].number, config_[i].offValue,
                              config_[i].channel);
      }
      active_[i] = false;
    }
    rebuildNotes();
    for (uint8_t channel = 1; channel <= 16; ++channel) {
      output_.controlChange(64, 0, channel);   // Sustain off.
      output_.controlChange(123, 0, channel);  // All notes off.
      output_.controlChange(120, 0, channel);  // All sound off.
    }
  }

  // Local reset on USB disconnect: no messages go to an unavailable host.
  void reset() {
    for (size_t i = 0; i < ButtonCount; ++i) {
      pressed_[i] = active_[i] = false;
    }
    for (auto& channel : noteUsers_) {
      for (auto& users : channel) {
        users = 0;
      }
    }
  }

 private:
  void rebuildNotes() {
    uint8_t desired[16][128] = {};
    uint8_t velocities[16][128] = {};
    for (size_t i = 0; i < ButtonCount; ++i) {
      const auto& config = config_[i];
      if (!active_[i] || config.action != ButtonAction::Chord) {
        continue;
      }
      const auto& chord = config.bank->voicings[static_cast<uint8_t>(mode_)];
      uint8_t highest = 0;
      for (size_t n = 0; n < chord.count; ++n) {
        if (chord.notes[n] > highest) {
          highest = chord.notes[n];
        }
      }
      const uint8_t channel = config.channel - 1;
      for (size_t n = 0; n < chord.count; ++n) {
        const uint8_t note = chord.notes[n];
        ++desired[channel][note];
        const uint8_t velocity = note == highest ? kTopNoteVelocity : kNoteVelocity;
        if (velocity > velocities[channel][note]) {
          velocities[channel][note] = velocity;
        }
      }
    }
    // Compute the union across all held chords before sending differences.
    // Shared notes keep sounding even if their owner changes with a voicing.
    for (uint8_t channel = 0; channel < 16; ++channel) {
      for (size_t note = 0; note < 128; ++note) {
        if (noteUsers_[channel][note] && !desired[channel][note]) {
          output_.noteOff(static_cast<uint8_t>(note), channel + 1);
        }
      }
    }
    for (uint8_t channel = 0; channel < 16; ++channel) {
      for (size_t note = 0; note < 128; ++note) {
        if (!noteUsers_[channel][note] && desired[channel][note]) {
          output_.noteOn(static_cast<uint8_t>(note), velocities[channel][note],
                         channel + 1);
        }
        noteUsers_[channel][note] = desired[channel][note];
      }
    }
  }

  Output& output_;
  const ButtonConfig (&config_)[ButtonCount];
  bool pressed_[ButtonCount] = {};
  bool active_[ButtonCount] = {};
  uint8_t noteUsers_[16][128] = {};
  Voicing mode_ = Voicing::Seventh;
};

}  // namespace controller

