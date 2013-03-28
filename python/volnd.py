#!/usr/bin/python
# -*- coding: UTF-8 -*-

"""
volnd is an easy way of extracting entire matrixes of data out of a DB-All.e
database.

This module allows to extract multidimensional matrixes of data given a list of
dimension definitions.  Every dimension definition defines what kind of data
goes along that dimension.

Dimension definitions can be shared across different extracted matrixes and
multiple extractions, allowing to have different matrixes whose indexes have
the same meaning.

This example code extracts temperatures in a station by datetime matrix::

        query = dballe.Record()
        query["var"] = "B12001"
        query["rep_memo"] = "synop"
        query["level"] = (105, 2)
        query["trange"] = (0,)
        vars = read(self.db.query(query), (AnaIndex(), DateTimeIndex()))
        data = vars["B12001"]
        # Data is now a 2-dimensional Masked Array with the data
        #
        # Information about what values correspond to an index in the various
        # directions can be accessed in data.dims, which contains one list per
        # dimension with all the information corresponding to every index.
        print "Ana dimension is", len(data.dims[0]), "items long"
        print "Datetime dimension is", len(data.dims[1]), "items long"
        print "First 10 stations along the Ana dimension:", data.dims[0][:10]
        print "First 10 datetimes along the DateTime dimension:", data.dims[1][:10]
"""


# TODO: aggiungere metodi di query negli indici (eg. qual'è l'indice di questo
#       ana_id?)
# TODO: leggere i dati di anagrafica

import dballe
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

import os
if os.environ.get("DBALLE_BUILDING_DOCS", "") != 'true':
        import numpy
        import numpy.ma as ma


class SkipDatum(Exception): pass

class Index(list):
        def __init__(self, shared=True, frozen=False):
                self._shared = shared
                self._frozen = frozen
        def freeze(self):
                """
                Set the index as frozen: indexing elements not already in the
                index will raise a SkipDatum exception
                """
                self._frozen = True
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

class ListIndex(Index, list):
        def __init__(self, shared=True, frozen=False, start=None):
                Index.__init__(self, shared, frozen)
                self._map = {}
                if start:
                        for el in start:
                                id, val = self._splitInit(el)
                                self._map[id] = len(self)
                                self.append(val)
        def __str__(self):
                return self.shortName() + ": " + list.__str__(self)
        def _indexKey(self, rec):
                "Extract the indexing key from the record"
                return None
        def _indexData(self, rec):
                "Extract the full data information from the record"
                return None
        def _splitInit(self, el):
                """
                Extract the indexing key and full data information from one of
                the objects passed as the start= value to the constructor to
                preinit an index
                """
                return el, el
        def approve(self, rec):
                key = self._indexKey(rec)
                return not self._frozen or key in self._map
        def getIndex(self, rec):
                key = self._indexKey(rec)
                pos = self._map.get(key, -1)
                if pos == -1:
                        pos = len(self)
                        self._map[key] = pos
                        self.append(self._indexData(rec))
                return pos

class AnaIndexEntry(tuple):
        """
        AnaIndex entry, with various data about a single station.

        It is a tuple of 4 values:
         * station id
         * latitude
         * longitude
         * mobile station identifier, or None
        """
        def __new__(self, rec_or_ana_id, lat=None, lon=None, ident=None):
                """
                Create an index entry.  The details can be given explitly, or
                a dballe.Record can be passed and all data will be fetched from
                it.
                """
                if type(rec_or_ana_id) == int:
                        if lat == None:
                                raise TypeError, "got ana_id but latitude is None"
                        if lon == None:
                                raise TypeError, "got ana_id and latitude but longitude is None"
                        return tuple.__new__(self, (rec_or_ana_id, lat, lon, ident))
                else:
                        rec = rec_or_ana_id
                        return tuple.__new__(self, (rec["ana_id"], rec["lat"], rec["lon"], rec.get("ident", None)))
        def __str__(self):
                if self[3] == None:
                        return "Station at lat %.5f lon %.5f" % self[1:3]
                else:
                        return "%s at lat %.5f lon %.5f" % (self[3], self[1], self[2])
        def __repr__(self):
                return "AnaIndexEntry" + tuple.__repr__(self)

class AnaIndex(ListIndex):
        """
        Index for stations, as they come out of the database.

        The constructor syntax is: ``AnaIndex(shared=True, frozen=False, start=None)``.

        The index saves all stations as AnaIndexEntry tuples, in the same order
        as they come out of the database.
        """
        def _indexKey(self, rec):
                return rec["ana_id"]
        def _indexData(self, rec):
                return AnaIndexEntry(rec)
        def _splitInit(self, el):
                return el[0], el
        def shortName(self):
                return "AnaIndex["+str(len(self))+"]"

class NetworkIndexEntry(tuple):
        """
        NetworkIndex entry, with various data about a single station.

        It is a tuple of 2 values:
         * network code
         * network name
        """
        def __new__(self, rec_or_rep_cod, rep_memo=None):
                """
                Create an index entry.  The details can be given explitly, or
                a dballe.Record can be passed and all data will be fetched from
                it.
                """
                if type(rec_or_rep_cod) == int:
                        if rep_memo == None:
                                raise TypeError, "got rep_cod but rep_memo is None"
                        return tuple.__new__(self, (rec_or_rep_cod, rep_memo))
                else:
                        rec = rec_or_rep_cod
                        return tuple.__new__(self, (rec["rep_cod"], rec["rep_memo"]))
        def __str__(self):
                return self[1]
        def __repr__(self):
                return "NetworkIndexEntry" + tuple.__repr__(self)

class NetworkIndex(ListIndex):
        """
        Index for networks, as they come out of the database.

        The constructor syntax is: ``NetworkIndex(shared=True, frozen=False, start=None)``.

        The index saves all networks as NetworkIndexEntry tuples, in the same
        order as they come out of the database.
        """
        def _indexKey(self, rec):
                return rec["rep_cod"]
        def _indexData(self, rec):
                return NetworkIndexEntry(rec)
        def _splitInit(self, el):
                return el[0], el
        def shortName(self):
                return "NetworkIndex["+str(len(self))+"]"

class LevelIndex(ListIndex):
        """
        Index for levels, as they come out of the database

        The constructor syntax is: ``LevelIndex(shared=True, frozen=False), start=None``.

        The index saves all levels as dballe.Level tuples, in the same order
        as they come out of the database.
        """
        def _indexKey(self, rec):
                return rec["level"]
        def _indexData(self, rec):
                return rec["level"]
        def shortName(self):
                return "LevelIndex["+str(len(self))+"]"

class TimeRangeIndex(ListIndex):
        """
        Index for time ranges, as they come out of the database.

        The constructor syntax is: ``TimeRangeIndex(shared=True, frozen=False, start=None)``.

        The index saves all time ranges as dballe.TimeRange tuples, in the same
        order as they come out of the database.
        """
        def _indexKey(self, rec):
                return rec["trange"]
        def _indexData(self, rec):
                return rec["trange"]
        def shortName(self):
                return "TimeRangeIndex["+str(len(self))+"]"

class DateTimeIndex(ListIndex):
        """
        Index for datetimes, as they come out of the database.

        The constructor syntax is: ``DateTimeIndex(shared=True, frozen=False, start=None)``.

        The index saves all datetime values as datetime.datetime objects, in
        the same order as they come out of the database.
        """
        def _indexKey(self, rec):
                return rec["date"]
        def _indexData(self, rec):
                return rec["date"]
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
        given tolerance from the interval.

        The constructor syntax is: ``IntervalIndex(start, step, tolerance=0, end=None, shared=True, frozen=False)``.

        ``start`` is a datetime.datetime object giving the starting time of the
        time interval of this index.

        ``step`` is a datetime.timedelta object with the interval between
        sampling points.

        ``tolerance`` is a datetime.timedelta object specifying the maximum
        allowed interval between a datum datetime and the sampling step.  If
        the interval is bigger than the tolerance, the data is discarded.

        ``end`` is an optional datetime.datetime object giving the ending time
        of the time interval of the index.  If omitted, the index will end at
        the latest accepted datum coming out of the database.
        """
        def __init__(self, start, step, tolerance = 0, end=None, *args, **kwargs):
                """
                start is a datetime with the starting moment
                step is a timedelta with the interval between times
                tolerance is a timedelta specifying how much skew a datum is
                  allowed to have from a sampling moment
                """
                Index.__init__(self, *args, **kwargs)
                self._start = start
                self._step = step
                self._end = end
                if end != None:
                        self._size = (end-start)/step
                else:
                        self._size = 0
                self._tolerance = timedelta(0)

        def approve(self, rec):
                t = rec["date"]
                # Skip all entries before the start
                if t < self._start:
                        return False
                if self._end and t > self._end:
                        return False
                # With integer division we get both the position and the skew
                pos, skew = tddivmod(t - self._start, self._step)
                if skew > self._step / 2:
                        pos += 1
                        skew = skew - self._step
                if skew > self._tolerance:
                        return False
                if self._frozen and pos >= self._size:
                        return False
                return True

        def getIndex(self, rec):
                t = rec["date"]
                # With integer division we get both the position and the skew
                pos, skew = tddivmod(t - self._start, self._step)
                if skew > self._step / 2:
                        pos += 1
                if pos >= self._size:
                        self._size = pos + 1
                return pos

        def __len__(self):
                return self._size
        def __iter__(self):
                for i in xrange(self._size):
                        yield self._start + self._step * i
        def __str__(self):
                return self.shortName() + ": " + ", ".join(self)
        def shortName(self):
                return "IntervalIndex["+str(self._size)+"]"
        def another(self):
                if self._shared:
                        return self
                else:
                        return IntervalIndex(self._start, self._step, self._tolerance, self._end)

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

                # Information about the variable
                self.info = dballe.varinfo(name)

                self._checkConflicts = checkConflicts

                self._lastPos = None

        def append(self, rec):
                """
                Collect a new value from the given dballe record.

                You need to call finalise() before the values can be used.
                """
                accepted = True
                for dim in self.dims:
                        if not dim.approve(rec):
                                accepted = False
                                break
                if accepted:
                        # Obtain the index for every dimension
                        pos = tuple([dim.getIndex(rec) for dim in self.dims])

                        # Save the value with its indexes
                        self.vals.append( (pos, rec[self.name]) )

                        # Save the last position for appendAttrs
                        self._lastPos = pos

                        return True
                else:
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
                for code in rec:
                    #print "Attr", var.code(), "for", self.name, "at", self._lastPos
                    if code in self.attrs:
                            data = self.attrs[code]
                    else:
                            data = Data(self.name, self.dims, False)
                            self.attrs[code] = data
                    # Append at the same position as the last variable
                    # collected
                    data.vals.append((self._lastPos, rec[code]))

        def _instantiateIntMatrix(self):
                if self.info.bit_ref == 0:
                        # bit_ref is 0, so we are handling unsigned
                        # numbers and we know the exact number of bits
                        # used for encoding
                        bits = self.info.bit_len
                        #print self.info, bits
                        if bits <= 8:
                                #print 'uint8'
                                a = numpy.empty(map(len, self.dims), dtype='uint8')
                        elif bits <= 16:
                                #print 'uint16'
                                a = numpy.empty(map(len, self.dims), dtype='uint16')
                        elif bits <= 32:
                                #print 'uint32'
                                a = numpy.empty(map(len, self.dims), dtype='uint32')
                        else:
                                #print 'uint64'
                                a = numpy.empty(map(len, self.dims), dtype='uint64')
                else:
                        # We have a bit_ref, so we can have negative
                        # values or we can have positive values bigger
                        # than usual (for example, for negative bit_ref
                        # values).  Therefore, choose the size of the
                        # int in the matrix according to the value
                        # range instead of bit_len()
                        range = self.info.imax - self.info.imin
                        #print self.info, range
                        if range < 256:
                                #print 'int8'
                                a = numpy.empty(map(len, self.dims), dtype='int8')
                        elif range < 65536:
                                #print 'int16'
                                a = numpy.empty(map(len, self.dims), dtype='int16')
                        elif range <= 4294967296:
                                #print 'int32'
                                a = numpy.empty(map(len, self.dims), dtype='int32')
                        else:
                                a = numpy.empty(map(len, self.dims), dtype=int)
                return a

        def finalise(self):
                """
                Stop collecting values and create a masked array with all the
                values collected so far.
                """
                # If one of the dimensions is empty, we don't have any valid data
                for i in self.dims:
                        if len(i) == 0:
                                return False

                # Create the data array, with all values set as missing
                #print "volnd finalise instantiate"
                if self.info.is_string:
                        #print self.info, "string"
                        a = numpy.empty(map(len, self.dims), dtype=object)
                        # Fill the array with all the values, at the given indexes
                        for pos, val in self.vals:
                                if not self._checkConflicts or a[pos] == None:
                                        a[pos] = val
                                else:
                                        raise IndexError, "Got more than one value for " + self.name + " at position " + str(pos)
                else:
                        if self.info.scale == 0:
                                a = self._instantiateIntMatrix()
                        else:
                                a = numpy.empty(map(len, self.dims), dtype=float)
                        mask = numpy.ones(map(len, self.dims), dtype=bool)

                        # Fill the array with all the values, at the given indexes
                        for pos, val in self.vals:
                                if not self._checkConflicts or mask[pos] == True:
                                        a[pos] = val
                                        mask[pos] = False
                                else:
                                        raise IndexError, "Got more than one value for " + self.name + " at position " + str(pos)
                        a = ma.array(a, mask=mask)

                # Replace the intermediate data with the results
                self.vals = a

                # Finalise all the attributes as well
                #print "volnd finalise fill"
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


def read(cursor, dims, filter=None, checkConflicts=True, attributes=None):
        """
        *cursor* is a dballe.Cursor resulting from a dballe query

        *dims* is the sequence of indexes to use for shaping the data matrixes

        *filter* is an optional filter function that can be used to discard
        values from the query: if filter is not None, it will be called for
        every output record and if it returns False, the record will be
        discarded

        *checkConflicts* tells if we should raise an exception if two values from
        the database would fill in the same position in the matrix

        *attributes* tells if we should read attributes as well: if it is None,
        no attributes will be read; if it is True, all attributes will be read;
        if it is a sequence, then it is the sequence of attributes that should
        be read.
        """
        ndims = len(dims)
        vars = {}
        #print "volnd iterate"
        # Iterate results
        for rec in cursor:
                # Discard the values that filter does not like
                if filter and not filter(rec):
                        continue

                varname = rec["var"]

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
                                arec = cursor.query_attrs([]);
                                var.appendAttrs(arec)
                        else:
                                arec = cursor.query_attrs(attributes)
                                var.appendAttrs(arec)


        # Now that we have collected all the values, create the arrays
        #print "volnd finalise"
        invalid = []
        for k, var in vars.iteritems():
                if not var.finalise():
                        invalid.append(k)
        for k in invalid:
                del vars[k]

        return vars
