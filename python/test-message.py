import dballe
import unittest


class TestExplorer(unittest.TestCase):
    def test_create(self):
        msg = dballe.Message("synop")
        self.assertEqual(str(msg), "Message")
        self.assertEqual(repr(msg), "dballe.Message object")
#    def setUp(self):
#        super().setUp()
#
#        data = dballe.Record(
#                lat=12.34560, lon=76.54320,
#                datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
#                level=(10, 11, 15, 22),
#                trange=(20, 111, 222),
#                rep_memo="synop",
#                B01011="test1",
#                B01012=500)
#        self.db.insert_data(data, False, True)
#
#        data = dballe.Record(
#                lat=12.34560, lon=76.54320,
#                ident="foo",
#                datetime=datetime.datetime(1945, 4, 25, 12, 0, 0),
#                level=(10, 11, 15, 22),
#                trange=(20, 111, 223),
#                rep_memo="amdar",
#                B01012=500)
#        self.db.insert_data(data, False, True)
#
#    def assertExplorerContents(self, explorer, count_unfiltered=3, count_filtered=1):
#        self.assertCountEqual(explorer.all_stations, [
#            self._station("synop", 1, 12.34560, 76.54320, None),
#            self._station("amdar", 2, 12.34560, 76.54320, "foo"),
#        ])
#        self.assertCountEqual(explorer.stations, [
#            self._station("amdar", 2, 12.34560, 76.54320, "foo"),
#        ])
#        self.assertEqual(explorer.all_reports, ["amdar", "synop"])
#        self.assertEqual(explorer.reports, ["amdar"])
#        self.assertEqual(explorer.all_levels, [(10, 11, 15, 22)])
#        self.assertEqual(explorer.levels, [(10, 11, 15, 22)])
#        self.assertEqual(explorer.all_tranges, [(20, 111, 222), (20, 111, 223)])
#        self.assertEqual(explorer.tranges, [(20, 111, 223)])
#        self.assertEqual(explorer.all_varcodes, ["B01011", "B01012"])
#        self.assertEqual(explorer.varcodes, ["B01012"])
#        self.assertEqual(explorer.all_stats, dballe.ExplorerStats((
#            datetime.datetime(1945, 4, 25,  8, 0), datetime.datetime(1945, 4, 25, 12, 0), count_unfiltered)))
#        self.assertEqual(explorer.stats, dballe.ExplorerStats((
#            datetime.datetime(1945, 4, 25, 12, 0), datetime.datetime(1945, 4, 25, 12, 0), count_filtered)))
#
#    def test_create(self):
#        explorer = self._explorer()
#        with explorer.rebuild() as update:
#            with self.db.transaction() as tr:
#                update.add_db(tr)
#        explorer.set_filter(dballe.Record(rep_memo="amdar"))
#
#        self.assertStrRepr(explorer)
#        self.assertExplorerContents(explorer)
#
#        json_string = explorer.to_json()
#        self.assertIn('{"summary":{', json_string)
#
#        explorer1 = self._explorer()
#        with explorer1.update() as update:
#            update.add_json(json_string)
#        explorer1.set_filter(dballe.Record(rep_memo="amdar"))
#        self.assertExplorerContents(explorer1)
#
#    def test_merge(self):
#        explorer = self._explorer()
#        with explorer.rebuild() as update:
#            with self.db.transaction() as tr:
#                update.add_db(tr)
#
#        explorer1 = self._explorer()
#        explorer1.set_filter(dballe.Record(rep_memo="amdar"))
#        with explorer1.rebuild() as update:
#            update.add_explorer(explorer)
#            update.add_explorer(explorer)
#
#        self.assertExplorerContents(explorer1, count_unfiltered=6, count_filtered=2)
#
#    def test_query_summary(self):
#        explorer = self._explorer()
#        with explorer.rebuild() as update:
#            with self.db.transaction() as tr:
#                update.add_db(tr)
#        explorer.set_filter({"rep_memo": "amdar"})
#
#        with explorer.query_summary_all() as cur:
#            rows = list(cur)
#        self.assertEqual(len(rows), 3)
#
#        with explorer.query_summary() as cur:
#            rows = list(cur)
#        self.assertEqual(len(rows), 1)
#
#        with explorer.query_summary_all({"rep_memo": "amdar"}) as cur:
#            rows = list(cur)
#        self.assertEqual(len(rows), 1)


if __name__ == "__main__":
    from testlib import main
    main("test-message")
