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

        lines = out.getvalue().splitlines()
        self.assertEquals(lines[0],
                          "Ana B01001: 12. Ana B01002: 123. Ana B01019: Test of long station name. ,,,,,,,,,,,,,,,")
        self.assertEquals(lines[1],
                          "Station,Latitude,Longitude,Report,Date,Level1,L1,Level2,L2,Time range,P1,P2,B10004,B13011,Attr B33007,Attr B33040")
        self.assertEquals(lines[2],
                          "1,10.0,15.0,synop,2007-01-01 00:00:00,1,-,-,-,0,-,-,73810,,35,")

if __name__ == "__main__":
    from testlib import main
    main("test_csv")
