# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
import shlex
import os
import sys
import unittest

def main(testname):
    args = os.environ.get("ARGS", None)
    if args is None:
        return unittest.main()

    args = shlex.split(args);
    if args[0] != testname:
        return 0

    argv = [sys.argv[0]] + args[1:]
    unittest.main(argv=argv)

def not_so_random(seed):
    """
    Predictable random number generator, independent from python versions
    """
    # see https://en.wikipedia.org/wiki/Linear_congruential_generator
    m = 2**31
    a = 1103515245
    c = 12345
    while True:
        seed = (a * seed + c) % m
        yield float(seed) / float(m)


def fill_volnd(db):
    import dballe
    import datetime

    # We want a predictable dataset
    rdata = not_so_random(1)
    rattr = not_so_random(2)

    # Wipe the test database
    db.remove_all()

    attrs = dballe.Record()
    rec = dballe.Record(mobile=0)

    def contexts():
        # 2 networks
        for net in ('synop', 'temp'):
            # 6 stations
            for lat in (10., 20., 30.):
                for lon in (15., 25.):
                    yield net, lat, lon

    def dtrange(start, stop, delta):
        while (start < stop):
            yield start
            start += delta

    def everyxhours(x):
        return dtrange(
                datetime.datetime(2007, 1, 1, 0, 0, 0),
                datetime.datetime(2007, 1, 7, 0, 0, 0),
                datetime.timedelta(0, x * 3600, 0))

    def maybe_insert(rec, aname):
        if next(rdata) > 0.9: return
        ids = db.insert_data(rec, False, True)
        attrs.clear()
        attrs[aname] = next(rattr) * 100.
        for code in rec:
            if not code.startswith("B"): continue
            db.attr_insert_data(ids[code], attrs)

    # Enter some sample data
    for net, lat, lon in contexts():
        rec["rep_memo"] = net
        if net == 'synop':
            aname = 'B33007'
        else:
            aname = 'B33040'
        rec["lat"] = lat
        rec["lon"] = lon

        # 6 hours precipitations
        rec["level"] = (1,)
        rec["trange"] = (4, -21600, 0)
        for dt in everyxhours(6):
            rec["datetime"] = dt
            rec["B13011"] = next(rdata) * 10.
            maybe_insert(rec, aname)

        # 12 hours precipitations at different times
        rec["level"] = (1,)
        rec["trange"] = (4, -43200, 0)
        for dt in everyxhours(12):
            rec["datetime"] = dt
            rec["B13011"] = next(rdata) * 10.
            maybe_insert(rec, aname)

        # Randomly measured
        # precipitations on a different
        # (meaningless) level
        # At slightly off times
        rec["level"] = (3, 2)
        rec["trange"] = (4, -21600, 0)
        for dt in everyxhours(6):
            rec["datetime"] = (dt + datetime.timedelta(0, - 600 + int(next(rdata) * 1200)))
            rec["B13011"] = next(rdata) * 10.
            maybe_insert(rec, aname)
        del rec["B13011"]

        # Pressures every 12 hours
        rec["level"] = (1,)
        rec["trange"] = (0,)
        for dt in everyxhours(12):
            rec["datetime"] = dt
            rec["B10004"] = float(70000 + next(rdata) * 35000)
            maybe_insert(rec, aname)
        del rec["B10004"]

    # Insert some pseudoana data for the station 1, to test
    # pseudoana export and mixed data types
    rec.clear()
    rec.update(ana_id=1, B01001=12, B01002=123, B01019="Test of long station name", rep_memo="synop")
    db.insert_station_data(rec, False, True)


class DballeDBMixin(object):
    def __init__(self, *args, **kw):
        super(DballeDBMixin, self).__init__(*args, **kw)
        if not hasattr(self, "assertCountEqual"):
            self.assertCountEqual = self.assertItemsEqual

    def get_db(self):
        db = dballe.DB.connect_test()
        db.reset()
        return db

    def setUp(self):
        if os.path.exists("test.sqlite"):
            os.unlink("test.sqlite")
        self.orig_db_format = dballe.DB.get_default_format()
        dballe.DB.set_default_format(self.DB_FORMAT)
        self.db = self.get_db()

    def tearDown(self):
        self.db = None
        dballe.DB.set_default_format(self.orig_db_format)
