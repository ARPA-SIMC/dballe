#!/usr/bin/python

#import rpy, MA, numpy, rconvert
import MA, numpy, rconvert, rpy
import dballe, dballe.volnd
from datetime import *

numpy.seterr(divide="raise", over="raise", under="raise", invalid="raise")

#def f(x,y,z):
#	return (x+1)*100+(y+1)*10+(z+1)
#a = MA.array(numpy.fromfunction(f, (4,3,2)))
##a = MA.array(numpy.array([[[1,2,3],[4,5,6]],[[7,8,9],[10,11,12]]], dtype='int8'))
#a[1,1,1]=MA.masked
#print "verypre", a
##rconvert.makeConvertible(a)
#rpy.r.print_(rconvert.ma_to_r(a, dimnames=[['a','b','c','d'],['e','f','g'],['h', 'i']]))


# Test from volnd
db = dballe.DB("test", "enrico", "")
query = dballe.Record()
#query.set("var", "B10004")
#query.settimerange(dballe.TimeRange(0,0,0))
query.setdate(datetime(2007, 1, 1, 0, 0, 0))
vars = dballe.volnd.read(db.query(query), (dballe.volnd.AnaIndex(), dballe.volnd.NetworkIndex(), dballe.volnd.LevelIndex(), dballe.volnd.TimeRangeIndex()))
#print "ana:", vars["B10004"].dims[0]
#print "net:", vars["B10004"].dims[1]
#print vars["B10004"]
#rpy.r.print_(rconvert.vnddata_to_r(vars['B10004']))
#rpy.r.assign('pippo', rconvert.vnddata_to_r(vars['B10004']))
#rpy.r.save('pippo', file='/tmp/pippo')

rconvert.vnd_save_to_r(vars, "/tmp/pippo")

