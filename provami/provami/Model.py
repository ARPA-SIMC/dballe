import dballe, dballe.dbacsv
import time
import datetime
import gc
import os
import sys
import traceback
from provami.Paths import DATADIR
from collections import namedtuple

try:
    import dballe.rconvert
    try:
        import rpy
    except ImportError:
        import rpy2
    HAS_RPY = True
except ImportError:
    HAS_RPY = False

try:
    import dballe.volnd
    HAS_VOLND = True
except ImportError:
    HAS_VOLND = False

def filePath(fileName):
    if os.access(fileName, os.R_OK):
        return fileName
    elif os.access(DATADIR + "/" + fileName, os.R_OK):
        return DATADIR + "/" + fileName
    else:
        print sys.stderr, "WARNING: file", fileName, "cannot be found either in the current directory or in", DATADIR
        return None

class DateUtils:
    fields = (
        ("year", "month", "day", "hour", "min", "sec"),
        ("yearmin", "monthmin", "daymin", "hourmin", "minumin", "secmin"),
        ("yearmax", "monthmax", "daymax", "hourmax", "minumax", "secmax"),
        )

    EXACT = 0
    MIN = 1
    MAX = 2

def completeDate(fields, how = DateUtils.EXACT):
    """
    Fill in the blanks from a partial date information.

    Round towards the past is how is EXACT or MIN; round towards the future
    if how is MAX.
    """
    year, month, day, hour, minute, second = tuple(fields)

    if year is None: return None

    if how == DateUtils.MIN or how == DateUtils.EXACT:
        return datetime.datetime(year, \
                month or 1,
                day or 1,
                hour or 0,
                minute or 0,
                second or 0)
    else:
        if hour is None: hour = 23
        if minute is None: minute = 59
        if second is None: second = 59

        time = datetime.time(hour, minute, second)

        month = month or 12
        if day is None:
            # To get the day as the last day of the month,
            # get the date as "the day before the first day of the next"
            if month == 12:
                month = 1
                year = year + 1
            else:
                month = month + 1
            date = datetime.date(year, month, 1) - datetime.timedelta(1)
        else:
            date = datetime.date(year, month, day)

        return datetime.datetime.combine(date, time)

def datetimeFromRecord(rec, fieldset = DateUtils.MIN):
    fn = DateUtils.fields[fieldset]
    return completeDate([rec.get(fn[x], None) for x in range(6)], fieldset)

class TTracer:
    def __init__(self, name):
        self.start = time.clock()
        self.name = name
        print "  TT", self.name, ": start"
    def __del__(self):
        print "  TT", self.name, ": ", time.clock() - self.start
    def partial(self, str):
        print "  TT", self.name, "[", str, "]: ", time.clock() - self.start

class ModelListener:
    def filterDirty(self, isDirty):
        pass
    def filterChanged(self, what):
        pass
    def invalidate(self):
        pass
    def hasData(self, what):
        pass

class ProgressListener:
    def progress(self, perc, text):
        pass
    def queryError(self, message):
        pass

class UpdateInterrupted:
    pass

SummaryKey = namedtuple("SummaryKey", ["ana_id", "rep_memo", "level", "trange", "var"])
SummaryVal = namedtuple("SummaryVal", ["datemin", "datemax", "count"])

def normalon(lon):
    """
    Normalise longitude values to the [-180..180[ interval
    """
    lon = round(lon * 100000)
    lon = ((lon + 18000000) % 36000000) - 18000000;
    return lon / 100000

class Summary(object):
    def __init__(self, db):
        self.db = db
        self.filter = None
        self.stations = dict()
        self.summary = dict()

    def update(self, filter):
        flt1 = dballe.Record()
        flt1["ana_filter"] = filter.get("ana_filter", None)
        flt1["data_filter"] = filter.get("data_filter", None)
        flt1["attr_filter"] = filter.get("attr_filter", None)
        flt1["date"] = filter.get("date", None)
        flt1["datemin"] = filter.get("datemin", None)
        flt1["datemax"] = filter.get("datemax", None)
        if self.filter != flt1:
            self.filter = flt1
            # Requery the summary
            self.stations = dict()
            self.summary = dict()
            for r in self.db.query_summary(self.filter):
                self.stations[r["ana_id"]] = {
                    "lat": r["lat"],
                    "lon": r["lon"],
                    "ident": r.get("ident", None),
                }
                # FIXME: quick fix now that a simplified summary query in
                # DB-All.e doesn't give us stats
                #self.summary[SummaryKey(r["ana_id"], r["rep_memo"], r["level"], r["trange"], r["var"])] = SummaryVal(
                #    r["datemin"], r["datemax"], r["limit"])
                self.summary[SummaryKey(r["ana_id"], r["rep_memo"], r["level"], r["trange"], r["var"])] = SummaryVal(
                    datetime.datetime(1000, 1, 1, 0, 0, 0), datetime.datetime(3000, 1, 1, 0, 0, 0), 1)

    def iter_stations_filtered(self, filter):
        ana_id = filter.get("ana_id", None)
        if ana_id:
            info = self.stations.get(ana_id, None)
            if info: yield ana_id, info
        else:
            lat = filter.get("lat", None)
            latmin = filter.get("latmin", None)
            latmax = filter.get("latmax", None)
            lon = filter.get("lon", None)
            lonmin = filter.get("lonmin", None)
            lonmax = filter.get("lonmax", None)
            if lonmin: lonmin = normalon(lonmin)
            if lonmax: lonmax = normalon(lonmax)
            for ana_id, info in self.stations.iteritems():
                if lat and info["lat"] != lat: continue
                if lon and info["lon"] != lon: continue
                if latmin and info["lat"] < latmin: continue
                if latmax and info["lat"] > latmax: continue
                if lonmin and lonmax:
                    if lonmin <= lonmax:
                        if info["lon"] < lonmin or info["lon"] > lonmax: continue
                    else:
                        if info["lon"] > lonmin and info["lon"] < lonmax: continue
                yield ana_id, info

    def iter_filtered(self, filter):
        # Filter stations
        station_whitelist = set(x[0] for x in self.iter_stations_filtered(filter))
        level = filter.get("level", None)
        trange = filter.get("trange", None)
        var = filter.get("var", None)
        rep_memo = filter.get("rep_memo", None)
        for k, v in self.summary.iteritems():
            if k.ana_id not in station_whitelist: continue
            if level and level != k.level: continue
            if trange and trange != k.trange: continue
            if var and var != k.var: continue
            if rep_memo and rep_memo != k.rep_memo: continue
            yield k, v

    def get_stations(self, filter):
        f = filter.copy()
        del f["ana_id"]
        del f["lat"]
        del f["latmin"]
        del f["latmax"]
        del f["lon"]
        del f["lonmin"]
        del f["lonmax"]
        # Build a set with the matched stations
        stations = set()
        for k, v in self.iter_filtered(f):
            stations.add(k.ana_id)
        for s in stations:
            yield s, self.stations[s]

    def get_dtimes(self, filter):
        f = filter.copy()
        del f["date"]
        del f["datemin"]
        del f["datemax"]
        res = [None, None]
        for k, v in self.iter_filtered(f):
            # TODO, skip entries that do not match filter
            if res[0] is None:
                res = [v.datemin, v.datemax]
            else:
                if v[0] < res[0]: res[0] = v[0]
                if v[1] < res[1]: res[1] = v[1]
        return res

    def get_levels(self, filter):
        f = filter.copy()
        del f["level"]
        levels = set()
        for k, v in self.iter_filtered(f):
            levels.add(k.level)
        return sorted(levels)

    def get_tranges(self, filter):
        f = filter.copy()
        del f["trange"]
        tranges = set()
        for k, v in self.iter_filtered(f):
            tranges.add(k.trange)
        return sorted(tranges)

    def get_vartypes(self, filter):
        f = filter.copy()
        del f["var"]
        vt = set()
        for k, v in self.iter_filtered(f):
            vt.add(k.var)
        return sorted(vt)

    def get_repinfo(self, filter):
        f = filter.copy()
        del f["rep_memo"]
        rt = set()
        for k, v in self.iter_filtered(f):
            rt.add((k.rep_memo, k.rep_memo))
        return sorted(rt)

class Model:
    HAS_RPY = HAS_RPY
    HAS_VOLND = HAS_VOLND
    def __init__(self, db):
        # Init DB-ALLe and connect to the database
        self.db = db
        self.truncateResults = True
        self.lowerTruncateThreshold = 250
        self.resultsMax = 500
        self.resultsCount = 0
        self.resultsTruncated = False
        self.summary = Summary(self.db)
        self.cached_results = []
        self.cached_stations = []
        self.cached_idents = []
        self.cached_dtimes = [None, None]
        self.cached_levels = []
        self.cached_tranges = []
        self.cached_vartypes = []
        self.cached_repinfo = []

        self.filter = dballe.Record()
        self.filterDirty = False
        self.activeFilter = self.filter.copy()

        self.updateListeners = []
        self.progressListeners = []

        self.updating = False
        self.updateCanceled = False

        #self.update()

    # Set the filter as dirty and notify listeners of the change
    def filterChanged(self, what=None):
        # If we are changing anything except the limits, reset the
        # limits
        if what is not None and what != 'limit':
            self.resetResultLimit()

        # See if the filter has been changed
        dirty = self.activeFilter != self.filter
        if self.filterDirty != dirty:
            for l in self.updateListeners: l.filterDirty(dirty)
            self.filterDirty = dirty
        if what is not None:
            for l in self.updateListeners: l.filterChanged(what)

    def hasStation(self, s_id):
        "Return true if the given station is among the current result set"
        for id, lat, lon, ident, vars in self.cached_stations:
            if id == s_id:
                return True
        return False

    def stations(self):
        "Generate all the station data"
        tracer = TTracer("model.stations (%d stations)" % (len(self.cached_stations)))
        for result in self.cached_stations:
            yield result

    def stationByID(self, station_id):
        "Return the data for one station, given its ID"
        for id, lat, lon, ident, vars in self.cached_stations:
            if id == station_id:
                return id, lat, lon, ident
        # Not found in cache, query through elencamele so we can give basic
        # details of stations not in the result set, or stations with no data
        filter = dballe.Record()
        filter["ana_id"] = station_id
        for result in self.db.query_stations(filter):
            return result["ana_id"], result["lat"], result["lon"], result.get("ident", None)
        return None, None, None, None

    def recordByContextAndVar(self, context, var):
        "Return a result record by context ID and variable type"
        for r in self.cached_results:
            if r["context_id"] == context and r["var"] == var:
                return r
        return None

    def idents(self):
        "Generate a list of idents"
        tracer = TTracer("model.idents")
        for result in self.cached_idents:
            yield result

    def hasVariable(self, context, var):
        "Returns true if the given context,var is in the current result set"
        for result in self.cached_results:
            if context == result["context_id"] and var == result["var"]:
                return True
        return False

    # Generate the list of all data
    def data(self):
        tracer = TTracer("model.data")
        for result in self.cached_results:
            yield result

    # Return a 2-tuple with the minimum and maximum datetime values
    def daterange(self):
        return self.cached_dtimes

    # Generate the list of all available levels
    def levels(self):
        tracer = TTracer("model.levels")
        for val in self.cached_levels:
            yield val

    # Generate the list of all available time ranges
    def timeranges(self):
        tracer = TTracer("model.timeranges")
        for val in self.cached_tranges:
            yield val

    # Generate the list of all available variable types
    def variableTypes(self):
        tracer = TTracer("model.variableTypes")
        for val in self.cached_vartypes:
            yield val

    # Generate the list of all available reports
    def reports(self):
        tracer = TTracer("model.reports")
        for results in self.cached_repinfo:
            yield results

    # Generate the list of all attributes available for the current context
    # and varcode
    def attributes(self):
        tracer = TTracer("model.attributes")
        result = dballe.Record()
        self.db.query_attrs(self.currentContext, self.currentVarcode, [], result)
        for var in result:
            yield var

    def registerUpdateListener(self, func):
        self.updateListeners.append(func)

    def registerProgressListener(self, func):
        self.progressListeners.append(func)

    def checkForCancelation(self):
        if self.updateCanceled:
            for l in self.progressListeners: l.progress(100, "Canceling update...")
            raise UpdateInterrupted()

    def notifyProgress(self, perc, text):
        self.checkForCancelation()
        for l in self.progressListeners: l.progress(perc, text)
        self.checkForCancelation()

    def cancelUpdate(self):
        self.updateCanceled = True

    def queryStations(self):
        filter = self.filter.copy()
        filter["query"] = "nosort"
        for i in ("latmin", "latmax", "lonmin", "lonmax", "ana_id", "ident", "mobile"):
            del filter[i]
        return self.db.query_stations(filter)

    def writeRecord(self, record):
        self.db.insert(record, True, False)
        #context = record.pop("context_id")
        #ana = record.pop("ana_id")

    def updateAttribute(self, context, varcode, var):
        attrs = dballe.Record()
        self.db.query_attrs(context, varcode, [], attrs)
        attrs.set(var)
        self.db.attr_insert(context, varcode, attrs)

    def deleteValues(self, values):
        for context, var in values:
            print "Removing", context, var
            filter = dballe.Record()
            filter["context_id"] = context
            filter["var"] = var
            self.db.remove(filter)
        self.update()

    def deleteAllAttrs(self, context, varcode):
        self.db.attr_remove(context, varcode, [])
        self.update()

    def deleteAttrs(self, context, varcode, attrs):
        self.db.attr_remove(context, varcode, attrs)
        self.update()

    def deleteCurrent(self):
        filter = self.activeFilter.copy()
        del filter["limit"]
        self.db.remove(filter)
        self.update()

    def deleteOrphans(self):
        self.db.removeOrphans()
        self.update()

    def exportToFile(self, file, encoding):
        filter = self.activeFilter.copy()
        del filter["limit"]
        if encoding == "CSV":
            exporter = dballe.dbacsv.export(self.db, filter, open(file, "w"))
        elif encoding == "R" and HAS_VOLND and HAS_RPY:
            idx = (dballe.volnd.AnaIndex(), \
                   dballe.volnd.DateTimeIndex(), \
                   dballe.volnd.LevelIndex(), \
                   dballe.volnd.TimeRangeIndex(), \
                   dballe.volnd.NetworkIndex())
            vars = dballe.volnd.read(self.db.query_data(filter), idx)
            dballe.rconvert.volnd_save_to_r(vars, file)
        #elif encoding == "VOLND":
        #   import dballe.volnd, cPickle
        #   idx = (dballe.volnd.AnaIndex(), \
        #          dballe.volnd.DateTimeIndex(), \
        #          dballe.volnd.LevelIndex(), \
        #          dballe.volnd.TimeRangeIndex(), \
        #          dballe.volnd.NetworkIndex())
        #   vars = dballe.volnd.read(self.db.query_data(filter), idx)
        #   cPickle.dump(vars, open(file, "w"))
        elif encoding == "gBUFR":
            self.db.export_to_file(filter, "BUFR", file, generic=True)
        else:
            self.db.export_to_file(filter, encoding, file)

    def update(self):
        try:
            if self.updating:
                print "ALREADY UPDATING"
                return
            self.updating = True
            self.updateCanceled = False

            print >>sys.stderr, self.filter

            self.notifyProgress(0, "Beginning update.")
            #for i in 'latmin', 'latmax', 'lonmin', 'lonmax', 'ana_id':
                #print "%s: %s" % (i, self.filter.enqc(i))

            # Notify invalidation of the data in the model
            self.notifyProgress(1, "Invalidating components...")
            for l in self.updateListeners: l.invalidate()

            # Save old data so we can restore it in case the query
            # is aborted or fails
            self.oldData = {}
            self.oldData['cached_stations'] = [x for x in self.cached_stations]
            self.oldData['cached_idents'] = [x for x in self.cached_idents]
            self.oldData['cached_dtimes'] = [x for x in self.cached_dtimes]
            self.oldData['resultsCount'] = self.resultsCount
            self.oldData['resultsTruncated'] = self.resultsTruncated
            self.oldData['cached_results'] = [x for x in self.cached_results]
            self.oldData['cached_levels'] = [x for x in self.cached_levels]
            self.oldData['cached_tranges'] = [x for x in self.cached_tranges]
            self.oldData['cached_vartypes'] = [x for x in self.cached_vartypes]
            self.oldData['cached_repinfo'] = [x for x in self.cached_repinfo]
            self.oldData['active_filter'] = self.activeFilter.copy()

            self.cached_stations = []
            self.cached_idents = []
            self.cached_dtimes = [None, None]
            self.resultsCount = 0
            self.resultsTruncated = False
            self.cached_results = []
            self.cached_levels = []
            self.cached_tranges = []
            self.cached_vartypes = []
            self.cached_repinfo = []

            # Take a note of the filter that is effectively active
            self.activeFilter = self.filter.copy()

            # This is a good moment to call the garbage collector,
            # since we're throwing away lots of data and
            # regenerating it
            gc.collect()

            t = TTracer("model.update")

            # Query station data
            self.notifyProgress(2, "Querying summary information...")
            self.summary.update(self.filter)
            idents = set()
            for ana_id, info in self.summary.get_stations(self.filter):
                self.cached_stations.append((
                    ana_id,
                    info["lat"],
                    info["lon"],
                    info["ident"],
                    {}))
                idents.add(info["ident"])
            self.cached_idents = sorted(idents)
            t.partial("got station (%d items) and ident (%d items) data" % (len(self.cached_stations), len(self.cached_idents)))

            self.notifyProgress(15, "Notifying station data...")
            for l in self.updateListeners: l.hasData("stations")
            t.partial("notified station data")
            for l in self.updateListeners: l.hasData("idents")
            t.partial("notified ident data")

            # Query datetimes
            self.notifyProgress(18, "Querying date and time data...")
            self.cached_dtimes = self.summary.get_dtimes(self.filter);
            t.partial("got date and time data (%d items)" % (len(self.cached_dtimes)))
            self.notifyProgress(30, "Notifying date and time data...")
            for l in self.updateListeners: l.hasData("dtimes")
            t.partial("notified date and time data")

            # Query variable data
            self.notifyProgress(35, "Querying result data...")
            if self.truncateResults:
                # We ask for 1 more than the upper bound, to detect the
                # case where the result set has been truncated
                self.filter["limit"] = self.resultsMax + 1
            else:
                del self.filter["limit"]
            self.filter["query"] = "nosort"
            for result in self.db.query_data(self.filter):
                self.cached_results.append(result.copy())
                self.resultsCount = self.resultsCount + 1
                # Bound to the real upper bound.
                if self.truncateResults and self.resultsCount > self.resultsMax:
                    self.resultsCount = self.resultsCount - 1
                    self.resultsTruncated = True
                    break
            t.partial("got variable data (%d items)" % (len(self.cached_results)));
            self.notifyProgress(52, "Notifying result data...")
            for l in self.updateListeners: l.hasData("data")
            t.partial("notified variable data");
            del self.filter["limit"]

            # Query levels
            self.notifyProgress(55, "Querying level data...")
            self.cached_levels = self.summary.get_levels(self.filter)
            t.partial("got level data (%d items)" % (len(self.cached_levels)));
            self.notifyProgress(60, "Notifying level data...")
            for l in self.updateListeners: l.hasData("levels")
            t.partial("notified level data");

            # Query time ranges
            self.notifyProgress(62, "Querying time range data...")
            self.cached_tranges = self.summary.get_tranges(self.filter)
            t.partial("got time range data (%d items)" % (len(self.cached_tranges)));
            self.notifyProgress(68, "Notifying time range data...")
            for l in self.updateListeners: l.hasData("tranges")
            t.partial("notified time range data");

            # Query variable types
            self.notifyProgress(70, "Querying variable types...")
            self.cached_vartypes = self.summary.get_vartypes(self.filter)
            t.partial("got variable type data (%d items)" % (len(self.cached_vartypes)));
            self.notifyProgress(82, "Notifying variable types...")
            for l in self.updateListeners: l.hasData("vartypes")
            t.partial("notified variable type data");

            # Query available reports
            self.notifyProgress(85, "Querying report types...")
            self.cached_repinfo = self.summary.get_repinfo(self.filter)
            t.partial("got repinfo data (%d items)" % (len(self.cached_repinfo)));
            self.notifyProgress(95, "Notifying report types...")
            for l in self.updateListeners: l.hasData("repinfo")
            t.partial("notified repinfo data");

            del self.filter["query"]

            # Notify that all data in the model are up to date
            self.notifyProgress(98, "Notifying completion...")
            for l in self.updateListeners: l.hasData("all")
            self.notifyProgress(100, "Completed.")

            # Notify the change in filter dirtyness, without any
            # specific changes in the filter itself
            self.filterChanged()

        except UpdateInterrupted:
            print "INTERRUPTED"
            self.tidyUpAfterInterruptedQuery()
        except RuntimeError, e:
            print "QUERY FAILED", str(e)
            traceback.print_tb(sys.exc_traceback)
            self.tidyUpAfterInterruptedQuery()
            for l in self.progressListeners: l.queryError(str(e))

        # This is another good moment to call the garbage collector, to get rid
        # of the database handles and temporary data that could still be around
        gc.collect()

        self.oldData = {}
        self.updating = False

    def tidyUpAfterInterruptedQuery(self):
        # Restore the old data
        self.activeFilter = self.oldData['active_filter']

        for l in self.updateListeners: l.invalidate()
        self.cached_stations = self.oldData['cached_stations']
        self.cached_idents = self.oldData['cached_idents']
        for l in self.updateListeners: l.hasData("stations")
        for l in self.updateListeners: l.hasData("idents")
        self.cached_dtimes = self.oldData['cached_dtimes']
        for l in self.updateListeners: l.hasData("dtimes")
        self.resultsCount = self.oldData['resultsCount']
        self.resultsTruncated = self.oldData['resultsTruncated']
        self.cached_results = self.oldData['cached_results']
        for l in self.updateListeners: l.hasData("data")
        self.cached_levels = self.oldData['cached_levels']
        for l in self.updateListeners: l.hasData("levels")
        self.cached_tranges = self.oldData['cached_tranges']
        for l in self.updateListeners: l.hasData("tranges")
        self.cached_vartypes = self.oldData['cached_vartypes']
        for l in self.updateListeners: l.hasData("vartypes")
        self.cached_repinfo = self.oldData['cached_repinfo']
        for l in self.updateListeners: l.hasData("repinfo")

        for l in self.updateListeners: l.hasData("all")
        self.notifyProgress(100, "Completed.")

    def setiFilter(self, name, value):
        """
        If the value has changed, set the value and call DirtyFilter.

        Returns True if the value has been changed, else False
        """
        if value is not None:
            value = int(value)
        if value != self.filter.get(name, None):
            if value is None:
                del self.filter[name]
            else:
                self.filter[name] = value
            return True
        return False

    def setdFilter(self, name, value):
        """
        If the value has changed, set the value and call DirtyFilter.

        Returns True if the value has been changed, else False
        """
        value = float(value) if value else None
        if value != self.filter.get(name, None):
            if value is None:
                del self.filter[name]
            else:
                self.filter[name] = value
            return True
        return False

    def setcFilter(self, name, value):
        """
        If the value has changed, set the value and call DirtyFilter.

        Returns True if the value has been changed, else False
        """
        value = str(value) if value else None
        if value != self.filter.get(name, None):
            if value is None:
                del self.filter[name]
            else:
                self.filter[name] = value
            return True
        return False

    def setReportFilter(self, rep_memo):
        """
        Set the filter to match a given report code

        Returns True if the filter has been changed, else False
        """
        r1 = self.setcFilter("rep_memo", rep_memo);
        if r1:
            self.filterChanged("repinfo")
            return True
        return False

    def setVarFilter(self, var):
        """
        Set the filter to match a given variable type

        Returns True if the filter has been changed, else False
        """
        r1 = self.setcFilter("var", var)
        if r1:
            self.filterChanged("var")
            return True
        return False

    def setAnaFilter(self, filter):
        """
        Set the ana filter expression

        Returns True if the filter has been changed, else False
        """
        #print "ANAFilter:", filter
        r1 = self.setcFilter("ana_filter", filter)
        if r1:
            self.filterChanged("ana_filter")
            return True
        return False

    def setDataFilter(self, filter):
        """
        Set the data filter expression

        Returns True if the filter has been changed, else False
        """
        #print "DATAFilter:", filter
        r1 = self.setcFilter("data_filter", filter)
        if r1:
            self.filterChanged("data_filter")
            return True
        return False

    def setAttrFilter(self, filter):
        """
        Set the attribute filter expression

        Returns True if the filter has been changed, else False
        """
        r1 = self.setcFilter("attr_filter", filter)
        if r1:
            self.filterChanged("attr_filter")
            return True
        return False

    def setStationFilter(self, id):
        """
        Set the filter to match a single station.

        Returns True if the filter has been changed, else False
        """
        r1 = self.setdFilter("latmin", None)
        r2 = self.setdFilter("latmax", None)
        r3 = self.setdFilter("lonmin", None)
        r4 = self.setdFilter("lonmax", None)
        r5 = self.setdFilter("mobile", None)
        r6 = self.setcFilter("ident", None)
        r7 = self.setiFilter("ana_id", id)
        if r1 or r2 or r3 or r4 or r5 or r6 or r7:
            self.filterChanged("stations")
            return True
        return False


    def setAreaFilter(self, latmin, latmax, lonmin, lonmax):
        """
        Set the filter to all stations in a given area.

        Returns True if the filter has been changed, else False
        """
        r1 = self.setdFilter("latmin", latmin)
        r2 = self.setdFilter("latmax", latmax)
        r3 = self.setdFilter("lonmin", lonmin)
        r4 = self.setdFilter("lonmax", lonmax)
        r5 = self.setiFilter("ana_id", None)
        if r1 or r2 or r3 or r4 or r5:
            self.filterChanged("stations")
            return True
        return False

    def setIdentFilter(self, mobile, ident):
        """
        Set the filter to match a given station ident.

        Returns True if the filter has been changed, else False
        """
        r1 = self.setiFilter("mobile", mobile)
        r2 = self.setcFilter("ident", ident)
        r3 = self.setiFilter("ana_id", None)
        if r1 or r2 or r3:
            self.filterChanged("stations")
            return True
        return False

    def getLevelFilter(self, record=None):
        return (record or self.filter).get("level", None)

    def setLevelFilter(self, level):
        """
        Set the filter to match a given level.

        Returns True if the filter has been changed, else False
        """
        if level != self.filter.get("level", None):
            self.filter["level"] = level
            self.filterChanged("level")
            return True
        return False

    def getTimeRangeFilter(self, record=None):
        return (record or self.filter).get("trange", None)

    def setTimeRangeFilter(self, tr):
        """
        Set the filter to match a given time range.

        Returns True if the filter has been changed, else False
        """
        if tr != self.filter.get("trange", None):
            self.filter["trange"] = tr
            self.filterChanged("trange")
            return True
        return False

    def getDateTimeFilter(self, filter=DateUtils.EXACT):
        return (self.filter.get(i, None) for i in DateUtils.fields[filter])

    def setDateTimeFilter(self, year, month=None, day=None, hour=None, min=None, sec=None, filter=DateUtils.EXACT):
        """
        Set the filter to match a given minimum, maximum or exact date.

        Returns True if the filter has been changed, else False
        """
        fn = DateUtils.fields[filter]
        r1 = self.setiFilter(fn[0], year)
        r2 = self.setiFilter(fn[1], month)
        r3 = self.setiFilter(fn[2], day)
        r4 = self.setiFilter(fn[3], hour)
        r5 = self.setiFilter(fn[4], min)
        r6 = self.setiFilter(fn[5], sec)
        if r1 or r2 or r3 or r4 or r5 or r6:
            self.filterChanged("datetime")
            return True
        return False

    def resetResultLimit(self):
        """
        Reset the result limit to its original value.

        Returns True if the filter has been changed, else False.
        """
        return self.setResultLimit(500)
        #if not self.truncateResults or self.resultsMax != 500:
        #   self.truncateResults = True
        #   self.resultsMax = 500
        #   changed = True
        #   self.filterChanged("limit")
        #   return True
        #return False

    def setResultLimit(self, limit):
        """
        Set the result limit.

        Returns True if the filter has been changed, else False
        """
        changed = False
        if limit is None and self.truncateResults:
            self.truncateResults = False;
            changed = True
        else:
            if not self.truncateResults:
                self.truncateResults = True
                changed = True
            if self.resultsMax != limit:
                self.resultsMax = limit
                changed = True
        if changed:
            self.filterChanged("limit")
            return True
        return False
