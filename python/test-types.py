#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import datetime
import unittest
import warnings

class TestLevel(unittest.TestCase):
    def testCreateEmpty(self):
        # structseq constructors have two arguments: sequence, dict=NULL)
        # this is undocumented, and possibly subject to change (see
        # https://bugs.python.org/issue1820)
        l = dballe.Level((None, None, None, None))
        self.assertEqual(l, (None, None, None, None))
        self.assertIsNone(l.ltype1)
        self.assertIsNone(l.l1)
        self.assertIsNone(l.ltype2)
        self.assertIsNone(l.l2)

    def testCreateFull(self):
        l = dballe.Level((1, 2, 3, 4))
        self.assertEqual(l, (1, 2, 3, 4))
        self.assertEqual(l.ltype1, 1)
        self.assertEqual(l.l1, 2)
        self.assertEqual(l.ltype2, 3)
        self.assertEqual(l.l2, 4)


class TestTrange(unittest.TestCase):
    def testCreateEmpty(self):
        t = dballe.Trange((None, None, None))
        self.assertEqual(t, (None, None, None))
        self.assertIsNone(t.pind)
        self.assertIsNone(t.p1)
        self.assertIsNone(t.p2)

    def testCreateFull(self):
        t = dballe.Trange((1, 2, 3))
        self.assertEqual(t, (1, 2, 3))
        self.assertEqual(t.pind, 1)
        self.assertEqual(t.p1, 2)
        self.assertEqual(t.p2, 3)


if __name__ == "__main__":
    from testlib import main
    main("test_types")
