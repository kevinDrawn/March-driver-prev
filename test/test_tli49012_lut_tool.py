import importlib.util
import math
from pathlib import Path
import unittest

path = Path(__file__).resolve().parents[1] / "tools/encoder/build_tli49012_lut.py"
spec = importlib.util.spec_from_file_location("lut_tool", path)
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class LutToolTests(unittest.TestCase):
    def test_reference_round_trip(self):
        pairs = []
        for direction in (-1, 1):
            for i in range(4096):
                reference = direction * i * math.tau / 4096
                measured = reference + 0.4 + 0.04 * math.sin(2 * reference) + 0.01 * math.cos(3 * reference)
                pairs.append((measured, reference))
        lut, coverage = tool.build(pairs)
        self.assertEqual(coverage, 128)
        for i in range(2048):
            reference = i * math.tau / 2048
            measured = (reference + 0.4 + 0.04 * math.sin(2 * reference) + 0.01 * math.cos(3 * reference)) % math.tau
            position = measured * 128 / math.tau
            index = int(position)
            error = lut[index] + (position - index) * tool.delta(lut[(index + 1) % 128], lut[index])
            self.assertLess(abs(tool.delta(measured - error, reference)), 0.001)

    def test_reject_incomplete_or_invalid(self):
        for pairs in ([], [(0.0, 0.0)], [(float("nan"), 0.0)]):
            with self.assertRaises(ValueError):
                tool.build(pairs)


if __name__ == "__main__":
    unittest.main()
