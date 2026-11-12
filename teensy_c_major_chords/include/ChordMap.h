#pragma once

#include <Controller.h>

namespace config {
using namespace controller;

// Banks follow the copied selector's order: ninth, normal triad, seventh.
// Every note stays in C major, including the flat ninths on E and B.
constexpr ChordBank kC = {{
    {{60, 64, 67, 71, 74}, 5},  // Cmaj9
    {{60, 64, 67}, 3},          // C
    {{60, 64, 67, 71}, 4},      // Cmaj7
}};
constexpr ChordBank kDm = {{
    {{62, 65, 69, 72, 76}, 5},  // Dm9
    {{62, 65, 69}, 3},          // Dm
    {{62, 65, 69, 72}, 4},      // Dm7
}};
constexpr ChordBank kEm = {{
    {{64, 67, 71, 74, 77}, 5},  // Em7(b9)
    {{64, 67, 71}, 3},          // Em
    {{64, 67, 71, 74}, 4},      // Em7
}};
constexpr ChordBank kF = {{
    {{65, 69, 72, 76, 79}, 5},  // Fmaj9
    {{65, 69, 72}, 3},          // F
    {{65, 69, 72, 76}, 4},      // Fmaj7
}};
constexpr ChordBank kG = {{
    {{67, 71, 74, 77, 81}, 5},  // G9 (dominant)
    {{67, 71, 74}, 3},          // G
    {{67, 71, 74, 77}, 4},      // G7
}};
constexpr ChordBank kAm = {{
    {{69, 72, 76, 79, 83}, 5},  // Am9
    {{69, 72, 76}, 3},          // Am
    {{69, 72, 76, 79}, 4},      // Am7
}};
constexpr ChordBank kBdim = {{
    {{71, 74, 77, 81, 84}, 5},  // Bm7b5(b9)
    {{71, 74, 77}, 3},          // Bdim
    {{71, 74, 77, 81}, 4},      // Bm7b5 (half-diminished)
}};

// Each button connects its pin to GND when pressed (INPUT_PULLUP).
constexpr ButtonConfig kButtons[] = {
    chordButton(2, kC),
    chordButton(3, kDm),
    chordButton(4, kEm),
    chordButton(5, kF),
    chordButton(6, kG),
    chordButton(7, kAm),
    chordButton(8, kBdim),
};
constexpr size_t kButtonCount = sizeof(kButtons) / sizeof(kButtons[0]);
}  // namespace config
