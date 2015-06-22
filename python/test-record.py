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

class RecordTest(unittest.TestCase):
    def setUp(self):
        if not hasattr(self, "assertCountEqual"):
            self.assertCountEqual = self.assertItemsEqual
        self.r = dballe.Record(
            block=1, station=123,
            lat=45.12345, lon=11.54321,
            datetime=datetime.datetime(2007, 2, 1, 1, 2, 3),
            level=(105, 2),
            trange=(2, 3, 4),
            var="B12101",
            B12101=285.0)
        self.knownkeys = ["lat", "lon", "year", "month", "day", "hour", "min", "sec", "leveltype1", "l1", "leveltype2", "l2", "pindicator", "p1", "p2"]
        self.knownvars = ["B12101", "B01002", "B01001"]
        self.knownkeyvals = [45.12345, 11.54321, 2007, 2, 1, 1, 2, 3, 105, 2, 0, 0, 2, 3, 4]
        self.knownvarvals = [285.0, 123, 1]

    def testGet(self):
        self.assertEqual(self.r.get("block"), 1)
        self.assertEqual(self.r.get(key="block"), 1)
        self.assertEqual(self.r.get(key="ana_id", default="ciao"), "ciao")
        self.assertEqual(self.r.get("ana_id", default="ciao"), "ciao")
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            # date, datemin, datemax, now, when queried, are treated like a single
            # range. Querying them will likely be deprecated soon.
            self.assertEqual(self.r.get("datemin", None), datetime.datetime(2007, 2, 1, 1, 2, 3))
            self.assertEqual(self.r.get("datemax", None), datetime.datetime(2007, 2, 1, 1, 2, 3))

    def testVar(self):
        with self.assertRaises(TypeError):
            self.r.var()
        self.assertEqual(self.r.var("B12101").code, "B12101")

    def testKey(self):
        self.assertEqual(self.r.key("lon").code, "B00000")
        self.assertEqual(self.r.key("lon").enqd(), 11.54321)
        self.assertEqual(self.r.key("lon").enqi(), 1154321)
        self.assertEqual(self.r.key("lon").enqc(), "1154321")
        self.assertEqual(self.r.key("lon").enq(), 11.54321)
        self.assertEqual(self.r.get("lon"), self.r.key("lon").enqd())

    def testMulti(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(self.r["date"], datetime.datetime(2007, 2, 1, 1, 2, 3))
            self.assertEqual(self.r["level"], (105, 2, None, None))
            self.assertEqual(self.r["timerange"], (2, 3, 4))
            self.assertEqual(self.r["trange"], (2, 3, 4))

    def testAlias(self):
        r = self.r.copy()
        r["t"] = 283.2
        self.assertEqual(r["B12101"], 283.2)
        self.assertEqual(r["t"], 283.2)

    def testReadDictOperators(self):
        r = self.r
        self.assertEqual(r["block"], 1)
        self.assertEqual(r["station"], 123)
        self.assertEqual(r["lat"], 45.12345)
        self.assertEqual(r["lon"], 11.54321)
        self.assertEqual(r["B12101"], 285.0)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(r["date"], datetime.datetime(2007, 2, 1, 1, 2, 3))
            self.assertEqual(r["level"], (105, 2, None, None))
            self.assertEqual(r["timerange"], (2, 3, 4))
    def testWriteDictOperators(self):
        r = self.r.copy()
        r["block"] = 2
        r["station"] = 321
        r["lat"] = 45.54321
        r["lon"] = 11.12345
        r["datetime"] = datetime.datetime(2006, 1, 2, 0, 1, 2)
        r["level"] = (104, 1, 105, 2)
        r["trange"] = (1, 2, 3)
        r["B12101"] = 294.5
        self.assertEqual(r["block"], 2)
        self.assertEqual(r["station"], 321)
        self.assertEqual(r["lat"], 45.54321)
        self.assertEqual(r["lon"], 11.12345)
        self.assertEqual(r["B12101"], 294.5)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(r["date"], datetime.datetime(2006, 1, 2, 0, 1, 2))
            self.assertEqual(r["level"], (104, 1, 105, 2))
            self.assertEqual(r["timerange"], (1, 2, 3))
    def testSpecials(self):
        r = self.r.copy()
        r["datetime"] = (datetime.datetime(2005, 3, 4, 5, 6, 7), datetime.datetime(2004, 4, 5, 6, 7, 8))
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            with self.assertRaises(KeyError):
                r["datetime"]
            self.assertEqual(r["datemin"], datetime.datetime(2005, 3, 4, 5, 6, 7))
            self.assertEqual(r["datemax"], datetime.datetime(2004, 4, 5, 6, 7, 8))
            self.assertEqual(r["level"], (105, 2, None, None))
            self.assertEqual(r["timerange"], (2, 3, 4))
            self.assertNotIn("date", r)
            self.assertIn("datemin", r)
            self.assertIn("datemax", r)
            self.assertIn("level", r)
            self.assertIn("timerange", r)
        del(r["datetime"])
        del(r["level"])
        del(r["trange"])
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(r.get("date", None), None)
            self.assertEqual(r.get("datemin", None), None)
            self.assertEqual(r.get("datemax", None), None)
            self.assertEqual(r.get("level", None), None)
            self.assertEqual(r.get("timerange", None), None)
            self.assertEqual("date" not in r, True)
            self.assertEqual("datemin" not in r, True)
            self.assertEqual("datemax" not in r, True)
            self.assertEqual("level" not in r, True)
            self.assertEqual("timerange" not in r, True)

    def testKeys(self):
        res = self.r.keys();
        self.assertCountEqual(res, [
            "lat", "lon",
            "year", "month", "day", "hour", "min", "sec",
            "leveltype1", "l1",
            "pindicator", "p1", "p2",
            "var",
            "B12101", "B01002", "B01001"])

    def testVars(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            r = dballe.Record()
            self.assertEqual(r.vars(), ())

            r["B33036"] = 75
            self.assertEqual(r.vars(), (dballe.var("B33036", 75),))

            res = self.r.vars()
            self.assertEqual(len(res), len(self.knownvars))
            self.assertEqual(sorted(x.enq() for x in res), sorted(self.knownvarvals))

    def testIter(self):
        r = dballe.Record()
        r["B33036"] = 75
        r["B12101"] = 273.15
        res = []
        ri = iter(r)
        res.append(next(ri))
        res.append(next(ri))
        self.assertEqual(res, ["B12101", "B33036"])

    def testSetDict(self):
        r = dballe.Record()
        r.update(ana_id=1, lat=12.34567, ident="ciao")
        self.assertEqual(r["ana_id"], 1)
        self.assertEqual(r["ident"], "ciao")
        self.assertEqual(r["lat"], 12.34567)
        self.assertEqual(len(r.keys()), 3)
        r.update(t=290.0)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(r.vars(), (dballe.var("B12101", 290.0),))

    def testSetFromString(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            r = dballe.Record()
            r.set_from_string("ana_id=1")
            r.set_from_string("lat=12.34567")
            r.set_from_string("ident=ciao")
            r.set_from_string("B12101=32.5")
            self.assertEqual(r["ana_id"], 1)
            self.assertEqual(r["ident"], "ciao")
            self.assertEqual(r["lat"], 12.34567)
            self.assertEqual(r["B12101"], 32.5)

    def testRecord(self):
        # Check basic set/get and variable iteration
        rec = dballe.Record()

        self.assertEqual("ana_id" in rec, False)
        rec["ana_id"] = 3
        self.assertEqual("ana_id" in rec, True)
        self.assertEqual(rec["ana_id"], 3)

        self.assertEqual("B04001" in rec, False)
        rec["B04001"] = 2001
        self.assertEqual("B04001" in rec, True)
        self.assertEqual(rec["B04001"], 2001)

        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            count = 0
            for var in rec.vars():
                self.assertEqual(var.code, "B04001")
                count += 1
            self.assertEqual(count, 1)

        del rec["block"]
        self.assertEqual("block" in rec, False)
        del rec["B04001"]
        self.assertEqual("B04001" in rec, False)

        d = datetime.datetime(2001, 2, 3, 4, 5, 6)
        rec["datetime"] = d
        self.assertEqual(rec["year"], 2001)
        self.assertEqual(rec["month"], 2)
        self.assertEqual(rec["day"], 3)
        self.assertEqual(rec["hour"], 4)
        self.assertEqual(rec["min"], 5)
        self.assertEqual(rec["sec"], 6)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(rec["date"], d)
            self.assertEqual(rec["datetime"], d)

        l = (1, 2, 1, 3)
        rec["level"] = l
        self.assertEqual(rec["leveltype1"], 1)
        self.assertEqual(rec["l1"], 2)
        self.assertEqual(rec["leveltype2"], 1)
        self.assertEqual(rec["l2"], 3)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(rec["level"], l)

        t = (4, 5, 6)
        rec["trange"] = t
        self.assertEqual(rec["pindicator"], 4)
        self.assertEqual(rec["p1"], 5)
        self.assertEqual(rec["p2"], 6)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(rec["timerange"], t)
            self.assertEqual(rec["trange"], t)

        # Test that KeyError is raised for several different types of lookup
        rec = dballe.Record()
        self.assertRaises(KeyError, rec.__getitem__, "year")
        self.assertRaises(KeyError, rec.__getitem__, "B01001")
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertRaises(KeyError, rec.__getitem__, "date")
            self.assertRaises(KeyError, rec.__getitem__, "level")
            self.assertRaises(KeyError, rec.__getitem__, "trange")
            self.assertRaises(KeyError, rec.__getitem__, "timerange")

        rec["datetime"] = None
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(rec.get("date", None), None)


    def testRecordClear(self):
        rec = dballe.Record(ana_id=1, B12101=21.5)
        self.assertIn("ana_id", rec);
        self.assertIn("B12101", rec);

        rec.clear()
        self.assertNotIn("ana_id", rec);
        self.assertNotIn("B12101", rec);

        rec.update(ana_id=1, B12101=21.5)
        self.assertIn("ana_id", rec);
        self.assertIn("B12101", rec);

        rec.clear_vars()
        self.assertIn("ana_id", rec);
        self.assertNotIn("B12101", rec);

    def testRecordConstructor(self):
        rec = dballe.Record(
            ana_id=1,
            datetime=datetime.datetime(2001, 2, 3, 4, 5, 6)
        )
        self.assertEqual(rec["ana_id"], 1)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(rec["date"], datetime.datetime(2001, 2, 3, 4, 5, 6))

    def testRecordCopying(self):
        # Try out all copying functions

        master = dballe.Record()
        master["block"] = 4
        master["latmin"] = 4.1234
        master["B01001"] = 4

        if True:
                r1 = master;
                self.assertEqual(r1["block"], 4)
                self.assertEqual(r1["latmin"], 4.1234)
                self.assertEqual(r1["B01001"], 4)

        r2 = master.copy()
        self.assertEqual(r2["block"], 4)
        self.assertEqual(r2["latmin"], 4.1234)
        self.assertEqual(r2["B01001"], 4)

        r3 = r2.copy()
        self.assertEqual(r3["block"], 4)
        self.assertEqual(r3["latmin"], 4.1234)
        self.assertEqual(r3["B01001"], 4)
        del r2["latmin"]
        self.assertEqual(r3["latmin"], 4.1234)
        r3["latmin"] = 4.3214
        self.assertEqual(r3["latmin"], 4.3214)

        r3 = r3
        self.assertEqual(r3["block"], 4)
        self.assertEqual(r3["latmin"], 4.3214)
        self.assertEqual(r3["B01001"], 4)

        master = r3
        self.assertEqual(master["block"], 4)
        self.assertEqual(master["latmin"], 4.3214)
        self.assertEqual(master["B01001"], 4)

    def testRecordCopying1(self):
        # This caused a repeatable segfault
        rec = dballe.Record()
        rec["query"] = "nosort"
        rec1 = rec.copy()
        rec1["query"] = "nosort"

    def testParseDateExtremes(self):
        # Test the parse_date_extremes implementation
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            rec = dballe.Record()

            a, b = rec.date_extremes()
            self.assertEqual(a, None)
            self.assertEqual(b, None)

            rec["yearmin"] = 2000
            a, b = rec.date_extremes()
            self.assertEqual(a, datetime.datetime(2000, 1, 1, 0, 0, 0))
            self.assertEqual(b, None)

            rec["yearmin"] = None
            rec["yearmax"] = 1900
            rec["monthmax"] = 2
            a, b = rec.date_extremes()
            self.assertEqual(a, None)
            self.assertEqual(b, datetime.datetime(1900, 2, 28, 23, 59, 59))

            rec["yearmax"] = 2000
            rec["monthmax"] = 2
            a, b = rec.date_extremes()
            self.assertEqual(a, None)
            self.assertEqual(b, datetime.datetime(2000, 2, 29, 23, 59, 59))

            rec["yearmax"] = 2001
            rec["monthmax"] = 2
            a, b = rec.date_extremes()
            self.assertEqual(a, None)
            self.assertEqual(b, datetime.datetime(2001, 2, 28, 23, 59, 59))

            rec["yearmax"] = 2004
            rec["monthmax"] = 2
            a, b = rec.date_extremes()
            self.assertEqual(a, None)
            self.assertEqual(b, datetime.datetime(2004, 2, 29, 23, 59, 59))

    def testCompare(self):
        a = dballe.Record(ana_id=1, ident="ciao", B12101=23.1)
        b = dballe.Record(ana_id=1, ident="ciao", B12101=23.1)
        self.assertTrue(a == b)
        self.assertFalse(a != b)

        a["ana_id"] = 2
        self.assertTrue(a != b)
        self.assertFalse(a == b)

    def testSetTuplesToNone(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            a = dballe.Record(level=(1, 2, 3, 4), trange=(1, 2, 3))
            self.assertEqual(a["level"], (1, 2, 3, 4))
            self.assertEqual(a["trange"], (1, 2, 3))
            a["level"] = None
            with self.assertRaises(KeyError):
                a["level"]
            self.assertEqual(a["trange"], (1, 2, 3))
            a["trange"] = None
            with self.assertRaises(KeyError):
                a["level"]
            with self.assertRaises(KeyError):
                a["trange"]


if __name__ == "__main__":
    from testlib import main
    main("test_record")
