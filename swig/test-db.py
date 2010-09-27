#!/usr/bin/python

import dballe
import datetime as dt
import unittest

class DballeTest(unittest.TestCase):
    def setUp(self):
        self.db = dballe.DB()
        self.db.connect_test();
        self.db.reset()

        data = dballe.Record()
        data.update(dict(
                lat=12.34560, lon=76.54320,
                mobile=0,
                date=dt.datetime(1945, 4, 25, 8, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20,111,222),
                rep_cod=1,
                B01011="Hey Hey!!",
                B01012=500))

        self.db.insert(data, False, True)
        self.context_id = data["context_id"]

        data.clear()
        data["B33007"] = 50
        data["B33036"] = 75
        self.db.attr_insert(self.context_id, "B01011", data)

#       def tearDown(self):
#               if self.db.valid():
#               self.db.disconnect()

    def testQueryAna(self):
        query = dballe.Record()
        cur = self.db.query_stations(query)
        self.assertEqual(cur.remaining(), 1)
        count = 0
        for result in cur:
                self.assertEqual(result["lat"], 12.34560)
                self.assertEqual(result["lon"], 76.54320)
                self.assert_("B01011" not in result)
                count = count + 1
        self.assertEqual(count, 1)
    def testQueryData(self):
        expected = {}
        expected["B01011"] = "Hey Hey!!";
        expected["B01012"] = 500;

        query = dballe.Record()
        query["latmin"] = 10.0
        cur = self.db.query_data(query)
        self.assertEqual(cur.remaining(), 2)
        count = 0
        for result in cur:
                self.assertEqual(cur.remaining(), 2-count-1)
                assert cur.out_varcode in expected
                self.assertEqual(result[cur.out_varcode], expected[cur.out_varcode])
                del expected[cur.out_varcode]
                count = count + 1
    def testQueryAttrs(self):
        data = dballe.Record()
        count = self.db.query_attrs(self.context_id, "B01011", [], data)
        self.assertEqual(count, 2)

        expected = {}
        expected["B33007"] = 50
        expected["B33036"] = 75

        count = 0
        for var in data:
                self.assert_(var.code() in expected)
                self.assertEqual(var.enq(), expected[var.code()])
                del expected[var.code()]
                count = count + 1
        self.assertEqual(count, 2)

    def testQuerySomeAttrs(self):
        # Try limiting the set of wanted attributes
        data = dballe.Record()
        count = self.db.query_attrs(self.context_id, "B01011", ("B33036",), data)
        self.assertEqual(count, 1)
        self.assertEqual(data.items(), [("B33036", 75)])

    def testQueryCursorAttrs(self):
        query = dballe.Record()
        query["var"] = "B01011"
        cur = self.db.query_data(query);

        self.failUnless(cur.next())

        data = dballe.Record()
        count = cur.query_attrs([], data)
        self.assertEqual(count, 2)

        expected = {}
        expected["B33007"] = 50
        expected["B33036"] = 75

        count = 0
        for var in data:
                assert var.code() in expected
                self.assertEqual(var.enq(), expected[var.code()])
                del expected[var.code()]
                count = count + 1
        self.assertEqual(count, 2)

        # Try limiting the set of wanted attributes
        data.clear()
        count = cur.query_attrs(["B33036"], data)
        self.assertEqual(count, 1)
        self.assertEqual(data.items(), [("B33036", 75)])
    def testQueryLevels(self):
        query = dballe.Record()
        cur = self.db.query_levels(query)
        self.assertEqual(cur.remaining(), 1)
        for result in cur:
            self.assertEqual(result["level"], (10, 11, 15, 22))
    def testQueryTimeRanges(self):
        query = dballe.Record()
        cur = self.db.query_tranges(query)
        self.assertEqual(cur.remaining(), 1)
        for result in cur:
            self.assertEqual(result["trange"], (20, 111, 222))
    def testQueryLevelsAndTimeRanges(self):
        query = dballe.Record()
        cur = self.db.query_levels_tranges(query)
        self.assertEqual(cur.remaining(), 1)
        for result in cur:
            self.assertEqual(result["level"], (10, 11, 15, 22))
            self.assertEqual(result["trange"], (20, 111, 222))
    def testQueryVariableTypes(self):
        query = dballe.Record()
        cur = self.db.query_variable_types(query)
        self.assertEqual(cur.remaining(), 2)
        expected = {}
        expected["B01011"] = 1
        expected["B01012"] = 1
        count = 0
        for result in cur:
                self.assert_(cur.out_varcode in expected)
                del expected[cur.out_varcode]
                count = count + 1
        self.assertEqual(count, 2)
    def testQueryIdents(self):
        query = dballe.Record()
        cur = self.db.query_idents(query)
        self.assertEqual(cur.remaining(), 1)
        for result in cur:
            self.assert_("ident" not in result)
    def testQueryReports(self):
        query = dballe.Record()
        cur = self.db.query_reports(query)
        self.assertEqual(cur.remaining(), 1)
        for result in cur:
            self.assertEqual(result["rep_cod"], 1)
            self.assertEqual(result["rep_memo"], "synop")
    def testQueryDateTimes(self):
        query = dballe.Record()
        cur = self.db.query_datetimes(query)
        self.assertEqual(cur.remaining(), 1)
        for result in cur:
            self.assertEqual(result["date"], dt.datetime(1945, 4, 25, 8, 0, 0))
    def testQueryExport(self):
        query = dballe.Record()
        self.db.exportResults(query, "BUFR", "/dev/null")
        self.db.exportResults(query, "CREX", "/dev/null")
    def testAttrRemove(self):
        #db.attrRemove(1, "B01011", [ "B33007" ])
        self.db.attr_remove(1, "B01011", ("B33007",))

if __name__ == "__main__":
        unittest.main()
