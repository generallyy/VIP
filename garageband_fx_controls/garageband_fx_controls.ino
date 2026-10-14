// Elegoo Uno R3 controller for GarageBand pan and reverb automation.
//
// Wiring for each 10k linear potentiometer:
//   Pan:    center pin -> A0, outside pins -> 5V and GND
//   Reverb: center pin -> A1, outside pins -> 5V and GND
//
// The Uno R3 is a USB-serial device, so this sketch emits readable control
// messages for midi_bridge.py to convert into MIDI Control Change messages.

const unsigned long SAMPLE_INTERVAL_MS = 5;
const byte FILTER_DIVISOR = 8;
const byte FILTER_SCALE = 16;

struct Control {
  byte analogPin;
  byte controllerNumber;
  long scaledFilteredReading;
  int lastMidiValue;
};

Control controls[] = {
  {A0, 10, 0, -1},  // MIDI CC10: pan
  {A1, 91, 0, -1}   // MIDI CC91: reverb send
};

const byte CONTROL_COUNT = sizeof(controls) / sizeof(controls[0]);
unsigned long lastSampleTime = 0;

byte readingToMidiValue(long reading) {
  // Dividing by eight maps the Uno's 10-bit ADC range (0-1023) to MIDI's
  // seven-bit range (0-127), including both endpoints.
  return constrain(reading / 8, 0, 127);
}

void sendControlChange(byte controllerNumber, byte value) {
  Serial.print(F("CC "));
  Serial.print(controllerNumber);
  Serial.print(F(" "));
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);

  // Start each low-pass filter at the actual knob position so startup does
  // not produce a slow sweep from zero to the selected value.
  for (byte i = 0; i < CONTROL_COUNT; i++) {
    const int reading = analogRead(controls[i].analogPin);
    controls[i].scaledFilteredReading = (long)reading * FILTER_SCALE;
    const byte initialValue = readingToMidiValue(reading);
    controls[i].lastMidiValue = initialValue;
    sendControlChange(controls[i].controllerNumber, initialValue);
  }
}

void loop() {
  const unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleTime = now;

  for (byte i = 0; i < CONTROL_COUNT; i++) {
    Control &control = controls[i];
    const int reading = analogRead(control.analogPin);

    // An integer low-pass filter makes recorded automation smoother and
    // prevents normal ADC noise from flooding GarageBand with MIDI events.
    const long scaledReading = (long)reading * FILTER_SCALE;
    control.scaledFilteredReading +=
        (scaledReading - control.scaledFilteredReading) / FILTER_DIVISOR;

    const long filteredReading =
        (control.scaledFilteredReading + FILTER_SCALE / 2) / FILTER_SCALE;
    const byte midiValue = readingToMidiValue(filteredReading);
    if (midiValue != control.lastMidiValue) {
      control.lastMidiValue = midiValue;
      sendControlChange(control.controllerNumber, midiValue);
    }
  }
}
