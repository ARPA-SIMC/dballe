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
        explorer = self._explorer()
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)
        explorer.set_filter({"rep_memo": "amdar"})

        self.assertStrRepr(explorer)
        self.assertExplorerContents(explorer)

        json_string = explorer.to_json()
        self.assertIn('{"summary":{', json_string)

        explorer1 = self._explorer()
        with explorer1.update() as update:
            update.add_json(json_string)
        explorer1.set_filter({"rep_memo": "amdar"})
        self.assertExplorerContents(explorer1)

    def test_merge(self):
        explorer = self._explorer()
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)

        explorer1 = self._explorer()
        explorer1.set_filter({"rep_memo": "amdar"})
        with explorer1.rebuild() as update:
            update.add_explorer(explorer)
            update.add_explorer(explorer)

        self.assertExplorerContents(explorer1, count_unfiltered=6, count_filtered=2)

    def test_query_summary(self):
        explorer = self._explorer()
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

        explorer = self._explorer()
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

    def test_persistence_json(self):
        if os.path.exists("test-explorer.json"):
            os.unlink("test-explorer.json")

        explorer = self._explorer("test-explorer.json")
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)

        explorer = self._explorer("test-explorer.json")
        explorer.set_filter({"rep_memo": "amdar"})
        self.assertStrRepr(explorer)
        self.assertExplorerContents(explorer)

    def test_persistence(self):
        if os.path.isdir("test-explorer"):
            shutil.rmtree("test-explorer")
        elif os.path.exists("test-explorer"):
            os.unlink("test-explorer")

        with self._explorer("test-explorer") as explorer:
            with explorer.rebuild() as update:
                with self.db.transaction() as tr:
                    update.add_db(tr)

        with self._explorer("test-explorer") as explorer:
            explorer.set_filter({"rep_memo": "amdar"})
            self.assertStrRepr(explorer)
            self.assertExplorerContents(explorer)

    def test_issue228(self):
        if os.path.isdir("test-explorer"):
            shutil.rmtree("test-explorer")
        elif os.path.exists("test-explorer"):
            os.unlink("test-explorer")

        # update from file
        with self._explorer("test-explorer") as explorer:
            with explorer.update() as updater:
                importer = dballe.Importer("BUFR")
                with importer.from_file(test_pathname("bufr/issue228.bufr")) as message:
                    updater.add_messages(message)
            self.assertEqual(explorer.stats.datetime_min, None)
            self.assertEqual(explorer.stats.datetime_max, None)
            self.assertEqual(explorer.stats.count, 5)


class ExplorerTestMixin(BaseExplorerTestMixin):
    def _explorer(self, *args, **kw):
        return dballe.Explorer(*args, **kw)

    def _station(self, rep, id, lat, lon, ident):
        return dballe.Station(rep, lat, lon, ident)

    def assertStrRepr(self, explorer):
        self.assertEqual(str(explorer), "Explorer")
        self.assertEqual(repr(explorer), "dballe.Explorer object")


class DBExplorerTestMixin(BaseExplorerTestMixin):
    def _explorer(self, *args, **kw):
        return dballe.DBExplorer(*args, **kw)

    def _station(self, rep, id, lat, lon, ident):
        return dballe.DBStation(rep, id, lat, lon, ident)

    def assertStrRepr(self, explorer):
        self.assertEqual(str(explorer), "DBExplorer")
        self.assertEqual(repr(explorer), "dballe.DBExplorer object")


class DballeV7ExplorerTest(ExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeV7DBExplorerTest(DBExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


if __name__ == "__main__":
    from testlib import main
    main("test-explorer")
