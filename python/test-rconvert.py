#!/usr/bin/python

from __future__ import absolute_import
import dballe
import volnd
#import rpy, MA, numpy, rconvert
import numpy, rconvert, rpy2
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
db = dballe.DB.connect_test()
query = dballe.Record(date=datetime(2007, 1, 1, 0, 0, 0))
#query.set("var", "B10004")
#query.settimerange(dballe.TimeRange(0,0,0))
vars = volnd.read(db.query_data(query), (volnd.AnaIndex(), volnd.NetworkIndex(), volnd.LevelIndex(), volnd.TimeRangeIndex()))
#print "ana:", vars["B10004"].dims[0]
#print "net:", vars["B10004"].dims[1]
#print vars["B10004"]
#rpy.r.print_(rconvert.vnddata_to_r(vars['B10004']))
#rpy.r.assign('pippo', rconvert.vnddata_to_r(vars['B10004']))
#rpy.r.save('pippo', file='/tmp/pippo')

rconvert.volnd_save_to_r(vars, "/tmp/pippo")



#
# Second test case reported by Paolo
#

query = dballe.Record()
#query.setd("latmin", 10.)
#query.setd("latmax", 60.)
#query.setd("lonmin", -10.)
#query.setd("lonmax", 40.)
query["var"] = "B13011"
vars = volnd.read(db.query_data(query), (volnd.AnaIndex(),volnd.DateTimeIndex()), checkConflicts=False)
rconvert.volnd_save_to_r(vars, "/tmp/pippo")

