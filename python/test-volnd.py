#!/usr/bin/python3
import dballe
from dballe.volnd import tddivmod1, tddivmod2, tddivmod3, read, AnaIndex, LevelIndex, TimeRangeIndex, DateTimeIndex, NetworkIndex
from testlib import DballeDBMixin
import unittest
import random
import datetime
import warnings
import numpy.ma as ma


class TestTddiv(unittest.TestCase):
    def dtest(self, td1, td2):
        self.assertEqual(tddivmod1(td1, td2), tddivmod2(td1, td2))
        q, r = tddivmod1(td1, td2)
        self.assertEqual(td2 * q + r, td1)

        if tddivmod3 is not None:
            self.assertEqual(tddivmod1(td1, td2), tddivmod3(td1, td2))

    def testtddiv(self):
        # self.assertEqual(tddivmod(datetime.timedelta(10, 0, 0), datetime.timedelta(2, 0, 0)), (5, datetime.timedelta(0)))
        # self.assertEqual(tddivmod(datetime.timedelta(10, 0, 1), datetime.timedelta(2, 0, 0)), (5, datetime.timedelta(0, 0, 1)))
        # self.assertEqual(tddivmod(datetime.timedelta(10, 0, 1), datetime.timedelta(3, 0, 0)), (3, datetime.timedelta(1, 0, 1)))
        # self.assertEqual(tddivmod(datetime.timedelta(10, 6, 18), datetime.timedelta(5, 3, 9)), (2, datetime.timedelta(0)))
        # self.assertEqual(tddivmod(datetime.timedelta(3, 4, 5), datetime.timedelta(1, 3, 10)), (2, datetime.timedelta(0, 86397, 999985)))
        # self.assertEqual(tddivmod(datetime.timedelta(0, 4, 5), datetime.timedelta(0, 3, 10)), (1, datetime.timedelta(0, 0, 999995)))
        # self.assertEqual(tddivmod(datetime.timedelta(2, 40, 10), datetime.timedelta(0, 0, 5)), (34568000002, datetime.timedelta(0)))

        self.dtest(datetime.timedelta(10, 0, 0), datetime.timedelta(2, 0, 0))
        self.dtest(datetime.timedelta(10, 0, 1), datetime.timedelta(2, 0, 0))
        self.dtest(datetime.timedelta(10, 0, 1), datetime.timedelta(3, 0, 0))
        self.dtest(datetime.timedelta(10, 6, 18), datetime.timedelta(5, 3, 9))
        self.dtest(datetime.timedelta(3, 4, 5), datetime.timedelta(1, 3, 10))
        self.dtest(datetime.timedelta(0, 4, 5), datetime.timedelta(0, 3, 10))
        self.dtest(datetime.timedelta(2, 40, 10), datetime.timedelta(0, 0, 5))

        # Re-enable when Debian bug #48872 has been fixed
        # self.dtest(datetime.timedelta(999999999, 86399, 999999), datetime.timedelta(0, 0, 2))

        random.seed(1)
        for i in range(100):
            td1 = datetime.timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
            td2 = datetime.timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
            self.dtest(td1, td2)

        # Re-enable when Debian bug #48872 has been fixed
        # for i in xrange(100):
        #       td1 = datetime.timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
        #       td2 = datetime.timedelta(0, random.randint(0, 86400), random.randint(0, 1000000))
        #       self.dtest(td1, td2)
        # for i in xrange(100):
        #       td1 = datetime.timedelta(random.randint(0, 999999999), random.randint(0, 86400), random.randint(0, 1000000))
        #       td2 = datetime.timedelta(0, 0, random.randint(0, 1000000))
        #       self.dtest(td1, td2)
        # for i in xrange(100):
        #       td1 = datetime.timedelta(0, random.randint(0, 86400), random.randint(0, 1000000))
        #       td2 = datetime.timedelta(0, 0, random.randint(0, 1000000))
        #       self.dtest(td1, td2)


class ReadMixin(DballeDBMixin):
    def setUp(self):
        super(ReadMixin, self).setUp()
        from testlib import fill_volnd
        fill_volnd(self.db)

    def testIndexFind(self):
        # Ana in one dimension, network in the other
        query = dict(ana_id=1, var="B13011", rep_memo="synop")
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        vars = read(self.db.query_data(query), (AnaIndex(), TimeRangeIndex()))
        self.assertEqual(vars["B13011"].dims[1].index((4, -21600, 0)), 1)

    def testFilter(self):
        # Ana in one dimension, network in the other
        query = dict(ana_id=1, var="B13011", rep_memo="synop")
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            vars = read(self.db.query_data(query),
                        (AnaIndex(), TimeRangeIndex()),
                        filter=lambda rec: rec["trange"] == (4, -21600, 0))
        self.assertEqual(vars["B13011"].dims[1].index((4, -21600, 0)), 0)

    def testUnsharedIndex(self):
        # Ana in one dimension, network in the other
        query = dict(ana_id=1, rep_memo="synop")

        vars = read(self.db.query_data(query),
                    (AnaIndex(), TimeRangeIndex(), DateTimeIndex()))
        self.assertEqual(len(vars["B13011"].dims[2]), len(vars["B10004"].dims[2]))
        self.assertEqual(vars["B13011"].dims[2], vars["B10004"].dims[2])

        vars = read(self.db.query_data(query),
                    (AnaIndex(), TimeRangeIndex(), DateTimeIndex(shared=False)))
        self.assertNotEqual(len(vars["B13011"].dims[2]), len(vars["B10004"].dims[2]))

    def testConflicts(self):
        # Ana in one dimension, network in the other
        query = dict(ana_id=1, var="B13011")
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        # Here conflicting values are overwritten
        vars = read(self.db.query_data(query), (AnaIndex(), ), checkConflicts=False)
        self.assertEqual(type(vars), dict)
        # Here insted they should be detected
        with self.assertRaises(IndexError):
            read(self.db.query_data(query), (AnaIndex(),), checkConflicts=True)

    def testFixedIndex(self):
        # Ana in one dimension, network in the other
        query = dict(ana_id=1, rep_memo="synop", year=2007, month=1, day=1)

        vars = read(self.db.query_data(query),
                    (AnaIndex(), TimeRangeIndex(frozen=True, start=(dballe.Trange(4, -21600, 0), (4, -43200, 0)))),
                    checkConflicts=False)
        self.assertEqual(len(vars["B13011"].dims[1]), 2)

        vars = read(self.db.query_data(query), (AnaIndex(), TimeRangeIndex()), checkConflicts=False)
        self.assertEqual(len(vars["B13011"].dims[1]), 3)

        vars = read(self.db.query_data(query),
                    (AnaIndex(), LevelIndex(frozen=True, start=(dballe.Level(1, None, None, None),))),
                    checkConflicts=False)
        self.assertEqual(len(vars["B13011"].dims[1]), 1)

        vars = read(self.db.query_data(query), (AnaIndex(), LevelIndex()), checkConflicts=False)
        self.assertEqual(len(vars["B13011"].dims[1]), 2)

    def testAnaNetwork(self):
        # Ana in one dimension, network in the other
        query = {}
        query["var"] = "B10004"
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        vars = read(self.db.query_data(query), (AnaIndex(), NetworkIndex()))
        self.assertEqual(len(vars), 1)
        self.assertCountEqual(vars.keys(), ["B10004"])
        data = vars["B10004"]
        self.assertEqual(data.name, "B10004")
        self.assertEqual(len(data.attrs), 0)
        self.assertEqual(len(data.dims), 2)
        self.assertEqual(len(data.dims[0]), 11)
        self.assertEqual(len(data.dims[1]), 2)
        self.assertEqual(data.vals.size, 22)
        self.assertEqual(data.vals.shape, (11, 2))
        self.assertEqual(sum(data.vals.mask.flat), 11)
        self.assertEqual(round(ma.average(data.vals)), 83185)
        self.assertEqual(data.dims[0][0], (1, 10., 15., None))
        self.assertEqual(data.dims[0][1], (2, 10., 25., None))
        self.assertEqual(data.dims[0][2], (3, 20., 15., None))
        self.assertEqual(data.dims[0][3], (4, 20., 25., None))
        self.assertEqual(data.dims[0][4], (5, 30., 15., None))
        self.assertEqual(data.dims[0][5], (6, 30., 25., None))
        self.assertEqual(set(data.dims[1]), set(("temp", "synop")))

    def testAnaTrangeNetwork(self):
        # 3 dimensions: ana, timerange, network
        # 2 variables
        query = dict(datetime=datetime.datetime(2007, 1, 1, 0, 0, 0))
        vars = read(self.db.query_data(query), (AnaIndex(), TimeRangeIndex(shared=False), NetworkIndex()))
        self.assertEqual(len(vars), 2)
        self.assertEqual(sorted(vars.keys()), ["B10004", "B13011"])

        data = vars["B10004"]
        self.assertEqual(data.name, "B10004")
        self.assertEqual(len(data.attrs), 0)
        self.assertEqual(len(data.dims), 3)
        self.assertEqual(len(data.dims[0]), 12)
        self.assertEqual(len(data.dims[1]), 1)
        self.assertEqual(len(data.dims[2]), 2)
        self.assertEqual(data.vals.size, 24)
        self.assertEqual(data.vals.shape, (12, 1, 2))
        self.assertEqual(sum(data.vals.mask.flat), 13)
        self.assertEqual(round(ma.average(data.vals)), 83185)
        self.assertEqual(data.dims[0][0], (1, 10., 15., None))
        self.assertEqual(data.dims[0][1], (2, 10., 25., None))
        self.assertEqual(data.dims[0][2], (3, 20., 15., None))
        self.assertEqual(data.dims[0][3], (4, 20., 25., None))
        self.assertEqual(data.dims[0][4], (5, 30., 15., None))
        self.assertEqual(data.dims[0][5], (6, 30., 25., None))
        self.assertEqual(data.dims[1][0], (0, None, None))
        self.assertEqual(set(data.dims[2]), set(("temp", "synop")))

        data = vars["B13011"]
        self.assertEqual(data.name, "B13011")
        self.assertEqual(len(data.attrs), 0)
        self.assertEqual(len(data.dims), 3)
        self.assertEqual(len(data.dims[0]), 12)
        self.assertEqual(len(data.dims[1]), 2)
        self.assertEqual(len(data.dims[2]), 2)
        self.assertEqual(data.vals.size, 48)
        self.assertEqual(data.vals.shape, (12, 2, 2))
        self.assertEqual(sum(data.vals.mask.flat), 24)
        self.assertAlmostEqual(ma.average(data.vals), 5.325, 6)
        self.assertEqual(data.dims[0][0], (1, 10., 15., None))
        self.assertEqual(data.dims[0][1], (2, 10., 25., None))
        self.assertEqual(data.dims[0][2], (3, 20., 15., None))
        self.assertEqual(data.dims[0][3], (4, 20., 25., None))
        self.assertEqual(data.dims[0][4], (5, 30., 15., None))
        self.assertEqual(data.dims[0][5], (6, 30., 25., None))
        self.assertEqual(data.dims[1][0], (4, -43200, 0))
        self.assertEqual(data.dims[1][1], (4, -21600, 0))
        self.assertEqual(set(data.dims[2]), set(("temp", "synop")))

        self.assertEqual(vars["B10004"].dims[0], vars["B13011"].dims[0])
        self.assertNotEqual(vars["B10004"].dims[1], vars["B13011"].dims[1])
        self.assertEqual(vars["B10004"].dims[2], vars["B13011"].dims[2])

    def testAttrs(self):
        # Same export as testAnaNetwork, but check that the
        # attributes are synchronised
        query = {}
        query["var"] = "B10004"
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        vars = read(self.db.query_data(query), (AnaIndex(), NetworkIndex()), attributes=True)
        self.assertEqual(len(vars), 1)
        self.assertCountEqual(vars.keys(), ["B10004"])
        data = vars["B10004"]
        self.assertEqual(len(data.attrs), 2)
        self.assertCountEqual(sorted(data.attrs.keys()), ['B33007', 'B33040'])

        for net, a in ('synop', 'B33007'), ('temp', 'B33040'):
            self.assertEqual(data.dims, data.attrs[a].dims)
            self.assertEqual(data.vals.size, data.attrs[a].vals.size)
            self.assertEqual(data.vals.shape, data.attrs[a].vals.shape)

            # Find what is the network dimension where we have the attributes
            netidx = -1
            for idx, n in enumerate(data.dims[1]):
                if n == net:
                    netidx = idx
                    break
            self.assertNotEqual(netidx, -1)

            # No attrs in the other network
            self.assertEqual([x for x in data.attrs[a].vals.mask[:, 1-netidx].flat], [True]*len(data.attrs[a].vals.mask[:, 1-netidx].flat))
            # Same attrs as values in this network
            self.assertEqual([x for x in data.vals.mask[:, netidx].flat], [x for x in data.attrs[a].vals.mask[:, netidx].flat])
        self.assertEqual(round(ma.average(data.attrs['B33007'].vals)), 32)
        self.assertEqual(round(ma.average(data.attrs['B33040'].vals)), 54)

    def testSomeAttrs(self):
        # Same export as testAnaNetwork, but check that the
        # attributes are synchronised
        query = {}
        query["var"] = "B10004"
        query["datetime"] = datetime.datetime(2007, 1, 1, 0, 0, 0)
        vars = read(self.db.query_data(query), (AnaIndex(), NetworkIndex()), attributes=('B33040',))
        self.assertEqual(len(vars), 1)
        self.assertCountEqual(vars.keys(), ["B10004"])
        data = vars["B10004"]
        self.assertEqual(len(data.attrs), 1)
        self.assertCountEqual(data.attrs.keys(), ['B33040'])

        a = data.attrs['B33040']
        self.assertEqual(data.dims, a.dims)
        self.assertEqual(data.vals.size, a.vals.size)
        self.assertEqual(data.vals.shape, a.vals.shape)

        # Find the temp index
        netidx = -1
        for idx, n in enumerate(data.dims[1]):
            if n == "temp":
                netidx = idx
                break
        self.assertNotEqual(netidx, -1)

        # Only compare the values on the temp index
        self.assertEqual([x for x in a.vals.mask[:, 1-netidx].flat], [True]*len(a.vals.mask[:, 1-netidx].flat))
        self.assertEqual([x for x in data.vals.mask[:, netidx].flat], [x for x in a.vals.mask[:, netidx].flat])
        self.assertEqual(round(ma.average(a.vals)), 54)

    def testEmptyExport(self):
        query = {}
        query["ana_id"] = 5000
        vars = read(self.db.query_data(query), (AnaIndex(), NetworkIndex()), attributes=True)
        self.assertEqual(len(vars), 0)

    def testGhostIndexes(self):
        # If an index rejects a variable after another index
        # has successfuly added an item, we used to end up with
        # a 'ghost' index entry with no items in it
        indexes = (TimeRangeIndex(), LevelIndex(frozen=True, start=(dballe.Level(3, 2, None, None),)))
        query = {}
        query['ana_id'] = 1
        query['var'] = 'B13011'
        vars = read(self.db.query_data(query), indexes, checkConflicts=False)
        self.assertCountEqual(vars.keys(), ["B13011"])
        self.assertEqual(len(vars["B13011"].dims[1]), 1)
        self.assertEqual(vars["B13011"].dims[0][0], (4, -21600, 0))

    def testBuggyExport1(self):
        indexes = (AnaIndex(),
                   LevelIndex(frozen=True, start=((1, None, None), (3, 2, None))),
                   TimeRangeIndex(),
                   DateTimeIndex())
        query = {}
        query['rep_memo'] = 'synop'
        read(self.db.query_data(query), indexes, checkConflicts=True, attributes=True)

    def testExportAna(self):
        indexes = (AnaIndex(),)
        query = {}
        query["rep_memo"] = "synop"
        vars = read(self.db.query_station_data(query), indexes, checkConflicts=True)
        self.assertEqual(sorted(vars.keys()), ["B01001", "B01002", "B01019"])

    def testExportSyncAna(self):
        # Export some data
        indexes = (AnaIndex(), DateTimeIndex())
        query = {}
        query["rep_memo"] = 'synop'
        query["level"] = (1,)
        query["trange"] = (4, -21600, 0)
        vars = read(self.db.query_data(query), indexes, checkConflicts=True)
        self.assertEqual(sorted(vars.keys()), ["B13011"])

        # Freeze all the indexes
        for i in range(len(indexes)):
            indexes[i].freeze()

        # Export the pseudoana data in sync with the data
        query.clear()
        query["rep_memo"] = "synop"
        anas = read(self.db.query_station_data(query), (indexes[0],), checkConflicts=True)

        self.assertEqual(sorted(anas.keys()), ["B01001", "B01002", "B01019"])
        self.assertEqual(anas["B01001"].dims[0], vars["B13011"].dims[0])


class TestReadV7(ReadMixin, unittest.TestCase):
    DB_FORMAT = "V7"


class TestReadMEM(ReadMixin, unittest.TestCase):
    DB_FORMAT = "MEM"


if __name__ == "__main__":
    from testlib import main
    main("test-volnd")
