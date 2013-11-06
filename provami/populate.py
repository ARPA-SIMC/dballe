#!/usr/bin/python

from Dballe import *
import unittest

contexts = []
anas = []

db = TestDB()

rec = Record()
rec.setd("lat", 12.34560)
rec.setd("lon", 76.54320)
rec.seti("mobile", 0)
rec.seti("block", 3)
rec.seti("station", 4)
rec.seti("year", 2006)
rec.seti("month", 5)
rec.seti("day", 3)
rec.seti("hour", 12)
rec.seti("min", 0)
rec.seti("sec", 0)
rec.seti("leveltype1", 100)
rec.seti("l1", 0)
rec.seti("leveltype2", 0)
rec.seti("l2", 0)
rec.seti("pindicator", 0)
rec.seti("p1", 0)
rec.seti("p2", 0)
rec.seti("rep_memo", "synop")
rec.setc("B01011", "Hey Hey !")
rec.seti("B01012", 500)

# Insert a fixed station
context, ana = db.insert(rec, True, True)
contexts.append(context)
anas.append(ana)

# Insert a moving station
rec.seti("mobile", 1)
rec.setc("ident", "cippolippo")
for i in range(-10, 10):
	rec.setd("lon", i*1.1)
	context, ana = db.insert(rec, True, True)
	contexts.append(context)
	anas.append(ana)

# Insert pseudoana values
for a in anas:
	rec.seti("ana_id", a)
	rec.setAnaContext()
	rec.setc("B01011", "AnaAnaYe")
	rec.seti("B01012", a)
	context, ana = db.insert(rec, True, True)
	contexts.append(context)

	rec.seti("rep_memo", "synop")
	rec.setc("B01011", "AnaSynoYe")
	rec.seti("B01012", a * 10)
	context, ana = db.insert(rec, True, True)
	contexts.append(context)

# Insert attributes for every variable
for c in contexts:
	attr = Record()
	attr.seti("B33007", c)
	attr.seti("B33040", c * 2)
	db.attrInsert(c, "B01011", attr)

	attr.seti("B33007", c + 1)
	attr.seti("B33040", c * 2 + 1)
	db.attrInsert(c, "B01012", attr)

