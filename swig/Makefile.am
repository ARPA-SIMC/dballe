## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

TESTS = test.py
if DB_TESTS
TESTS += test-db.py test-volnd.py run-unittests
endif

doc_DATA = python-dballe.rst python-dballe.html

pyexec_LTLIBRARIES = _dballe.la

nodist_pkgpython_PYTHON = dballe.py
pkgpython_PYTHON = dbacsv.py volnd.py rconvert.py __init__.py

nodist__dballe_la_SOURCES = dballe_wrap_python.cc
_dballe_la_LDFLAGS = -module
_dballe_la_LIBADD = ../dballe++/libdballepp.la

dballe_wrap_python.cc Dballe.py: dballe.i ../doc/dballe-doc.i
	swig -python -c++ -I.. -I/usr/include -o $@ $<

python-dballe.rst: doc-dballe dballe.py dballe volnd.py rconvert.py dballe.i _dballe.so
	./doc-dballe > $@ || (rm -f $@; false)

python-dballe.html: python-dballe.rst
	rst2html $< > $@ || (rm -f $@; false)

_dballe.so: _dballe.la
	ln -fs .libs/_dballe.so $@

dballe:
	ln -s . $@

INCLUDES = -I/usr/include/python$(PYTHON_VERSION)

EXTRA_DIST = README dballe.i test.py doc-dballe

CLEANFILES = dballe.py dballe.pyc dballe_wrap_python.cc dballe _dballe.so
