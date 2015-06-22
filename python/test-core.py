#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import unittest
from six import string_types

class Varinfo(unittest.TestCase):
    def testEmptyVarinfo(self):
        self.assertRaises(NotImplementedError, dballe.Varinfo)

    def testQueryLocal(self):
        info = dballe.varinfo("t")
        self.assertEqual(info.unit, "K")

    def testData(self):
        info = dballe.varinfo("B01001")
        self.assertEqual(info.var, "B01001")
        self.assertEqual(info.desc, "WMO BLOCK NUMBER")
        self.assertEqual(info.unit, "NUMERIC")
        self.assertEqual(info.scale, 0)
        self.assertEqual(info.ref, 0)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.is_string, False)

    def testStringification(self):
        info = dballe.varinfo("B01001")
        self.assert_(str(info).startswith("B01001"))
        self.assert_(repr(info).startswith("Varinfo('B01001"))

    def testFromAlias(self):
        info = dballe.varinfo("t")
        self.assertEqual(info.var, "B12101")

class Vartable(unittest.TestCase):
    def testEmpty(self):
        table = dballe.Vartable()
        self.assertEqual(table.id, None)
        self.assertEqual(str(table), "<empty>")
        self.assertEqual(repr(table), "Vartable()")
        self.assertRaises(KeyError, table.query, "B01001")

    def testCreate(self):
        table = dballe.Vartable.get("dballe")
        self.assertEqual(table.id, "dballe")
        self.assertEqual(str(table), "dballe")
        self.assertEqual(repr(table), "Vartable('dballe')")

    def testContains(self):
        table = dballe.Vartable("dballe")
        self.assertIn("B01001", table)
        self.assertNotIn("B63254", table)

    def testIndexing(self):
        table = dballe.Vartable("dballe")
        info = table[0]
        self.assertEqual(info.var, "B01001")

    def testQuery(self):
        table = dballe.Vartable("dballe")
        info = table.query("B01001")
        self.assertEqual(info.is_string, False)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.unit, "NUMERIC")

        info = table["B01001"]
        self.assertEqual(info.is_string, False)
        self.assertEqual(info.len, 3)
        self.assertEqual(info.unit, "NUMERIC")

    def testQueryMissing(self):
        table = dballe.Vartable("dballe")
        self.assertRaises(KeyError, table.query, "B63254")

    def testIterate(self):
        table = dballe.Vartable("dballe")
        selected = None
        count = 0
        for entry in table:
            if entry.var == "B12101":
                selected = entry
            count += 1
        self.assertGreater(count, 100)
        self.assertEquals(count, len(table))
        self.assertIsNotNone(selected)


class Var(unittest.TestCase):
    def testUndefCreation(self):
        var = dballe.var("B01001")
        self.assertEqual(var.code, "B01001")
        self.assertEqual(var.info.var, "B01001")
        self.assertFalse(var.isset)
    def testIntCreation(self):
        var = dballe.var("B05001", 12)
        self.assertEqual(var.code, "B05001")
        self.assertEqual(var.isset, True)
        self.assertEqual(var.enqi(), 12)
        self.assertEqual(var.enqd(), 0.00012)
        self.assertEqual(var.enqc(), "12")
    def testFloatCreation(self):
        var = dballe.var("B05001", 12.4)
        self.assertEqual(var.code, "B05001")
        self.assertEqual(var.isset, True)
        self.assertEqual(var.enqi(), 1240000)
        self.assertEqual(var.enqd(), 12.4)
        self.assertEqual(var.enqc(), "1240000")
    def testStringCreation(self):
        var = dballe.var("B05001", "123456")
        self.assertEqual(var.code, "B05001")
        self.assertEqual(var.isset, True)
        self.assertEqual(var.enqi(), 123456)
        self.assertEqual(var.enqd(), 1.23456)
        self.assertEqual(var.enqc(), "123456")
    def testAliasCreation(self):
        var = dballe.var("t", 280.3)
        self.assertEqual(var.code, "B12101")
        self.assertEqual(var.isset, True)
        self.assertEqual(var.enqi(), 28030)
        self.assertEqual(var.enqd(), 280.3)
        self.assertEqual(var.enqc(), "28030")
    def testStringification(self):
        var = dballe.var("B01001")
        self.assertEqual(str(var), "None")
        self.assertEqual(repr(var), "Var('B01001', None)")
        self.assertEqual(var.format("foo"), "foo")

        var = dballe.var("B05001", 12.4)
        self.assertEqual(str(var), "12.40000")
        self.assertEqual(repr(var), "Var('B05001', 12.40000)")
        self.assertEqual(var.format("foo"), "12.40000")
    def testEnq(self):
        var = dballe.var("B01001", 1)
        self.assertEqual(type(var.enq()), int)
        self.assertEqual(var.enq(), 1)
        var = dballe.var("B05001", 1.12345)
        self.assertEqual(type(var.enq()), float)
        self.assertEqual(var.enq(), 1.12345)
        var = dballe.var("B01019", "ciao")
        self.assertIsInstance(var.enq(), string_types)
        self.assertEqual(var.enq(), "ciao")
    def testGet(self):
        var = dballe.var("B01001")
        self.assertIsNone(var.get())
        self.assertIs(var.get("foo"), "foo")
        var = dballe.var("B01001", 1)
        self.assertIs(var.get(), 1)
        var = dballe.var("B05001", 1.12345)
        self.assertEquals(var.get(), 1.12345)
        var = dballe.var("B01019", "ciao")
        self.assertEqual(var.get(), "ciao")
    def testEq(self):
        var = dballe.var("B01001", 1)
        self.assertEqual(var, var)
        self.assertEqual(var, dballe.var("B01001", 1))
        self.assertNotEqual(var, dballe.var("B01001", 2))
        self.assertNotEqual(var, dballe.var("B01002", 1))
        self.assertIsNot(var, None)
        self.assertIsNot(dballe.var("B01001"), None)


if __name__ == "__main__":
    from testlib import main
    main("test_core")
