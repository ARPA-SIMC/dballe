"""
volnd is a way of extracting entire matrixes of data out of a DB-All.e
database.

This module allows to extract multidimensional matrixes of data given a list of
dimension definitions.  Every dimension definition defines what kind of data
goes along that dimension.

Dimension definitions can be shared across different extracted matrixes and
multiple extractions, allowing to have different matrixes whose indexes have
the same meaning.

This example code extracts temperatures in a station by datetime matrix::

        from dballe import volnd
        query = dict()
        query["var"] = "B12101"
        query["rep_memo"] = "synop"
        query["level"] = (103, 2000)
        query["trange"] = (254, 0, 0)
        with db.transaction() as tr:
            vars = volnd.read(tr.query_data(query), (volnd.AnaIndex(), volnd.DateTimeIndex()))
        data = vars["B12101"]
        # Data is now a 2-dimensional Masked Array with the data
        #
        # Information about what values correspond to an index in the various
        # directions can be accessed in data.dims, which contains one list per
        # dimension with all the information corresponding to every index.
        print("Ana dimension is", len(data.dims[0]), "items long")
        print("Datetime dimension is", len(data.dims[1]), "items long")
        print("First 10 stations along the Ana dimension:", data.dims[0][:10])
        print("First 10 datetimes along the DateTime dimension:", data.dims[1][:10])
"""


# TODO: aggiungere metodi di query negli indici (eg. qual'è l'indice di questo
#       ana_id?)
# TODO: leggere i dati di anagrafica

import dballe
from collections import namedtuple
import datetime
import sys
import warnings


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

# Alternative hack to run without numpy's C code
#from . import tinynumpy as numpy
#class ma:
#    @classmethod
#    def array(cls, a, *args, **kw):
#        return a

class SkipDatum(Exception): pass

class Index(object):
    """
    Base class for all volnd indices.

    An Index describes each entry along one dimension of a volnd volume. There
    is an entry in the index for each point along that axis, and each entry can
    be an arbitrary structure with details.

    Index objects can be shared between homogeneous volumes.
    """
    def __init__(self, shared=True, frozen=False):
        self._shared = shared
        self._frozen = frozen

    def freeze(self):
        """
        Set the index as frozen: indexing elements not already in the
        index will raise a SkipDatum exception
        """
        self._frozen = True

    def copy(self):
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
    """
    Indexes records along an axis.

    Each index entry is an arbitrary details structure extracted from records.
    All records at a given index position will have the same details.
    """
    def __init__(self, shared=True, frozen=False, start=None):
        super(ListIndex, self).__init__(shared, frozen)
        # Maps indexing keys to list positions
        # A key is a short, unique version of the details. Details can be
        # thought as verbose, useful versions of keys.
        self._map = {}
        if start:
            for el in start:
                id, val = self._splitInit(el)
                self._map[id] = len(self)
                self.append(val)

    def __str__(self):
        return self.short_name() + ": " + list.__str__(self)

    def key_from_record(self, rec):
        "Extract the indexing key from the record"
        return None

    def details_from_record(self, rec):
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
        """
        Return true if the record can be placed along this index
        """
        if not self._frozen: return True
        return self.key_from_record(rec) in self._map

    def index_record(self, rec):
        """
        Return an integer index along this axis for the given record
        """
        key = self.key_from_record(rec)
        pos = self._map.get(key, None)
        if pos is None:
            self._map[key] = pos = len(self)
            self.append(self.details_from_record(rec))
        return pos


class AnaIndexEntry(namedtuple("AnaIndexEntry", ("id", "lat", "lon", "ident"))):
    """
    AnaIndex entry, with various data about a single station.

    It is a named tuple of 4 values:
        * id: station id
        * lat: latitude
        * lon: longitude
        * ident: mobile station identifier, or None
    """
    @classmethod
    def from_record(cls, rec):
        """
        Create an index entry from the contents of a Dict[str, Any]
        """
        return cls(rec["ana_id"], rec["lat"], rec["lon"], rec["ident"])

    def __str__(self):
        if self[3] is None:
            return "Station at lat %.5f lon %.5f" % (self.lat, self.lon)
        else:
            return "%s at lat %.5f lon %.5f" % (self.ident, self.lat, self.lon)

    def __repr__(self):
        return "AnaIndexEntry" + tuple.__repr__(self)


class AnaIndex(ListIndex):
    """
    Index for stations, as they come out of the database.

    The constructor syntax is: ``AnaIndex(shared=True, frozen=False, start=None)``.

    The index saves all stations as AnaIndexEntry tuples, in the same order
    as they come out of the database.
    """
    def key_from_record(self, rec):
        return rec["ana_id"]

    def details_from_record(self, rec):
        return AnaIndexEntry.from_record(rec)

    def _splitInit(self, el):
        return el[0], el

    def short_name(self):
        return "AnaIndex["+str(len(self))+"]"


class NetworkIndex(ListIndex):
    """
    Index for networks, as they come out of the database.

    The constructor syntax is: ``NetworkIndex(shared=True, frozen=False, start=None)``.

    The index saves all networks as NetworkIndexEntry tuples, in the same
    order as they come out of the database.
    """
    def key_from_record(self, rec):
        return rec["rep_memo"]

    def details_from_record(self, rec):
        return rec["rep_memo"]

    def _splitInit(self, el):
        return el[0], el

    def short_name(self):
        return "NetworkIndex["+str(len(self))+"]"

class LevelIndex(ListIndex):
    """
    Index for levels, as they come out of the database

    The constructor syntax is: ``LevelIndex(shared=True, frozen=False, start=None)``.

    The index saves all levels as dballe.Level tuples, in the same order
    as they come out of the database.
    """
    def key_from_record(self, rec):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            return rec["level"]

    def details_from_record(self, rec):
        return self.key_from_record(rec)

    def short_name(self):
        return "LevelIndex["+str(len(self))+"]"

class TimeRangeIndex(ListIndex):
    """
    Index for time ranges, as they come out of the database.

    The constructor syntax is: ``TimeRangeIndex(shared=True, frozen=False, start=None)``.

    The index saves all time ranges as dballe.TimeRange tuples, in the same
    order as they come out of the database.
    """
    def key_from_record(self, rec):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            return rec["trange"]

    def details_from_record(self, rec):
        return self.key_from_record(rec)

    def short_name(self):
        return "TimeRangeIndex["+str(len(self))+"]"


class DateTimeIndex(ListIndex):
    """
    Index for datetimes, as they come out of the database.

    The constructor syntax is: ``DateTimeIndex(shared=True, frozen=False, start=None)``.

    The index saves all datetime values as datetime.datetime objects, in
    the same order as they come out of the database.
    """
    def key_from_record(self, rec):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            return rec["datetime"]

    def details_from_record(self, rec):
        return self.key_from_record(rec)

    def short_name(self):
        return "DateTimeIndex["+str(len(self))+"]"


def tddivmod1(td1, td2):
    "Division and quotient between time deltas"
    if td2 > td1:
        return 0, td1
    if td2 == 0:
        raise ZeroDivisionError("Dividing by a 0 time delta")
    mults = (86400, 1000000, 1)
    n1 = (td1.days, td1.seconds, td1.microseconds)
    n2 = (td2.days, td2.seconds, td2.microseconds)
    d = 0
    q = 0
    for i in range(3):
        d += n1[i]
        if d != 0:
            if n2[i] == 0:
                d *= mults[i]
            else:
                q = d // n2[i]
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
    q = std1 // std2
    return q, td1 - (td2 * q)

# Choose which implementation to use
if sys.version_info[0] >= 3:
    def tddivmod3(td1, td2):
        return td1 // td2, td1 % td2
    tddivmod = tddivmod3
else:
    tddivmod3 = None
    tddivmod = tddivmod2


class IntervalIndex(Index):
    """
    Index into equally spaced points in time, starting at ``start``, with a
    point every ``step`` time.

    Index points are at fixed time intervals, and data is acquired in one point
    only if it is within a given tolerance from the interval.

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
    def __init__(self, start, step, tolerance=0, end=None, *args, **kwargs):
        """
        start is a datetime with the starting moment
        step is a timedelta with the interval between times
        tolerance is a timedelta specifying how much skew a datum is
            allowed to have from a sampling moment
        """
        super(IntervalIndex, self).__init__(*args, **kwargs)
        self._start = start
        self._step = step
        self._end = end
        if end != None:
            self._size = (end-start)/step
        else:
            self._size = 0
        self._tolerance = datetime.timedelta(0)

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

    def index_record(self, rec):
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
        for i in range(self._size):
            yield self._start + self._step * i

    def __str__(self):
        return self.short_name() + ": " + ", ".join(self)

    def short_name(self):
        return "IntervalIndex["+str(self._size)+"]"

    def copy(self):
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

    def append(self, rec, val):
        """
        Collect a new value from the given dballe record.

        You need to call finalise() before the values can be used.
        """
        accepted = all(dim.approve(rec) for dim in self.dims)
        if accepted:
            # Obtain the index for every dimension
            pos = tuple(dim.index_record(rec) for dim in self.dims)

            # Save the value with its indexes
            self.vals.append((pos, val))

            # Save the last position for appendAttrs
            self._lastPos = pos

            return True
        else:
            # If the value cannot be mapped along this dimension,
            # skip it
            self._lastPos = None
            return False

    def appendAttrs(self, rec, codes=None):
        """
        Collect attributes to append to the record.

        You need to call finalise() before the values can be used.
        """
        if not self._lastPos:
            return
        for code, var in rec.items():
            if codes is not None and code not in codes:
                continue
            # print "Attr", var.code(), "for", self.name, "at", self._lastPos
            if code in self.attrs:
                data = self.attrs[code]
            else:
                data = Data(self.name, self.dims, False)
                self.attrs[code] = data
            # Append at the same position as the last variable
            # collected
            data.vals.append((self._lastPos, var))

    def _instantiateIntMatrix(self):
        shape = tuple(len(d) for d in self.dims)
        if self.info.bit_ref == 0:
            # bit_ref is 0, so we are handling unsigned
            # numbers and we know the exact number of bits
            # used for encoding
            bits = self.info.bit_len
            if bits <= 8:
                # print 'uint8'
                a = numpy.empty(shape, dtype='uint8')
            elif bits <= 16:
                #print 'uint16'
                a = numpy.empty(shape, dtype='uint16')
            elif bits <= 32:
                #print 'uint32'
                a = numpy.empty(shape, dtype='uint32')
            else:
                #print 'uint64'
                a = numpy.empty(shape, dtype='uint64')
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
                a = numpy.empty(shape, dtype='int8')
            elif range < 65536:
                #print 'int16'
                a = numpy.empty(shape, dtype='int16')
            elif range <= 4294967296:
                #print 'int32'
                a = numpy.empty(shape, dtype='int32')
            else:
                a = numpy.empty(shape, dtype=int)
        return a

    def finalise(self):
        """
        Stop collecting values and create a masked array with all the
        values collected so far.
        """
        # If one of the dimensions is empty, we don't have any valid data
        if any(len(d) == 0 for d in self.dims):
            return False

        shape = tuple(len(x) for x in self.dims)

        # Create the data array, with all values set as missing
        # print "volnd finalise instantiate"
        if self.info.type == "string":
            # print self.info, "string"
            a = numpy.empty(shape, dtype=object)
            # Fill the array with all the values, at the given indexes
            for pos, val in self.vals:
                if self._checkConflicts and a[pos] is not None:
                    raise IndexError("Got more than one value for " + self.name + " at position " + str(pos))
                a[pos] = val
        else:
            if self.info.type == "integer":
                a = self._instantiateIntMatrix()
            else:
                a = numpy.empty(shape, dtype=numpy.float64)
            mask = numpy.ones(shape, dtype=numpy.bool)

            # Fill the array with all the values, at the given indexes
            for pos, val in self.vals:
                if self._checkConflicts and not mask[pos]:
                    raise IndexError("Got more than one value for " + self.name + " at position " + str(pos))
                a[pos] = val.enqd()
                mask[pos] = False
            a = ma.array(a, mask=mask)

        # Replace the intermediate data with the results
        self.vals = a

        # Finalise all the attributes as well
        # print "volnd finalise fill"
        invalid = []
        for key, d in self.attrs.items():
            if not d.finalise():
                invalid.append(key)
        # Delete empty attributes
        for k in invalid:
            del self.addrs[k]
        return True

    def __str__(self):
        return "Data("+", ".join(x.short_name() for x in self.dims)+"):"+str(self.vals)

    def __repr__(self):
        return "Data("+", ".join(x.short_name() for x in self.dims)+"):"+self.vals.__repr__()


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
    vars = {}
    # Iterate results
    for rec in cursor:
        # Discard the values that filter does not like
        if filter and not filter(rec):
            continue

        v = rec["variable"]

        # Instantiate the index objects here for every variable
        # when it appears the first time, sharing those indexes that
        # need to be shared and creating new indexes for the individual
        # ones
        if v.code not in vars:
            var = Data(v.code, [x.copy() for x in dims], checkConflicts)
            vars[v.code] = var
        else:
            var = vars[v.code]

        # Save every value with its indexes
        if not var.append(rec, v):
            continue

        # Add the attributes
        if attributes is not None:
            arec = cursor.query_attrs()
            if attributes is True:
                var.appendAttrs(arec)
            else:
                var.appendAttrs(arec, attributes)


    # Now that we have collected all the values, create the arrays
    #print "volnd finalise"
    invalid = []
    for k, var in vars.items():
        if not var.finalise():
            invalid.append(k)
    for k in invalid:
        del vars[k]

    return vars
