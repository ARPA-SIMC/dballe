#!/usr/bin/python

import dballe, dballe.ma

db = dballe.DB("test", "enrico", "")

q = dballe.Record()
q.seti("ana_id", 1)
res = dballe.ma.read(db.query(q), (dballe.ma.AnaIndex(), dballe.ma.DateTimeIndex(), dballe.ma.LevelIndex(), dballe.ma.TimeRangeIndex(), dballe.ma.NetworkIndex()))

print res
