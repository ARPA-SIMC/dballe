#!/usr/bin/python

import dballe
#from dballe import *
#from datetime import *
import unittest

class VartableTest(unittest.TestCase):
    def testCreate(self):
        table = dballe.Vartable.get("dballe")
        self.assertEqual(table.id(), "dballe")

    def testQuery(self):
        table = dballe.Vartable.get("dballe")
        info = table.query("B01001")
        self.assertEqual(info.is_string(), False)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.unit, "NUMERIC")

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
        self.assertEqual(info.is_string(), False)

    def testStringification(self):
        info = dballe.varinfo("B01001")
        self.assert_(str(info).startswith("B01001"))
        self.assert_(repr(info).startswith("<Varinfo B01001"))

    def testFromAlias(self):
        info = dballe.varinfo("t")
        self.assertEqual(info.var, "B12101")

class VarTest(unittest.TestCase):
    def testUndefCreation(self):
        var = dballe.var("B01001")
        self.assertEqual(var.code(), "B01001")
        self.assertEqual(var.value(), None)
    def testIntCreation(self):
        var = dballe.var("B05001", 12)
        self.assertEqual(var.code(), "B05001")
        self.assertEqual(var.isset(), True)
        self.assertEqual(var.enqi(), 12)
        self.assertEqual(var.enqd(), 0.00012)
        self.assertEqual(var.enqc(), "12")
    def testFloatCreation(self):
        var = dballe.var("B05001", 12.4)
        self.assertEqual(var.code(), "B05001")
        self.assertEqual(var.isset(), True)
        self.assertEqual(var.enqi(), 1240000)
        self.assertEqual(var.enqd(), 12.4)
        self.assertEqual(var.enqc(), "1240000")
    def testStringCreation(self):
        var = dballe.var("B05001", "123456")
        self.assertEqual(var.code(), "B05001")
        self.assertEqual(var.isset(), True)
        self.assertEqual(var.enqi(), 123456)
        self.assertEqual(var.enqd(), 1.23456)
        self.assertEqual(var.enqc(), "123456")
    def testAliasCreation(self):
        var = dballe.var("t", 280.3)
        self.assertEqual(var.code(), "B12101")
        self.assertEqual(var.isset(), True)
        self.assertEqual(var.enqi(), 28030)
        self.assertEqual(var.enqd(), 280.3)
        self.assertEqual(var.enqc(), "28030")
    def testStringification(self):
        var = dballe.var("B01001")
        self.assertEqual(str(var), "None")
        self.assertEqual(repr(var), "<Var B01001, None>")

        var = dballe.var("B05001", 12.4)
        self.assertEqual(str(var), "12.40000")
        self.assertEqual(repr(var), "<Var B05001, 12.40000>")
    def testEnq(self):
        var = dballe.var("B01001", 1)
        self.assertEqual(type(var.enq()), int)
        self.assertEqual(var.enq(), 1)
        var = dballe.var("B05001", 1.12345)
        self.assertEqual(type(var.enq()), float)
        self.assertEqual(var.enq(), 1.12345)
        var = dballe.var("B01019", "ciao")
        self.assertEqual(type(var.enq()), str)
        self.assertEqual(var.enq(), "ciao")
    def testEq(self):
        var = dballe.var("B01001", 1)
        self.assertEqual(var, var)
        self.assertEqual(var, dballe.var("B01001", 1))
        self.assertNotEqual(var, dballe.var("B01001", 2))
        self.assertNotEqual(var, dballe.var("B01002", 1))
        self.assertNotEqual(var, None)
        self.assertNotEqual(dballe.var("B01001"), None)


#class RecordTest(unittest.TestCase):
#        def setUp(self):
#                self.r = Record()
#                self.r.set("block", 1)
#                self.r.set("station", 123)
#                self.r.set("lat", 45.12345)
#                self.r.set("lon", 11.54321)
#                self.r.setdate(datetime(2007, 2, 1, 1, 2, 3))
#                self.r.setlevel(Level(105, 2, 0, 0))
#                self.r.settimerange(TimeRange(2, 3, 4))
#                self.r.set("B12101", 285.0)
#                self.knownkeys = ["lat", "lon", "year", "month", "day", "hour", "min", "sec", "leveltype1", "l1", "leveltype2", "l2", "pindicator", "p1", "p2", "B12101", "B01002", "B01001"]
#                self.known = [45.12345, 11.54321, 2007, 2, 1, 1, 2, 3, 105, 2, 0, 0, 2, 3, 4, 285.0, 123, 1]
#        def testAlias(self):
#                r = self.r.copy()
#                r.set("t", 282.3)
#                self.assertEqual(r["B12101"], 282.3)
#                r["t"] = 283.2
#                self.assertEqual(r["B12101"], 283.2)
#        def testReadDictOperators(self):
#                r = self.r
#                self.assertEqual(r["block"], 1)
#                self.assertEqual(r["station"], 123)
#                self.assertEqual(r["lat"], 45.12345)
#                self.assertEqual(r["lon"], 11.54321)
#                self.assertEqual(r["date"], datetime(2007, 2, 1, 1, 2, 3))
#                self.assertEqual(r["level"], Level(105, 2, 0, 0))
#                self.assertEqual(r["timerange"], TimeRange(2, 3, 4))
#                self.assertEqual(r["B12101"], 285.0)
#        def testWriteDictOperators(self):
#                r = self.r.copy()
#                r["block"] = 2
#                r["station"] = 321
#                r["lat"] = 45.54321
#                r["lon"] = 11.12345
#                r["date"] = datetime(2006, 1, 2, 0, 1, 2)
#                r["level"] = Level(104, 1, 105, 2)
#                r["timerange"] = TimeRange(1, 2, 3)
#                r["B12101"] = 294.5
#                self.assertEqual(r["block"], 2)
#                self.assertEqual(r["station"], 321)
#                self.assertEqual(r["lat"], 45.54321)
#                self.assertEqual(r["lon"], 11.12345)
#                self.assertEqual(r["date"], datetime(2006, 1, 2, 0, 1, 2))
#                self.assertEqual(r["level"], Level(104, 1, 105, 2))
#                self.assertEqual(r["timerange"], TimeRange(1, 2, 3))
#                self.assertEqual(r["B12101"], 294.5)
#        def testSpecials(self):
#                r = self.r.copy()
#                r.set("datemin", datetime(2005, 3, 4, 5, 6, 7))
#                r.set("datemax", datetime(2004, 4, 5, 6, 7, 8))
#                self.assertEqual(r["date"], datetime(2007, 2, 1, 1, 2, 3))
#                self.assertEqual(r["datemin"], datetime(2005, 3, 4, 5, 6, 7))
#                self.assertEqual(r["datemax"], datetime(2004, 4, 5, 6, 7, 8))
#                self.assertEqual(r["level"], Level(105, 2, 0, 0))
#                self.assertEqual(r["timerange"], TimeRange(2, 3, 4))
#                self.assertEqual(r.enq("date"), datetime(2007, 2, 1, 1, 2, 3))
#                self.assertEqual(r.enq("datemin"), datetime(2005, 3, 4, 5, 6, 7))
#                self.assertEqual(r.enq("datemax"), datetime(2004, 4, 5, 6, 7, 8))
#                self.assertEqual(r.enq("level"), Level(105, 2, 0, 0))
#                self.assertEqual(r.enq("timerange"), TimeRange(2, 3, 4))
#                self.assertEqual("date" in r, True)
#                self.assertEqual("datemin" in r, True)
#                self.assertEqual("datemax" in r, True)
#                self.assertEqual("level" in r, True)
#                self.assertEqual("timerange" in r, True)
#                del(r["date"])
#                del(r["datemin"])
#                del(r["datemax"])
#                del(r["level"])
#                del(r["timerange"])
#                self.assertEqual(r["date"], None)
#                self.assertEqual(r["datemin"], None)
#                self.assertEqual(r["datemax"], None)
#                self.assertEqual(r["level"], None)
#                self.assertEqual(r["timerange"], None)
#                self.assertEqual("date" not in r, True)
#                self.assertEqual("datemin" not in r, True)
#                self.assertEqual("datemax" not in r, True)
#                self.assertEqual("level" not in r, True)
#                self.assertEqual("timerange" not in r, True)
#
#        def testIterEmpty(self):
#                r = Record()
#                self.assertEqual([x for x in r], [])
#                self.assertEqual([x for x in r.iterkeys()], [])
#                self.assertEqual([x for x in r.itervalues()], [])
#                self.assertEqual([x for x in r.iteritems()], [])
#                self.assertEqual([x for x in r.itervars()], [])
#        def testIterOne(self):
#                # Used to throw the wrong exception to stop iteration
#                r = Record()
#                r.seti("B33036", 75)
#                self.assertEqual([x for x in r.iteritems()], [("B33036", 75)])
#        def testIter(self):
#                res = [n.enq() for n in self.r]
#                self.assertEqual(len(res), len(self.known))
#                self.assertEqual(res, self.known)
#                res = []
#                for n in self.r.itervalues():
#                        res += [n.enq()]
#                self.assertEqual(len(res), len(self.known))
#                self.assertEqual(res, self.known)
#        def testIterkeys(self):
#                res = []
#                for n in self.r.iterkeys():
#                        res += [n]
#                self.assertEqual(len(res), len(self.knownkeys))
#                self.assertEqual(res, self.knownkeys)
#        def testIteritems(self):
#                known = zip(self.knownkeys, self.known)
#                res = []
#                for key, val in self.r.iteritems():
#                        res += [(key, val.enq())]
#                self.assertEqual(len(res), len(known))
#                self.assertEqual(res, known)
#        def testSetDict(self):
#                r = Record()
#                r.set({"ana_id": 1, "lat": 12.34567, "ident": "ciao"})
#                self.assertEqual([x for x in r.iteritems()], [("ana_id", 1), ("ident", "ciao"), ("lat", 12.34567)])
#
#
#        def testRecord(self):
#                # Check basic set/get and variable iteration
#                rec = Record()
#
#                self.assertEqual(rec.contains("ana_id"), False)
#                rec.set("ana_id", 3)
#                self.assertEqual(rec.contains("ana_id"), True)
#                self.assertEqual(rec.enqi("ana_id"), 3)
#
#                self.assertEqual(rec.contains("B04001"), False)
#                rec.set("B04001", 2001)
#                self.assertEqual(rec.contains("B04001"), True)
#                self.assertEqual(rec.enqi("B04001"), 2001)
#
#                count = 0
#                for var in rec.itervars():
#                        self.assertEqual(var.code(), "B04001")
#                        count = count + 1
#                self.assertEqual(count, 1)
#
#                rec.unset("block")
#                self.assertEqual(rec.contains("block"), False)
#                rec.unset("B04001")
#                self.assertEqual(rec.contains("B04001"), False)
#
#                rec.set("B01001", 1)
#                var = rec.enqvar("B01001")
#                var.set(4)
#                rec.set(var)
#                self.assertEqual(rec.enqi("B01001"), 4)
#
#                dt = datetime(2001, 2, 3, 4, 5, 6)
#                rec.setdate(dt)
#                self.assertEqual(rec.enqdate(), dt)
#                self.assertEqual(rec.enqi("year"), 2001)
#                self.assertEqual(rec.enqi("month"), 2)
#                self.assertEqual(rec.enqi("day"), 3)
#                self.assertEqual(rec.enqi("hour"), 4)
#                self.assertEqual(rec.enqi("min"), 5)
#                self.assertEqual(rec.enqi("sec"), 6)
#
#                l = Level(1, 2, 1, 3)
#                rec.setlevel(l)
#                self.assertEqual(rec.enqlevel(), l)
#                self.assertEqual(rec.enqi("leveltype1"), 1)
#                self.assertEqual(rec.enqi("l1"), 2)
#                self.assertEqual(rec.enqi("leveltype2"), 1)
#                self.assertEqual(rec.enqi("l2"), 3)
#
#                t = TimeRange(4, 5, 6)
#                rec.settimerange(t)
#                self.assertEqual(rec.enqtimerange(), t)
#                self.assertEqual(rec.enqi("pindicator"), 4)
#                self.assertEqual(rec.enqi("p1"), 5)
#                self.assertEqual(rec.enqi("p2"), 6)
#
#
#        def testRecordCopying(self):
#                # Try out all copying functions
#
#                master = Record()
#                master.set("block", 4)
#                master.set("latmin", 4.1234)
#                master.set("B01001", 4)
#
#                if True:
#                        r1 = master;
#                        self.assertEqual(r1.enqi("block"), 4)
#                        self.assertEqual(r1.enqd("latmin"), 4.1234)
#                        self.assertEqual(r1.enqi("B01001"), 4)
#
#                r2 = master.copy()
#                self.assertEqual(r2.enqi("block"), 4)
#                self.assertEqual(r2.enqd("latmin"), 4.1234)
#                self.assertEqual(r2.enqi("B01001"), 4)
#
#                r3 = r2.copy()
#                self.assertEqual(r3.enqi("block"), 4)
#                self.assertEqual(r3.enqd("latmin"), 4.1234)
#                self.assertEqual(r3.enqi("B01001"), 4)
#                r2.unset("latmin")
#                self.assertEqual(r3.enqd("latmin"), 4.1234)
#                r3.setd("latmin", 4.3214)
#                self.assertEqual(r3.enqd("latmin"), 4.3214)
#
#                r3 = r3
#                self.assertEqual(r3.enqi("block"), 4)
#                self.assertEqual(r3.enqd("latmin"), 4.3214)
#                self.assertEqual(r3.enqi("B01001"), 4)
#
#                master = r3
#                self.assertEqual(master.enqi("block"), 4)
#                self.assertEqual(master.enqd("latmin"), 4.3214)
#                self.assertEqual(master.enqi("B01001"), 4)
#
#        def testRecordCopying1(self):
#                # This caused a repeatable segfault
#                rec = Record()
#                rec.setc("query", "nosort")
#                rec1 = rec.copy()
#                rec1.setc("query", "nosort")
#
#class BufrexTest(unittest.TestCase):
#    def testBUFRCreation(self):
#        # Generate a synop message
#        msg = Bufrex.createBUFR(98, 0, 6, 1)
#        msg.appendDatadesc("D07005")
#        msg.appendDatadesc("B13011")
#        msg.appendDatadesc("B13013")
#        subset = msg.append()
#        self.assertEqual(msg.size(), 1)
#        subset.appendi("B01001", 60)
#        subset.appendi("B01002", 150)
#        subset.appendi("B02001", 1)
#        subset.appendi("B04001", 2004)
#        subset.appendi("B04002", 11)
#        subset.appendi("B04003", 30)
#        subset.appendi("B04004", 12)
#        subset.appendi("B04005", 0)
#        subset.appendd("B05001", 33.88000)
#        subset.appendd("B06001", -5.53000)
#        subset.appendd("B07001", 560)
#        subset.appendd("B10004", 94190)
#        subset.appendd("B10051", 100540)
#        subset.appendd("B10061", -180)
#        subset.appendi("B10063", 8)
#        subset.appendd("B11011", 80)
#        subset.appendd("B11012", 4.0)
#        subset.appendd("B12004", 289.2)
#        subset.appendd("B12006", 285.7)
#        subset.appendu("B13003")
#        subset.appendd("B20001", 8000)
#        subset.appendi("B20003", 2)
#        subset.appendi("B20004", 6)
#        subset.appendi("B20005", 2)
#        subset.appendd("B20010", 100)
#        subset.appendi("B08002", 1)
#        subset.appendd("B20011", 8)
#        subset.appendd("B20013", 250)
#        subset.appendi("B20012", 39)
#        subset.appendi("B20012", 61)
#        subset.appendi("B20012", 60)
#        subset.appendi("B08002", 1)
#        subset.appendi("B20011", 2)
#        subset.appendi("B20012", 8)
#        subset.appendd("B20013", 320)
#        subset.appendi("B08002", 2)
#        subset.appendi("B20011", 5)
#        subset.appendi("B20012", 8)
#        subset.appendd("B20013", 620)
#        subset.appendi("B08002", 3)
#        subset.appendi("B20011", 2)
#        subset.appendi("B20012", 9)
#        subset.appendd("B20013", 920)
#        subset.appendu("B08002")
#        subset.appendu("B20011")
#        subset.appendu("B20012")
#        subset.appendu("B20013")
#        subset.appendd("B13011", 0.5)
#        subset.appendu("B13013")
#        msg.setTemplate(0, 255, 1)
#        msg.setEdition(3)
#        msg.setTime(2004, 11, 30, 12, 0, 0)
#        buf = msg.encode()
#        assert len(buf) > 8
#        self.assertEqual(buf[:4], "BUFR")
#        self.assertEqual(buf[-4:], "7777")
#
#        msg.resetSections()
#        self.assertEqual(msg.size(), 0)
#
#class MsgTest(unittest.TestCase):
#    def testBUFRCreation(self):
#        msg = Msg()
#        msg.setd("B12101", 289.2, 100, 1, 0, 0, 0, 0, 0, 0)
#        buf = msg.encodeBUFR(0, 0, 0)
#        assert len(buf) > 8
#        self.assertEqual(buf[:4], "BUFR")
#        self.assertEqual(buf[-4:], "7777")
#
#class FormatterTest(unittest.TestCase):
#    def testFormatter(self):
#        for i in range(258):
#            describeLevel(i, 0, 0, 0)
#            describeLevel(i, 0, 1, 0)
#        for i in range(256):
#            describeTrange(i, 0, 0)

if __name__ == "__main__":
        unittest.main()
