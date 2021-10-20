#!/usr/bin/python3
import dballe
import unittest


class TestVersion(unittest.TestCase):
    def test_version(self):
        version = dballe.__version__
