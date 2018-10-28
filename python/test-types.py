import dballe
import unittest
import sys


@unittest.skipIf(sys.version_info[0] < 3, "python3 only")
class TestLevel(unittest.TestCase):
    def testCreateEmpty(self):
        # structseq constructors have two arguments: sequence, dict=NULL)
        # this is undocumented, and possibly subject to change (see
        # https://bugs.python.org/issue1820)
        lev = dballe.Level(None, None, None, None)
        self.assertTrue(lev == lev)
        self.assertTrue(lev <= lev)
        self.assertEqual(lev, (None, None, None, None))
        self.assertEqual(lev, dballe.Level(None, None, None, None))
        self.assertIsNone(lev.ltype1)
        self.assertIsNone(lev.l1)
        self.assertIsNone(lev.ltype2)
        self.assertIsNone(lev.l2)

    def testCreateFull(self):
        lev = dballe.Level(1, 2, 3, 4)
        self.assertEqual(lev, (1, 2, 3, 4))
        self.assertEqual(lev, dballe.Level(1, 2, 3, 4))
        self.assertEqual(lev.ltype1, 1)
        self.assertEqual(lev.l1, 2)
        self.assertEqual(lev.ltype2, 3)
        self.assertEqual(lev.l2, 4)


@unittest.skipIf(sys.version_info[0] < 3, "python3 only")
class TestTrange(unittest.TestCase):
    def testCreateEmpty(self):
        t = dballe.Trange(None, None, None)
        self.assertEqual(t, (None, None, None))
        self.assertIsNone(t.pind)
        self.assertIsNone(t.p1)
        self.assertIsNone(t.p2)

    def testCreateFull(self):
        t = dballe.Trange(1, 2, 3)
        self.assertEqual(t, (1, 2, 3))
        self.assertEqual(t.pind, 1)
        self.assertEqual(t.p1, 2)
        self.assertEqual(t.p2, 3)


@unittest.skipIf(sys.version_info[0] < 3, "python3 only")
class TestStation(unittest.TestCase):
    def testCreateEmpty(self):
        t = dballe.Station((None, None, None, None))
        self.assertEqual(t, (None, None, None, None))
        self.assertIsNone(t.report)
        self.assertIsNone(t.lat)
        self.assertIsNone(t.lon)
        self.assertIsNone(t.ident)

    def testCreateFull(self):
        t = dballe.Station(("foo", 3.0, 4.0, "bar"))
        self.assertEqual(t, ("foo", 3.0, 4.0, "bar"))
        self.assertEqual(t.report, "foo")
        self.assertEqual(t.lat, 3.0)
        self.assertEqual(t.lon, 4.0)
        self.assertEqual(t.ident, "bar")


@unittest.skipIf(sys.version_info[0] < 3, "python3 only")
class TestDBStation(unittest.TestCase):
    def testCreateEmpty(self):
        t = dballe.DBStation((None, None, None, None, None))
        self.assertEqual(t, (None, None, None, None, None))
        self.assertIsNone(t.report)
        self.assertIsNone(t.id)
        self.assertIsNone(t.lat)
        self.assertIsNone(t.lon)
        self.assertIsNone(t.ident)

    def testCreateFull(self):
        t = dballe.DBStation(("foo", 2, 3.0, 4.0, "bar"))
        self.assertEqual(t, ("foo", 2, 3.0, 4.0, "bar"))
        self.assertEqual(t.report, "foo")
        self.assertEqual(t.id, 2)
        self.assertEqual(t.lat, 3.0)
        self.assertEqual(t.lon, 4.0)
        self.assertEqual(t.ident, "bar")


if __name__ == "__main__":
    from testlib import main
    main("test-types")
