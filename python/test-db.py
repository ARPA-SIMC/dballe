#!/usr/bin/env python3
import dballe
import io
import datetime
import unittest
from decimal import Decimal
import warnings
from testlib import DballeDBMixin, test_pathname


class CommonDBTestMixin(DballeDBMixin):
    def setUp(self):
        super(CommonDBTestMixin, self).setUp()

        data = dict(
                lat=12.34560, lon=76.54320,
                datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20, 111, 222),
                rep_memo="synop",
                B01011="Hey Hey!!",
                B01012=500)
        ids = self.db.insert_data(data, False, True)

        data.clear()
        data["B33007"] = 50
        data["B33036"] = 75
        self.db.attr_insert_data(ids["B01011"], data)

        for rec in self.db.query_data(dict(var="B01011")):
            self.attr_ref = rec["context_id"]

    def testQueryExport(self):
        self.db.export_to_file({}, "BUFR", "/dev/null")
        self.db.export_to_file({}, "CREX", "/dev/null")
        self.db.export_to_file({}, "BUFR", "/dev/null", generic=True)
        self.db.export_to_file({}, "CREX", "/dev/null", generic=True)

    def testExportFileObject(self):
        out = io.BytesIO()
        self.db.export_to_file({}, "BUFR", out)
        self.assertTrue(out.getvalue().startswith(b"BUFR"))

    def testQueryStations(self):
        cur = self.db.query_stations()
        self.assertEqual(cur.remaining, 1)
        count = 0
        for idx, result in enumerate(cur):
            self.assertEqual(result["lat"], Decimal("12.34560"))
            self.assertEqual(result["lon"], Decimal("76.54320"))
            self.assertNotIn("B01011", result)
            count += 1
        self.assertEqual(count, 1)

    def testQueryData(self):
        expected = [
            {"code": "B01011", "val": "Hey Hey!!"},
            {"code": "B01012", "val": 500},
        ]

        cur = self.db.query_data({"latmin": 10.0})
        self.assertEqual(cur.remaining, 2)
        for idx, result in enumerate(cur):
            self.assertEqual(cur.remaining, 2-idx-1)
            var = result["var"]
            self.assertEqual(var.code, expected[idx]["code"])
            self.assertEqual(var.enq(), expected[idx]["val"])
            self.assertFalse(result.attrs(result["var"]))

    def testQueryDataAttrs(self):
        expected = [
            {"code": "B01011", "val": "Hey Hey!!"},
            {"code": "B01012", "val": 500},
        ]

        cur = self.db.query_data({"latmin": 10.0, "query": "attrs"})
        self.assertEqual(cur.remaining, 2)
        for idx, result in enumerate(cur):
            self.assertEqual(cur.remaining, 2-idx-1)
            var = result["var"]
            self.assertEqual(var.code, expected[idx]["code"])
            self.assertEqual(var.enq(), expected[idx]["val"])
            self.assertFalse(result.attrs(result["var"]))

    def testQueryDataLimit(self):
        cur = self.db.query_data({"limit": 1})
        self.assertEqual(cur.remaining, 1)

    def testQueryAttrs(self):
        data = self.db.attr_query_data(self.attr_ref)
        self.assertCountEqual(data.keys(), ["B33007", "B33036"])

        expected = {}
        expected["B33007"] = 50
        expected["B33036"] = 75

        count = 0
        for code in data:
            self.assertIn(code, expected)
            self.assertEqual(data[code].enq(), expected[code])
            del expected[code]
            count += 1
        self.assertEqual(count, 2)

    def testQueryCursorAttrs(self):
        # Query a variable
        cur = self.db.query_data({"var": "B01011"})
        data = next(cur)
        self.assertTrue(data)

        attrs = cur.attr_query()

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

    def testQuerySummary(self):
        cur = self.db.query_summary({"query": "details"})
        res = dict()
        for result in cur:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", DeprecationWarning)
                res[(result["ana_id"], result["rep_memo"], result["level"], result["trange"], result["var"])] = (
                    result["datetimemin"], result["datetimemax"], result["count"])
        self.assertEqual(
                res[(1, "synop", dballe.Level(10, 11, 15, 22), dballe.Trange(20, 111, 222), 'B01011')],
                (datetime.datetime(1945, 4, 25, 8, 0), datetime.datetime(1945, 4, 25, 8, 0), 1))
        self.assertEqual(
                res[(1, "synop", dballe.Level(10, 11, 15, 22), dballe.Trange(20, 111, 222), 'B01012')],
                (datetime.datetime(1945, 4, 25, 8, 0), datetime.datetime(1945, 4, 25, 8, 0), 1))

    def testAttrRemove(self):
        self.db.attr_remove_data(self.attr_ref, ("B33007",))

    def testLoadFile(self):
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp)
            self.assertTrue(self.db.query_data().remaining > 0)

    def testLoadFileLike(self):
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            s = io.BytesIO(fp.read())
            self.db.remove_all()
            self.db.load(s)
            self.assertTrue(self.db.query_data().remaining > 0)

    def testLoadFileWithAttrs(self):
        with io.open(test_pathname("bufr/issue91-withB33196.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp, attrs=True)
            r = next(self.db.query_data())
            a = self.db.attr_query_data(r["context_id"])
            self.assertTrue("B33196" in a)

    def testLoadFileOverwrite(self):
        with io.open(test_pathname("bufr/issue91-withoutB33196.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp, overwrite=True)
            r = next(self.db.query_data())
            self.db.attr_query_data(r["context_id"])  # cannot verify the result, but expecting not to raise
            var = r["var"]
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enqd(), 274.15)

        with io.open(test_pathname("bufr/issue91-withB33196.bufr"), "rb") as fp:
            self.db.load(fp, overwrite=True)
            r = next(self.db.query_data())
            self.db.attr_query_data(r["context_id"])  # cannot verify the result, but expecting not to raise)
            var = r["var"]
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enqd(), 273.15)

    def testLoadFileno(self):
        class F(object):
            def __init__(self, path):
                self.path = path

            def read(*args):
                raise AttributeError()

            def fileno(self):
                return self.fp.fileno()

            def __enter__(self):
                self.fp = open(self.path, "r")
                return self

            def __exit__(self, type, value, traceback):
                self.fp.close()

        with F(test_pathname("bufr/vad.bufr")) as f:
            self.db.remove_all()
            self.db.load(f)
            self.assertTrue(self.db.query_data().remaining > 0)

    def testLoadAutodetect(self):
        # BUFR, autodetectable
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp), 25)

        # BUFR, not autodetectable
        with io.open(test_pathname("bufr/synop-groundtemp.bufr"), "rb") as fp:
            self.db.remove_all()
            with self.assertRaises(KeyError):
                self.db.load(fp)

        # CREX, autodetectable

        with io.open(test_pathname("crex/test-synop0.crex"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp), 1)

        # BUFR
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp, "BUFR"), 25)

        # BUFR
        with io.open(test_pathname("bufr/synop-groundtemp.bufr"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp, "BUFR"), 1)

        # CREX loaded as BUFR yields no results
        with io.open(test_pathname("crex/test-synop0.crex"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp, "BUFR"), 0)

        # CREX
        with io.open(test_pathname("crex/test-synop0.crex"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp, "CREX"), 1)

        # BUFR loaded as CREX yields no results
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            self.db.remove_all()
            self.assertEqual(self.db.load(fp, "CREX"), 0)

    def testQueryVolnd(self):
        from testlib import fill_volnd
        self.db.remove_all()
        fill_volnd(self.db)
        query = {}
        query["var"] = "B10004"
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        reports = []
        for cur in self.db.query_data(query):
            reports.append(cur["rep_memo"])
        s = "synop"
        t = "temp"
        self.assertEqual(reports, [s, s, s, s, s, s, t, t, t, t, t])

    def test_import_message(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            for binmsg in fp:
                for msg in importer.from_binary(binmsg):
                    self.db.import_messages(msg)
        self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_message_sequence(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            for binmsg in fp:
                self.db.import_messages(importer.from_binary(binmsg))
        self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_message_iter(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            for binmsg in fp:
                self.db.import_messages((x for x in importer.from_binary(binmsg)))
        self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_importerfile(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            self.db.import_messages(importer.from_file(fp))
        self.assertEqual(self.db.query_data({}).remaining, 371)


class FullDBTestMixin(CommonDBTestMixin):
    def test_transaction_enter_exit(self):
        with self.db.transaction() as tr:
            tr.insert_data({
                "lat": 12.34560, "lon": 76.54320,
                "mobile": 0,
                "datetime": datetime.datetime(1945, 4, 25, 9, 0, 0),
                "level": (10, 11, 15, 22),
                "trange": (20, 111, 222),
                "rep_memo": "synop",
                "B01011": "test",
            })
        self.assertEqual(len(list(self.db.query_data({"rep_memo": "synop"}))), 3)

        with self.assertRaises(RuntimeError):
            with self.db.transaction() as tr:
                tr.insert_data({
                    "lat": 12.34560, "lon": 76.54320,
                    "mobile": 0,
                    "datetime": datetime.datetime(1945, 4, 25, 10, 0, 0),
                    "level": (10, 11, 15, 22),
                    "trange": (20, 111, 222),
                    "rep_memo": "synop",
                    "B01011": "test",
                })
                raise RuntimeError("test rollback")
        self.assertEqual(len(list(self.db.query_data({"rep_memo": "synop"}))), 3)


class AttrTestMixin(object):
    def testLoadFileOverwriteAttrs(self):
        with io.open(test_pathname("bufr/issue91-withoutB33196.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp, attrs=True, overwrite=True)
            r = next(self.db.query_data())
            a = self.db.attr_query_data(r["context_id"])
            var = r["var"]
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enq(), 274.15)
            self.assertTrue("B33196" not in a)

        with io.open(test_pathname("bufr/issue91-withB33196.bufr"), "rb") as fp:
            self.db.load(fp, attrs=True, overwrite=True)
            r = next(self.db.query_data())
            a = self.db.attr_query_data(r["context_id"])
            var = r["var"]
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enq(), 273.15)
            self.assertTrue("B33196" in a)


class TransactionTestMixin(object):
    def get_db(self):
        db = super(TransactionTestMixin, self).get_db()
        return db.transaction()

#    def testConcurrentWrites(self):
# This deadlocks
#         insert_ids = self.db.insert_data({
#             "lat": 12.34560, "lon": 76.54320,
#             "mobile": 0,
#             "datetime": datetime.datetime(1945, 4, 25, 10, 0, 0),
#             "level": (10, 11, 15, 22),
#             "trange": (20, 111, 222),
#             "rep_memo": "synop",
#             "B01011": "test",
#             }, can_replace=True, can_add_stations=True)
#         self.db.commit()
#         data_id = insert_ids["B01011"]
#         db1 = dballe.DB.connect_test()
#         db2 = dballe.DB.connect_test()
#         with db1.transaction() as tr1:
#             with db2.transaction() as tr2:
#                 tr1.insert_data({
#                     "lat": 12.34560, "lon": 76.54320,
#                     "mobile": 0,
#                     "datetime": datetime.datetime(1945, 4, 25, 10, 0, 0),
#                     "level": (10, 11, 15, 22),
#                     "trange": (20, 111, 222),
#                     "rep_memo": "synop",
#                     "B01011": "test1",
#                 }, can_replace=True)
#                 tr2.attr_insert_data(data_id, { "B33007": 50.0 })

# This deadlocks:
#         db1 = dballe.DB.connect_test()
#         db2 = dballe.DB.connect_test()
#         with db1.transaction() as tr1:
#             with db2.transaction() as tr2:
#                 tr1.insert_data({
#                     "lat": 12.34560, "lon": 76.54320,
#                     "mobile": 0,
#                     "datetime": datetime.datetime(1945, 4, 25, 10, 0, 0),
#                     "level": (10, 11, 15, 22),
#                     "trange": (20, 111, 222),
#                     "rep_memo": "synop",
#                     "B01011": "test",
#                 }, can_replace=True, can_add_stations=True)
#                 tr2.insert_data({
#                     "lat": 12.34560, "lon": 76.54320,
#                     "mobile": 0,
#                     "datetime": datetime.datetime(1945, 4, 25, 10, 0, 0),
#                     "level": (10, 11, 15, 22),
#                     "trange": (20, 111, 222),
#                     "rep_memo": "synop",
#                     "B01011": "test",
#                 }, can_replace=True, can_add_stations=True)


class DballeV7Test(FullDBTestMixin, AttrTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeV7TransactionTest(TransactionTestMixin, CommonDBTestMixin, AttrTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeMEMTest(FullDBTestMixin, AttrTestMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


class DballeMEMTransactionTest(TransactionTestMixin, CommonDBTestMixin, AttrTestMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


if __name__ == "__main__":
    from testlib import main
    main("test-db")
