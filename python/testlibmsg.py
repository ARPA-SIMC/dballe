import dballe


class MessageTestMixin:
    def assert_gts_acars_uk1_contents(self, msg):
        self.assertEqual(msg.get_named("year"), dballe.var("B04001", 2009))
        self.assertEqual(msg.get_named("month"), dballe.var("B04002", 2))
        self.assertEqual(msg.get_named("day"), dballe.var("B04003", 24))
        self.assertEqual(msg.get_named("hour"), dballe.var("B04004", 11))
        self.assertEqual(msg.get_named("minute"), dballe.var("B04005", 31))
        self.assertEqual(msg.get_named("ident"), dballe.var("B01011", "EU3375"))
        self.assertEqual(msg.get_named("latitude"), dballe.var("B05001", 48.90500))
        self.assertEqual(msg.get_named("longitude"), dballe.var("B06001", 10.63667))

        lv = dballe.Level(102, 6260000, None, None)
        tr = dballe.Trange(254, 0, 0)
        self.assertEqual(msg.get(lv, tr, "B01006"), dballe.var("B01006", "LH968"))
        self.assertEqual(msg.get(lv, tr, "B02061"), dballe.var("B02061", 0))
        self.assertEqual(msg.get(lv, tr, "B02062"), dballe.var("B02062", 3))
        self.assertEqual(msg.get(lv, tr, "B02064"), dballe.var("B02064", 0))
        self.assertEqual(msg.get(lv, tr, "B07030"), dballe.var("B07030", 6260.0))
        self.assertEqual(msg.get(lv, tr, "B08004"), dballe.var("B08004", 3))
        self.assertEqual(msg.get(lv, tr, "B11001"), dballe.var("B11001", 33))
        self.assertEqual(msg.get(lv, tr, "B11002"), dballe.var("B11002", 33.4))
        self.assertEqual(msg.get(lv, tr, "B12101"), dballe.var("B12101", 240.0))
        self.assertEqual(msg.get(lv, tr, "B13002"), dballe.var("B13002", 0.0))

    def make_gts_acars_uk1_message(self):
        msg = dballe.Message("amdar")
        msg.set_named("year", dballe.var("B04001", 2009))
        msg.set_named("month", dballe.var("B04002", 2))
        msg.set_named("day", dballe.var("B04003", 24))
        msg.set_named("hour", dballe.var("B04004", 11))
        msg.set_named("minute", dballe.var("B04005", 31))
        msg.set_named("ident", dballe.var("B01011", "EU3375"))
        msg.set_named("latitude", dballe.var("B05001", 48.90500))
        msg.set_named("longitude", dballe.var("B06001", 10.63667))

        lv = dballe.Level(102, 6260000, None, None)
        tr = dballe.Trange(254, 0, 0)
        msg.set(lv, tr, dballe.var("B01006", "LH968"))
        msg.set(lv, tr, dballe.var("B02061", 0))
        msg.set(lv, tr, dballe.var("B02062", 3))
        msg.set(lv, tr, dballe.var("B02064", 0))
        msg.set(lv, tr, dballe.var("B07030", 6260.0))
        msg.set(lv, tr, dballe.var("B08004", 3))
        msg.set(lv, tr, dballe.var("B11001", 33))
        msg.set(lv, tr, dballe.var("B11002", 33.4))
        msg.set(lv, tr, dballe.var("B12101", 240.0))
        msg.set(lv, tr, dballe.var("B13002", 0.0))
        return msg
