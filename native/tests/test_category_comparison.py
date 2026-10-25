"""Ensure CPU-tolerant positions cannot hide other rendering regressions."""
from pathlib import Path
import struct
import unittest
from compare_category import compare, records


class CategoryComparisonTest(unittest.TestCase):
    def setUp(self):
        self.reference = list(records(Path(__file__).with_name("category_geometry.bin").read_bytes()))

    def altered(self, offset, value):
        actual = list(self.reference)
        frame, count, indices, data = actual[0]
        modified = bytearray(data)
        modified[offset:offset + len(value)] = value
        actual[0] = frame, count, indices, bytes(modified)
        return actual

    def test_subpixel_position(self):
        x = struct.unpack_from("<f", self.reference[0][3])[0]
        compare(self.reference, self.altered(0, struct.pack("<f", x + 0.0005)))
        with self.assertRaises(ValueError):
            compare(self.reference, self.altered(0, struct.pack("<f", x + 0.01)))

    def test_uv_color_and_indices_are_exact(self):
        for offset in (8, 16, self.reference[0][1] * 20):
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                compare(self.reference, self.altered(offset, bytes([self.reference[0][3][offset] ^ 1])))

    def test_missing_frame_and_nan_fail(self):
        with self.assertRaises(ValueError):
            compare(self.reference, self.reference[:-1])
        with self.assertRaises(ValueError):
            compare(self.reference, self.altered(0, struct.pack("<f", float("nan"))))


if __name__ == "__main__":
    unittest.main()
