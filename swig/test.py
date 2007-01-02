#!/usr/bin/python

from Dballe import *
import unittest

class DballeTest(unittest.TestCase):
	#def setUp(self):
	#	self.lib = LibInit()
	
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

	def testDB(self):
		db = DB("test", "enrico", "")

		db.reset()
			
		data = Record()
		data.setd("lat", 12.34560)
		data.setd("lon", 76.54320)
		data.seti("mobile", 0)
		data.seti("year", 1945)
		data.seti("month", 4)
		data.seti("day", 25)
		data.seti("hour", 8)
		data.seti("min", 0)
		data.seti("leveltype", 10)
		data.seti("l1", 11)
		data.seti("l2", 22)
		data.seti("pindicator", 20)
		data.seti("p1", 111)
		data.seti("p2", 222)
		data.seti("rep_cod", 1)
		data.setc("B01011", "Hey Hey!!")
		data.seti("B01012", 500)

		(context, ana) = db.insert(data, False, True)

		data.clear()
		data.set("B33007", 50)
		data.set("B33036", 75)
		db.attrInsert(context, "B01011", data)

		query = Record()
		cur = db.queryAna(query)
		self.assertEqual(cur.remaining(), 1)
		count = 0
		for result in cur:
			self.assertEqual(result.enqd("lat"), 12.34560)
			self.assertEqual(result.enqd("lon"), 76.54320)
			self.assertEqual(result.contains("B01011"), False)
			count = count + 1
		self.assertEqual(count, 1)

		expected = {}
		expected["B01011"] = "Hey Hey!!";
		expected["B01012"] = "500";

		query.keySetd("latmin", 10.0)
		cur = db.query(query)
		self.assertEqual(cur.remaining(), 2)
		count = 0
		for result in cur:
			self.assertEqual(cur.remaining(), 2-count-1)
			assert expected.has_key(cur.varcode())
			self.assertEqual(result.enqc(cur.varcode()), expected[cur.varcode()])
			del expected[cur.varcode()]
			count = count + 1

		count = db.attrQuery(context, "B01011", data)
		self.assertEqual(count, 2)

		expected = {}
		expected["B33007"] = 50
		expected["B33036"] = 75

		count = 0
		for var in data:
			assert expected.has_key(var.code())
			self.assertEqual(var.enqi(), expected[var.code()])
			del expected[var.code()]
			count = count + 1
		self.assertEqual(count, 2)

		query.clear()
		cur = db.queryLevels(query)
		self.assertEqual(cur.remaining(), 1)
		for result in cur:
			self.assertEqual(result.enqi("leveltype"), 10)
			self.assertEqual(result.enqi("l1"), 11)
			self.assertEqual(result.enqi("l2"), 22)

		query.clear()
		cur = db.queryTimeRanges(query)
		self.assertEqual(cur.remaining(), 1)
		for result in cur:
			self.assertEqual(result.enqi("pindicator"), 20)
			self.assertEqual(result.enqi("p1"), 111)
			self.assertEqual(result.enqi("p2"), 222)

		query.clear()
		cur = db.queryLevelsAndTimeRanges(query)
		self.assertEqual(cur.remaining(), 1)
		for result in cur:
			self.assertEqual(result.enqi("leveltype"), 10)
			self.assertEqual(result.enqi("l1"), 11)
			self.assertEqual(result.enqi("l2"), 22)
			self.assertEqual(result.enqi("pindicator"), 20)
			self.assertEqual(result.enqi("p1"), 111)
			self.assertEqual(result.enqi("p2"), 222)

		query.clear()
		cur = db.queryVariableTypes(query)
		self.assertEqual(cur.remaining(), 2)
		expected = {}
		expected["B01011"] = 1
		expected["B01012"] = 1
		count = 0
		for result in cur:
			assert expected.has_key(cur.varcode())
			del expected[cur.varcode()]
			count = count + 1
		self.assertEqual(count, 2)

		query.clear()
		cur = db.queryIdents(query)
		self.assertEqual(cur.remaining(), 1)
		for result in cur:
			assert not result.contains("ident")

		query.clear()
		cur = db.queryReports(query)
		self.assertEqual(cur.remaining(), 1)
		for result in cur:
			self.assertEqual(result.enqi("rep_cod"), 1)
			self.assertEqual(result.enqc("rep_memo"), "synop")

		query.clear()
		cur = db.queryDateTimes(query)
		self.assertEqual(cur.remaining(), 1)
		for result in cur:
			self.assertEqual(result.enqi("year"), 1945)
			self.assertEqual(result.enqi("month"), 4)
			self.assertEqual(result.enqi("day"), 25)
			self.assertEqual(result.enqi("hour"), 8)
			self.assertEqual(result.enqi("min"), 0)
			self.assertEqual(result.enqi("sec"), 0)

		query.clear()
		db.exportResults(query, "BUFR", "/dev/null")
		db.exportResults(query, "CREX", "/dev/null")

		#db.attrRemove(1, "B01011", [ "B33007" ])
		db.attrRemove(1, "B01011", "B33007")

	def testFormatter(self):
		for i in range(258):
			describeLevel(i, 0, 0)
		for i in range(256):
			describeTrange(i, 0, 0)



if __name__ == "__main__":
        unittest.main()
