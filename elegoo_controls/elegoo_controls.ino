// Elegoo Uno R3 MIDI chord controller
//
// Wiring:
//   C chord button: pin 2 to GND
//   F chord button: pin 3 to GND
//   Potentiometer: center pin to A0, outside pins to 5V and GND
//   Light: pin 13 (the Uno's built-in LED also uses this pin)

const byte BUTTON_PINS[] = {2, 3};
const byte BUTTON_COUNT = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);
const byte POT_PIN = A0;
const byte LIGHT_PIN = 13;

// Middle 30% of the 0-1023 range (35% through 65%).
const int POT_CENTER_LOW = 359;
const int POT_CENTER_HIGH = 664;

const unsigned long DEBOUNCE_MS = 25;
const unsigned long POT_SETTLE_MS = 40;
const byte NOTE_VELOCITY = 100;
const byte TOP_NOTE_VELOCITY = 120;
const byte NO_NOTE = 255;
const byte MAX_CHORD_NOTES = 5;

enum PotMode : byte {
  MAJOR_NINTH = 0,
  MAJOR_TRIAD = 1,
  MAJOR_SEVENTH = 2
};

// MIDI note numbers:
//   Button 0: C4 E4 G4 [B4 D5]
//   Button 1: F4 A4 C5 [E5 G5]
// The array order is button, potentiometer mode, chord note.
const byte CHORDS[BUTTON_COUNT][3][MAX_CHORD_NOTES] = {
  {
    {60, 64, 67, 71, 74},              // C major ninth
    {60, 64, 67, 72, NO_NOTE},          // C major with octave root
    {60, 64, 67, 71, NO_NOTE}           // C major seventh
  },
  {
    {65, 69, 72, 76, 79},              // F major ninth
    {65, 69, 72, 77, NO_NOTE},          // F major with octave root
    {65, 69, 72, 76, NO_NOTE}           // F major seventh
  }
};

bool lastReading[BUTTON_COUNT];
bool stableState[BUTTON_COUNT];
unsigned long lastChangeTime[BUTTON_COUNT];
byte noteUseCount[128];

PotMode activePotMode;
PotMode pendingPotMode;
unsigned long pendingPotModeSince;

PotMode modeForPotValue(int value) {
  if (value < POT_CENTER_LOW) {
    return MAJOR_NINTH;
  }
  if (value > POT_CENTER_HIGH) {
    return MAJOR_SEVENTH;
  }
  return MAJOR_TRIAD;
}

void sendNoteMessage(bool turnOn, byte note, byte velocity) {
  if (turnOn) {
    Serial.print(F("ON "));
    Serial.print(note);
    Serial.print(F(" "));
    Serial.println(velocity);
  } else {
    Serial.print(F("OFF "));
    Serial.print(note);
    Serial.println(F(" 0"));
  }
}

void startNote(byte note, byte velocity) {
  if (noteUseCount[note] == 0) {
    sendNoteMessage(true, note, velocity);
  }
  noteUseCount[note]++;
}

void stopNote(byte note) {
  if (noteUseCount[note] == 0) {
    return;
  }

  noteUseCount[note]--;
  if (noteUseCount[note] == 0) {
    sendNoteMessage(false, note, 0);
  }
}

byte highestNote(byte buttonIndex, PotMode mode) {
  byte highest = 0;
  for (byte i = 0; i < MAX_CHORD_NOTES; i++) {
    const byte note = CHORDS[buttonIndex][mode][i];
    if (note != NO_NOTE && note > highest) {
      highest = note;
    }
  }
  return highest;
}

byte velocityForNote(byte buttonIndex, PotMode mode, byte note) {
  return note == highestNote(buttonIndex, mode)
      ? TOP_NOTE_VELOCITY
      : NOTE_VELOCITY;
}

void sendChord(byte buttonIndex, PotMode mode, bool turnOn) {
  for (byte i = 0; i < MAX_CHORD_NOTES; i++) {
    const byte note = CHORDS[buttonIndex][mode][i];
    if (note != NO_NOTE) {
      if (turnOn) {
        startNote(note, velocityForNote(buttonIndex, mode, note));
      } else {
        stopNote(note);
      }
    }
  }
}

bool chordContainsNote(byte buttonIndex, PotMode mode, byte targetNote) {
  for (byte i = 0; i < MAX_CHORD_NOTES; i++) {
    if (CHORDS[buttonIndex][mode][i] == targetNote) {
      return true;
    }
  }
  return false;
}

void changePotMode(PotMode newMode) {
  // Change only notes that differ. Shared chord tones keep sounding and are
  // not sent another Note On message.
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    if (stableState[i] == LOW) {
      for (byte noteIndex = 0; noteIndex < MAX_CHORD_NOTES; noteIndex++) {
        const byte oldNote = CHORDS[i][activePotMode][noteIndex];
        if (oldNote != NO_NOTE && !chordContainsNote(i, newMode, oldNote)) {
          stopNote(oldNote);
        }
      }
    }
  }

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    if (stableState[i] == LOW) {
      for (byte noteIndex = 0; noteIndex < MAX_CHORD_NOTES; noteIndex++) {
        const byte newNote = CHORDS[i][newMode][noteIndex];
        if (newNote != NO_NOTE &&
            !chordContainsNote(i, activePotMode, newNote)) {
          startNote(newNote, velocityForNote(i, newMode, newNote));
        }
      }
    }
  }

  activePotMode = newMode;
}

void updatePotentiometer(unsigned long now) {
  const PotMode observedMode = modeForPotValue(analogRead(POT_PIN));

  if (observedMode != pendingPotMode) {
    pendingPotMode = observedMode;
    pendingPotModeSince = now;
  }

  // Require a stable reading briefly so analog noise does not retrigger chords.
  if (pendingPotMode != activePotMode &&
      now - pendingPotModeSince >= POT_SETTLE_MS) {
    changePotMode(pendingPotMode);
  }

  digitalWrite(LIGHT_PIN, activePotMode == MAJOR_TRIAD ? HIGH : LOW);
}

void updateButtons(unsigned long now) {
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    const bool reading = digitalRead(BUTTON_PINS[i]);

    if (reading != lastReading[i]) {
      lastReading[i] = reading;
      lastChangeTime[i] = now;
    }

    if (now - lastChangeTime[i] >= DEBOUNCE_MS &&
        reading != stableState[i]) {
      stableState[i] = reading;

      // INPUT_PULLUP means LOW is pressed and HIGH is released.
      sendChord(i, activePotMode, stableState[i] == LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastReading[i] = digitalRead(BUTTON_PINS[i]);
    stableState[i] = HIGH;
    lastChangeTime[i] = millis();
  }

  pinMode(LIGHT_PIN, OUTPUT);

  activePotMode = modeForPotValue(analogRead(POT_PIN));
  pendingPotMode = activePotMode;
  pendingPotModeSince = millis();
  digitalWrite(LIGHT_PIN, activePotMode == MAJOR_TRIAD ? HIGH : LOW);
}

void loop() {
  const unsigned long now = millis();
  updatePotentiometer(now);
  updateButtons(now);

}
