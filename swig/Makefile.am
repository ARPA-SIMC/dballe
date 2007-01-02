## Process this file with automake to produce Makefile.in

TESTS = test.py

python_LTLIBRARIES = _Dballe.la
nodist_python_PYTHON = Dballe.py

nodist__Dballe_la_SOURCES = dballe_wrap_python.cc
_Dballe_la_LDFLAGS = -module
_Dballe_la_LIBADD = ../dballe++/libdballepp.la

dballe_wrap_python.cc Dballe.py: dballe.i
	swig -python -c++ -I.. -I/usr/include -o $@ $<

INCLUDES = -I/usr/include/python$(PYTHON_VERSION)

EXTRA_DIST = README dballe.i test.py

CLEANFILES = Dballe.py Dballe.pyc dballe_wrap_python.cc
