## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

TESTS = test.py
if DB_TESTS
TESTS += test-db.py run-unittests
endif

pyexec_LTLIBRARIES = _dballe.la
nodist_python_PYTHON = dballe.py

pkgpython_PYTHON = dballe/ma.py dballe/__init__.py

nodist__dballe_la_SOURCES = dballe_wrap_python.cc
_dballe_la_LDFLAGS = -module
_dballe_la_LIBADD = ../dballe++/libdballepp.la

dballe_wrap_python.cc Dballe.py: dballe.i
	swig -python -c++ -I.. -I/usr/include -o $@ $<

INCLUDES = -I/usr/include/python$(PYTHON_VERSION)

EXTRA_DIST = README dballe.i test.py

CLEANFILES = dballe.py dballe.pyc dballe_wrap_python.cc
