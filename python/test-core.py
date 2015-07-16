#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
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
