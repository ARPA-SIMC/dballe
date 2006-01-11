if DO_SWIG

## Process this file with automake to produce Makefile.in

python_LTLIBRARIES = _Dballe.la
python_DATA = Dballe.py

nodist__Dballe_la_SOURCES = dballe_wrap_python.cc
_Dballe_la_LDFLAGS = -module
_Dballe_la_LIBADD = ../dballe/libdballe.la
	
dballe_wrap_python.cc: dballe.i
	swig -python -c++ -o $@ $<

INCLUDES = -I/usr/include/python2.3

EXTRA_DIST = dballe.i DBAlle.h

CLEANFILES = dballe_wrap_python.cc Dballe.py Dballe.pyc

endif
