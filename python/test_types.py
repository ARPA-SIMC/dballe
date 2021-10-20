import dballe
import unittest
import datetime


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


class StationTestMixin:
    def assertContents(self, init_tuple):
        t = self.Station(*init_tuple)
        self.assertEqual(t, init_tuple)
        self.assertEqual(t, self.Station(*init_tuple))
        self.assertCountEqual([t], [self.Station(*init_tuple)])
        return t


class TestStation(StationTestMixin, unittest.TestCase):
    Station = dballe.Station

    def testCreateEmpty(self):
        t = self.assertContents((None, None, None, None))
        self.assertIsNone(t.report)
        self.assertIsNone(t.lat)
        self.assertIsNone(t.lon)
        self.assertIsNone(t.ident)

    def testCreatePartial(self):
        t = self.assertContents(("foo", 3.0, 4.0, None))
        self.assertEqual(t.report, "foo")
        self.assertEqual(t.lat, 3.0)
        self.assertEqual(t.lon, 4.0)
        self.assertIsNone(t.ident)

    def testCreateFull(self):
        t = self.assertContents(("foo", 3.0, 4.0, "bar"))
        self.assertEqual(t.report, "foo")
        self.assertEqual(t.lat, 3.0)
        self.assertEqual(t.lon, 4.0)
        self.assertEqual(t.ident, "bar")


class TestDBStation(StationTestMixin, unittest.TestCase):
    Station = dballe.DBStation

    def testCreateEmpty(self):
        t = self.assertContents((None, None, None, None, None))
        self.assertIsNone(t.report)
        self.assertIsNone(t.id)
        self.assertIsNone(t.lat)
        self.assertIsNone(t.lon)
        self.assertIsNone(t.ident)

    def testCreatePartial(self):
        t = self.assertContents(("foo", 2, 3.0, 4.0, None))
        self.assertEqual(t.report, "foo")
        self.assertEqual(t.id, 2)
        self.assertEqual(t.lat, 3.0)
        self.assertEqual(t.lon, 4.0)
        self.assertIsNone(t.ident)

    def testCreateFull(self):
        t = self.assertContents(("foo", 2, 3.0, 4.0, "bar"))
        self.assertEqual(t.report, "foo")
        self.assertEqual(t.id, 2)
        self.assertEqual(t.lat, 3.0)
        self.assertEqual(t.lon, 4.0)
        self.assertEqual(t.ident, "bar")


if __name__ == "__main__":
    from testlib import main
    main("test-types")
