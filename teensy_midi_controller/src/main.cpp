#include <Arduino.h>
#include <Controller.h>
#include <Inputs.h>
#include <usb_dev.h>

#include <cstdarg>
#include <cstdio>

#include "ControllerConfig.h"

namespace {
// Never wait for a monitor or serial buffer space in the input/MIDI loop.
void debugPrint(const char* format, ...) {
  if (!config::kSerialDebug || !Serial) return;
  char line[160];
  const int prefix = snprintf(line, sizeof(line), "[%lu ms] ",
                              static_cast<unsigned long>(millis()));
  if (prefix < 0 || static_cast<size_t>(prefix) >= sizeof(line)) return;
  va_list arguments;
  va_start(arguments, format);
  const int body = vsnprintf(line + prefix, sizeof(line) - prefix, format, arguments);
  va_end(arguments);
  if (body < 0 || static_cast<size_t>(prefix + body) >= sizeof(line) - 1) return;
  const size_t length = static_cast<size_t>(prefix + body);
  line[length] = '\n';
  if (Serial.availableForWrite() >= static_cast<int>(length + 1)) {
    Serial.write(reinterpret_cast<const uint8_t*>(line), length + 1);
  }
}

// USB MIDI is the transport. Serial is only a diagnostic/command interface;
// the old serial-to-MIDI bridges must not run with this firmware.
class UsbMidiOutput {
 public:
  void noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    usbMIDI.sendNoteOn(note, velocity, channel);
  }
  void noteOff(uint8_t note, uint8_t channel) {
    usbMIDI.sendNoteOff(note, 0, channel);
  }
  void controlChange(uint8_t number, uint8_t value, uint8_t channel) {
    usbMIDI.sendControlChange(number, value, channel);
  }
  void programChange(uint8_t number, uint8_t channel) {
    usbMIDI.sendProgramChange(number, channel);
  }
};

UsbMidiOutput midi;
controller::Controller<UsbMidiOutput, config::kButtonCount> controls(midi, config::kButtons);
controller::DebouncedButton buttons[config::kButtonCount];
controller::AnalogFilter analogs[config::kAnalogCount];
controller::VoicingSelector selector;
bool rawButtonPressed[config::kButtonCount];
bool buttonPressed[config::kButtonCount];
uint16_t rawAnalogValues[config::kAnalogCount];
int lastDebugMidiValues[config::kAnalogCount];
int lastAnalogValues[config::kAnalogCount];
uint16_t rawVoicingValue = 0;
controller::Voicing activeVoicing = controller::Voicing::Seventh;
uint32_t lastSampleAt = 0;
uint32_t lastPotDebugAt = 0;
bool debugPortWasOpen = false;
bool wasConnected = false;

void invalidateAnalogValues() {
  for (auto& value : lastAnalogValues) value = -1;
}

void debugVoicing() {
  const char* name = activeVoicing == controller::Voicing::Ninth ? "NINTH"
      : (activeVoicing == controller::Voicing::Major ? "MAJOR" : "SEVENTH");
  debugPrint("VOICING pin=%u raw=%u mode=%s", config::kVoicingPin,
             rawVoicingValue, name);
}

void updateVoicing(uint32_t now) {
  if (!config::kEnableVoicing) return;
  const auto previousVoicing = activeVoicing;
  rawVoicingValue = analogRead(config::kVoicingPin);
  activeVoicing = selector.update(rawVoicingValue, now, config::kVoicingSettleMs);
  controls.setVoicing(activeVoicing);
  if (activeVoicing != previousVoicing) debugVoicing();
}

void debugButton(size_t index) {
  debugPrint("BUTTON pin=%u raw=%s state=%s ch=%u", config::kButtons[index].pin,
             rawButtonPressed[index] ? "LOW" : "HIGH",
             buttonPressed[index] ? "PRESSED" : "RELEASED",
             config::kButtons[index].channel);
}

void debugPot(size_t index) {
  const auto& analog = config::kAnalogs[index];
  debugPrint("POT pin=%u cc=%u ch=%u raw=%u filtered=%u midi=%u", analog.pin,
             analog.controller, analog.channel, rawAnalogValues[index],
             analogs[index].reading(), analogs[index].midiValue());
  lastDebugMidiValues[index] = analogs[index].midiValue();
}

void debugStatus() {
  debugPrint("STATUS usb=%s reverb=%u voicing=%u", wasConnected ? "READY" : "OFFLINE",
             MIDI_ENABLE_REVERB != 0, config::kEnableVoicing);
  for (size_t i = 0; i < config::kButtonCount; ++i) debugButton(i);
  for (size_t i = 0; i < config::kAnalogCount; ++i) {
    if (config::kAnalogs[i].enabled) debugPot(i);
  }
  if (config::kEnableVoicing) debugVoicing();
}

void updateDebug(uint32_t now) {
  if (!config::kSerialDebug) return;
  if (!Serial) {
    debugPortWasOpen = false;
    return;
  }
  // Start silently at the current knob positions when the monitor opens.
  if (!debugPortWasOpen) {
    debugPortWasOpen = true;
    for (size_t i = 0; i < config::kAnalogCount; ++i) {
      lastDebugMidiValues[i] = analogs[i].midiValue();
    }
    lastPotDebugAt = now;
    return;
  }
  if (now - lastPotDebugAt >= config::kPotDebugIntervalMs) {
    lastPotDebugAt = now;
    for (size_t i = 0; i < config::kAnalogCount; ++i) {
      if (config::kAnalogs[i].enabled &&
          analogs[i].midiValue() != lastDebugMidiValues[i]) {
        debugPot(i);
      }
    }
  }
}

void sendAnalogValues() {
  for (size_t i = 0; i < config::kAnalogCount; ++i) {
    const auto& analog = config::kAnalogs[i];
    if (!analog.enabled) continue;
    const auto value = analogs[i].midiValue();
    if (lastAnalogValues[i] != value) {
      midi.controlChange(analog.controller, value, analog.channel);
      lastAnalogValues[i] = value;
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  // Keep the original 0-1023 thresholds and filtering on the Teensy's ADC.
  analogReadResolution(10);
  analogReadAveraging(8);
  const uint32_t now = millis();
  pinMode(config::kLedPin, OUTPUT);
  digitalWrite(config::kLedPin, LOW);
  for (size_t i = 0; i < config::kButtonCount; ++i) {
    pinMode(config::kButtons[i].pin, INPUT_PULLUP);
    rawButtonPressed[i] = digitalRead(config::kButtons[i].pin) == LOW;
    buttons[i].begin(rawButtonPressed[i], now);
  }
  for (size_t i = 0; i < config::kAnalogCount; ++i) {
    if (config::kAnalogs[i].enabled) {
      rawAnalogValues[i] = analogRead(config::kAnalogs[i].pin);
      analogs[i].begin(rawAnalogValues[i]);
    }
  }
  if (config::kEnableVoicing) {
    rawVoicingValue = analogRead(config::kVoicingPin);
    selector.begin(rawVoicingValue, now);
    updateVoicing(now);
  }
  invalidateAnalogValues();
  lastSampleAt = now;
}

void loop() {
  const uint32_t now = millis();
  const bool connected = usb_configuration != 0;
  if (connected != wasConnected) {
    controls.reset();
    invalidateAnalogValues();
    wasConnected = connected;
  }

  // Bound receive/command work so incoming traffic cannot starve the controls.
  for (uint8_t i = 0; i < 16 && usbMIDI.read(); ++i) {}
  if (now - lastSampleAt >= config::kAnalogSampleMs) {
    lastSampleAt = now;
    updateVoicing(now);
    for (size_t i = 0; i < config::kAnalogCount; ++i) {
      if (config::kAnalogs[i].enabled) {
        rawAnalogValues[i] = analogRead(config::kAnalogs[i].pin);
        analogs[i].update(rawAnalogValues[i]);
      }
    }
  }
  for (size_t i = 0; i < config::kButtonCount; ++i) {
    rawButtonPressed[i] = digitalRead(config::kButtons[i].pin) == LOW;
    const bool pressed = buttons[i].update(
        rawButtonPressed[i], now, config::kDebounceMs);
    if (pressed != buttonPressed[i]) {
      buttonPressed[i] = pressed;
      debugButton(i);
    }
    // Follow the debounced physical button, independently of USB MIDI status.
    if (config::kButtons[i].pin == config::kLedButtonPin) {
      digitalWrite(config::kLedPin, pressed ? HIGH : LOW);
    }
    if (connected) controls.setPressed(i, pressed);
  }
  for (uint8_t i = 0; i < 16 && Serial.available(); ++i) {
    const char command = Serial.read();
    if (connected && (command == 'p' || command == 'P')) {
      controls.panic();
    }
    if (command == 'r' || command == 'R') {
      invalidateAnalogValues();
    }
    if (command == 's' || command == 'S') debugStatus();
  }
  if (connected) {
    sendAnalogValues();
    usbMIDI.send_now();
  }
  updateDebug(now);
}
