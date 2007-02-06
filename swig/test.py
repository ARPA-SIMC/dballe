#!/usr/bin/python

from dballe import *
import unittest

class VarinfoTest(unittest.TestCase):
	def testData(self):
		info = Varinfo.create("B01001")
		self.assertEqual(info.var(), "B01001")
        	self.assertEqual(info.desc(), "WMO BLOCK NUMBER")
        	self.assertEqual(info.unit(), "NUMERIC")
        	self.assertEqual(info.scale(), 0)
        	self.assertEqual(info.ref(), 0)
        	self.assertEqual(info.len(), 2)
        	self.assertEqual(info.is_string(), False)

	def testStringification(self):
		info = Varinfo.create("B01001")
		self.assertEqual(str(info).startswith("B01001"), True)
		self.assertEqual(repr(info).startswith("Varinfo(B01001"), True)

class VarTest(unittest.TestCase):
	def testCreation(self):
		var = Var("B01001")
		self.assertEqual(var.code(), "B01001")
		self.assertEqual(var.isset(), False)

		var = Var("B05001", 12)
		self.assertEqual(var.code(), "B05001")
		self.assertEqual(var.isset(), True)
		self.assertEqual(var.enqi(), 12)
		self.assertEqual(var.enqd(), 0.00012)
		self.assertEqual(var.enqc(), "12")

		var = Var("B05001", 12.4)
		self.assertEqual(var.code(), "B05001")
		self.assertEqual(var.isset(), True)
		self.assertEqual(var.enqi(), 1240000)
		self.assertEqual(var.enqd(), 12.4)
		self.assertEqual(var.enqc(), "1240000")

		var = Var("B05001", "123456")
		self.assertEqual(var.code(), "B05001")
		self.assertEqual(var.isset(), True)
		self.assertEqual(var.enqi(), 123456)
		self.assertEqual(var.enqd(), 1.23456)
		self.assertEqual(var.enqc(), "123456")

class RecordTest(unittest.TestCase):
	def testRecord(self):
		# Check basic set/get and variable iteration
		rec = Record()

		self.assertEqual(rec.contains("block"), False)
		rec.set("block", 3)
		self.assertEqual(rec.contains("block"), True)
		self.assertEqual(rec.enqi("block"), 3)

		self.assertEqual(rec.contains("B04001"), False)
		rec.set("B04001", 2001)
		self.assertEqual(rec.contains("B04001"), True)
		self.assertEqual(rec.enqi("B04001"), 2001)

		count = 0
		for var in rec:
			self.assertEqual(var.code(), "B04001")
			count = count + 1
		self.assertEqual(count, 1)

		rec.unset("block")
		self.assertEqual(rec.contains("block"), False)
		rec.unset("B04001")
		self.assertEqual(rec.contains("B04001"), False)

		rec.set("B01001", 1)
		var = rec.enq("B01001")
		var.set(4)
		rec.set(var)
		self.assertEqual(rec.enqi("B01001"), 4)

		dt = datetime.datetime(2001, 2, 3, 4, 5, 6)
		rec.setdate(dt)
		self.assertEqual(rec.enqdate(), dt)
		self.assertEqual(rec.enqi("year"), 2001)
		self.assertEqual(rec.enqi("month"), 2)
		self.assertEqual(rec.enqi("day"), 3)
		self.assertEqual(rec.enqi("hour"), 4)
		self.assertEqual(rec.enqi("min"), 5)
		self.assertEqual(rec.enqi("sec"), 6)

		l = Level(1, 2, 3)
		rec.setlevel(l)
		self.assertEqual(rec.enqlevel(), l)
		self.assertEqual(rec.enqi("leveltype"), 1)
		self.assertEqual(rec.enqi("l1"), 2)
		self.assertEqual(rec.enqi("l2"), 3)

		t = TimeRange(4, 5, 6)
		rec.settimerange(t)
		self.assertEqual(rec.enqtimerange(), t)
		self.assertEqual(rec.enqi("pindicator"), 4)
		self.assertEqual(rec.enqi("p1"), 5)
		self.assertEqual(rec.enqi("p2"), 6)


	def testRecordCopying(self):
		# Try out all copying functions

		master = Record()
		master.set("block", 4)
		master.set("latmin", 4.1234)
		master.set("B01001", 4)

		if True:
			r1 = master;
			self.assertEqual(r1.enqi("block"), 4)
			self.assertEqual(r1.enqd("latmin"), 4.1234)
			self.assertEqual(r1.enqi("B01001"), 4)

		r2 = master.copy()
		self.assertEqual(r2.enqi("block"), 4)
		self.assertEqual(r2.enqd("latmin"), 4.1234)
		self.assertEqual(r2.enqi("B01001"), 4)

		r3 = r2.copy()
		self.assertEqual(r3.enqi("block"), 4)
		self.assertEqual(r3.enqd("latmin"), 4.1234)
		self.assertEqual(r3.enqi("B01001"), 4)
		r2.unset("latmin")
		self.assertEqual(r3.enqd("latmin"), 4.1234)
		r3.setd("latmin", 4.3214)
		self.assertEqual(r3.enqd("latmin"), 4.3214)

		r3 = r3
		self.assertEqual(r3.enqi("block"), 4)
		self.assertEqual(r3.enqd("latmin"), 4.3214)
		self.assertEqual(r3.enqi("B01001"), 4)

		master = r3
		self.assertEqual(master.enqi("block"), 4)
		self.assertEqual(master.enqd("latmin"), 4.3214)
		self.assertEqual(master.enqi("B01001"), 4)

	def testRecordCopying1(self):
		# This caused a repeatable segfault
		rec = Record()
		rec.setc("query", "nosort")
		rec1 = rec.copy()
		rec1.setc("query", "nosort")

class FormatterTest(unittest.TestCase):
	def testFormatter(self):
		for i in range(258):
			describeLevel(i, 0, 0)
		for i in range(256):
			describeTrange(i, 0, 0)



if __name__ == "__main__":
        unittest.main()
