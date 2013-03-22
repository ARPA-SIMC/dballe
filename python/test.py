#!/usr/bin/python

import dballe
#from dballe import *
import datetime as dt
import unittest

class VartableTest(unittest.TestCase):
    def testEmpty(self):
        table = dballe.Vartable()
        self.assertEqual(table.id, None)
        self.assertEqual(str(table), "<empty>")
        self.assertEqual(repr(table), "Vartable()")
        self.assertRaises(KeyError, table.query, "B01001")

    def testEmptyVarinfo(self):
        self.assertRaises(NotImplementedError, dballe.Varinfo)

    def testCreate(self):
        table = dballe.Vartable.get("dballe")
        self.assertEqual(table.id, "dballe")
        self.assertEqual(str(table), "dballe")
        self.assertEqual(repr(table), "Vartable('dballe')")

    def testContains(self):
        table = dballe.Vartable.get("dballe")
        self.assertIn("B01001", table)
        self.assertNotIn("B63254", table)

    def testIndexing(self):
        table = dballe.Vartable.get("dballe")
        info = table[0]
        self.assertEqual(info.var, "B01001")

    def testQuery(self):
        table = dballe.Vartable.get("dballe")
        info = table.query("B01001")
        self.assertEqual(info.is_string, False)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.unit, "NUMERIC")

        info = table["B01001"]
        self.assertEqual(info.is_string, False)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.unit, "NUMERIC")

    def testQueryMissing(self):
        table = dballe.Vartable.get("dballe")
        self.assertRaises(KeyError, table.query, "B63254")

    def testQueryLocal(self):
        info = dballe.varinfo("t")
        self.assertEqual(info.unit, "K")

    def testIterate(self):
        table = dballe.Vartable.get("dballe")
        selected = None
        count = 0
        for entry in table:
            if entry.var == "B12101":
                selected = entry
            count += 1
        #self.assertGreater(count, 100)
        self.assert_(count > 100)
        self.assertNotEqual(selected, None)

    def testData(self):
        info = dballe.varinfo("B01001")
        self.assertEqual(info.var, "B01001")
        self.assertEqual(info.desc, "WMO BLOCK NUMBER")
        self.assertEqual(info.unit, "NUMERIC")
        self.assertEqual(info.scale, 0)
        self.assertEqual(info.ref, 0)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.is_string, False)

    def testStringification(self):
        info = dballe.varinfo("B01001")
        self.assert_(str(info).startswith("B01001"))
        self.assert_(repr(info).startswith("Varinfo('B01001"))

    def testFromAlias(self):
        info = dballe.varinfo("t")
        self.assertEqual(info.var, "B12101")

#class VarTest(unittest.TestCase):
#    def testUndefCreation(self):
#        var = dballe.var("B01001")
#        self.assertEqual(var.code(), "B01001")
#        self.assertEqual(var.value(), None)
#    def testIntCreation(self):
#        var = dballe.var("B05001", 12)
#        self.assertEqual(var.code(), "B05001")
#        self.assertEqual(var.isset(), True)
#        self.assertEqual(var.enqi(), 12)
#        self.assertEqual(var.enqd(), 0.00012)
#        self.assertEqual(var.enqc(), "12")
#    def testFloatCreation(self):
#        var = dballe.var("B05001", 12.4)
#        self.assertEqual(var.code(), "B05001")
#        self.assertEqual(var.isset(), True)
#        self.assertEqual(var.enqi(), 1240000)
#        self.assertEqual(var.enqd(), 12.4)
#        self.assertEqual(var.enqc(), "1240000")
#    def testStringCreation(self):
#        var = dballe.var("B05001", "123456")
#        self.assertEqual(var.code(), "B05001")
#        self.assertEqual(var.isset(), True)
#        self.assertEqual(var.enqi(), 123456)
#        self.assertEqual(var.enqd(), 1.23456)
#        self.assertEqual(var.enqc(), "123456")
#    def testAliasCreation(self):
#        var = dballe.var("t", 280.3)
#        self.assertEqual(var.code(), "B12101")
#        self.assertEqual(var.isset(), True)
#        self.assertEqual(var.enqi(), 28030)
#        self.assertEqual(var.enqd(), 280.3)
#        self.assertEqual(var.enqc(), "28030")
#    def testStringification(self):
#        var = dballe.var("B01001")
#        self.assertEqual(str(var), "None")
#        self.assertEqual(repr(var), "<Var B01001, None>")
#
#        var = dballe.var("B05001", 12.4)
#        self.assertEqual(str(var), "12.40000")
#        self.assertEqual(repr(var), "<Var B05001, 12.40000>")
#    def testEnq(self):
#        var = dballe.var("B01001", 1)
#        self.assertEqual(type(var.enq()), int)
#        self.assertEqual(var.enq(), 1)
#        var = dballe.var("B05001", 1.12345)
#        self.assertEqual(type(var.enq()), float)
#        self.assertEqual(var.enq(), 1.12345)
#        var = dballe.var("B01019", "ciao")
#        self.assertEqual(type(var.enq()), str)
#        self.assertEqual(var.enq(), "ciao")
#    def testEq(self):
#        var = dballe.var("B01001", 1)
#        self.assertEqual(var, var)
#        self.assertEqual(var, dballe.var("B01001", 1))
#        self.assertNotEqual(var, dballe.var("B01001", 2))
#        self.assertNotEqual(var, dballe.var("B01002", 1))
#        self.assertIsNot(var, None)
#        self.assertIsNot(dballe.var("B01001"), None)
#
#
#class RecordTest(unittest.TestCase):
#    def setUp(self):
#        self.r = dballe.Record()
#        self.r["block"] = 1
#        self.r["station"] = 123
#        self.r["lat"] = 45.12345
#        self.r["lon"] = 11.54321
#        self.r["date"] = dt.datetime(2007, 2, 1, 1, 2, 3)
#        self.r["level"] = 105, 2
#        self.r["timerange"] = 2, 3, 4
#        self.r["B12101"] = 285.0
#        self.knownkeys = ["lat", "lon", "year", "month", "day", "hour", "min", "sec", "leveltype1", "l1", "leveltype2", "l2", "pindicator", "p1", "p2"]
#        self.knownvars = ["B12101", "B01002", "B01001"]
#        self.knownkeyvals = [45.12345, 11.54321, 2007, 2, 1, 1, 2, 3, 105, 2, 0, 0, 2, 3, 4]
#        self.knownvarvals = [285.0, 123, 1]
#    def testMulti(self):
#        self.assertEqual(self.r["date"], dt.datetime(2007, 2, 1, 1, 2, 3))
#        self.assertEqual(self.r["level"], (105, 2, None, None))
#        self.assertEqual(self.r["timerange"], (2, 3, 4))
#        self.assertEqual(self.r["trange"], (2, 3, 4))
#    def testAlias(self):
#        r = self.r.copy()
#        r["t"] = 283.2
#        self.assertEqual(r["B12101"], 283.2)
#        self.assertEqual(r["t"], 283.2)
#
#    def testReadDictOperators(self):
#        r = self.r
#        self.assertEqual(r["block"], 1)
#        self.assertEqual(r["station"], 123)
#        self.assertEqual(r["lat"], 45.12345)
#        self.assertEqual(r["lon"], 11.54321)
#        self.assertEqual(r["date"], dt.datetime(2007, 2, 1, 1, 2, 3))
#        self.assertEqual(r["level"], (105, 2, None, None))
#        self.assertEqual(r["timerange"], (2, 3, 4))
#        self.assertEqual(r["B12101"], 285.0)
#    def testWriteDictOperators(self):
#        r = self.r.copy()
#        r["block"] = 2
#        r["station"] = 321
#        r["lat"] = 45.54321
#        r["lon"] = 11.12345
#        r["date"] = dt.datetime(2006, 1, 2, 0, 1, 2)
#        r["level"] = (104, 1, 105, 2)
#        r["timerange"] = (1, 2, 3)
#        r["B12101"] = 294.5
#        self.assertEqual(r["block"], 2)
#        self.assertEqual(r["station"], 321)
#        self.assertEqual(r["lat"], 45.54321)
#        self.assertEqual(r["lon"], 11.12345)
#        self.assertEqual(r["date"], dt.datetime(2006, 1, 2, 0, 1, 2))
#        self.assertEqual(r["level"], (104, 1, 105, 2))
#        self.assertEqual(r["timerange"], (1, 2, 3))
#        self.assertEqual(r["B12101"], 294.5)
#    def testSpecials(self):
#        r = self.r.copy()
#        r["datemin"] = dt.datetime(2005, 3, 4, 5, 6, 7)
#        r["datemax"] = dt.datetime(2004, 4, 5, 6, 7, 8)
#        self.assertEqual(r["date"], dt.datetime(2007, 2, 1, 1, 2, 3))
#        self.assertEqual(r["datemin"], dt.datetime(2005, 3, 4, 5, 6, 7))
#        self.assertEqual(r["datemax"], dt.datetime(2004, 4, 5, 6, 7, 8))
#        self.assertEqual(r["level"], (105, 2, None, None))
#        self.assertEqual(r["timerange"], (2, 3, 4))
#        self.assertEqual("date" in r, True)
#        self.assertEqual("datemin" in r, True)
#        self.assertEqual("datemax" in r, True)
#        self.assertEqual("level" in r, True)
#        self.assertEqual("timerange" in r, True)
#        del(r["date"])
#        del(r["datemin"])
#        del(r["datemax"])
#        del(r["level"])
#        del(r["timerange"])
#        self.assertEqual(r.get("date", None), None)
#        self.assertEqual(r.get("datemin", None), None)
#        self.assertEqual(r.get("datemax", None), None)
#        self.assertEqual(r.get("level", None), None)
#        self.assertEqual(r.get("timerange", None), None)
#        self.assertEqual("date" not in r, True)
#        self.assertEqual("datemin" not in r, True)
#        self.assertEqual("datemax" not in r, True)
#        self.assertEqual("level" not in r, False)
#        self.assertEqual("timerange" not in r, False)
#
#    def testIterEmpty(self):
#        r = dballe.Record()
#        self.assertEqual([x for x in r], [])
#        self.assertEqual([x for x in r.iterkeys()], [])
#        self.assertEqual([x for x in r.itervalues()], [])
#        self.assertEqual([x for x in r.iteritems()], [])
#    def testIterOne(self):
#        # Used to throw the wrong exception to stop iteration
#        r = dballe.Record()
#        r["B33036"] = 75
#        self.assertEqual([x for x in r.iteritems()], [("B33036", 75)])
#    def testIter(self):
#        res = [n.enq() for n in self.r]
#        self.assertEqual(len(res), len(self.knownvars))
#        self.assertEqual(set(res), set(self.knownvarvals))
#        res = []
#        for n in self.r:
#                res += [n.enq()]
#        self.assertEqual(len(res), len(self.knownvars))
#        self.assertEqual(set(res), set(self.knownvarvals))
#    def testIterkeys(self):
#        res = []
#        for n in self.r.iterkeys():
#                res += [n]
#        self.assertEqual(len(res), len(self.knownvars))
#        self.assertEqual(sorted(res), sorted(self.knownvars))
#    def testIteritems(self):
#        known = zip(self.knownvars, self.knownvarvals)
#        res = self.r.items()
#        self.assertEqual(len(res), len(known))
#        self.assertEqual(set(res), set(known))
#    def testSetDict(self):
#        r = dballe.Record()
#        r.update({"ana_id": 1, "lat": 12.34567, "ident": "ciao"})
#        #self.assertEqual([x for x in r.iteritems()], [("ana_id", 1), ("ident", "ciao"), ("lat", 12.34567)])
#        self.assertEqual([x for x in r.iteritems()], [])
#        r.update({"t": 290.0})
#        self.assertEqual([x for x in r.iteritems()], [("B12101", 290.0)])
#
#
#    def testRecord(self):
#        # Check basic set/get and variable iteration
#        rec = dballe.Record()
#
#        self.assertEqual("ana_id" in rec, False)
#        rec["ana_id"] = 3
#        self.assertEqual("ana_id" in rec, True)
#        self.assertEqual(rec["ana_id"], 3)
#
#        self.assertEqual("B04001" in rec, False)
#        rec["B04001"] = 2001
#        self.assertEqual("B04001" in rec, True)
#        self.assertEqual(rec["B04001"], 2001)
#
#        count = 0
#        for var in rec:
#                self.assertEqual(var.code(), "B04001")
#                count = count + 1
#        self.assertEqual(count, 1)
#
#        del rec["block"]
#        self.assertEqual("block" in rec, False)
#        del rec["B04001"]
#        self.assertEqual("B04001" in rec, False)
#
#        rec["B01001"] = 1
#        var = rec.getvar("B01001")
#        var.set(4)
#        rec.update(var)
#        self.assertEqual(rec["B01001"], 4)
#
#        d = dt.datetime(2001, 2, 3, 4, 5, 6)
#        rec["date"] = d
#        self.assertEqual(rec["date"], d)
#        self.assertEqual(rec["year"], 2001)
#        self.assertEqual(rec["month"], 2)
#        self.assertEqual(rec["day"], 3)
#        self.assertEqual(rec["hour"], 4)
#        self.assertEqual(rec["min"], 5)
#        self.assertEqual(rec["sec"], 6)
#
#        l = (1, 2, 1, 3)
#        rec["level"] = l
#        self.assertEqual(rec["level"], l)
#        self.assertEqual(rec["leveltype1"], 1)
#        self.assertEqual(rec["l1"], 2)
#        self.assertEqual(rec["leveltype2"], 1)
#        self.assertEqual(rec["l2"], 3)
#
#        t = (4, 5, 6)
#        rec["timerange"] = t
#        self.assertEqual(rec["timerange"], t)
#        self.assertEqual(rec["trange"], t)
#        self.assertEqual(rec["pindicator"], 4)
#        self.assertEqual(rec["p1"], 5)
#        self.assertEqual(rec["p2"], 6)
#
#        # Test that KeyError is raised for several different types of lookup
#        rec = dballe.Record()
#        self.assertRaises(KeyError, rec.__getitem__, "year")
#        self.assertRaises(KeyError, rec.__getitem__, "B01001")
#        self.assertRaises(KeyError, rec.__getitem__, "date")
#
#    def testRecordCopying(self):
#        # Try out all copying functions
#
#        master = dballe.Record()
#        master["block"] = 4
#        master["latmin"] = 4.1234
#        master["B01001"] = 4
#
#        if True:
#                r1 = master;
#                self.assertEqual(r1["block"], 4)
#                self.assertEqual(r1["latmin"], 4.1234)
#                self.assertEqual(r1["B01001"], 4)
#
#        r2 = master.copy()
#        self.assertEqual(r2["block"], 4)
#        self.assertEqual(r2["latmin"], 4.1234)
#        self.assertEqual(r2["B01001"], 4)
#
#        r3 = r2.copy()
#        self.assertEqual(r3["block"], 4)
#        self.assertEqual(r3["latmin"], 4.1234)
#        self.assertEqual(r3["B01001"], 4)
#        del r2["latmin"]
#        self.assertEqual(r3["latmin"], 4.1234)
#        r3["latmin"] = 4.3214
#        self.assertEqual(r3["latmin"], 4.3214)
#
#        r3 = r3
#        self.assertEqual(r3["block"], 4)
#        self.assertEqual(r3["latmin"], 4.3214)
#        self.assertEqual(r3["B01001"], 4)
#
#        master = r3
#        self.assertEqual(master["block"], 4)
#        self.assertEqual(master["latmin"], 4.3214)
#        self.assertEqual(master["B01001"], 4)
#
#    def testRecordCopying1(self):
#        # This caused a repeatable segfault
#        rec = dballe.Record()
#        rec["query"] = "nosort"
#        rec1 = rec.copy()
#        rec1["query"] = "nosort"
#
#    def testParseDateExtremes(self):
#        # Test the parse_date_extremes reimplementation
#        rec = dballe.Record()
#
#        a, b = rec.parse_date_extremes()
#        self.assertEqual(a, None)
#        self.assertEqual(b, None)
#
#        rec["yearmin"] = 2000
#        a, b = rec.parse_date_extremes()
#        self.assertEqual(a, dt.datetime(2000, 1, 1, 0, 0, 0))
#        self.assertEqual(b, None)
#
#        rec["yearmin"] = None
#        rec["yearmax"] = 1900
#        rec["monthmax"] = 2
#        a, b = rec.parse_date_extremes()
#        self.assertEqual(a, None)
#        self.assertEqual(b, dt.datetime(1900, 2, 28, 23, 59, 59))
#
#        rec["yearmax"] = 2000
#        rec["monthmax"] = 2
#        a, b = rec.parse_date_extremes()
#        self.assertEqual(a, None)
#        self.assertEqual(b, dt.datetime(2000, 2, 29, 23, 59, 59))
#
#        rec["yearmax"] = 2001
#        rec["monthmax"] = 2
#        a, b = rec.parse_date_extremes()
#        self.assertEqual(a, None)
#        self.assertEqual(b, dt.datetime(2001, 2, 28, 23, 59, 59))
#
#        rec["yearmax"] = 2004
#        rec["monthmax"] = 2
#        a, b = rec.parse_date_extremes()
#        self.assertEqual(a, None)
#        self.assertEqual(b, dt.datetime(2004, 2, 29, 23, 59, 59))
#
#class BulletinTest(unittest.TestCase):
#    def testBUFRCreation(self):
#        # Generate a synop message
#        msg = dballe.BufrBulletin()
#        msg.entre = 98
#        msg.subcentre = 0
#        msg.master_table = 14
#        msg.local_table = 0
#        msg.type = 0
#        msg.subtype = 255
#        msg.localsubtype = 1
#        msg.edition = 4
#        msg.rep_year = 2004
#        msg.rep_month = 11
#        msg.rep_day = 30
#        msg.rep_hour = 12
#        msg.rep_minute = 0
#        msg.rep_second = 0
#        msg.datadesc_append("D07005")
#        msg.datadesc_append("B13011")
#        msg.datadesc_append("B13013")
#        self.assertRaises(Exception, msg.obtain_subset, 0)
#        msg.load_tables()
#        subset = msg.obtain_subset(0)
#        subset.store_variable_i("B01001", 60)
#        subset.store_variable_i("B01002", 150)
#        subset.store_variable_i("B02001", 1)
#        subset.store_variable_i("B04001", 2004)
#        subset.store_variable_i("B04002", 11)
#        subset.store_variable_i("B04003", 30)
#        subset.store_variable_i("B04004", 12)
#        subset.store_variable_i("B04005", 0)
#        subset.store_variable_d("B05001", 33.88000)
#        subset.store_variable_d("B06001", -5.53000)
#        subset.store_variable_d("B07001", 560)
#        subset.store_variable_d("B10004", 94190)
#        subset.store_variable_d("B10051", 100540)
#        subset.store_variable_d("B10061", -180)
#        subset.store_variable_i("B10063", 8)
#        subset.store_variable_d("B11011", 80)
#        subset.store_variable_d("B11012", 4.0)
#        subset.store_variable_d("B12004", 289.2)
#        subset.store_variable_d("B12006", 285.7)
#        subset.store_variable_undef("B13003")
#        subset.store_variable_d("B20001", 8000)
#        subset.store_variable_i("B20003", 2)
#        subset.store_variable_i("B20004", 6)
#        subset.store_variable_i("B20005", 2)
#        subset.store_variable_d("B20010", 100)
#        subset.store_variable_i("B08002", 1)
#        subset.store_variable_d("B20011", 8)
#        subset.store_variable_d("B20013", 250)
#        subset.store_variable_i("B20012", 39)
#        subset.store_variable_i("B20012", 61)
#        subset.store_variable_i("B20012", 60)
#        subset.store_variable_i("B08002", 1)
#        subset.store_variable_i("B20011", 2)
#        subset.store_variable_i("B20012", 8)
#        subset.store_variable_d("B20013", 320)
#        subset.store_variable_i("B08002", 2)
#        subset.store_variable_i("B20011", 5)
#        subset.store_variable_i("B20012", 8)
#        subset.store_variable_d("B20013", 620)
#        subset.store_variable_i("B08002", 3)
#        subset.store_variable_i("B20011", 2)
#        subset.store_variable_i("B20012", 9)
#        subset.store_variable_d("B20013", 920)
#        subset.store_variable_undef("B08002")
#        subset.store_variable_undef("B20011")
#        subset.store_variable_undef("B20012")
#        subset.store_variable_undef("B20013")
#        subset.store_variable_d("B13011", 0.5)
#        subset.store_variable_undef("B13013")
#        buf = msg.encode()
#        assert len(buf) > 8
#        self.assertEqual(buf[:4], "BUFR")
#        self.assertEqual(buf[-4:], "7777")
#
#        msg.subsets_clear()
#        self.assertEqual(msg.subsets_size(), 0)
#
##class MsgTest(unittest.TestCase):
##    def testBUFRCreation(self):
##        msg = Msg()
##        msg.setd("B12101", 289.2, 100, 1, 0, 0, 0, 0, 0, 0)
##        buf = msg.encodeBUFR(0, 0, 0)
##        assert len(buf) > 8
##        self.assertEqual(buf[:4], "BUFR")
##        self.assertEqual(buf[-4:], "7777")
#
#class FormatterTest(unittest.TestCase):
#    def testFormatter(self):
#        for i in range(258):
#            dballe.Level(i, dballe.MISSING_INT).describe()
#            dballe.Level(i, dballe.MISSING_INT, i, dballe.MISSING_INT).describe()
#        for i in range(256):
#            dballe.Trange(i, dballe.MISSING_INT, dballe.MISSING_INT).describe()

if __name__ == "__main__":
        unittest.main()
