#!/usr/bin/python
# -*- coding: UTF-8 -*-

# TODO: aggiungere segnalazione di errore in caso di sovrascrittura
# TODO: aggiungere un filtro ai dati in uscita (per selezionare, ad esempio, solo alcuni time range)
# TODO: rendere piú facile decidere quali indici sono linkati
# TODO: aggiungere metodi di query negli indici
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
        def __init__(self):
                self._map = {}
        def __str__(self):
                return self.shortName() + ": " + list.__str__(self)
        def cloneEmpty(self):
                return self.__class__()

class AnaIndex(Index):
        def obtainIndex(self, rec):
                id = rec.enqi("ana_id")
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append((id, rec.enqd("lat"), rec.enqd("lon"), rec.enqd("ident")))
                return self._map[id]
        def shortName(self):
                return "AnaIndex["+str(len(self))+"]"

class NetworkIndex(Index):
        def obtainIndex(self, rec):
                id = rec.enqi("rep_cod")
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append((id, rec.enqc("rep_memo")))
                return self._map[id]
        def shortName(self):
                return "NetworkIndex["+str(len(self))+"]"

class LevelIndex(Index):
        def obtainIndex(self, rec):
                id = rec.enqlevel()
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append(id)
                return self._map[id]
        def shortName(self):
                return "LevelIndex["+str(len(self))+"]"

class TimeRangeIndex(Index):
        def obtainIndex(self, rec):
                id = rec.enqtimerange()
                if id not in self._map:
                        self._map[id] = len(self)
                        self.append(id)
                return self._map[id]
        def shortName(self):
                return "TimeRangeIndex["+str(len(self))+"]"

class DateTimeIndex(Index):
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
        def cloneEmpty(self):
                return IntervalIndex(self._start, self._step, self._tolerance)

# TODO: Indexes to implement
# + Level
# + Time range
# + Date
# + Interval (like Date, but at regular intervals)
# + IntervalWithTolerance (like Date, but at regular intervals, and catching in
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
                        return index.cloneEmpty()

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
                dims = list of Index objects one for every dimension
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
                Collect a new value from the given dballe record.

                You need to call finalise() before the values can be used.
                """
                try:
                        # Obtain the index for every dimension
                        pos = map(lambda dim: dim.obtainIndex(rec), self.dims)

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
			if a.mask()[pos] == 1:
				a[pos] = val
			else:
				raise IndexError, "Got more than one value for " + self.name + " at position " + str(pos)

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

                        self.db = dballe.DB("test", "enrico", "")

                        # Wipe the test database
                        self.db.reset()

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
                                                end = datetime(2007, 01, 16, 0, 0, 0)

                                                # 6 hours precipitations
                                                cur = datetime(2007, 01, 01, 0, 0, 0)
                                                rec.setlevel(Level(1, 0, 0))
                                                rec.settimerange(TimeRange(4, -21600, 0))
                                                while cur < end:
                                                        rec.setdate(cur)
                                                        rec.setd("B13011", random.random()*10.)
                                                        if random.random() <= 0.9:
                                                                self.db.insert(rec, False, True)
                                                        cur += timedelta(0, 6*3600, 0)

                                                # 12 hours precipitations at different times
                                                cur = datetime(2007, 01, 01, 0, 0, 0)
                                                rec.setlevel(Level(1, 0, 0))
                                                rec.settimerange(TimeRange(4, -43200, 0))
                                                while cur < end:
                                                        rec.setdate(cur)
                                                        rec.setd("B13011", random.random()*10.)
                                                        if random.random() <= 0.9:
                                                                self.db.insert(rec, False, True)
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
                                                                self.db.insert(rec, False, True)
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
                                                                self.db.insert(rec, False, True)
                                                        cur += timedelta(0, 12*3600, 0)

                                                rec.unset("B10004")

		def testIndexFind(self):
                        # Ana in one dimension, network in the other
                        query = dballe.Record()
			query.set({'ana_id': 1, 'var': "B13011", 'rep_memo': "synop"})
                        query.setdate(datetime(2007, 1, 1, 0, 0, 0))
                        vars = read(self.db.query(query), (AnaIndex(), TimeRangeIndex()))
			self.assertEquals(vars["B13011"].dims[1].index(TimeRange(4, -21600, 0)), 1)

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
                        self.assertEquals(int(average(data.vals.compressed())), 84339)
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

