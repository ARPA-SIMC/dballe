#!/usr/bin/python

import dballe
import datetime as dt
import unittest

class DballeTest(unittest.TestCase):
    def setUp(self):
        self.db = dballe.DB.connect_test()
        self.db.connect_test();
        self.db.reset()

        data = dballe.Record(
                lat=12.34560, lon=76.54320,
                mobile=0,
                date=dt.datetime(1945, 4, 25, 8, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20,111,222),
                rep_cod=1,
                B01011="Hey Hey!!",
                B01012=500)

        self.attr_ref = self.db.insert(data, False, True)

        data.clear()
        data["B33007"] = 50
        data["B33036"] = 75
        self.db.attr_insert(self.attr_ref, "B01011", data)

    def tearDown(self):
        self.db = None

    def testQueryAna(self):
        query = dballe.Record()
        cur = self.db.query_stations(query)
        self.assertEqual(cur.remaining, 1)
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
        self.assertEqual(cur.remaining, 2)
        count = 0
        for result in cur:
            self.assertEqual(cur.remaining, 2-count-1)
            var = result.var();
            assert var.code in expected
            self.assertEqual(var.enq(), expected[var.code])
            del expected[var.code]
            count += 1
    def testQueryAttrs(self):
        data = self.db.query_attrs(self.attr_ref, "B01011")
        self.assertEqual(len(data), 2)

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
        data = self.db.query_attrs(self.attr_ref, "B01011", ("B33036",))
        self.assertEqual(len(data), 1)
        self.assertEqual(data.vars(), (dballe.var("B33036", 75),))

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
        self.assertEqual(cur.remaining, 1)
        for result in cur:
            self.assertEqual(result["level"], (10, 11, 15, 22))
    def testQueryTimeRanges(self):
        query = dballe.Record()
        cur = self.db.query_tranges(query)
        self.assertEqual(cur.remaining, 1)
        for result in cur:
            self.assertEqual(result["trange"], (20, 111, 222))
    def testQueryVariableTypes(self):
        query = dballe.Record()
        cur = self.db.query_variable_types(query)
        self.assertEqual(cur.remaining, 2)
        expected = {}
        expected["B01011"] = 1
        expected["B01012"] = 1
        count = 0
        for result in cur:
            code = result["var"]
            self.assertIn(code, expected)
            del expected[code]
            count += 1
        self.assertEqual(count, 2)
    def testQueryReports(self):
        query = dballe.Record()
        cur = self.db.query_reports(query)
        self.assertEqual(cur.remaining, 1)
        for result in cur:
            self.assertEqual(result["rep_cod"], 1)
            self.assertEqual(result["rep_memo"], "synop")
    def testQueryDateTimes(self):
        query = dballe.Record()
        cur = self.db.query_datetimes(query)
        self.assertEqual(cur.remaining, 1)
        for result in cur:
            self.assertEqual(result["date"], dt.datetime(1945, 4, 25, 8, 0, 0))
    def testQueryExport(self):
        query = dballe.Record()
        self.db.export_results(query, "BUFR", "/dev/null")
        self.db.export_results(query, "CREX", "/dev/null")
    def testAttrRemove(self):
        #db.attrRemove(1, "B01011", [ "B33007" ])
        self.db.attr_remove(1, "B01011", ("B33007",))

if __name__ == "__main__":
        unittest.main()
