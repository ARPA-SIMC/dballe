#!/usr/bin/python3
from dballe import dbacsv
from testlib import DballeDBMixin
import datetime
import unittest
import io


class CSVMixin(DballeDBMixin):
    def setUp(self):
        super(CSVMixin, self).setUp()
        from testlib import fill_volnd
        with self.db.transaction() as tr:
            fill_volnd(tr)

    def testExport(self):
        # query["rep_memo"] = "synop"
        out = io.StringIO()
        with self.db.transaction() as tr:
            dbacsv.export(tr, {}, out)

        lines = out.getvalue().splitlines()
        self.assertEqual(lines[0],
                         "Station B01001: 12; Station B01002: 123; Station B01019: Test of long station name,,,,,,,,,,,,,,,")
        self.assertEqual(lines[1],
                         "Station,Latitude,Longitude,Network,Datetime,Level1,L1,Level2,L2,Time range,P1,P2,Variable,Value,Attr B33007,Attr B33040")
        self.assertEqual(lines[2],
                         "1,10.00000,15.00000,synop,2006-12-31 23:57:09,3,2,-,-,4,-21600,0,B13011,3.8,69,")

    def testAttrs(self):
        self.db.reset()
        with self.db.transaction() as tr:
            data = dict(
                rep_memo="synop",
                lat=0.0, lon=0.0,
                ident="#000000",
                level=(103, 2000),
                trange=(254, 0, 0),
                datetime=datetime.datetime(1005, 1, 1, 1, 1, 0),
                B12101=270.96)
            ids = tr.insert_data(data, False, True)
            attrs = {}
            attrs["B33209"] = 98
            tr.attr_insert_data(ids["B12101"], attrs)

            data.update(
                rep_memo="synop",
                lat=0.0, lon=0.0,
                ident="#000000",
                level=(103, 2000),
                trange=(254, 0, 0),
                datetime=datetime.datetime(1005, 1, 1, 1, 1, 1),
                B12101=271.96)
            ids = tr.insert_data(data, False, True)
            attrs = {}
            attrs["B33209"] = 100
            tr.attr_insert_data(ids["B12101"], attrs)

            out = io.StringIO()
            dbacsv.export(tr, {}, out)

            lines = out.getvalue().splitlines()
            self.assertEqual(lines[2], "1005-01-01 01:01:00,270.96,98")
            self.assertEqual(lines[3], "1005-01-01 01:01:01,271.96,100")


class TestCSVV7(CSVMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class TestCSVMEM(CSVMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


if __name__ == "__main__":
    from testlib import main
    main("test-csv")
