#!/usr/bin/python
# -*- coding: UTF-8 -*-

import Dballe
from datetime import *

#
#  * Dati
#
# Input:
#  - elenco di dimensioni interessanti
#    
# Output:
#  - una matrice multidimensionale per variabile, con dentro i dati
#  - un vettore per dimensione, corrispondente alla lunghezza della matrice in
#    quella dimensione, con i dati su quella dimensione relativi a quel punto
#    della matrice.  Nel caso dell'anagrafica, per esempio, questo vettore dice
#    lat,lon,ident,ana_id di ogni dato nella fetta di matrice tagliata in quel
#    punto.
#
# Integrando in provami, posso sapere in anticipo il numero di livelli etc
# perché lo calcolo comunque per i menu
#
# Sincronizzate tra le varie matrici:
#  - ana, data
# Vettori diversi per ogni matrice:
#  - livello, scadenza
#

from MA import *

# TODO: move to python-dballe
class Level(tuple):
	"""
	Represents a level value as a 3-tuple
	"""
	def __init__(self, *args):
		if len(*args) != 3:
			raise ValueError, "Level wants exactly 3 values ("+str(len(args))+" provided)"
		tuple.__init__(self, *args)
	def type(self):
		"Return the level type"
		return self[0]
	def l1(self):
		"Return l1"
		return self[1]
	def l2(self):
		"Return l2"
		return self[2]

# TODO: move to python-dballe
class TimeRange(tuple):
	"""
	Represents a time range value as a 3-tuple
	"""
	def __init__(self, *args):
		if len(*args) != 3:
			raise ValueError, "TimeRange wants exactly 3 values ("+str(len(args))+" provided)"
		tuple.__init__(self, *args)
	def type(self):
		"Return the time range type"
		return self[0]
	def p1(self):
		"Return p1"
		return self[1]
	def p2(self):
		"Return p2"
		return self[2]

class SkipDatum(Exception): pass

class Index(list):
	def __init__(self):
		self._map = {}
	def __str__(self):
		return self.shortName() + ": " + list.__str__(self)
	def clone(self):
		return self.__class__()

class AnaIndex(Index):
	def index(self, rec):
		id = rec.enqi("ana_id")
		if id not in self._map:
			self._map[id] = len(self)
			# TODO: make a tuple with other ana info
			self.append(id)
		return self._map[id]
	def shortName(self):
		return "AnaIndex["+str(len(self))+"]"

class NetworkIndex(Index):
	def index(self, rec):
		id = rec.enqi("rep_cod")
		if id not in self._map:
			self._map[id] = len(self)
			# TODO: add to the tuple other record info
			self.append((id, rec.enqc("rep_memo")))
		return self._map[id]
	def shortName(self):
		return "NetworkIndex["+str(len(self))+"]"

class LevelIndex(Index):
	def index(self, rec):
		id = Level(map(rec.enqi, ("leveltype", "l1", "l2")))
		if id not in self._map:
			self._map[id] = len(self)
			self.append(id)
		return self._map[id]
	def shortName(self):
		return "LevelIndex["+str(len(self))+"]"

class TimeRangeIndex(Index):
	def index(self, rec):
		id = TimeRange(map(rec.enqi, ("pindicator", "p1", "p2")))
		if id not in self._map:
			self._map[id] = len(self)
			self.append(id)
		return self._map[id]
	def shortName(self):
		return "TimeRangeIndex["+str(len(self))+"]"

class DateTimeIndex(Index):
	def index(self, rec):
		id = datetime(*(map(rec.enqi, ("year", "month", "day", "hour", "min")) + [0]))
		if id not in self._map:
			self._map[id] = len(self)
			self.append(id)
		return self._map[id]
	def shortName(self):
		return "DateTimeIndex["+str(len(self))+"]"

def tddivmod1(td1, td2):
	"Division and quotient between time deltas"
	if td2 > td1:
		return 0, td1
	if td2 == 0:
		raise ZeroDivisionError, "Dividing by a 0 time delta"
	mults = (86400, 1000000, 1)
	n1 = (td1.days, td1.seconds, td1.microseconds)
	n2 = (td2.days, td2.seconds, td2.microseconds)
	d = 0
	q = 0
	for i in xrange(3):
		d += n1[i]
		if d != 0:
			if n2[i] == 0:
				d *= mults[i]
			else:
				q = d / n2[i]
				break
		else:
			if n2[i] == 0:
				pass
			else:
				break
	t = td2 * q
	if t > td1:
		q = q - 1
		return q, td1 - td2 * q
	else:
		return q, td1 - t

def tddivmod2(td1, td2):
	"""
	Division and quotient between time deltas
	(alternate implementation using longs)
	"""
	std1 = td1.days*(3600*24*1000000) + td1.seconds*1000000 + td1.microseconds
	std2 = td2.days*(3600*24*1000000) + td2.seconds*1000000 + td2.microseconds
	q = std1 / std2
	return q, td1 - (td2 * q)

# Choose which implementation to use
tddivmod = tddivmod2

class IntervalIndex(Index):
	def __init__(self, start, step, tolerance = 0):
		"""
		start is a datetime with the starting moment
		step is a timedelta with the interval between times
		tolerance is a timedelta specifying how much skew a datum is
		  allowed to have from a sampling moment
		"""
		Index.__init__(self)
		self._start = start
		self._step = step
		self._tolerance = timedelta(0)

	def index(self, rec):
		t = datetime(*(map(rec.enqi, ("year", "month", "day", "hour", "min")) + [0]))
		# Skip all entries before the start
		if (t < self._start):
			raise SkipDatum

		# With integer division we get both the position and the skew
		pos, skew = tddivmod(t - self._start, self._step)
		if skew > self._step / 2:
			pos += 1
			skew = skew - self._step
		if skew > self._tolerance:
			raise SkipDatum

		if pos >= len(self):
			# Extend the list with the missing places
			for i in xrange(len(self), pos+1):
				self.append(self._start + self._step * i)
			
		return pos

	def shortName(self):
		return "IntervalIndex["+str(len(self))+"]"
	def clone(self):
		return IntervalIndex(self._start, self._step, self._tolerance)

# TODO: Indexes to implement
# + Level
# + Time range
# + Date
# - Interval (like Date, but at regular intervals)
# - IntervalWithTolerance (like Date, but at regular intervals, and catching in
#   the interval everything within a give range)

class IndexMaker:
	"""
	Instantiate Index classes as shared or nonshared, as appropriate for
	the Index class type.
	"""
	def __init__(self):
		# set of Index classes that should be shared
		self._toshare = set((AnaIndex, NetworkIndex))

	def make(self, index):
		"""
		Return the instance for the given Index class
		"""
		# If it's not an index to be shared, create it
		if index.__class__ not in self._toshare:
			return index.clone()

		# Otherwise use the shared version
		return index

class Data:
	"""
	Container for collecting variable data.  It contains the variable data
	array and the dimension indexes.

	If v is a Data object, you can access the tuple with the dimensions
	as v.dims, and the masked array with the values as v.vals.
	"""
	def __init__(self, name, dims):
		"""
		name = name of the variable (eg. "B12001")
		dims = array of Index objects one for every dimension
		"""
		# Variable name, as a B table entry (e.g. "B12001")
		self.name = name

		# Tuple with all the dimension Index objects
		self.dims = dims

		# After finalise() has been called, it is the masked array with
		# all the values.  Before calling finalise(), it is the list of
		# collected data.
		self.vals = []
	
	def append(self, rec):
		"""
		Collect a new value from the given Dballe record.

		You need to call finalise() before the values can be used.
		"""
		try:
			# Obtain the index for every dimension
			pos = map(lambda dim: dim.index(rec), self.dims)

			# Save the value with its indexes
			val = rec.enqd(self.name)
			self.vals += [(pos, val)]
		except SkipDatum:
			# If the value cannot be mapped along this dimension,
			# skip it
			pass

	def finalise(self):
		"""
		Stop collecting values and create a masked array with all the
		values collected so far.
		"""
		# Create the data array, with all values set as missing
		a = array(zeros(map(len, self.dims)), typecode=float, mask = 1)

		# Fill the array with all the values, at the given indexes
		for pos, val in self.vals:
			a[pos] = val

		# Replace the intermediate data with the results
		self.vals = a
	
	def __str__(self):
		return "Data("+", ".join(map(lambda x: x.shortName(), self.dims))+"):"+str(self.vals)

	def __repr__(self):
		return "Data("+", ".join(map(lambda x: x.shortName(), self.dims))+"):"+self.vals.__repr__()


def read(q, dimdefs):
	imaker = IndexMaker()
	ndims = len(dimdefs)
	vars = {}
	# Iterate results
	for rec in q:
		varname = rec.enqc("var")

		# Skip string variables, because they do not fit into an array
		if rec.enq(varname).info().is_string():
			continue

		# Instantiate the index objects here for every variable
		# when it appears the first time, sharing those indexes that
		# need to be shared and creating new indexes for the individual
		# ones
		if varname not in vars:
			var = Data(varname, map(imaker.make, dimdefs))
			vars[varname] = var
		else:
			var = vars[varname]

		# Save every value with its indexes
		var.append(rec)

	# Now that we have collected all the values, create the arrays
	for var in vars.itervalues():
		var.finalise()

	return vars


db = Dballe.DB("test", "enrico", "")

query = Dballe.Record()
#query.set("var", "B12001")
#vars = readv7d(db.query(query), (AnaIndex,NetworkIndex))
#vars = readv7d(db.query(query), (AnaIndex,LevelIndex))
#vars = read(db.query(query), (AnaIndex(),DateTimeIndex()))

query.set("rep_memo", "noaa")
vars = read(db.query(query), (AnaIndex(),IntervalIndex(datetime(2007,01,11,11,24), timedelta(0, 120), timedelta(0, 60))))

print vars
#print vars['B12001'].dims[0][:20]
#print vars['B12001'].dims[1][:20]

#print "Lat:", res[0][:10]
#print "Lon:", res[1][:10]
#print "Id :", res[2][:10]

if __name__ == '__main__':
	import unittest
	import random
	class Test(unittest.TestCase):
#		def tons(td):
#			return td.days * 86400000000 + td.seconds * 1000000 + td.microseconds
		def dtest(self, td1, td2):
			self.assertEquals(tddivmod1(td1, td2), tddivmod2(td1, td2))
			q, r = tddivmod1(td1, td2)
			self.assertEquals(td2 * q + r, td1)

		def testtddiv(self):
			#self.assertEquals(tddivmod(timedelta(10, 0, 0), timedelta(2, 0, 0)), (5, timedelta(0)))
			#self.assertEquals(tddivmod(timedelta(10, 0, 1), timedelta(2, 0, 0)), (5, timedelta(0, 0, 1)))
			#self.assertEquals(tddivmod(timedelta(10, 0, 1), timedelta(3, 0, 0)), (3, timedelta(1, 0, 1)))
			#self.assertEquals(tddivmod(timedelta(10, 6, 18), timedelta(5, 3, 9)), (2, timedelta(0)))
			#self.assertEquals(tddivmod(timedelta(3, 4, 5), timedelta(1, 3, 10)), (2, timedelta(0, 86397, 999985)))
			#self.assertEquals(tddivmod(timedelta(0, 4, 5), timedelta(0, 3, 10)), (1, timedelta(0, 0, 999995)))
			#self.assertEquals(tddivmod(timedelta(2, 40, 10), timedelta(0, 0, 5)), (34568000002, timedelta(0)))

			self.dtest(timedelta(10, 0, 0), timedelta(2, 0, 0))
			self.dtest(timedelta(10, 0, 1), timedelta(2, 0, 0))
			self.dtest(timedelta(10, 0, 1), timedelta(3, 0, 0))
			self.dtest(timedelta(10, 6, 18), timedelta(5, 3, 9))
			self.dtest(timedelta(3, 4, 5), timedelta(1, 3, 10))
			self.dtest(timedelta(0, 4, 5), timedelta(0, 3, 10))
			self.dtest(timedelta(2, 40, 10), timedelta(0, 0, 5))
			self.dtest(timedelta(999999999, 86399, 999999), timedelta(0, 0, 2))

			random.seed(1)
			for i in xrange(100):
				td1 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
				td2 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
				self.dtest(td1, td2)
			for i in xrange(100):
				td1 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
				td2 = timedelta(0, random.randint(0, 86400), random.randint(0, 1000000))
				self.dtest(td1, td2)
			for i in xrange(100):
				td1 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
				td2 = timedelta(0, 0, random.randint(0, 1000000))
				self.dtest(td1, td2)
			for i in xrange(100):
				td1 = timedelta(0, random.randint(0, 86400), random.randint(0, 1000000))
				td2 = timedelta(0, 0, random.randint(0, 1000000))
				self.dtest(td1, td2)


	unittest.main()

	: ((999999999 * 86400 * 1000000 + 86399 * 1000000 + 999999)/5) * timedelta(0,0,5)
Out[5]: datetime.timedelta(999999999, 86399, 999995)

In [6]:

