#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import csv
import sys

"""
Export data from DB-All.e into CSV format
"""

def intormiss(x):
    """
    Format an integer to a string, returning '-' if the integer is None.
    """
    if x is None:
        return "-"
    else:
        return "%d" % x

class UnicodeCSVWriter(object):
    """
    Hack to work around the csv module being unable to handle unicode rows in
    input, and unicode files in output
    """

    class UnicodeFdWrapper(object):
        """
        Wrap an output file descriptor into something that accepts utf8 byte
        strings and forwards unicode
        """
        def __init__(self, outfd):
            self.outfd = outfd

        def write(self, bytestr):
            self.outfd.write(bytestr.decode("utf-8"))

    def __init__(self, outfd, *writer_args, **writer_kw):
        self.writer = csv.writer(self.UnicodeFdWrapper(outfd), *writer_args, **writer_kw)

    def encode_field(self, val):
        encode = getattr(val, "encode", None)
        if encode is not None:
            return encode("utf-8")
        else:
            return val

    def writerow(self, row):
        enc = self.encode_field
        self.writer.writerow([enc(row) for s in row])


class Exporter:
    def __init__(self, db):
        self.db = db
        self.title = ""
        self.cols = []
        self.anaData = {}
        self.attrData = {}

    def getpaval(self, x, var):
        id = x["ana_id"]
        data = self.anaData[id]
        #print "***********", id, av, data
        if data.has_key(var):
            return data[var].format("")
        else:
            return ""

    def getattrval(self, x, v):
        data = self.attrData["%d,%s"%(x["context_id"],x["var"])]
        if v in data:
            return data.var(v).format()
        else:
            return ""

    def compute_columns(self, filter):
        title = ""
        cols = []

        # Do one pass over the result set to compute the columns
        stations = {}
        idents = set()
        anaVars = {}
        reps = set()
        dates = set()
        levels = set()
        tranges = set()
        vars = set()
        attrs = {}
        for d in self.db.query_data(filter):
            # Add columns about the station
            id = d["ana_id"]
            stations[id] = [d["lat"], d["lon"], d.get("ident", None)]
            idents.add(d.get("ident", None))

            # Get info about the pseudoana extra data
            if id not in self.anaData:
                query = dballe.Record(ana_id=id)
                query.set_station_context()
                items = {}
                for record in self.db.query_data(query):
                    v = record.var()
                    items[v.code] = v
                    val = v.get()
                    if v.code in anaVars:
                        if anaVars[v.code] != val:
                            anaVars[v.code] = None
                    else:
                        anaVars[v.code] = val
                    #print id, v, items[v].format('none')
                self.anaData[id] = items

            # Add columns about the context

            # Repcod
            reps.add(d["rep_memo"])

            # Date
            dates.add(d["date"])

            # Level layer
            levels.add(",".join([intormiss(x) for x in d["level"]]))

            # Time range
            tranges.add(",".join([intormiss(x) for x in d["trange"]]))

            # Variables
            vars.add(d["var"])

            # Attributes
            attributes = self.db.query_attrs(d["var"], d["context_id"])
            self.attrData["%d,%s"%(d["context_id"], d["var"])] = attributes
            for code in attributes:
                v = attributes.var(code)
                val = v.format("");
                if code in attrs:
                    if attrs[code] != val:
                        attrs[code] = None
                else:
                    attrs[code] = val

        # Now that we have detailed info, compute the columns

        if len(stations) == 1:
            # Get the data on the station
            for i in stations.itervalues():
                data = i
                break;
            if data[2] == None:
                title = title + "Fixed station, lat %f, lon %f. " % (data[0], data[1])
            else:
                title = title + "Mobile station %s, lat %f, lon %f. " % (data[2], data[0], data[1])
        else:
            cols.append(["Station", lambda x: x["ana_id"]])
            cols.append(["Latitude", lambda x: x["lat"]])
            cols.append(["Longitude", lambda x: x["lon"]])
            if len(idents) > 1:
                cols.append(["Ident", lambda x: x.get("ident", None) or ""])

        # Repcod
        if len(reps) == 1:
            title = title + "Report: %s." % reps.pop()
        elif len(reps) > 1:
            cols.append(["Report", lambda x: x["rep_memo"]])

        # Date
        if len(dates) == 1:
            title = title + "Date: %s." % (dates.pop())
        elif len(dates) > 1:
            cols.append(["Date", lambda x: x["date"]])

        # Level layer
        if len(levels) == 1:
            title = title + "Level: %s." % (levels.pop())
        elif len(levels) > 1:
            cols.append(["Level1", lambda x: intormiss(x["level"][0])])
            cols.append(["L1", lambda x: intormiss(x["level"][1])])
            cols.append(["Level2", lambda x: intormiss(x["level"][2])])
            cols.append(["L2", lambda x: intormiss(x["level"][3])])

        # Time range
        if len(tranges) == 1:
            title = title + "Time range: %s." % (tranges.pop())
        elif len(tranges) > 1:
            cols.append(["Time range", lambda x: intormiss(x["trange"][0])])
            cols.append(["P1", lambda x: intormiss(x["trange"][1])])
            cols.append(["P2", lambda x: intormiss(x["trange"][2])])

        # Variables
        for v in sorted(vars):
            cols.append([v, lambda x, v=v: x.var(v).format("")])

        # Column for special station ana data
        for av in sorted(anaVars.keys()):
            if anaVars[av] == None:
                # Dirty workaround to compensate Python's lack of proper
                # anonymous functions: see http://www.jnetworld.com/python.htm
                cols.append(["Ana "+av, lambda a, av=av: self.getpaval(a, av)])
            else:
                title = title + "Ana %s: %s. " % (av, anaVars[av])

        # Attributes
        for v in sorted(attrs.keys()):
            if attrs[v] == None:
                # Dirty workaround to compensate Python's lack of proper
                # anonymous functions: see http://www.jnetworld.com/python.htm
                cols.append(["Attr "+v, lambda a, v=v: self.getattrval(a, v)])
            else:
                title = title + "Attr %s: %s. " % (v, attrs[v])

        self.title = title
        self.cols = cols

    def output(self, query, fd):
        """
        Perform a DB-All.e query using the given query and output the results
        in CSV format on the given file object
        """
        #writer = csv.writer(fd, dialect="excel")
        writer = UnicodeCSVWriter(fd)

        self.compute_columns(query)

        # Don't query an empty result set
        if len(self.cols) == 0:
            print("Warning: result is empty.", file=sys.stderr)
            return

        # Print the title if we have it
        if len(self.title) > 0:
            row = ["" for x in range(len(self.cols))]
            row[0] = self.title
            writer.writerow(row)

        # Print the column titles
        writer.writerow([x[0] for x in self.cols])

        for rec in self.db.query_data(query):
            fields = []
            for c in self.cols:
                fields.append(c[1](rec))
            writer.writerow(fields)

def export(db, query, fd):
    """
    Perform a DB-All.e query using the given db and query query, and output
    the results in CSV format on the given file object
    """
    e = Exporter(db)
    e.output(query, fd)
