import unittest

from midi_bridge import MidiEvent, parse_serial_message


class ParseSerialMessageTests(unittest.TestCase):
    def test_parses_note_on(self):
        self.assertEqual(
            parse_serial_message("ON 65 100\n"),
            MidiEvent("note_on", 65, 100),
        )

    def test_parses_note_off(self):
        self.assertEqual(
            parse_serial_message("OFF 65 0"),
            MidiEvent("note_off", 65, 0),
        )

    def test_parses_control_change(self):
        self.assertEqual(
            parse_serial_message("CC 10 64"),
            MidiEvent("control_change", 10, 64),
        )

    def test_accepts_midi_boundaries(self):
        self.assertEqual(
            parse_serial_message("CC 0 127"),
            MidiEvent("control_change", 0, 127),
        )

    def test_rejects_unknown_message_type(self):
        self.assertIsNone(parse_serial_message("PITCH 60 100"))

    def test_rejects_wrong_field_count(self):
        self.assertIsNone(parse_serial_message("ON 60"))

    def test_rejects_non_numeric_data(self):
        self.assertIsNone(parse_serial_message("CC pan center"))

    def test_rejects_out_of_range_data(self):
        self.assertIsNone(parse_serial_message("ON 128 100"))
        self.assertIsNone(parse_serial_message("CC 10 -1"))


if __name__ == "__main__":
    unittest.main()
