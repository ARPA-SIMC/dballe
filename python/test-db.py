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
                rep_memo="synop",
                B01011="Hey Hey!!",
                B01012=500)
        self.db.insert(data, False, True)

        data.clear()
        data["B33007"] = 50
        data["B33036"] = 75
        self.db.attr_insert("B01011", data)

        for rec in self.db.query_data(dballe.Record(var="B01011")):
            self.attr_ref = rec["context_id"]

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
            var = result.var()
            assert var.code in expected
            self.assertEqual(var.enq(), expected[var.code])
            del expected[var.code]
            count += 1
    def testQueryAttrs(self):
        data = self.db.query_attrs("B01011", self.attr_ref)
        self.assertEqual(len(data), 2)

        expected = {}
        expected["B33007"] = 50
        expected["B33036"] = 75

        count = 0
        for code in data:
            self.assertIn(code, expected)
            self.assertEqual(data[code], expected[code])
            del expected[code]
            count += 1
        self.assertEqual(count, 2)

    def testQuerySomeAttrs(self):
        # Try limiting the set of wanted attributes
        data = self.db.query_attrs("B01011", self.attr_ref, ("B33036",))
        self.assertEqual(len(data), 1)
        self.assertEqual(data.vars(), (dballe.var("B33036", 75),))

    def testQueryCursorAttrs(self):
        # Query a variable
        query = dballe.Record(var="B01011")
        cur = self.db.query_data(query);
        data = cur.next()
        self.failUnless(data)

        attrs = cur.query_attrs()

        expected = {}
        expected["B33007"] = 50
        expected["B33036"] = 75

        count = 0
        for code in attrs:
                var = attrs.var(code)
                assert var.code in expected
                self.assertEqual(var.enq(), expected[var.code])
                del expected[var.code]
                count = count + 1
        self.assertEqual(count, 2)

        # Try limiting the set of wanted attributes
        attrs = cur.query_attrs(["B33036"])
        for code in attrs:
            var = attrs.var(code)
            self.assertEqual(var.code, "B33036")
            self.assertEqual(var.enqi(), 75)

    def testQuerySummary(self):
        query = dballe.Record()
        cur = self.db.query_summary(query)
        res = dict()
        for result in cur:
            res[(result["ana_id"], result["rep_memo"], result["level"], result["trange"], result["var"])] = (
                result["datemin"], result["datemax"], result["limit"])
        self.assertEqual(res[(1, "synop", (10, 11, 15, 22), (20, 111, 222), 'B01011')], (dt.datetime(1945, 4, 25, 8, 0), dt.datetime(1945, 4, 25, 8, 0), 1))
        self.assertEqual(res[(1, "synop", (10, 11, 15, 22), (20, 111, 222), 'B01012')], (dt.datetime(1945, 4, 25, 8, 0), dt.datetime(1945, 4, 25, 8, 0), 1))

    def testQueryExport(self):
        query = dballe.Record()
        self.db.export_to_file(query, "BUFR", "/dev/null")
        self.db.export_to_file(query, "CREX", "/dev/null")
        self.db.export_to_file(query, "BUFR", "/dev/null", generic=True)
        self.db.export_to_file(query, "CREX", "/dev/null", generic=True)

    def testAttrRemove(self):
        #db.attrRemove(1, "B01011", [ "B33007" ])
        self.db.attr_remove("B01011", self.attr_ref, ("B33007",))

if __name__ == "__main__":
        unittest.main()
