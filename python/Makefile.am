## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

dballe_python_tests = test-core.py test-types.py test-record.py test-db.py test-volnd.py test-csv.py
if HAVE_PYTHON3
dballe_python_tests += test-explorer.py
endif

# TESTS = $(dballe_python_tests)

pkgpython_PYTHON = dballe/dbacsv.py dballe/__init__.py dballe/volnd.py

pyexec_LTLIBRARIES = _dballe.la

_dballe_la_SOURCES = \
    common.cc \
    types.cc \
    record.cc \
    dballe.cc \
    db.cc \
    cursor.cc
_dballe_la_CPPFLAGS = $(PYTHON_CFLAGS)
_dballe_la_LDFLAGS = -module -avoid-version -export-symbols-regex init_dballe
_dballe_la_LIBADD = ../dballe/libdballe.la

if HAVE_PYTHON3
_dballe_la_SOURCES += explorer.cc
endif

doc_DATA = python-dballe.rst

if HAVE_RST2HTML
doc_DATA += python-dballe.html
endif

_dballe.so: _dballe.la
	ln -fs .libs/_dballe.so $@

python-dballe.rst: doc-dballe $(pkgpython_PYTHON) _dballe.so
	$(PYTHON) $(srcdir)/doc-dballe > $@ || (rm -f $@; false)

python-dballe.html: python-dballe.rst
	rst2html $< > $@ || (rm -f $@; false)

clean-local:
	rm -f *.pyc dballe/*.pyc _dballe.so
	rm -rf __pycache__ dballe/__pycache__

EXTRA_DIST = \
    doc-dballe \
    common.h \
    types.h \
    record.h \
    db.h \
    cursor.h \
    explorer.h \
    testlib.py \
    MANIFEST.in \
    setup.py \
    $(dballe_python_tests)

AM_CPPFLAGS = -I/usr/include/python$(PYTHON_VERSION) -I$(top_srcdir)

# TODO: environment variable to skip a test
check-local: _dballe.so
	PYTHON="$(PYTHON)" $(TESTS_ENVIRONMENT) $(dballe_python_tests)

CLEANFILES = python-dballe.rst python-dballe.html
