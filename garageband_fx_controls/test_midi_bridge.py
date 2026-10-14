import unittest

from midi_bridge import parse_control_change


class ParseControlChangeTests(unittest.TestCase):
    def test_parses_valid_message(self):
        self.assertEqual(parse_control_change("CC 10 64\n"), (10, 64))

    def test_accepts_midi_boundaries(self):
        self.assertEqual(parse_control_change("CC 0 127"), (0, 127))

    def test_rejects_wrong_message_type(self):
        self.assertIsNone(parse_control_change("NOTE 60 100"))

    def test_rejects_wrong_field_count(self):
        self.assertIsNone(parse_control_change("CC 10"))

    def test_rejects_non_numeric_data(self):
        self.assertIsNone(parse_control_change("CC pan center"))

    def test_rejects_out_of_range_controller(self):
        self.assertIsNone(parse_control_change("CC 128 64"))

    def test_rejects_out_of_range_value(self):
        self.assertIsNone(parse_control_change("CC 10 -1"))


if __name__ == "__main__":
    unittest.main()
