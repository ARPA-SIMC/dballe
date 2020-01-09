#!/usr/bin/env python3
import dballe
import io
import datetime
import unittest
import warnings
from contextlib import contextmanager
from decimal import Decimal
from testlib import DballeDBMixin, test_pathname


class CommonDBTestMixin(DballeDBMixin):
    @contextmanager
    def deprecated_on_db(self):
        """
        Make sure that the test within, when run ouside a transaction, raises a
        DeprecationWarning
        """
        with warnings.catch_warnings(record=True) as warning_list:
            yield
        if self.raise_db_method_deprecation_warnings:
            for w in warning_list:
                if w.category != DeprecationWarning:
                    continue
                if "without a transaction is deprecated" not in w.message.args[0]:
                    continue
                # found = w["message"]
                break
            else:
                self.fail("DeprecationWarning not raised")

    @contextmanager
    def assert_deprecated(self, msg):
        """
        Make sure this function raises DeprecationWarning
        """
        with warnings.catch_warnings(record=True) as warning_list:
            yield

        for w in warning_list:
            if w.category != DeprecationWarning:
                continue
            if "please use" not in w.message.args[0]:
                continue
            if msg not in w.message.args[0]:
                self.fail("{} does not contain {}".format(repr(w.message.args[0]), repr(msg)))
            break
        else:
            self.fail("DeprecationWarning not raised")

    def setUp(self):
        super(CommonDBTestMixin, self).setUp()
        self.raise_db_method_deprecation_warnings = True

        with self.transaction() as tr:
            data = dict(
                    lat=12.34560, lon=76.54320,
                    datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
                    level=(10, 11, 15, 22),
                    trange=(20, 111, 222),
                    rep_memo="synop",
                    B01011="Hey Hey!!",
                    B01012=500)
            ids = tr.insert_data(data, False, True)
            tr.insert_station_data({"ana_id": ids["ana_id"], "B02005": 0.5})

            data.clear()
            data["B33007"] = 50
            data["B33036"] = 75
            tr.attr_insert_data(ids["B01011"], data)

            for rec in tr.query_data(dict(var="B01011")):
                self.attr_ref = rec["context_id"]

    def testQueryExport(self):
        with self.assert_deprecated("please use query_messages instead of export_to_file"):
            self.db.export_to_file({}, "BUFR", "/dev/null")
        with self.assert_deprecated("please use query_messages instead of export_to_file"):
            self.db.export_to_file({}, "CREX", "/dev/null")
        with self.assert_deprecated("please use query_messages instead of export_to_file"):
            self.db.export_to_file({}, "BUFR", "/dev/null", generic=True)
        with self.assert_deprecated("please use query_messages instead of export_to_file"):
            self.db.export_to_file({}, "CREX", "/dev/null", generic=True)

    def testExportFileObject(self):
        out = io.BytesIO()
        with self.assert_deprecated("please use query_messages instead of export_to_file"):
            self.db.export_to_file({}, "BUFR", out)
        self.assertTrue(out.getvalue().startswith(b"BUFR"))

    def testQueryStations(self):
        with self.deprecated_on_db():
            cur = self.db.query_stations()
        self.assertEqual(cur.remaining, 1)
        count = 0
        for idx, result in enumerate(cur):
            self.assertEqual(result["lat"], Decimal("12.34560"))
            self.assertEqual(result["lon"], Decimal("76.54320"))
            self.assertEqual(result.enqi("lat"), 1234560)
            self.assertEqual(result.enqd("lat"), 12.34560)
            self.assertEqual(result.enqs("lat"), "1234560")
            self.assertEqual(result.enqf("lat"), "12.34560")
            self.assertNotIn("B01011", result)
            count += 1
        self.assertEqual(count, 1)

    def testQueryDecimal(self):
        with self.deprecated_on_db():
            cur = self.db.query_stations({"lat": Decimal("12.34560"), "lon": Decimal("76.54320")})
        self.assertEqual(cur.remaining, 1)
        count = 0
        for idx, result in enumerate(cur):
            self.assertEqual(result["lat"], Decimal("12.34560"))
            self.assertEqual(result["lon"], Decimal("76.54320"))
            self.assertEqual(result.enqi("lat"), 1234560)
            self.assertEqual(result.enqd("lat"), 12.34560)
            self.assertEqual(result.enqs("lat"), "1234560")
            self.assertEqual(result.enqf("lat"), "12.34560")
            self.assertNotIn("B01011", result)
            count += 1
        self.assertEqual(count, 1)

    def testQueryStationData(self):
        with self.deprecated_on_db():
            cur = self.db.query_station_data({"latmin": 10.0})
        self.assertEqual(cur.remaining, 1)
        for idx, result in enumerate(cur):
            self.assertEqual(cur.remaining, 0)
            self.assertEqual(result.enqi("lat"), 1234560)
            self.assertEqual(result.enqd("lat"), 12.34560)
            self.assertEqual(result.enqs("lat"), "1234560")
            self.assertEqual(result.enqf("lat"), "12.34560")
            var = result["variable"]
            self.assertEqual(var.code, "B02005")
            self.assertEqual(var.enq(), Decimal("0.5"))
            # FIXME: this should trigger a query: how do we test it?
            # self.assertEqual({k: v.enq() for k, v in result.query_attrs().items()}, expected[idx]["attrs"])
            self.assertEqual(result.query_attrs(), {})

    def testQueryData(self):
        expected = [
            {"code": "B01011", "val": "Hey Hey!!", "attrs": {'B33007': 50, 'B33036': 75}},
            {"code": "B01012", "val": 500, "attrs": {}},
        ]

        with self.deprecated_on_db():
            cur = self.db.query_data({"latmin": 10.0})
        self.assertEqual(cur.remaining, 2)
        for idx, result in enumerate(cur):
            self.assertEqual(cur.remaining, 2-idx-1)
            self.assertEqual(result["lat"], Decimal("12.34560"))
            self.assertEqual(result.enqi("lat"), 1234560)
            self.assertEqual(result.enqd("lat"), 12.34560)
            self.assertEqual(result.enqs("lat"), "1234560")
            self.assertEqual(result.enqf("lat"), "12.34560")
            self.assertIsNone(result["ident"])
            self.assertIsNone(result.enqs("ident"))
            self.assertIsNone(result.enqf("ident"))
            var = result["variable"]
            self.assertEqual(var.code, expected[idx]["code"])
            self.assertEqual(var.enq(), expected[idx]["val"])
            # FIXME: this should trigger a query: how do we test it?
            self.assertEqual({k: v.enq() for k, v in result.query_attrs().items()}, expected[idx]["attrs"])

    def testQueryDataCursorAccess(self):
        def assertResultIntEqual(result, name, value):
            self.assertEqual(result[name], value)
            self.assertEqual(result.enqi(name), value)
            self.assertEqual(result.enqs(name), str(value))
            self.assertEqual(result.enqd(name), float(value))
            self.assertEqual(result.enqf(name), str(value))

        def assertResultStringEqual(result, name, value):
            self.assertEqual(result[name], value)
            self.assertRaises(RuntimeError, result.enqi, name)
            self.assertRaises(RuntimeError, result.enqd, name)
            self.assertEqual(result.enqs(name), value)
            self.assertEqual(result.enqf(name), value)

        with self.deprecated_on_db():
            with self.db.query_data({"latmin": 10.0, "var": "B01011", "query": "attrs"}) as cur:
                self.assertEqual(cur.remaining, 1)
                for result in cur:
                    self.assertEqual(cur.remaining, 0)

                    assertResultIntEqual(result, "priority", 101)

                    assertResultStringEqual(result, "rep_memo", "synop")
                    assertResultStringEqual(result, "report", "synop")

                    # self.assertEqual(result.enqs("ana_id"), ?)

                    self.assertIsNone(result["ident"])
                    self.assertRaises(RuntimeError, result.enqi, "ident")
                    self.assertRaises(RuntimeError, result.enqd, "ident")
                    self.assertIsNone(result.enqs("ident"))
                    self.assertIsNone(result.enqf("ident"))

                    self.assertIs(result["mobile"], False)
                    self.assertEqual(result.enqi("mobile"), 0)
                    self.assertEqual(result.enqd("mobile"), 0)
                    self.assertEqual(result.enqs("mobile"), "0")
                    self.assertEqual(result.enqf("mobile"), "0")

                    self.assertEqual(result["lat"], Decimal("12.34560"))
                    self.assertEqual(result.enqi("lat"), 1234560)
                    self.assertEqual(result.enqd("lat"), 12.34560)
                    self.assertEqual(result.enqs("lat"), "1234560")
                    self.assertEqual(result.enqf("lat"), "12.34560")

                    self.assertEqual(result["lon"], Decimal("76.54320"))
                    self.assertEqual(result.enqi("lon"), 7654320)
                    self.assertEqual(result.enqd("lon"), 76.54320)
                    self.assertEqual(result.enqs("lon"), "7654320")
                    self.assertEqual(result.enqf("lon"), "76.54320")

                    self.assertEqual(result["coords"], (Decimal("12.34560"), Decimal("76.54320")))
                    self.assertRaises(RuntimeError, result.enqi, "coords")
                    self.assertRaises(RuntimeError, result.enqd, "coords")
                    self.assertRaises(RuntimeError, result.enqs, "coords")
                    self.assertRaises(RuntimeError, result.enqf, "coords")

                    self.assertEqual(result["station"], dballe.Station("synop", 12.345600, 76.543200, None))
                    self.assertRaises(RuntimeError, result.enqi, "station")
                    self.assertRaises(RuntimeError, result.enqd, "station")
                    self.assertRaises(RuntimeError, result.enqs, "station")
                    self.assertRaises(RuntimeError, result.enqf, "station")

                    self.assertEqual(result["datetime"], datetime.datetime(1945, 4, 25, 8, 0))
                    self.assertRaises(RuntimeError, result.enqi, "datetime")
                    self.assertRaises(RuntimeError, result.enqd, "datetime")
                    self.assertRaises(RuntimeError, result.enqs, "datetime")
                    self.assertRaises(RuntimeError, result.enqf, "datetime")

                    assertResultIntEqual(result, "year", 1945)
                    assertResultIntEqual(result, "month", 4)
                    assertResultIntEqual(result, "day", 25)
                    assertResultIntEqual(result, "hour", 8)
                    assertResultIntEqual(result, "min", 0)
                    assertResultIntEqual(result, "sec", 0)

                    self.assertEqual(result["level"], dballe.Level(10, 11, 15, 22))
                    self.assertRaises(RuntimeError, result.enqi, "level")
                    self.assertRaises(RuntimeError, result.enqd, "level")
                    self.assertRaises(RuntimeError, result.enqs, "level")
                    self.assertRaises(RuntimeError, result.enqf, "level")

                    assertResultIntEqual(result, "leveltype1", 10)
                    assertResultIntEqual(result, "l1", 11)
                    assertResultIntEqual(result, "leveltype2", 15)
                    assertResultIntEqual(result, "l2", 22)

                    self.assertEqual(result["trange"], dballe.Trange(20, 111, 222))
                    self.assertRaises(RuntimeError, result.enqi, "trange")
                    self.assertRaises(RuntimeError, result.enqd, "trange")
                    self.assertRaises(RuntimeError, result.enqs, "trange")
                    self.assertRaises(RuntimeError, result.enqf, "trange")

                    assertResultIntEqual(result, "pindicator", 20)
                    assertResultIntEqual(result, "p1", 111)
                    assertResultIntEqual(result, "p2", 222)

                    assertResultStringEqual(result, "var", "B01011")

                    # Because we use query=attrs, result["variable"] includes attributes
                    expected = dballe.var('B01011', 'Hey Hey!!')
                    expected.seta(dballe.var('B33007', 50))
                    expected.seta(dballe.var('B33036', 75))
                    self.assertEqual(result["variable"], expected)
                    self.assertRaises(RuntimeError, result.enqi, "variable")
                    self.assertRaises(RuntimeError, result.enqd, "variable")
                    self.assertRaises(RuntimeError, result.enqs, "variable")
                    self.assertRaises(RuntimeError, result.enqf, "variable")

                    self.assertEqual(result["attrs"], [dballe.var('B33007', 50), dballe.var('B33036', 75)])
                    self.assertRaises(RuntimeError, result.enqi, "attrs")
                    self.assertRaises(RuntimeError, result.enqd, "attrs")
                    self.assertRaises(RuntimeError, result.enqs, "attrs")
                    self.assertRaises(RuntimeError, result.enqf, "attrs")

                    # self.assertEqual(result.enqs("context_id"), ?)

                    # # FIXME: when not using query=attrs, this should trigger a query: how do we test it?
                    # self.assertEqual({k: v.enq() for k, v in result.query_attrs().items()}, expected[idx]["attrs"])

    def testQueryFromCommandLine(self):
        with self.deprecated_on_db():
            cur = self.db.query_data({
                'yearmin': '1945', 'monthmin': '4', 'daymin': '25', 'hourmin': '08',
                'yearmax': '1945', 'monthmax': '4', 'daymax': '25', 'hourmax': '08',
                'latmin': '12.34560', 'latmax': '12.34560',
                'lonmin': '76.54320', 'lonmax': '76.54320',
            })
        self.assertEqual(cur.remaining, 2)

    def testQueryValidation(self):
        with self.deprecated_on_db():
            cur = self.db.query_data({"yearmax": 1945, "monthmax": 4, "daymax": 25, "hourmax": 8})
        self.assertEqual(cur.remaining, 2)

    def testQueryLongitude(self):
        # See issue #177
        with self.deprecated_on_db():
            cur = self.db.query_data({"latmin": 12.0, "latmax": 13.0, "lonmin": 76.0, "lonmax": 77.0})
        self.assertEqual(cur.remaining, 2)

    def testQueryDataAttrs(self):
        expected = [
            {"code": "B01011", "val": "Hey Hey!!", "attrs": {'B33007': 50, 'B33036': 75}},
            {"code": "B01012", "val": 500, "attrs": {}},
        ]

        with self.deprecated_on_db():
            cur = self.db.query_data({"latmin": 10.0, "query": "attrs"})
        self.assertEqual(cur.remaining, 2)
        for idx, result in enumerate(cur):
            self.assertEqual(cur.remaining, 2-idx-1)
            var = result["variable"]
            self.assertEqual(var.code, expected[idx]["code"])
            self.assertEqual(var.enq(), expected[idx]["val"])
            # FIXME: this should NOT trigger a query: how do we test it?
            self.assertEqual({k: v.enq() for k, v in result.query_attrs().items()}, expected[idx]["attrs"])

    def testQueryDataLimit(self):
        with self.deprecated_on_db():
            cur = self.db.query_data({"limit": 1})
        self.assertEqual(cur.remaining, 1)

    def testQueryAttrs(self):
        with self.deprecated_on_db():
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
        with self.deprecated_on_db():
            cur = self.db.query_data({"var": "B01011"})
        data = next(cur)
        self.assertTrue(data)

        attrs = cur.query_attrs()

        expected = {}
        expected["B33007"] = 50
        expected["B33036"] = 75

        count = 0
        for code, var in attrs.items():
            assert var.code in expected
            self.assertEqual(var.enq(), expected[var.code])
            del expected[var.code]
            count = count + 1
        self.assertEqual(count, 2)

    def testQuerySummary(self):
        res = {}
        with self.deprecated_on_db():
            for result in self.db.query_summary({"query": "details"}):
                self.assertEqual(result["lat"], Decimal("12.34560"))
                self.assertEqual(result.enqi("lat"), 1234560)
                self.assertEqual(result.enqd("lat"), 12.34560)
                self.assertEqual(result.enqs("lat"), "1234560")
                self.assertEqual(result.enqf("lat"), "12.34560")
                res[(result["ana_id"], result["rep_memo"], result["level"], result["trange"], result["var"])] = (
                    result["datetimemin"], result["datetimemax"], result["count"])
        self.assertEqual(
                res[(1, "synop", dballe.Level(10, 11, 15, 22), dballe.Trange(20, 111, 222), 'B01011')],
                (datetime.datetime(1945, 4, 25, 8, 0), datetime.datetime(1945, 4, 25, 8, 0), 1))
        self.assertEqual(
                res[(1, "synop", dballe.Level(10, 11, 15, 22), dballe.Trange(20, 111, 222), 'B01012')],
                (datetime.datetime(1945, 4, 25, 8, 0), datetime.datetime(1945, 4, 25, 8, 0), 1))

    def testQueryMessages(self):
        with self.deprecated_on_db():
            cur = self.db.query_messages({"latmin": 10.0})
        self.assertEqual(cur.remaining, 1)
        for idx, result in enumerate(cur):
            self.assertEqual(cur.remaining, 2-idx-1)
            msg = cur.message
            self.assertEqual(msg.type, "synop")
            self.assertEqual(msg.datetime, datetime.datetime(1945, 4, 25, 8, 0, 0))
            self.assertEqual(msg.coords, (Decimal("12.34560"), Decimal("76.54320")))
            self.assertIsNone(msg.ident)
            self.assertEqual(msg.report, "synop")

    def testCursorStationsRemove(self):
        with self.deprecated_on_db():
            with self.db.query_stations() as cur:
                for row in cur:
                    row.remove()

    def testCursorStationDataRemove(self):
        with self.deprecated_on_db():
            with self.db.query_station_data() as cur:
                for row in cur:
                    row.remove()

    def testCursorDataRemove(self):
        with self.deprecated_on_db():
            with self.db.query_data() as cur:
                for row in cur:
                    row.remove()

    def testCursorSummaryRemove(self):
        with self.deprecated_on_db():
            with self.db.query_summary() as cur:
                for row in cur:
                    row.remove()

    def testAttrRemove(self):
        with self.deprecated_on_db():
            self.db.attr_remove_data(self.attr_ref, ("B33007",))

    def testAttrInsertCursorStation(self):
        with self.deprecated_on_db():
            with self.db.query_station_data() as cur:
                for row in cur:
                    row.insert_attrs({"B33007": 42})

    def testAttrInsertCursorData(self):
        with self.deprecated_on_db():
            with self.db.query_data() as cur:
                for row in cur:
                    row.insert_attrs({"B33007": 42})

    def testAttrRemoveCursorStation(self):
        with self.deprecated_on_db():
            with self.db.query_station_data() as cur:
                for row in cur:
                    row.remove_attrs()

    def testAttrRemoveCursorData(self):
        with self.deprecated_on_db():
            with self.db.query_data() as cur:
                for row in cur:
                    row.remove_attrs()

    def testLoadFile(self):
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp)
            with self.deprecated_on_db():
                self.assertTrue(self.db.query_data().remaining > 0)

    def testLoadFileLike(self):
        with io.open(test_pathname("bufr/vad.bufr"), "rb") as fp:
            s = io.BytesIO(fp.read())
            self.db.remove_all()
            self.db.load(s)
            with self.deprecated_on_db():
                self.assertTrue(self.db.query_data().remaining > 0)

    def testLoadFileWithAttrs(self):
        with io.open(test_pathname("bufr/issue91-withB33196.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp, attrs=True)
            with self.deprecated_on_db():
                with self.db.query_data() as cur:
                    rec = next(cur)
                    context_id = rec["context_id"]
            with self.deprecated_on_db():
                a = self.db.attr_query_data(context_id)
            self.assertTrue("B33196" in a)

    def testLoadFileOverwrite(self):
        with io.open(test_pathname("bufr/issue91-withoutB33196.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp, overwrite=True)
            with self.deprecated_on_db():
                with self.db.query_data() as cur:
                    rec = next(cur)
                    context_id = rec["context_id"]
                    var = rec["variable"]
            with self.deprecated_on_db():
                self.db.attr_query_data(context_id)  # cannot verify the result, but expecting not to raise
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enqd(), 274.15)

        with io.open(test_pathname("bufr/issue91-withB33196.bufr"), "rb") as fp:
            self.db.load(fp, overwrite=True)
            with self.deprecated_on_db():
                with self.db.query_data() as cur:
                    rec = next(cur)
                    context_id = rec["context_id"]
                    var = rec["variable"]
            with self.deprecated_on_db():
                self.db.attr_query_data(context_id)  # cannot verify the result, but expecting not to raise)
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
            with self.deprecated_on_db():
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
        with self.transaction() as tr:
            fill_volnd(tr)
        query = {}
        query["var"] = "B10004"
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        reports = []
        with self.deprecated_on_db():
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
        with self.deprecated_on_db():
            self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_message_sequence(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            for binmsg in fp:
                self.db.import_messages(importer.from_binary(binmsg))
        with self.deprecated_on_db():
            self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_message_iter(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            for binmsg in fp:
                self.db.import_messages((x for x in importer.from_binary(binmsg)))
        with self.deprecated_on_db():
            self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_importerfile(self):
        importer = dballe.Importer("BUFR")
        with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
            self.db.import_messages(importer.from_file(fp))
        with self.deprecated_on_db():
            self.assertEqual(self.db.query_data({}).remaining, 371)

    def test_import_varlist(self):
        with self.transaction() as tr:
            tr.remove_all()
            importer = dballe.Importer("BUFR")
            with dballe.File(test_pathname("bufr/vad.bufr")) as fp:
                tr.import_messages(importer.from_file(fp), varlist="B11001,B11002")
            for cur in tr.query_data():
                self.assertIn(cur["var"], ("B11001", "B11002"))

    def test_query_attrs(self):
        # See #114
        with self.deprecated_on_db():
            with self.db.query_data({"var": "B01011"}) as cur:
                self.assertEqual(cur.remaining, 1)
                for row in cur:
                    self.assertEqual(row["variable"].code, "B01011")
                    self.assertCountEqual(row["attrs"], [])

        with self.deprecated_on_db():
            with self.db.query_data({"var": "B01011", "query": "attrs"}) as cur:
                self.assertEqual(cur.remaining, 1)
                for row in cur:
                    self.assertEqual(row["variable"].code, "B01011")
                    self.assertCountEqual((repr(x) for x in row["attrs"]), ["Var('B33007', 50)", "Var('B33036', 75)"])

    def test_delete_by_context_id(self):
        # See issue #140
        records_to_del = []
        with self.deprecated_on_db():
            with self.db.query_data() as cur:
                for rec in cur:
                    records_to_del.append(rec.query)
        self.assertEqual(len(records_to_del), 2)

        query = records_to_del[0]
        self.assertEqual(query, {
            "report": "synop",
            "mobile": False,
            "lat": Decimal("12.34560"),
            "lon": Decimal("76.54320"),
            "level": dballe.Level(10, 11, 15, 22),
            "trange": dballe.Trange(20, 111, 222),
            "datetime": datetime.datetime(1945, 4, 25, 8, 0),
            "var": "B01011",
        })

        with self.deprecated_on_db():
            self.db.remove_data(records_to_del[0])
        with self.deprecated_on_db():
            self.assertEqual(self.db.query_data().remaining, 1)

    def test_delete_by_var(self):
        # See issue #141
        with self.deprecated_on_db():
            with self.db.query_data() as cur:
                self.assertEqual(cur.remaining, 2)
        with self.deprecated_on_db():
            self.db.remove_data({"var": "B01011"})
        with self.deprecated_on_db():
            with self.db.query_data() as cur:
                self.assertEqual(cur.remaining, 1)

        data = {
            "lat": 12.34560, "lon": 76.54320, "rep_memo": "synop",
            "B01011": "Hey Hey!!",
        }
        with self.deprecated_on_db():
            self.db.insert_station_data(data, False, True)
        with self.deprecated_on_db():
            with self.db.query_station_data() as cur:
                self.assertEqual(cur.remaining, 2)
        with self.deprecated_on_db():
            self.db.remove_station_data({"var": "B02005"})
        with self.deprecated_on_db():
            with self.db.query_station_data() as cur:
                self.assertEqual(cur.remaining, 1)

    def test_data(self):
        with self.deprecated_on_db():
            with self.db.query_station_data() as cur:
                self.assertEqual(cur.remaining, 1)
                for row in cur:
                    self.assertEqual(row.data_dict, {
                        "report": "synop",
                        "lat": Decimal("12.34560"),
                        "lon": Decimal("76.54320"),
                        "B02005": 0.5,
                    })

        with self.deprecated_on_db():
            with self.db.query_data({"var": "B01012"}) as cur:
                self.assertEqual(cur.remaining, 1)
                for row in cur:
                    self.assertEqual(row.data_dict, {
                        "report": "synop",
                        "lat": Decimal("12.34560"),
                        "lon": Decimal("76.54320"),
                        "level": dballe.Level(10, 11, 15, 22),
                        "trange": dballe.Trange(20, 111, 222),
                        "datetime": datetime.datetime(1945, 4, 25, 8, 0),
                        "B01012": 500,
                    })

    def test_insert_cursor(self):
        with self.another_db() as db1:
            with db1.transaction() as tr1:
                with self.deprecated_on_db():
                    with self.db.query_station_data() as cur:
                        for row in cur:
                            tr1.insert_station_data(row, can_add_stations=True)

                with self.deprecated_on_db():
                    with self.db.query_data() as cur:
                        for row in cur:
                            tr1.insert_data(row, can_add_stations=True)

            with db1.transaction() as tr:
                with tr.query_station_data() as cur:
                    self.assertEqual(cur.remaining, 1)
                    for row in cur:
                        self.assertEqual(row.data_dict, {
                            "report": "synop",
                            "lat": Decimal("12.34560"),
                            "lon": Decimal("76.54320"),
                            "B02005": 0.5,
                        })

                with tr.query_data({"var": "B01012"}) as cur:
                    self.assertEqual(cur.remaining, 1)
                    for row in cur:
                        self.assertEqual(row.data_dict, {
                            "report": "synop",
                            "lat": Decimal("12.34560"),
                            "lon": Decimal("76.54320"),
                            "level": dballe.Level(10, 11, 15, 22),
                            "trange": dballe.Trange(20, 111, 222),
                            "datetime": datetime.datetime(1945, 4, 25, 8, 0),
                            "B01012": 500,
                        })

    def test_insert_cursor_data(self):
        with self.another_db() as db1:
            with db1.transaction() as tr1:
                with self.deprecated_on_db():
                    with self.db.query_station_data() as cur:
                        for row in cur:
                            tr1.insert_station_data(row.data, can_add_stations=True)

                with self.deprecated_on_db():
                    with self.db.query_data() as cur:
                        for row in cur:
                            tr1.insert_data(row.data, can_add_stations=True)

            with db1.transaction() as tr:
                with tr.query_station_data() as cur:
                    self.assertEqual(cur.remaining, 1)
                    for row in cur:
                        self.assertEqual(row.data_dict, {
                            "report": "synop",
                            "lat": Decimal("12.34560"),
                            "lon": Decimal("76.54320"),
                            "B02005": 0.5,
                        })

                with tr.query_data({"var": "B01012"}) as cur:
                    self.assertEqual(cur.remaining, 1)
                    for row in cur:
                        self.assertEqual(row.data_dict, {
                            "report": "synop",
                            "lat": Decimal("12.34560"),
                            "lon": Decimal("76.54320"),
                            "level": dballe.Level(10, 11, 15, 22),
                            "trange": dballe.Trange(20, 111, 222),
                            "datetime": datetime.datetime(1945, 4, 25, 8, 0),
                            "B01012": 500,
                        })

    def test_issue158(self):
        with self.transaction() as tr:
            tr.remove_all()
            tr.insert_data({
                "lon": 1212345,
                "lat": 4312345,
                "rep_memo": "test1",
                "datetime": datetime.datetime.now(),
                "level": (103, 2000),
                "trange": (254, 0, 0),
                "B12101": 0
            }, can_add_stations=True)

            for row in tr.query_data({"report": "test1"}):
                data = row.data
                self.assertEqual(data["report"], "test1")
                self.assertEqual(data["rep_memo"], "test1")
                data["rep_memo"] = "test2"
                self.assertEqual(data["report"], "test2")
                self.assertEqual(data["rep_memo"], "test2")
                tr.insert_data(data, can_add_stations=True)

        with self.transaction() as tr:
            reports = []
            for row in tr.query_data():
                reports.append(row["report"])

        self.assertEqual(reports, ["test1", "test2"])

    def test_insert_new(self):
        with self.transaction() as tr:
            with self.assertRaises(KeyError) as e:
                tr.insert_data({
                    "report": "synop",
                    "lat": 44.5, "lon": 11.4,
                    "level": dballe.Level(1),
                    "trange": dballe.Trange(254),
                    "datetime": datetime.datetime(2013, 4, 25, 12, 0, 0),
                    "B12101": 22.4,
                    "B12103": 17.2,
                })
            self.assertEqual(str(e.exception), "'station not found in the database'")

    def test_cursor_delete(self):
        # See: #140
        with self.transaction() as tr:
            for v in tr.query_station_data({"var": "B02005"}):
                self.assertEqual(v["B02005"].enqd(), 0.5)
                v.remove()

            count = 0
            for v in tr.query_station_data():
                count += 1
            self.assertEqual(count, 0)

        with self.transaction() as tr:
            for v in tr.query_data({"var": "B01011"}):
                self.assertEqual(v["B01011"].enqc(), "Hey Hey!!")
                v.remove()

            for v in tr.query_data():
                self.assertEqual(v["var"], "B01012")
                self.assertEqual(v["B01012"].enqi(), 500)

    def test_var_rename(self):
        # See: #191
        with self.transaction() as tr:
            with tr.query_data({"var": "B01011"}) as cur:
                self.assertEqual(cur.remaining, 1)
                for rec in cur:
                    data = rec.data
                    rec.remove()
                    data["B01008"] = data["B01011"]
                    del data["B01011"]
                    tr.insert_data(data)

        with self.transaction() as tr:
            with tr.query_data({"var": "B01011"}) as cur:
                self.assertEqual(cur.remaining, 0)

            with tr.query_data({"var": "B01008"}) as cur:
                self.assertEqual(cur.remaining, 1)


class FullDBTestMixin(CommonDBTestMixin):
    def test_transaction_enter_exit(self):
        with self.db.transaction() as tr:
            tr.insert_data({
                "lat": 12.34560, "lon": 76.54320,
                "datetime": datetime.datetime(1945, 4, 25, 9, 0, 0),
                "level": (10, 11, 15, 22),
                "trange": (20, 111, 222),
                "rep_memo": "synop",
                "B01011": "test",
            })
        with self.deprecated_on_db():
            self.assertEqual(len(list(self.db.query_data({"rep_memo": "synop"}))), 3)

        with self.assertRaises(RuntimeError):
            with self.db.transaction() as tr:
                tr.insert_data({
                    "lat": 12.34560, "lon": 76.54320,
                    "datetime": datetime.datetime(1945, 4, 25, 10, 0, 0),
                    "level": (10, 11, 15, 22),
                    "trange": (20, 111, 222),
                    "rep_memo": "synop",
                    "B01011": "test",
                })
                raise RuntimeError("test rollback")
        with self.deprecated_on_db():
            self.assertEqual(len(list(self.db.query_data({"rep_memo": "synop"}))), 3)


class AttrTestMixin(object):
    def testLoadFileOverwriteAttrs(self):
        with io.open(test_pathname("bufr/issue91-withoutB33196.bufr"), "rb") as fp:
            self.db.remove_all()
            self.db.load(fp, attrs=True, overwrite=True)
            with self.deprecated_on_db():
                with self.db.query_data() as cur:
                    rec = next(cur)
                    context_id = rec["context_id"]
                    var = rec["variable"]
            with self.deprecated_on_db():
                a = self.db.attr_query_data(context_id)
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enq(), 274.15)
            self.assertTrue("B33196" not in a)

        with io.open(test_pathname("bufr/issue91-withB33196.bufr"), "rb") as fp:
            self.db.load(fp, attrs=True, overwrite=True)
            with self.deprecated_on_db():
                with self.db.query_data() as cur:
                    rec = next(cur)
                    context_id = rec["context_id"]
                    var = rec["variable"]
            with self.deprecated_on_db():
                a = self.db.attr_query_data(context_id)
            self.assertEqual(var.code, "B12101")
            self.assertEqual(var.enq(), 273.15)
            self.assertTrue("B33196" in a)


class TransactionTestMixin(object):
    def setUp(self):
        super().setUp()
        self.raise_db_method_deprecation_warnings = False

    def get_db(self):
        db = super(TransactionTestMixin, self).get_db()
        return db.transaction()

    @contextmanager
    def transaction(self):
        yield self.db

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


if __name__ == "__main__":
    from testlib import main
    main("test-db")
