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
	def peekIndex(self, rec):
                id = rec.enqi("ana_id")
                if id not in self._map:
                        return len(self)
                return self._map[id]
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
        def peekIndex(self, rec):
                id = rec.enqi("rep_cod")
                if id not in self._map:
                        return len(self)
                return self._map[id]
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
        def peekIndex(self, rec):
                id = rec.enqlevel()
                if id not in self._map:
                        return len(self)
                return self._map[id]
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

        def peekIndex(self, rec):
                id = rec.enqlevel()
                if id not in self._map:
                        raise SkipDatum
                return self._map[id]
        def obtainIndex(self, rec):
		return self.peekIndex(rec)

        def shortName(self):
                return "FixedLevelIndex["+str(len(self))+"]"

class TimeRangeIndex(Index):
        """
        Index for time ranges, as they come out of the database
        """
        def peekIndex(self, rec):
                id = rec.enqtimerange()
                if id not in self._map:
                        return len(self)
                return self._map[id]
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

        def peekIndex(self, rec):
                id = rec.enqtimerange()
                if id not in self._map:
                        raise SkipDatum
                return self._map[id]
        def obtainIndex(self, rec):
		return self.peekIndex(rec)

        def shortName(self):
                return "FixedTimeRangeIndex["+str(len(self))+"]"

class DateTimeIndex(Index):
        """
        Index for datetimes, as they come out of the database
        """
        def peekIndex(self, rec):
                id = rec.enqdate()
                if id not in self._map:
                        return len(self)
                return self._map[id]
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

        def peekIndex(self, rec):
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
                return pos
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

                self._lastPos = None

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
                        pos = map(lambda dim: dim.peekIndex(rec), self.dims)

                        # Save the value with its indexes
                        self.appendAtPos(pos, rec.enqvar(self.name))

			# The data has been accepted: commit the new index
			# values
			for dim in self.dims:
				dim.obtainIndex(rec)

                        # Save the last position for appendAttrs
                        self._lastPos = pos
                        return True
                except SkipDatum:
                        # If the value cannot be mapped along this dimension,
                        # skip it
                        self._lastPos = None
                        return False

        def appendAttrs(self, rec):
                """
                Collect attributes to append to the record.

                You need to call finalise() before the values can be used.
                """
                if not self._lastPos:
                        return
                for var in rec.itervars():
                        if var.code() in self.attrs:
                                data = self.attrs[var.code()]
                        else:
                                data = Data(self.name, self.dims, False)
                                self.attrs[var.code()] = data
                        # Append at the same position as the last variable
                        # collected
                        data.appendAtPos(self._lastPos, var)


        def finalise(self):
                """
                Stop collecting values and create a masked array with all the
                values collected so far.
                """
                # If one of the dimensions is empty, we don't have any valid data
                # FIXME: this is a work-around: all the dimensions should be
                # empty in this case, and at the moment we have spurious
                # dimension entries that happen when the first dimension
                # succeeds but the second raises SkipDatum
                for i in self.dims:
                        if len(i) == 0:
                                return False

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
                invalid = []
                for key, d in self.attrs.iteritems():
                        if not d.finalise():
                                invalid.append(key)
                # Delete empty attributes
                for k in invalid:
                        del self.addrs[k]
                return True
        
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
        rec = dballe.Record()
        arec = dballe.Record()
        # Iterate results
        while True:
                if not query.next(rec):
                        break;

                # Discard the values that filter does not like
                if filter and not filter(rec):
                        continue

                varname = rec.enqc("var")

                # Skip string variables, because they do not fit into an array
                if rec.enqvar(varname).info().is_string():
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
                if not var.append(rec):
                        continue

                # Add the attributes
                if attributes != None:
                        if attributes == True:
                                count = query.attributes(arec)
                                var.appendAttrs(arec)


        # Now that we have collected all the values, create the arrays
        invalid = []
        for k, var in vars.iteritems():
                if not var.finalise():
                        invalid.append(k)
        for k in invalid:
                del vars[k]

        return vars
