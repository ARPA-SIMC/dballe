import dballe
import datetime
import unittest
import shutil
import os
from testlib import DballeDBMixin, test_pathname


class BaseExplorerTestMixin(DballeDBMixin):
    def setUp(self):
        super().setUp()
        with self.db.transaction() as tr:
            data = dict(
                    lat=12.34560, lon=76.54320,
                    datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
                    level=(10, 11, 15, 22),
                    trange=(20, 111, 222),
                    rep_memo="synop",
                    B01011="test1",
                    B01012=500)
            tr.insert_data(data, False, True)

            data = dict(
                    lat=12.34560, lon=76.54320,
                    ident="foo",
                    datetime=datetime.datetime(1945, 4, 25, 12, 0, 0),
                    level=(10, 11, 15, 22),
                    trange=(20, 111, 223),
                    rep_memo="amdar",
                    B01012=500)
            tr.insert_data(data, False, True)

        # Track explorers created during the test case
        self.explorers = set()

    def tearDown(self):
        # Cleanup persistent explorers created during the test case
        for name in self.explorers:
            self._cleanup(name)

    def _cleanup(self, name):
        if os.path.isdir(name):
            shutil.rmtree(name)
        elif os.path.exists(name):
            os.unlink(name)

    def _explorer(self, name=None, *args, **kw):
        if name is None:
            name = self.DEFAULT_EXPLORER_NAME
        if name not in self.explorers:
            self._cleanup(name)
        self.explorers.add(name)
        return self._make_explorer(name, *args, **kw)

    def assertExplorerContents(self, explorer, count_unfiltered=3, count_filtered=1):
        self.assertCountEqual(explorer.all_stations, [
            self._station("synop", 1, 12.34560, 76.54320, None),
            self._station("amdar", 2, 12.34560, 76.54320, "foo"),
        ])
        self.assertCountEqual(explorer.stations, [
            self._station("amdar", 2, 12.34560, 76.54320, "foo"),
        ])
        self.assertCountEqual(explorer.all_reports, ["amdar", "synop"])
        self.assertEqual(explorer.reports, ["amdar"])
        self.assertEqual(explorer.all_levels, [(10, 11, 15, 22)])
        self.assertEqual(explorer.levels, [(10, 11, 15, 22)])
        self.assertEqual(explorer.all_tranges, [(20, 111, 222), (20, 111, 223)])
        self.assertEqual(explorer.tranges, [(20, 111, 223)])
        self.assertEqual(explorer.all_varcodes, ["B01011", "B01012"])
        self.assertEqual(explorer.varcodes, ["B01012"])
        self.assertEqual(explorer.all_stats, dballe.ExplorerStats((
            datetime.datetime(1945, 4, 25,  8, 0), datetime.datetime(1945, 4, 25, 12, 0), count_unfiltered)))
        self.assertEqual(explorer.stats, dballe.ExplorerStats((
            datetime.datetime(1945, 4, 25, 12, 0), datetime.datetime(1945, 4, 25, 12, 0), count_filtered)))

    def test_create(self):
        explorer = self._explorer("e1")
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)
        explorer.set_filter({"rep_memo": "amdar"})

        self.assertStrRepr(explorer)
        self.assertExplorerContents(explorer)

        json_string = explorer.to_json()
        self.assertIn('{"summary":{', json_string)

        explorer1 = self._explorer("e2")
        with explorer1.update() as update:
            update.add_json(json_string)
        explorer1.set_filter({"rep_memo": "amdar"})
        self.assertExplorerContents(explorer1)

    def test_merge(self):
        explorer = self._explorer("e1")
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)

        explorer1 = self._explorer("e2")
        with explorer1.rebuild() as update:
            update.add_explorer(explorer)
            update.add_explorer(explorer)
        explorer1.set_filter({"rep_memo": "amdar"})

        self.assertExplorerContents(explorer1, count_unfiltered=6, count_filtered=2)

    def test_query_summary(self):
        with self._explorer() as explorer:
            with explorer.rebuild() as update:
                with self.db.transaction() as tr:
                    update.add_db(tr)
            explorer.set_filter({"rep_memo": "amdar"})

            with explorer.query_summary_all() as cur:
                rows = list(cur)
            self.assertEqual(len(rows), 3)

            with explorer.query_summary() as cur:
                rows = list(cur)
            self.assertEqual(len(rows), 1)

            with explorer.query_summary_all({"rep_memo": "amdar"}) as cur:
                rows = list(cur)
            self.assertEqual(len(rows), 1)

    def test_create_from_msgs(self):
        importer = dballe.Importer("BUFR")

        with self._explorer() as explorer:
            with explorer.rebuild() as update:
                with importer.from_file(test_pathname("bufr/gts-acars-uk1.bufr")) as imp:
                    update.add_messages(imp)
            explorer.set_filter({"level": dballe.Level(102, 6260000)})

            self.assertCountEqual(explorer.all_stations, [
                self._station("amdar", None, 48.90500, 10.63667, "EU3375"),
            ])
            self.assertCountEqual(explorer.stations, [
                self._station("amdar", None, 48.90500, 10.63667, "EU3375"),
            ])
            self.assertEqual(explorer.all_reports, ["amdar"])
            self.assertEqual(explorer.reports, ["amdar"])
            self.assertEqual(explorer.all_levels, [dballe.Level(102, 6260000), None])
            self.assertEqual(explorer.levels, [dballe.Level(102, 6260000)])
            self.assertEqual(explorer.all_tranges, [dballe.Trange(254, 0, 0), None])
            self.assertEqual(explorer.tranges, [dballe.Trange(254, 0, 0)])
            self.assertCountEqual(explorer.all_varcodes, [
                "B04001", "B04002", "B04003", "B04004", "B04005", "B01011", "B05001", "B06001",
                "B01006", "B02061", "B02062", "B02064", "B07030", "B08004", "B11001", "B11002", "B12101", "B13002",
            ])
            self.assertCountEqual(explorer.varcodes, [
                "B01006", "B02061", "B02062", "B02064", "B07030", "B08004", "B11001", "B11002", "B12101", "B13002",
            ])
            self.assertEqual(explorer.all_stats, dballe.ExplorerStats((
                datetime.datetime(2009, 2, 24, 11, 31), datetime.datetime(2009, 2, 24, 11, 31), 18)))
            self.assertEqual(explorer.stats, dballe.ExplorerStats((
                datetime.datetime(2009, 2, 24, 11, 31), datetime.datetime(2009, 2, 24, 11, 31), 10)))

    def test_persistence(self):
        with self._explorer() as explorer:
            with explorer.rebuild() as update:
                with self.db.transaction() as tr:
                    update.add_db(tr)

        with self._explorer() as explorer:
            explorer.set_filter({"rep_memo": "amdar"})
            self.assertStrRepr(explorer)
            self.assertExplorerContents(explorer)

    def test_issue228(self):
        # update from file
        with self._explorer() as explorer:
            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/issue228.bufr")) as message:
                    updater.add_messages(message)
            self.assertEqual(explorer.stats.datetime_min, None)
            self.assertEqual(explorer.stats.datetime_max, None)
            self.assertEqual(explorer.stats.count, 5)

            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/issue228.bufr")) as message:
                    updater.add_messages(message)
            self.assertEqual(explorer.stats.datetime_min, None)
            self.assertEqual(explorer.stats.datetime_max, None)
            self.assertEqual(explorer.stats.count, 10)

    def test_issue216(self):
        """
        Check that one can import messages with either all data, station data
        only, measured data only
        """
        with self._explorer("e1") as explorer:
            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/gts-acars1.bufr")) as messages:
                    updater.add_messages(messages)
            self.assertEqual(explorer.all_varcodes, [
                'B01011', 'B02062', 'B02064', 'B04001', 'B04002', 'B04003',
                'B04004', 'B04005', 'B04006', 'B05001', 'B06001', 'B07030',
                'B08004', 'B11001', 'B11002', 'B12101', 'B13002'])

        with self._explorer("e2") as explorer:
            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/gts-acars1.bufr")) as messages:
                    updater.add_messages(messages, station_data=True, data=False)
            self.assertEqual(explorer.all_varcodes, [
                'B01011', 'B04001', 'B04002', 'B04003', 'B04004', 'B04005',
                'B04006', 'B05001', 'B06001'])

        with self._explorer("e3") as explorer:
            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/gts-acars1.bufr")) as messages:
                    updater.add_messages(messages, station_data=False, data=True)
            self.assertEqual(explorer.all_varcodes, [
                'B02062', 'B02064', 'B07030', 'B08004', 'B11001', 'B11002', 'B12101', 'B13002'])

        with self._explorer("e4") as explorer:
            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/gts-acars1.bufr")) as messages:
                    updater.add_messages(messages, station_data=False, data=False)
            self.assertEqual(explorer.all_varcodes, [])

    def test_issue232(self):
        explorer = dballe.DBExplorer()
        with explorer.update() as updater:
            with open(test_pathname("json/issue232.json"), "rt") as fd:
                updater.add_json(fd.read())

        query = {
            'datetimemin': datetime.datetime(2020, 1, 1, 1, 0),
            'datetimemax': datetime.datetime(2020, 6, 1, 15, 13),
            'rep_memo': 'simnpr',
            'query': 'details'
        }
        for cur in explorer.query_summary_all(query):
            cur["yearmin"]

    def test_issue235(self):
        with dballe.Explorer(test_pathname("json/issue235.json")) as explorer:
            for cur in explorer.query_summary({"var": "B12101"}):
                cur["datetimemax"]


class ExplorerTestMixin(BaseExplorerTestMixin):
    def _make_explorer(self, name, *args, **kw):
        return dballe.Explorer(name, *args, **kw)

    def _station(self, rep, id, lat, lon, ident):
        return dballe.Station(rep, lat, lon, ident)

    def assertStrRepr(self, explorer):
        self.assertEqual(str(explorer), "Explorer")
        self.assertEqual(repr(explorer), "dballe.Explorer object")


class DBExplorerTestMixin(BaseExplorerTestMixin):
    def _make_explorer(self, name, *args, **kw):
        return dballe.DBExplorer(name, *args, **kw)

    def _station(self, rep, id, lat, lon, ident):
        return dballe.DBStation(rep, id, lat, lon, ident)

    def assertStrRepr(self, explorer):
        self.assertEqual(str(explorer), "DBExplorer")
        self.assertEqual(repr(explorer), "dballe.DBExplorer object")


class JSONExplorerTestMixin:
    DEFAULT_EXPLORER_NAME = "test-explorer.json"

    def _explorer(self, name=None, *args, **kw):
        if name is not None and not name.endswith(".json"):
            name += ".json"
        return super()._explorer(name, *args, **kw)


class XapianExplorerTestMixin:
    DEFAULT_EXPLORER_NAME = "test-explorer"

    def _explorer(self, name=None, *args, **kw):
        if name is not None and name.endswith(".json"):
            raise ValueError("Do not use .json extension in explorer name")
        return super()._explorer(name, *args, **kw)


class DballeV7ExplorerXapianTest(XapianExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeV7ExplorerJSONTest(JSONExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeV7DBExplorerXapianTest(XapianExplorerTestMixin, DBExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeV7DBExplorerJSONTest(JSONExplorerTestMixin, DBExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"
