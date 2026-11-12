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
controller::VoicingSelector selector;
bool rawButtonPressed[config::kButtonCount];
bool buttonPressed[config::kButtonCount];
uint16_t rawVoicingValue = 0;
controller::Voicing activeVoicing = controller::Voicing::Seventh;
uint32_t lastSampleAt = 0;
bool wasConnected = false;

void debugVoicing() {
  const char* name = activeVoicing == controller::Voicing::Ninth ? "NINTH"
      : (activeVoicing == controller::Voicing::Major ? "NORMAL" : "SEVENTH");
  debugPrint("TAG pin=%u raw=%u mode=%s", config::kVoicingPin,
             rawVoicingValue, name);
}

void updateVoicing(uint32_t now) {
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

void debugStatus() {
  debugPrint("STATUS usb=%s buttons=%u", wasConnected ? "READY" : "OFFLINE",
             static_cast<unsigned>(config::kButtonCount));
  for (size_t i = 0; i < config::kButtonCount; ++i) debugButton(i);
  debugVoicing();
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
  rawVoicingValue = analogRead(config::kVoicingPin);
  selector.begin(rawVoicingValue, now);
  updateVoicing(now);
  lastSampleAt = now;
}

void loop() {
  const uint32_t now = millis();
  const bool connected = usb_configuration != 0;
  if (connected != wasConnected) {
    controls.reset();
    wasConnected = connected;
  }

  // Bound receive/command work so incoming traffic cannot starve the controls.
  for (uint8_t i = 0; i < 16 && usbMIDI.read(); ++i) {}
  if (now - lastSampleAt >= config::kAnalogSampleMs) {
    lastSampleAt = now;
    updateVoicing(now);
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
    if (command == 's' || command == 'S') debugStatus();
  }
  if (connected) {
    usbMIDI.send_now();
  }
}
