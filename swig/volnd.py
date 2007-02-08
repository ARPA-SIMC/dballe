#!/usr/bin/python
# -*- coding: UTF-8 -*-

# TODO: aggiungere metodi di query negli indici (eg. qual'è l'indice di questo
#       ana_id?)
# TODO: leggere gli attributi (per dire, in res["B12001.B33001"])
# TODO: leggere i dati di anagrafica

import dballe
from dballe import Level, TimeRange
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


class SkipDatum(Exception): pass

class Index(list):
        def __init__(self, shared=True):
                self._map = {}
		self._shared = shared
        def __str__(self):
                return self.shortName() + ": " + list.__str__(self)
        def another(self):
		"""
		Return another version of this index: it can be a reference to
		the exact same index if shared=True; otherwise it's a new,
		empty version.
		"""
		if self._shared:
			return self
		else:
			return self.__class__()

class AnaIndex(Index):
	"""
	Index for stations, as they come out of the database
	"""
        def obtainIndex(self, rec):
                id = rec.enqi("ana_id")
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append((id, rec.enqd("lat"), rec.enqd("lon"), rec.enqd("ident")))
                return self._map[id]
        def shortName(self):
                return "AnaIndex["+str(len(self))+"]"

class NetworkIndex(Index):
	"""
	Index for networks, as they come out of the database
	"""
        def obtainIndex(self, rec):
                id = rec.enqi("rep_cod")
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append((id, rec.enqc("rep_memo")))
                return self._map[id]
        def shortName(self):
                return "NetworkIndex["+str(len(self))+"]"

class LevelIndex(Index):
	"""
	Index for levels, as they come out of the database
	"""
        def obtainIndex(self, rec):
                id = rec.enqlevel()
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append(id)
                return self._map[id]
        def shortName(self):
                return "LevelIndex["+str(len(self))+"]"

class FixedLevelIndex(Index):
	"""
	Index for levels, using a pregiven list of levels, and throwing away
	all data that does not fit
	"""
        def __init__(self, levels, *args, **kwargs):
                """
		levels is the list of all allowed Level objects, in the wanted
		order.
                """
                Index.__init__(self, *args, **kwargs)
                self.extend(levels)
		for pos, l in enumerate(self):
			self._map[l] = pos

        def obtainIndex(self, rec):
                id = rec.enqlevel()
                if id not in self._map:
			raise SkipDatum
                return self._map[id]

        def shortName(self):
                return "FixedLevelIndex["+str(len(self))+"]"

class TimeRangeIndex(Index):
	"""
	Index for time ranges, as they come out of the database
	"""
        def obtainIndex(self, rec):
                id = rec.enqtimerange()
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append(id)
                return self._map[id]
        def shortName(self):
                return "TimeRangeIndex["+str(len(self))+"]"

class FixedTimeRangeIndex(Index):
	"""
	Index for time ranges, using a pregiven list of time ranges, and
	throwing away all data that does not fit
	"""
        def __init__(self, tranges, *args, **kwargs):
                """
		tranges is the list of all allowed TimeRange objects, in the
		wanted order.
                """
                Index.__init__(self, *args, **kwargs)
                self.extend(tranges)
		for pos, l in enumerate(self):
			self._map[l] = pos

        def obtainIndex(self, rec):
                id = rec.enqtimerange()
                if id not in self._map:
			raise SkipDatum
                return self._map[id]

        def shortName(self):
                return "FixedTimeRangeIndex["+str(len(self))+"]"

class DateTimeIndex(Index):
	"""
	Index for datetimes, as they come out of the database
	"""
        def obtainIndex(self, rec):
                id = rec.enqdate()
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
	"""
	Index by fixed time intervals: index points are at fixed time
	intervals, and data is acquired in one point only if it is within a
	given tolerance from the interval.  The interval start at a pregiven
	point in time, and continue for as long as there are fitting data
	coming out of the database.
	"""
        def __init__(self, start, step, tolerance = 0, *args, **kwargs):
                """
                start is a datetime with the starting moment
                step is a timedelta with the interval between times
                tolerance is a timedelta specifying how much skew a datum is
                  allowed to have from a sampling moment
                """
                Index.__init__(self, *args, **kwargs)
                self._start = start
                self._step = step
                self._tolerance = timedelta(0)

        def obtainIndex(self, rec):
                t = rec.enqdate()
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

        def another(self):
		if self._shared:
			return self
		else:
			return IntervalIndex(self._start, self._step, self._tolerance)

class Data:
        """
        Container for collecting variable data.  It contains the variable data
        array and the dimension indexes.

        If v is a Data object, you can access the tuple with the dimensions
        as v.dims, and the masked array with the values as v.vals.
        """
        def __init__(self, name, dims, checkConflicts=True):
                """
                name = name of the variable (eg. "B12001")
                dims = list of Index objects one for every dimension
		if checkConflicts is True, then an exception is raised if two
		  output values would end up filling the same matrix element
                """
                # Variable name, as a B table entry (e.g. "B12001")
                self.name = name

                # Tuple with all the dimension Index objects
                self.dims = dims

                # After finalise() has been called, it is the masked array with
                # all the values.  Before calling finalise(), it is the list of
                # collected data.
                self.vals = []

		# Maps attribute names to Data objects with the attribute
		# values.  The dimensions of the Data objects are fully
		# synchronised with this one.
		self.attrs = {}

		self._checkConflicts = checkConflicts

	def appendAtPos(self, pos, var):
		"""
		Collect a variable to be written on the specific indexes
		"""
		# TODO: handle the data type properly (object array for
		# strings, int array for ints, byte array for small ints, float
		# array for floats...)
		val = var.enqd()
		self.vals.append( (pos, val) )
        
        def append(self, rec):
                """
                Collect a new value from the given dballe record.

                You need to call finalise() before the values can be used.
                """
                try:
                        # Obtain the index for every dimension
                        pos = map(lambda dim: dim.obtainIndex(rec), self.dims)

                        # Save the value with its indexes
			self.appendAtPos(pos, rec.enq(self.name))
                except SkipDatum:
                        # If the value cannot be mapped along this dimension,
                        # skip it
                        pass

	def appendAttrs(self, rec):
		"""
		Collect attributes to append to the record.

                You need to call finalise() before the values can be used.
		"""
		for var in rec.itervars():
			if var.code() in self.attrs:
				data = self.attrs[var.code()]
			else:
				data = Data(self.name, self.dims, False)
				self.attrs[var.code()] = data
			# Append at the same position as the last variable
			# collected
			data.appendAtPos(self.vals[-1][0], var)


        def finalise(self):
                """
                Stop collecting values and create a masked array with all the
                values collected so far.
                """
                # Create the data array, with all values set as missing
                a = array(zeros(map(len, self.dims)), typecode=float, mask = 1)

                # Fill the array with all the values, at the given indexes
                for pos, val in self.vals:
			if not self._checkConflicts or a.mask()[pos] == 1:
				a[pos] = val
			else:
				raise IndexError, "Got more than one value for " + self.name + " at position " + str(pos)

                # Replace the intermediate data with the results
                self.vals = a

		# Finalise all the attributes as well
		for d in self.attrs.itervalues():
			d.finalise();
        
        def __str__(self):
                return "Data("+", ".join(map(lambda x: x.shortName(), self.dims))+"):"+str(self.vals)

        def __repr__(self):
                return "Data("+", ".join(map(lambda x: x.shortName(), self.dims))+"):"+self.vals.__repr__()


def read(query, dims, filter=None, checkConflicts=True, attributes=None):
	"""
	query is a dballe.Cursor resulting from a dballe query
	dims is the sequence of indexes to use for shaping the data matrixes
	filter is an optional filter function that can be used to discard
	  values from the query: if filter is not None, it will be called for
	  every output record and if it returns False, the record will be
	  discarded
	checkConflicts tells if we should raise an exception if two values from
	  the database would fill in the same position in the matrix
	attributes tells if we should read attributes as well: if it is None,
	  no attributes will be read; if it is True, all attributes will be
	  read; if it is a sequence, then it is the sequence of attributes that
	  should be read.
	"""
        ndims = len(dims)
        vars = {}
	arec = dballe.Record()
        # Iterate results
        for rec in query:
		# Discard the values that filter does not like
		if filter and not filter(rec):
			continue

                varname = rec.enqc("var")

                # Skip string variables, because they do not fit into an array
                if rec.enq(varname).info().is_string():
                        continue

                # Instantiate the index objects here for every variable
                # when it appears the first time, sharing those indexes that
                # need to be shared and creating new indexes for the individual
                # ones
                if varname not in vars:
			var = Data(varname, map(lambda x: x.another(), dims), checkConflicts)
                        vars[varname] = var
                else:
                        var = vars[varname]

                # Save every value with its indexes
                var.append(rec)

		# Add the attributes
		if attributes != None:
			if attributes == True:
				count = query.attributes(arec)
				var.appendAttrs(arec)


        # Now that we have collected all the values, create the arrays
        for var in vars.itervalues():
                var.finalise()

        return vars


if __name__ == '__main__':
        import unittest
        import random
        class TestTddiv(unittest.TestCase):

#               def tons(td):
#                       return td.days * 86400000000 + td.seconds * 1000000 + td.microseconds
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

                        # Re-enable when Debian bug #48872 has been fixed
                        #self.dtest(timedelta(999999999, 86399, 999999), timedelta(0, 0, 2))

                        random.seed(1)
                        for i in xrange(100):
                                td1 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
                                td2 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
                                self.dtest(td1, td2)

                        # Re-enable when Debian bug #48872 has been fixed
                        #for i in xrange(100):
                        #       td1 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
                        #       td2 = timedelta(0, random.randint(0, 86400), random.randint(0, 1000000))
                        #       self.dtest(td1, td2)
                        #for i in xrange(100):
                        #       td1 = timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
                        #       td2 = timedelta(0, 0, random.randint(0, 1000000))
                        #       self.dtest(td1, td2)
                        #for i in xrange(100):
                        #       td1 = timedelta(0, random.randint(0, 86400), random.randint(0, 1000000))
                        #       td2 = timedelta(0, 0, random.randint(0, 1000000))
                        #       self.dtest(td1, td2)

        class TestRead(unittest.TestCase):
                def setUp(self):
                        # We want a predictable dataset
                        random.seed(1)
			rattr = random.Random()
			rattr.seed(1)

                        self.db = dballe.DB("test", "enrico", "")

                        # Wipe the test database
                        self.db.reset()

			attrs = dballe.Record()
                        rec = dballe.Record()
                        rec.seti("mobile", 0)

                        # Enter some sample data
                        for net in ('synop', 'noaa'):
                                rec.setc("rep_memo", net)
                                # 2 networks
                                for lat in (10., 20., 30.):
                                        rec.setd("lat", lat)
                                        for lon in (15., 25.):
                                                rec.setd("lon", lon)
                                                # 6 stations

                                                start = datetime(2007, 01, 01, 0, 0, 0)
                                                end = datetime(2007, 01, 7, 0, 0, 0)

                                                # 6 hours precipitations
                                                cur = datetime(2007, 01, 01, 0, 0, 0)
                                                rec.setlevel(Level(1, 0, 0))
                                                rec.settimerange(TimeRange(4, -21600, 0))
                                                while cur < end:
                                                        rec.setdate(cur)
                                                        rec.setd("B13011", random.random()*10.)
                                                        if random.random() <= 0.9:
                                                                a, c = self.db.insert(rec, False, True)
								attrs["B33007"] = rattr.random()*100.
								self.db.attrInsert(c, "B13011", attrs)
								
                                                        cur += timedelta(0, 6*3600, 0)

                                                # 12 hours precipitations at different times
                                                cur = datetime(2007, 01, 01, 0, 0, 0)
                                                rec.setlevel(Level(1, 0, 0))
                                                rec.settimerange(TimeRange(4, -43200, 0))
                                                while cur < end:
                                                        rec.setdate(cur)
                                                        rec.setd("B13011", random.random()*10.)
                                                        if random.random() <= 0.9:
                                                                a, c = self.db.insert(rec, False, True)
								attrs["B33007"] = rattr.random()*100.
								self.db.attrInsert(c, "B13011", attrs)
                                                        cur += timedelta(0, 12*3600, 0)

                                                # Randomly measured
                                                # precipitations on a different
                                                # (meaningless) level
                                                # At slightly off times
                                                cur = datetime(2007, 01, 01, 0, 0, 0)
                                                rec.setlevel(Level(3, 2, 0))
                                                rec.settimerange(TimeRange(4, -21600, 0))
                                                while cur < end:
                                                        rec.setdate(cur + timedelta(0, random.randint(-600, 600)))
                                                        rec.setd("B13011", random.random()*10.)
                                                        if random.random() <= 0.9:
                                                                a, c = self.db.insert(rec, False, True)
								attrs["B33007"] = rattr.random()*100.
								self.db.attrInsert(c, "B13011", attrs)
                                                        cur += timedelta(0, 6*3600, 0)

                                                rec.unset("B13011")

                                                # Pressures every 12 hours
                                                cur = datetime(2007, 01, 01, 0, 0, 0)
                                                rec.setlevel(Level(1, 0, 0))
                                                rec.settimerange(TimeRange(0, 0, 0))
                                                while cur < end:
                                                        rec.setdate(cur)
                                                        rec.setd("B10004", random.randint(70000, 105000))
                                                        if random.random() <= 0.9:
                                                                a, c = self.db.insert(rec, False, True)
								attrs["B33007"] = rattr.random()*100.
								self.db.attrInsert(c, "B10004", attrs)
                                                        cur += timedelta(0, 12*3600, 0)

                                                rec.unset("B10004")

		def testIndexFind(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
			query.set({'ana_id': 1, 'var': "B13011", 'rep_memo': "synop"})
                        query.setdate(datetime(2007, 1, 1, 0, 0, 0))
                        vars = read(self.db.query(query), (AnaIndex(), TimeRangeIndex()))
			self.assertEquals(vars["B13011"].dims[1].index(TimeRange(4, -21600, 0)), 1)

		def testFilter(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
			query.set({'ana_id': 1, 'var': "B13011", 'rep_memo': "synop"})
                        query.setdate(datetime(2007, 1, 1, 0, 0, 0))
			vars = read(self.db.query(query), \
				(AnaIndex(), TimeRangeIndex()), \
				filter=lambda rec: rec.enqtimerange() == TimeRange(4, -21600, 0))
			self.assertEquals(vars["B13011"].dims[1].index(TimeRange(4, -21600, 0)), 0)

		def testUnsharedIndex(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
			query.set({'ana_id': 1, 'rep_memo': "synop"})

			vars = read(self.db.query(query), \
				(AnaIndex(), TimeRangeIndex(), DateTimeIndex()))
			self.assertEquals(len(vars["B13011"].dims[2]), len(vars["B10004"].dims[2]))
			self.assertEquals(vars["B13011"].dims[2], vars["B10004"].dims[2])

			vars = read(self.db.query(query), \
				(AnaIndex(), TimeRangeIndex(), DateTimeIndex(shared=False)))
			self.assertNotEquals(len(vars["B13011"].dims[2]), len(vars["B10004"].dims[2]))

		def testConflicts(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
			query.set({'ana_id': 1, 'var': "B13011"})
                        query.setdate(datetime(2007, 1, 1, 0, 0, 0))
			# Here conflicting values are overwritten
			vars = read(self.db.query(query), (AnaIndex(), ), checkConflicts=False)
			self.assertEquals(type(vars), dict)
			# Here insted they should be detected
			self.assertRaises(IndexError, read, \
				self.db.query(query),
				(AnaIndex(),),
				checkConflicts=True)

		def testFixedIndex(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
			query.set({'ana_id': 1, 'rep_memo': "synop", 'year': 2007, 'month': 1, 'day': 1})

			vars = read(self.db.query(query), \
				(AnaIndex(), FixedTimeRangeIndex( \
						(TimeRange(4, -21600, 0), TimeRange(4, -43200, 0)) ) ), \
				checkConflicts = False)
			self.assertEquals(len(vars["B13011"].dims[1]), 2)

			vars = read(self.db.query(query), \
				(AnaIndex(), TimeRangeIndex()), \
				checkConflicts = False)
			self.assertEquals(len(vars["B13011"].dims[1]), 3)

			vars = read(self.db.query(query), \
				(AnaIndex(), FixedLevelIndex( (Level(1, 0, 0),) )), \
				checkConflicts = False)
			self.assertEquals(len(vars["B13011"].dims[1]), 1)

			vars = read(self.db.query(query), \
				(AnaIndex(), LevelIndex()), \
				checkConflicts = False)
			self.assertEquals(len(vars["B13011"].dims[1]), 2)

                def testAnaNetwork(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
                        query.set("var", "B10004")
                        query.setdate(datetime(2007, 1, 1, 0, 0, 0))
                        vars = read(self.db.query(query), (AnaIndex(), NetworkIndex()))
                        self.assertEquals(len(vars), 1)
                        self.assertEquals(vars.keys(), ["B10004"])
                        data = vars["B10004"]
                        self.assertEquals(data.name, "B10004")
                        self.assertEquals(len(data.dims), 2)
                        self.assertEquals(len(data.dims[0]), 6)
                        self.assertEquals(len(data.dims[1]), 2)
                        self.assertEquals(data.vals.size(), 12)
                        self.assertEquals(data.vals.shape, (6, 2))
                        self.assertEquals(sum(data.vals.mask().flat), 1)
                        self.assertEquals(int(average(data.vals.compressed())), 86890)
                        self.assertEquals(data.dims[0][0], (1, 10., 15., None))
                        self.assertEquals(data.dims[0][1], (2, 10., 25., None))
                        self.assertEquals(data.dims[0][2], (3, 20., 15., None))
                        self.assertEquals(data.dims[0][3], (4, 20., 25., None))
                        self.assertEquals(data.dims[0][4], (5, 30., 15., None))
                        self.assertEquals(data.dims[0][5], (6, 30., 25., None))
                        self.assertEquals(set(data.dims[1]), set(((200, "noaa"), (1, "synop"))))

                def testAnaTrangeNetwork(self):
                        # 3 dimensions: ana, timerange, network
                        # 2 variables
                        query = dballe.Record()
                        query.setdate(datetime(2007, 1, 1, 0, 0, 0))
                        vars = read(self.db.query(query), (AnaIndex(), TimeRangeIndex(), NetworkIndex()))
                        self.assertEquals(len(vars), 2)
                        self.assertEquals(sorted(vars.keys()), ["B10004", "B13011"])

                        data = vars["B10004"]
                        self.assertEquals(data.name, "B10004")
                        self.assertEquals(len(data.dims), 3)
                        self.assertEquals(len(data.dims[0]), 6)
                        self.assertEquals(len(data.dims[1]), 1)
                        self.assertEquals(len(data.dims[2]), 2)
                        self.assertEquals(data.vals.size(), 12)
                        self.assertEquals(data.vals.shape, (6, 1, 2))
                        self.assertEquals(sum(data.vals.mask().flat), 1)
                        self.assertEquals(int(average(data.vals.compressed())), 84339)
                        self.assertEquals(data.dims[0][0], (1, 10., 15., None))
                        self.assertEquals(data.dims[0][1], (2, 10., 25., None))
                        self.assertEquals(data.dims[0][2], (3, 20., 15., None))
                        self.assertEquals(data.dims[0][3], (4, 20., 25., None))
                        self.assertEquals(data.dims[0][4], (5, 30., 15., None))
                        self.assertEquals(data.dims[0][5], (6, 30., 25., None))
                        self.assertEquals(data.dims[1][0], (0, 0, 0))
                        self.assertEquals(data.dims[2][0], (200, "noaa"))
                        self.assertEquals(data.dims[2][1], (1, "synop"))

                        data = vars["B13011"]
                        self.assertEquals(data.name, "B13011")
                        self.assertEquals(len(data.dims), 3)
                        self.assertEquals(len(data.dims[0]), 6)
                        self.assertEquals(len(data.dims[1]), 2)
                        self.assertEquals(len(data.dims[2]), 2)
                        self.assertEquals(data.vals.size(), 24)
                        self.assertEquals(data.vals.shape, (6, 2, 2))
                        self.assertEquals(sum(data.vals.mask().flat), 0)
                        self.assertEquals(int(average(data.vals.compressed())), 5)
                        self.assertEquals(data.dims[0][0], (1, 10., 15., None))
                        self.assertEquals(data.dims[0][1], (2, 10., 25., None))
                        self.assertEquals(data.dims[0][2], (3, 20., 15., None))
                        self.assertEquals(data.dims[0][3], (4, 20., 25., None))
                        self.assertEquals(data.dims[0][4], (5, 30., 15., None))
                        self.assertEquals(data.dims[0][5], (6, 30., 25., None))
                        self.assertEquals(data.dims[1][0], (4, -43200, 0))
                        self.assertEquals(data.dims[1][1], (4, -21600, 0))
                        self.assertEquals(data.dims[2][0], (200, "noaa"))
                        self.assertEquals(data.dims[2][1], (1, "synop"))

                        self.assertEquals(vars["B10004"].dims[0], vars["B13011"].dims[0])
                        self.assertNotEquals(vars["B10004"].dims[1], vars["B13011"].dims[1])
                        self.assertEquals(vars["B10004"].dims[2], vars["B13011"].dims[2])

                def testAnaTrangeNetwork(self):
                        # One station
                        # 3 dimensions: timerange, network, datetime
                        # 2 variables
                        pass

        unittest.main()

        #query = dballe.Record()
        ##query.set("var", "B12001")
        ##vars = readv7d(db.query(query), (AnaIndex,NetworkIndex))
        ##vars = readv7d(db.query(query), (AnaIndex,LevelIndex))
        ##vars = read(db.query(query), (AnaIndex(),DateTimeIndex()))
        #
        #query.set("rep_memo", "noaa")
        #vars = read(db.query(query), (AnaIndex(),IntervalIndex(datetime(2007,01,11,11,24), timedelta(0, 120), timedelta(0, 60))))
        #
        #print vars
        ##print vars['B12001'].dims[0][:20]
        ##print vars['B12001'].dims[1][:20]
        #
        ##print "Lat:", res[0][:10]
        ##print "Lon:", res[1][:10]
        ##print "Id :", res[2][:10]

