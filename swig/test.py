#!/usr/bin/python

from Dballe import *
import unittest

class DballeTest(unittest.TestCase):
	def testVarinfo(self):
		info = Varinfo.create("B01001")
		self.assertEqual(info.var(), "B01001")
        	self.assertEqual(info.desc(), "WMO BLOCK NUMBER")
        	self.assertEqual(info.unit(), "NUMERIC")
        	self.assertEqual(info.scale(), 0)
        	self.assertEqual(info.ref(), 0)
        	self.assertEqual(info.len(), 2)
        	self.assertEqual(info.is_string(), False)

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

	def testFormatter(self):
		for i in range(258):
			describeLevel(i, 0, 0)
		for i in range(256):
			describeTrange(i, 0, 0)



if __name__ == "__main__":
        unittest.main()
