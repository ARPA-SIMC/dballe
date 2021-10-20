#!/usr/bin/python3
import dballe
import unittest


class Describe(unittest.TestCase):
    def testLevel(self):
        self.assertIn("surface", dballe.describe_level(1))

    def testTrange(self):
        self.assertIn("Accumulation", dballe.describe_trange(1))
