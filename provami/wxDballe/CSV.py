#!/usr/bin/python

import dballe
import csv

class Exporter:
    def __init__(self, db):
        self.db = db
        self.title = ""
        self.cols = []
        self.anaData = {}
        self.attrData = {}

    def getpaval (self, x, var):
        id = x.enqi("ana_id")
        data = self.anaData[id]
        #print "***********", id, av, data
        if data.has_key(var):
            return data[var].format("")
        else:
            return ""

    def getattrval (self, x, v):
        data = self.attrData["%d,%s"%(x.enqi("context_id"),x.enqc("var"))]
        if data.contains(v):
            return data.enqvar(v).format()
        else:
            return ""

    def computeColumns(self, filter):
        title = ""
        cols = []

        # Do one pass over the result set to compute the columns
        stations = {}
        idents = set()
        anaVars = {}
        reps = set()
        rcod = {}
        dates = set()
        levels = set()
        tranges = set()
        vars = set()
        attrs = {}
        for d in self.db.query(filter):
            # Add columns about the station
            id = d.enqi("ana_id")
            stations[id] = [d.enqd("lat"), d.enqd("lon"), d.enqc("ident")]
            idents.add(d.enqc("ident"))

            # Get info about the pseudoana extra data
            if not self.anaData.has_key(id):
                query = dballe.Record()
                query.seti("ana_id", id)
                query.setAnaContext()
                items = {}
                for record in self.db.query(query):
                    v = record.enqc("var")
                    items[v] = record.enqvar(v).copy()
                    val = record.enqvar(v).format("");
                    if anaVars.has_key(v):
                        if anaVars[v] != val:
                            anaVars[v] = None
                    else:
                        anaVars[v] = val
                    #print id, v, items[v].format('none')
                self.anaData[id] = items

            # Add columns about the context

            # Repcod
            reps.add(d.enqi("rep_cod"))
            rcod[d.enqi("rep_cod")] = d.enqc("rep_memo")

            # Date
            dates.add(d.enqc("datetime"))

            # Level layer
            levels.add("%d,%d,%d" % (d.enqi("leveltype"), d.enqi("l1"), d.enqi("l2")))

            # Time range
            tranges.add("%d,%d,%d" % (d.enqi("pindicator"), d.enqi("p1"), d.enqi("p2")))

            # Variables
            vars.add(d.enqc("var"))

            # Attributes
            attributes = dballe.Record()
            self.db.attrQuery(d.enqi("context_id"), d.enqc("var"), attributes)
            self.attrData["%d,%s"%(d.enqi("context_id"), d.enqc("var"))] = attributes
            for v in attributes:
                #attrs.add(v.code())
                code = v.code()
                val = v.format("");
                if attrs.has_key(code):
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
            cols.append(["Station", lambda x: x.enqi("ana_id")])
            cols.append(["Latitude", lambda x: x.enqd("lat")])
            cols.append(["Longitude", lambda x: x.enqd("lon")])
            if len(idents) > 1:
                cols.append(["Ident", lambda x: x.enqc("ident") or ""])

        # Repcod
        if len(reps) == 1:
            title = title + "Report: %s." % rcod[reps.pop()]
        elif len(reps) > 1:
            cols.append(["Report", lambda x: x.enqc("rep_memo")])

        # Date
        if len(dates) == 1:
            title = title + "Date: %s." % (dates.pop())
        elif len(dates) > 1:
            cols.append(["Date", lambda x: x.enqc("datetime")])

        # Level layer
        if len(levels) == 1:
            title = title + "Level: %s." % (levels.pop())
        elif len(levels) > 1:
            cols.append(["Level", lambda x: x.enqi("leveltype")])
            cols.append(["L1", lambda x: x.enqi("l1")])
            cols.append(["L2", lambda x: x.enqi("l2")])

        # Time range
        if len(tranges) == 1:
            title = title + "Time range: %s." % (tranges.pop())
        elif len(tranges) > 1:
            cols.append(["Time range", lambda x: x.enqi("pindicator")])
            cols.append(["P1", lambda x: x.enqi("p1")])
            cols.append(["P2", lambda x: x.enqi("p2")])

        # Variables
        for v in sorted(vars):
            cols.append([v, lambda x: x.enqvar(x.enqc("var")).format("")])

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

    def output(self, filter, fd):
        #writer = csv.writer(fd, dialect="excel")
        writer = csv.writer(fd)

        self.computeColumns(filter)

        # Don't query an empty result set
        if len(self.cols) == 0:
            sys.stderr.write("Warning: result is empty.\n")
            return

        # Print the title if we have it
        if len(self.title) > 0:
            row = ["" for x in range(len(self.cols))]
            row[0] = self.title
            writer.writerow(row)

        # Print the column titles
        writer.writerow([x[0] for x in self.cols])

        for result in self.db.query(filter):
            fields = []
            for c in self.cols:
                fields.append(c[1](result))
            writer.writerow(fields)

# vim:set ts=4 sw=4 expandtab:
