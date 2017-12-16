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
                mobile=0,
                datetime=datetime.datetime(1945, 4, 25, 8, 0, 0),
                level=(10, 11, 15, 22),
                trange=(20,111,222),
                rep_memo="synop",
                B01011="Hey Hey!!",
                B01012=500)
        ids = self.db.insert_data(data, False, True)

        data.clear()
        data["B33007"] = 50
        data["B33036"] = 75
        self.db.attr_insert_data(ids["B01011"], data)

        for rec in self.db.query_data(dballe.Record(var="B01011")):
            self.attr_ref = rec["context_id"]

    def testCreate(self):
        explorer = dballe.Explorer(self.db)
        self.assertEqual(str(explorer), "Explorer")
        self.assertEqual(repr(explorer), "Explorer object")


class DballeV6Test(DballeTestMixin, unittest.TestCase):
    DB_FORMAT = "V6"

class DballeV7Test(DballeTestMixin, unittest.TestCase):
    DB_FORMAT = "V7"

class DballeMEMTest(DballeTestMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


if __name__ == "__main__":
    from testlib import main
    main("test_explorer")
