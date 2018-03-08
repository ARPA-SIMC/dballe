#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import os
import io
import datetime
import unittest
import warnings
from testlib import DballeDBMixin

class DballeTestMixin(DballeDBMixin):
    def setUp(self):
        super(DballeTestMixin, self).setUp()

        data = dballe.Record(
                lat=12.34560, lon=76.54320,
                datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20,111,222),
                rep_memo="synop",
                B01011="test1",
                B01012=500)
        ids = self.db.insert_data(data, False, True)

        data = dballe.Record(
                lat=12.34560, lon=76.54320,
                ident="foo",
                datetime=datetime.datetime(1945, 4, 25, 12, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20,111,223),
                rep_memo="amdar",
                B01012=500)
        ids = self.db.insert_data(data, False, True)

    def testCreate(self):
        explorer = dballe.Explorer()
        explorer.revalidate(self.db.transaction())
        explorer.set_filter(dballe.Record(rep_memo="amdar"))

        self.assertEqual(str(explorer), "Explorer")
        self.assertEqual(repr(explorer), "Explorer object")
        self.assertEqual(explorer.all_stations, [
            ("synop", 1, 12.34560, 76.54320, None),
            ("amdar", 2, 12.34560, 76.54320, "foo"),
        ])
        self.assertEqual(explorer.stations, [
            ("amdar", 2, 12.34560, 76.54320, "foo"),
        ])
        self.assertEqual(explorer.all_reports, ["amdar", "synop"])
        self.assertEqual(explorer.reports, ["amdar"])
        self.assertEqual(explorer.all_levels, [(10, 11, 15, 22)])
        self.assertEqual(explorer.levels, [(10, 11, 15, 22)])
        self.assertEqual(explorer.all_tranges, [(20, 111, 222), (20, 111, 223)])
        self.assertEqual(explorer.tranges, [(20, 111, 223)])
        self.assertEqual(explorer.all_varcodes, ["B01011", "B01012"])
        self.assertEqual(explorer.varcodes, ["B01012"])


class DballeV6Test(DballeTestMixin, unittest.TestCase):
    DB_FORMAT = "V6"

class DballeV7Test(DballeTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"

class DballeMEMTest(DballeTestMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


if __name__ == "__main__":
    from testlib import main
    main("test_explorer")
