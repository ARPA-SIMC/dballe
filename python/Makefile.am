## Process this file with automake to produce Makefile.in

EXTRA_DIST =

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

dballe_python_tests = test-version.py test-core.py testlibmsg.py test-types.py test-binarymessage.py test-file.py test-message.py test-importer.py test-exporter.py test-db.py test-explorer.py test-volnd.py test-csv.py

# TESTS = $(dballe_python_tests)

pkgpython_PYTHON = dballe/dbacsv.py dballe/__init__.py dballe/volnd.py

pyexec_LTLIBRARIES = _dballe.la

_dballe_la_SOURCES = \
    utils/core.cc \
    utils/methods.cc \
    utils/values.cc \
    utils/wreport.cc \
    common.cc \
    types.cc \
    data.cc \
    data-access.cc \
    query-access.cc \
    binarymessage.cc \
    file.cc \
    message.cc \
    importer.cc \
    exporter.cc \
    dballe.cc \
    db.cc \
    cursor.cc \
    explorer.cc
_dballe_la_CPPFLAGS = $(PYTHON_CFLAGS)
_dballe_la_LDFLAGS = -module -avoid-version -export-symbols-regex init_dballe
_dballe_la_LIBADD = ../dballe/libdballe.la

data-access.cc: data-access.in.cc $(top_srcdir)/dballe/mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

data-access.tcc: data-access.in.tcc $(top_srcdir)/dballe/mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

query-access.cc: query-access.in.cc $(top_srcdir)/dballe/mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

BUILT_SOURCES = data-access.cc data-access.tcc query-access.cc
EXTRA_DIST += \
	data-access.in.cc data-access.cc \
	data-access.in.tcc data-access.tcc \
	query-access.in.cc query-access.cc

clean-local:
	rm -f *.pyc dballe/*.pyc
	rm -rf __pycache__ dballe/__pycache__

EXTRA_DIST += \
    doc-dballe \
    utils/core.h \
    utils/methods.h \
    utils/type.h \
    utils/values.h \
    utils/dict.h \
    utils/wreport.h \
    enq.h \
    common.h \
    types.h \
    data.h \
    binarymessage.h \
    file.h \
    message.h \
    db.h \
    cursor.h \
    importer.h \
    exporter.h \
    explorer.h \
    testlib.py \
    MANIFEST.in \
    setup.py \
    $(dballe_python_tests)

AM_CPPFLAGS = -I/usr/include/python$(PYTHON_VERSION) -I$(top_srcdir)

check-local:
	PYTHON="$(PYTHON)" $(TESTS_ENVIRONMENT) -v $(dballe_python_tests)

CLEANFILES = python-dballe.rst python-dballe.html
