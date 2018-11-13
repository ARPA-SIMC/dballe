#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import csv
import sys
import warnings

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
        self.writer.writerow([enc(s) for s in row])


class Column(object):
    def __init__(self):
        self.values = set()

    def is_single_val(self):
        return len(self.values) == 1

    def column_labels(self):
        return [self.LABEL]

    def title(self):
        return "{} {}".format(self.LABEL, next(iter(self.values)))


class ColumnStation(Column):
    def __init__(self, stations):
        super(ColumnStation, self).__init__()
        self.stations = stations
        self.idents = set()

    def add(self, row):
        self.values.add(row["ana_id"])
        ident = row.get("ident", None)
        if ident is not None:
            self.idents.add(ident)

    def title(self):
        id = next(iter(self.values))
        lat, lon, ident = self.stations[id]
        if ident is None:
            return "Fixed station, lat {}, lon {}".format(lat, lon)
        else:
            return "Mobile station {}, lat {}, lon {}".format(ident, lat, lon)

    def column_labels(self):
        res = ["Station", "Latitude", "Longitude"]
        if self.idents:
            res.append("Ident")
        return res

    def column_data(self, rec):
        id = rec["ana_id"]
        lat, lon, ident = self.stations[id]
        res = [id, lat, lon]
        if self.idents:
            res.append(ident or "")
        return res


class ColumnNetwork(Column):
    LABEL = "Network"

    def add(self, row):
        self.values.add(row["rep_memo"])

    def column_data(self, rec):
        return [rec["rep_memo"]]


class ColumnDatetime(Column):
    LABEL = "Datetime"

    def add(self, row):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.values.add(row["date"])

    def title(self):
        return str(next(iter(self.values)))

    def column_data(self, rec):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            return [rec["date"]]


class ColumnLevel(Column):
    def add(self, row):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.values.add(",".join([intormiss(x) for x in row["level"]]))

    def title(self):
        return "Level {}".format(next(iter(self.values)))

    def column_labels(self):
        return ["Level1", "L1", "Level2", "L2"]

    def column_data(self, rec):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            lev = rec["level"]
        return [
            intormiss(lev.ltype1),
            intormiss(lev.l1),
            intormiss(lev.ltype2),
            intormiss(lev.l2)
        ]


class ColumnTrange(Column):
    def add(self, row):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.values.add(",".join([intormiss(x) for x in row["trange"]]))

    def title(self):
        return "Time range {}".format(next(iter(self.values)))

    def column_labels(self):
        return ["Time range", "P1", "P2"]

    def column_data(self, rec):
        # Suppress deprecation warnings until we have something better
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            tr = rec["trange"]
        return [intormiss(tr.pind), intormiss(tr.p1), intormiss(tr.p2)]


class ColumnVar(Column):
    LABEL = "Variable"

    def add(self, row):
        self.values.add(row["var"])

    def column_data(self, rec):
        return [rec["var"]]


class ColumnValue(Column):
    LABEL = "Value"

    def add(self, row):
        self.values.add(row.var(row["var"]).format(""))

    def column_data(self, rec):
        return [rec.var(rec["var"]).format("")]


class ColumnStationData(Column):
    def __init__(self, varcode, station_data):
        super(ColumnStationData, self).__init__()
        self.varcode = varcode
        # { Station id: { varcode: value } }
        self.station_data = station_data

    def add(self, row):
        self.values.add(row[self.varcode])

    def title(self):
        data = next(iter(self.station_data.values()))
        var = data.get(self.varcode, None)
        if var is None:
            value = ""
        else:
            value = var.format("")
        return "Station {}: {}".format(self.varcode, value)

    def column_labels(self):
        return ["Station {}".format(self.varcode)]

    def column_data(self, rec):
        id = rec["ana_id"]
        data = self.station_data[id]
        var = data.get(self.varcode, None)
        if var is None:
            return [""]
        else:
            return [var.format("")]


class ColumnAttribute(Column):
    def __init__(self, varcode, attributes):
        super(ColumnAttribute, self).__init__()
        self.varcode = varcode
        # { "context_id,varcode": { varcode: value } }
        self.attributes = attributes

    def add(self, var):
        self.values.add(var.format(""))

    def title(self):
        data = next(iter(self.attributes.itervalues()))
        var = data.get(self.varcode, None)
        if var is None:
            value = ""
        else:
            value = var.format("")
        return "Attr {}: {}".format(self.varcode, value)

    def column_labels(self):
        return ["Attr {}".format(self.varcode)]

    def column_data(self, rec):
        data = self.attributes["{},{}".format(rec["context_id"], rec["var"])]
        var = data.get(self.varcode, None)
        if var is None:
            return [""]
        else:
            return [var.format("")]


class Exporter:
    def __init__(self, db):
        self.db = db
        self.title = ""
        self.cols = []
        # { Station id: { varcode: value } }
        self.station_data = {}
        # { "context_id,varcode": { varcode: value } }
        self.attributes = {}

    def compute_columns(self, tr, filter):
        """
        Compute what columns need to be in the output CSV.

        It queries DB-All.e once; another query will be needed later to output
        data.
        """
        # Station data indexed by station id
        stations = {}

        # Do one pass over the result set to compute the columns
        columns = [
            ColumnStation(stations),
            ColumnNetwork(),
            ColumnDatetime(),
            ColumnLevel(),
            ColumnTrange(),
            ColumnVar(),
            ColumnValue(),
        ]
        station_var_cols = {}
        attribute_cols = {}
        rowcount = 0
        for row in tr.query_data(filter):
            rowcount += 1
            # Let the columns examine this row
            for c in columns:
                c.add(row)

            # Index station data by ana_id
            id = row["ana_id"]
            stations[id] = [row["lat"], row["lon"], row.get("ident", None)]

            # Load station variables for this station
            if id not in self.station_data:
                query = dict(ana_id=id)
                items = {}
                for record in tr.query_station_data(query):
                    v = record.var(record["var"])
                    items[v.code] = v
                    col = station_var_cols.get(v.code, None)
                    if col is None:
                        station_var_cols[v.code] = col = ColumnStationData(v.code, self.station_data)
                    col.add(record)
                self.station_data[id] = items

            # Load attributes
            attributes = {}
            for key, v in tr.attr_query_data(row["context_id"]).varitems():
                code = v.code
                attributes[code] = v
                col = attribute_cols.get(code, None)
                if col is None:
                    attribute_cols[code] = col = ColumnAttribute(code, self.attributes)
                col.add(v)
            self.attributes["{},{}".format(row["context_id"], row["var"])] = attributes
        self.rowcount = rowcount

        # Now that we have detailed info, compute the columns

        # Build a list of all candidate columns
        all_columns = []
        all_columns.extend(columns)
        for k, v in sorted(station_var_cols.items()):
            all_columns.append(v)
        for k, v in sorted(attribute_cols.items()):
            all_columns.append(v)

        # Dispatch them between title and csv body
        self.title_columns = []
        self.csv_columns = []
        for col in all_columns:
            if col.is_single_val():
                self.title_columns.append(col)
            else:
                self.csv_columns.append(col)

    def output(self, query, fd):
        """
        Perform a DB-All.e query using the given query and output the results
        in CSV format on the given file object
        """
        if sys.version_info[0] >= 3:
            writer = csv.writer(fd, dialect="excel")
        else:
            writer = UnicodeCSVWriter(fd)

        with self.db.transaction() as tr:
            self.compute_columns(tr, query)

            # Don't query an empty result set
            if self.rowcount == 0:
                print("Result is empty.", file=sys.stderr)
                return

            row_headers = []
            for c in self.csv_columns:
                row_headers.extend(c.column_labels())

            # Print the title if we have it
            if self.title_columns:
                title = "; ".join(c.title() for c in self.title_columns)
                row = ["" for x in row_headers]
                row[0] = title
                writer.writerow(row)

            # Print the column headers
            writer.writerow(row_headers)

            for rec in tr.query_data(query):
                row = []
                for c in self.csv_columns:
                    row.extend(c.column_data(rec))
                writer.writerow(row)


def export(db, query, fd):
    """
    Perform a DB-All.e query using the given db and query query, and output
    the results in CSV format on the given file object
    """
    e = Exporter(db)
    e.output(query, fd)
