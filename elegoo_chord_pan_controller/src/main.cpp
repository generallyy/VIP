#include <Arduino.h>

// Elegoo Uno R3 chord and pan MIDI controller.
//
// Wiring:
//   Fmaj7 button: pin 2 to GND
//   Cmaj7 button: pin 3 to GND
//   Pan potentiometer: center pin to A0, outside pins to 5V and GND
//
// The Uno R3 is a USB-serial device. This program emits readable messages for
// midi_bridge.py (macOS) or run_midi.ps1 (Windows) to convert to MIDI.

const byte BUTTON_PINS[] = {2, 3};
const byte BUTTON_COUNT = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);
const byte CHORD_NOTE_COUNT = 4;
const byte PAN_PIN = A0;
const byte PAN_CONTROLLER = 10;

const unsigned long DEBOUNCE_MS = 25;
const unsigned long POT_SAMPLE_INTERVAL_MS = 5;
const byte FILTER_DIVISOR = 8;
const byte FILTER_SCALE = 16;
const byte NOTE_VELOCITY = 100;
const byte TOP_NOTE_VELOCITY = 120;

// MIDI note numbers. Button 0 is pin 2; button 1 is pin 3.
const byte CHORDS[BUTTON_COUNT][CHORD_NOTE_COUNT] = {
    {65, 69, 72, 76}, // Fmaj7: F4 A4 C5 E5
    {60, 64, 67, 71}  // Cmaj7: C4 E4 G4 B4
};

bool lastButtonReading[BUTTON_COUNT];
bool stableButtonState[BUTTON_COUNT];
unsigned long lastButtonChangeTime[BUTTON_COUNT];

long scaledFilteredPanReading = 0;
int lastPanValue = -1;
unsigned long lastPotSampleTime = 0;

byte readingToMidiValue(long reading) {
  // Dividing by eight maps the Uno's 10-bit ADC range to MIDI's 0-127 range.
  return constrain(reading / 8, 0, 127);
}

void sendNoteMessage(bool turnOn, byte note, byte velocity) {
  Serial.print(turnOn ? F("ON ") : F("OFF "));
  Serial.print(note);
  Serial.print(F(" "));
  Serial.println(turnOn ? velocity : 0);
}

void sendChord(byte buttonIndex, bool turnOn) {
  for (byte i = 0; i < CHORD_NOTE_COUNT; i++) {
    const byte velocity =
        i == CHORD_NOTE_COUNT - 1 ? TOP_NOTE_VELOCITY : NOTE_VELOCITY;
    sendNoteMessage(turnOn, CHORDS[buttonIndex][i], velocity);
  }
}

void sendPan(byte value) {
  Serial.print(F("CC "));
  Serial.print(PAN_CONTROLLER);
  Serial.print(F(" "));
  Serial.println(value);
}

void updateButtons(unsigned long now) {
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    const bool reading = digitalRead(BUTTON_PINS[i]);

    if (reading != lastButtonReading[i]) {
      lastButtonReading[i] = reading;
      lastButtonChangeTime[i] = now;
    }

    if (now - lastButtonChangeTime[i] >= DEBOUNCE_MS &&
        reading != stableButtonState[i]) {
      stableButtonState[i] = reading;

      // INPUT_PULLUP means LOW is pressed and HIGH is released.
      sendChord(i, stableButtonState[i] == LOW);
    }
  }
}

void updatePan(unsigned long now) {
  if (now - lastPotSampleTime < POT_SAMPLE_INTERVAL_MS) {
    return;
  }
  lastPotSampleTime = now;

  const int reading = analogRead(PAN_PIN);
  const long scaledReading = (long)reading * FILTER_SCALE;
  scaledFilteredPanReading +=
      (scaledReading - scaledFilteredPanReading) / FILTER_DIVISOR;

  const long filteredReading =
      (scaledFilteredPanReading + FILTER_SCALE / 2) / FILTER_SCALE;
  const byte panValue = readingToMidiValue(filteredReading);

  if (panValue != lastPanValue) {
    lastPanValue = panValue;
    sendPan(panValue);
  }
}

void setup() {
  Serial.begin(115200);

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastButtonReading[i] = digitalRead(BUTTON_PINS[i]);
    stableButtonState[i] = HIGH;
    lastButtonChangeTime[i] = millis();
  }

  // Begin at the knob's actual position instead of slowly sweeping from zero.
  const int initialReading = analogRead(PAN_PIN);
  scaledFilteredPanReading = (long)initialReading * FILTER_SCALE;
  lastPanValue = readingToMidiValue(initialReading);
  sendPan(lastPanValue);
}

void loop() {
  const unsigned long now = millis();
  updateButtons(now);
  updatePan(now);
}
