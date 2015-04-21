#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import dbacsv
import unittest
import io

class TestCSV(unittest.TestCase):
    def setUp(self):
        from testlib import fill_volnd
        self.db = dballe.DB.connect_test()
        fill_volnd(self.db)

    def testNothing(self):
        query = dballe.Record()
        #query["rep_memo"] = "synop"
        out = io.StringIO()
        dbacsv.export(self.db, query, out)

if __name__ == "__main__":
    from testlib import main
    main("test_csv")
