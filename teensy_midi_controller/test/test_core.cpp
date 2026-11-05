#include <Controller.h>
#include <Inputs.h>

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace controller;

namespace {
struct Message {
  char type;
  uint8_t number;
  uint8_t value;
  uint8_t channel;
};
struct Output {
  std::vector<Message> messages;
  void noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    messages.push_back({'N', note, velocity, channel});
  }
  void noteOff(uint8_t note, uint8_t channel) {
    messages.push_back({'O', note, 0, channel});
  }
  void controlChange(uint8_t cc, uint8_t value, uint8_t channel) {
    messages.push_back({'C', cc, value, channel});
  }
  void programChange(uint8_t program, uint8_t channel) {
    messages.push_back({'P', program, 0, channel});
  }
  size_t count(char type, uint8_t number, uint8_t value = 0,
               uint8_t channel = 1) const {
    size_t result = 0;
    for (const auto& message : messages) {
      if (message.type == type && message.number == number &&
          message.value == value && message.channel == channel) ++result;
    }
    return result;
  }
};

// Independent expected musical fixtures, including shared notes and an
// intentionally unsorted chord to check that the highest pitch is accented.
constexpr ChordBank fMajor = {{
    {{65, 69, 72, 76, 79}, 5}, {{65, 69, 72, 77}, 4}, {{65, 69, 72, 76}, 4}}};
constexpr ChordBank cMajor = {{
    {{60, 64, 67, 71, 74}, 5}, {{60, 64, 67, 72}, 4}, {{60, 64, 67, 71}, 4}}};
constexpr ChordBank overlap = {{
    {{60, 69}, 2}, {{60, 69}, 2}, {{60, 69}, 2}}};
constexpr ChordBank unsorted = {{
    {{80, 60, 70}, 3}, {{80, 60, 70}, 3}, {{80, 60, 70}, 3}}};

#define CHECK(expression) do { \
  if (!(expression)) { \
    std::cerr << __func__ << ":" << __LINE__ << ": " #expression "\n"; \
    std::exit(1); \
  } \
} while (false)

void default_chords_and_release() {
  Output out;
  const ButtonConfig buttons[] = {chordButton(2, fMajor), chordButton(3, cMajor)};
  Controller<Output, 2> engine(out, buttons);
  engine.setPressed(0, true);
  CHECK(out.messages.size() == 4);
  const uint8_t lowerNotes[] = {65, 69, 72};
  for (uint8_t note : lowerNotes) CHECK(out.count('N', note, 100) == 1);
  CHECK(out.count('N', 76, 120) == 1);
  engine.setPressed(1, true);
  CHECK(out.messages.size() == 8);
  CHECK(out.count('N', 71, 120) == 1);
  engine.setPressed(0, false);
  CHECK(out.messages.size() == 12);
  const uint8_t releasedNotes[] = {65, 69, 72, 76};
  for (uint8_t note : releasedNotes) CHECK(out.count('O', note) == 1);
  CHECK(out.count('O', 60) == 0);
  engine.setPressed(1, false);
  CHECK(out.messages.size() == 16);
  engine.setPressed(1, false);
  CHECK(out.messages.size() == 16);
}

void overlapping_buttons_keep_notes_until_last_release() {
  Output out;
  const ButtonConfig buttons[] = {chordButton(2, cMajor), chordButton(3, overlap)};
  Controller<Output, 2> engine(out, buttons);
  engine.setPressed(0, true);
  engine.setPressed(1, true);
  CHECK(out.count('N', 60, 100) == 1);
  engine.setPressed(0, false);
  CHECK(out.count('O', 60) == 0);
  CHECK(out.count('O', 64) == 1);
  engine.setPressed(1, false);
  CHECK(out.count('O', 60) == 1);
  CHECK(out.count('O', 69) == 1);
}

void midi_channels_are_independent() {
  Output out;
  const ButtonConfig buttons[] = {chordButton(2, cMajor), chordButton(3, cMajor, 16)};
  Controller<Output, 2> engine(out, buttons);
  engine.setPressed(0, true);
  engine.setPressed(1, true);
  CHECK(out.count('N', 60, 100, 1) == 1);
  CHECK(out.count('N', 60, 100, 16) == 1);
  engine.setPressed(0, false);
  CHECK(out.count('O', 60, 0, 16) == 0);
  engine.setPressed(1, false);
  CHECK(out.count('O', 60, 0, 16) == 1);
}

void voicing_changes_only_changed_notes() {
  Output out;
  const ButtonConfig buttons[] = {chordButton(2, cMajor), chordButton(3, fMajor)};
  Controller<Output, 2> engine(out, buttons);
  engine.setPressed(0, true);
  engine.setPressed(1, true);
  out.messages.clear();
  engine.setVoicing(Voicing::Ninth);
  CHECK(out.messages.size() == 2);
  CHECK(out.count('N', 74, 120) == 1);
  CHECK(out.count('N', 79, 120) == 1);
  out.messages.clear();
  engine.setVoicing(Voicing::Major);
  CHECK(out.count('O', 71) == 1);
  CHECK(out.count('O', 74) == 1);
  CHECK(out.count('O', 76) == 1);
  CHECK(out.count('O', 79) == 1);
  CHECK(out.count('N', 77, 120) == 1);
  // C's new 72 was already held by F: neither off nor a new on.
  CHECK(out.count('O', 72) == 0);
  CHECK(out.count('N', 72, 120) == 0);
  CHECK(out.messages.size() == 5);
  out.messages.clear();
  engine.setPressed(1, false);
  CHECK(out.count('O', 72) == 0);
  engine.setPressed(0, false);
  CHECK(out.count('O', 72) == 1);
}

void highest_pitch_is_accented() {
  Output out;
  const ButtonConfig buttons[] = {chordButton(2, unsorted)};
  Controller<Output, 1> engine(out, buttons);
  engine.setPressed(0, true);
  CHECK(out.count('N', 80, 120) == 1);
  CHECK(out.count('N', 70, 100) == 1);
}

void cc_buttons_and_programs() {
  Output out;
  const ButtonConfig buttons[] = {
      ccButton(2, 64), ccButton(3, 91, true, 2, 100, 10), programButton(4, 127, 16)};
  Controller<Output, 3> engine(out, buttons);
  engine.setPressed(0, true);
  engine.setPressed(0, false);
  CHECK(out.count('C', 64, 127) == 1);
  CHECK(out.count('C', 64, 0) == 1);
  engine.setPressed(1, true);
  engine.setPressed(1, false);
  CHECK(out.count('C', 91, 100, 2) == 1);
  CHECK(out.count('C', 91, 10, 2) == 0);
  engine.setPressed(1, true);
  CHECK(out.count('C', 91, 10, 2) == 1);
  engine.setPressed(2, true);
  engine.setPressed(2, false);
  CHECK(out.count('P', 127, 0, 16) == 1);
}

void panic_stops_sound_and_requires_repress() {
  Output out;
  const ButtonConfig buttons[] = {
      chordButton(2, cMajor), ccButton(3, 91, true), panicButton(4)};
  Controller<Output, 3> engine(out, buttons);
  engine.setPressed(0, true);
  engine.setPressed(1, true);
  out.messages.clear();
  engine.setPressed(2, true);
  CHECK(out.count('O', 60) == 1);
  CHECK(out.count('C', 91, 0) == 1);
  for (uint8_t channel = 1; channel <= 16; ++channel) {
    CHECK(out.count('C', 64, 0, channel) == 1);
    CHECK(out.count('C', 123, 0, channel) == 1);
    CHECK(out.count('C', 120, 0, channel) == 1);
  }
  out.messages.clear();
  engine.setPressed(0, true);
  engine.setVoicing(Voicing::Ninth);
  CHECK(out.messages.empty());
  engine.setPressed(0, false);
  engine.setPressed(0, true);
  CHECK(out.messages.size() == 5);
}

void disconnect_reset_allows_a_fresh_press() {
  Output out;
  const ButtonConfig buttons[] = {chordButton(2, cMajor)};
  Controller<Output, 1> engine(out, buttons);
  engine.setPressed(0, true);
  out.messages.clear();
  engine.reset();
  CHECK(out.messages.empty());
  engine.setPressed(0, true);
  CHECK(out.messages.size() == 4);
}

void button_bounce_and_startup() {
  DebouncedButton button;
  button.begin(false, 0);
  CHECK(!button.update(true, 10, 25));
  CHECK(!button.update(false, 20, 25));
  CHECK(!button.update(true, 21, 25));
  CHECK(!button.update(true, 45, 25));
  CHECK(button.update(true, 46, 25));
  CHECK(button.update(false, 50, 25));
  CHECK(!button.update(false, 75, 25));
  button.begin(true, 100);
  CHECK(!button.update(true, 124, 25));
  CHECK(button.update(true, 125, 25));
}

void debounce_handles_clock_rollover() {
  DebouncedButton button;
  button.begin(false, UINT32_MAX - 20);
  CHECK(!button.update(true, UINT32_MAX - 10, 25));
  CHECK(!button.update(true, 13, 25));
  CHECK(button.update(true, 14, 25));
}

void analog_starts_at_actual_position_and_filters() {
  AnalogFilter pot;
  pot.begin(0);
  CHECK(pot.midiValue() == 0);
  pot.begin(512);
  CHECK(pot.midiValue() == 64);
  pot.begin(1023);
  CHECK(pot.midiValue() == 127);
  pot.begin(512);
  pot.update(1023);
  CHECK(pot.midiValue() > 64 && pot.midiValue() < 127);
  for (int i = 0; i < 200; ++i) pot.update(1023);
  CHECK(pot.midiValue() == 127);
  for (int i = 0; i < 200; ++i) pot.update(0);
  CHECK(pot.midiValue() == 0);
}

void voicing_thresholds_and_settling() {
  CHECK(voicingForReading(358) == Voicing::Ninth);
  CHECK(voicingForReading(359) == Voicing::Major);
  CHECK(voicingForReading(664) == Voicing::Major);
  CHECK(voicingForReading(665) == Voicing::Seventh);
  VoicingSelector selector;
  selector.begin(1023, 0);
  CHECK(selector.update(100, 5, 40) == Voicing::Seventh);
  CHECK(selector.update(100, 44, 40) == Voicing::Seventh);
  CHECK(selector.update(100, 45, 40) == Voicing::Ninth);
  CHECK(selector.update(500, 50, 40) == Voicing::Ninth);
  CHECK(selector.update(100, 60, 40) == Voicing::Ninth);
  CHECK(selector.update(500, 70, 40) == Voicing::Ninth);
  CHECK(selector.update(500, 109, 40) == Voicing::Ninth);
  CHECK(selector.update(500, 110, 40) == Voicing::Major);
}

void invalid_midi_configuration_is_rejected() {
  CHECK(validButton(chordButton(2, cMajor)));
  CHECK(!validButton(chordButton(2, cMajor, 0)));
  CHECK(!validButton(chordButton(2, cMajor, 17)));
  CHECK(!validButton(ccButton(2, 128)));
  const ChordBank bad = {{{{60, 60}, 2}, {{60}, 1}, {{60}, 1}}};
  CHECK(!validButton(chordButton(2, bad)));
}
}  // namespace

int main() {
  const struct { const char* name; void (*run)(); } tests[] = {
      {"default chords and release", default_chords_and_release},
      {"overlapping notes", overlapping_buttons_keep_notes_until_last_release},
      {"independent channels", midi_channels_are_independent},
      {"voicing note differences", voicing_changes_only_changed_notes},
      {"highest note velocity", highest_pitch_is_accented},
      {"CC and program buttons", cc_buttons_and_programs},
      {"panic and repress", panic_stops_sound_and_requires_repress},
      {"disconnect reset", disconnect_reset_allows_a_fresh_press},
      {"button debounce and startup", button_bounce_and_startup},
      {"clock rollover", debounce_handles_clock_rollover},
      {"analog startup and filtering", analog_starts_at_actual_position_and_filters},
      {"voicing thresholds and settling", voicing_thresholds_and_settling},
      {"configuration validation", invalid_midi_configuration_is_rejected},
  };
  for (const auto& test : tests) {
    test.run();
    std::cout << "PASS: " << test.name << '\n';
  }
  std::cout << sizeof(tests) / sizeof(tests[0]) << " tests passed\n";
}

