import dballe
import datetime
import unittest
from testlib import DballeDBMixin


class BaseExplorerTestMixin(DballeDBMixin):
    def setUp(self):
        super().setUp()

        data = dballe.Record(
                lat=12.34560, lon=76.54320,
                datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20, 111, 222),
                rep_memo="synop",
                B01011="test1",
                B01012=500)
        self.db.insert_data(data, False, True)

        data = dballe.Record(
                lat=12.34560, lon=76.54320,
                ident="foo",
                datetime=datetime.datetime(1945, 4, 25, 12, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20, 111, 223),
                rep_memo="amdar",
                B01012=500)
        self.db.insert_data(data, False, True)

    def assertExplorerContents(self, explorer, count_unfiltered=3, count_filtered=1):
        self.assertCountEqual(explorer.all_stations, [
            self._station("synop", 1, 12.34560, 76.54320, None),
            self._station("amdar", 2, 12.34560, 76.54320, "foo"),
        ])
        self.assertCountEqual(explorer.stations, [
            self._station("amdar", 2, 12.34560, 76.54320, "foo"),
        ])
        self.assertEqual(explorer.all_reports, ["amdar", "synop"])
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

    def testCreate(self):
        explorer = self._explorer()
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)
        explorer.set_filter(dballe.Record(rep_memo="amdar"))

        self.assertStrRepr(explorer)
        self.assertExplorerContents(explorer)

        json_string = explorer.to_json()
        self.assertIn('{"summary":{', json_string)

        explorer1 = self._explorer()
        with explorer1.update() as update:
            update.add_json(json_string)
        explorer1.set_filter(dballe.Record(rep_memo="amdar"))
        self.assertExplorerContents(explorer1)

    def testMerge(self):
        explorer = self._explorer()
        with explorer.rebuild() as update:
            with self.db.transaction() as tr:
                update.add_db(tr)

        explorer1 = self._explorer()
        explorer1.set_filter(dballe.Record(rep_memo="amdar"))
        with explorer1.rebuild() as update:
            update.add_explorer(explorer)
            update.add_explorer(explorer)

        self.assertExplorerContents(explorer1, count_unfiltered=6, count_filtered=2)


class ExplorerTestMixin(BaseExplorerTestMixin):
    def _explorer(self):
        return dballe.Explorer()

    def _station(self, rep, id, lat, lon, ident):
        return dballe.Station((rep, lat, lon, ident))

    def assertStrRepr(self, explorer):
        self.assertEqual(str(explorer), "Explorer")
        self.assertEqual(repr(explorer), "Explorer object")


class DBExplorerTestMixin(BaseExplorerTestMixin):
    def _explorer(self):
        return dballe.DBExplorer()

    def _station(self, rep, id, lat, lon, ident):
        return dballe.DBStation((rep, id, lat, lon, ident))

    def assertStrRepr(self, explorer):
        self.assertEqual(str(explorer), "DBExplorer")
        self.assertEqual(repr(explorer), "DBExplorer object")


class DballeV7ExplorerTest(ExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeV7DBExplorerTest(DBExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class DballeMEMExplorerTest(ExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


class DballeMEMDBExplorerTest(DBExplorerTestMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


if __name__ == "__main__":
    from testlib import main
    main("test-explorer")
