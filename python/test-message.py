import dballe
import unittest
from decimal import Decimal
from testlibmsg import MessageTestMixin


class TestMessage(MessageTestMixin, unittest.TestCase):
    def test_empty(self):
        msg = dballe.Message("synop")
        self.assertEqual(str(msg), "Message")
        self.assertEqual(repr(msg), "dballe.Message object")
        self.assertEqual(msg.type, "synop")
        self.assertIsNone(msg.datetime)
        self.assertIsNone(msg.coords)
        self.assertIsNone(msg.ident)
        self.assertEqual(msg.report, "synop")

    def test_create(self):
        msg = self.make_gts_acars_uk1_message()
        self.assert_gts_acars_uk1_contents(msg)

        # auto msgs = read_msgs("bufr/gts-acars-uk1.bufr", Encoding::BUFR);
        # wassert(actual(msg->diff(*msgs[0])) == 0);

    def test_iterate(self):
        """
        Try iterating the message with cursors
        """
        msg = dballe.Message("synop")
        msg.set_named("year", dballe.var("B04001", 2009))
        msg.set_named("month", dballe.var("B04002", 2))
        msg.set_named("day", dballe.var("B04003", 24))
        msg.set_named("hour", dballe.var("B04004", 11))
        msg.set_named("minute", dballe.var("B04005", 31))
        msg.set_named("latitude", dballe.var("B05001", 48.90500))
        msg.set_named("longitude", dballe.var("B06001", 10.63667))
        msg.set(None, None, dballe.var("B01019", "Test"))
        lv = dballe.Level(1, 0, None, None)
        tr = dballe.Trange(254, 0, 0)
        msg.set(lv, tr, dballe.var("B11001", 33))
        msg.set(lv, tr, dballe.var("B12101", 240.0))

        count = 0
        for cur in msg.query_stations():
            self.assertEqual(cur["lat"], Decimal('48.90500'))
            self.assertEqual(cur["lon"], Decimal('10.63667'))
            count += 1
        self.assertEqual(count, 1)

        count = 0
        for cur in msg.query_station_data():
            self.assertEqual(cur["var"], "B01019")
            self.assertEqual(cur.enqs("B01019"), "Test")
            count += 1
        self.assertEqual(count, 1)

        res = []
        for cur in msg.query_data():
            res.append((cur["var"], cur[cur["var"]].enq()))
        self.assertEqual(res, [("B11001", 33), ("B12101", 240.0)])


if __name__ == "__main__":
    from testlib import main
    main("test-message")
