#!/usr/bin/python3
import dballe
import unittest


class Describe(unittest.TestCase):
    def testLevel(self):
        self.assertIn("surface", dballe.describe_level(1))

    def testTrange(self):
        self.assertIn("Accumulation", dballe.describe_trange(1))


if __name__ == "__main__":
    from testlib import main
    main("test_core")
