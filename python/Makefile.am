## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

TESTS = test.py
if DO_DBALLE_DB
TESTS += test-db.py test-volnd.py test-rconvert.py
endif

pkgpython_PYTHON = dbacsv.py rconvert.py __init__.py
if DO_DBALLE_DB
pkgpython_PYTHON += volnd.py
endif

pyexec_LTLIBRARIES = _dballe.la

_dballe_la_SOURCES = \
    common.cc \
    vartable.cc \
    varinfo.cc \
    var.cc \
    record.cc \
    dballe.cc
if DO_DBALLE_DB
_dballe_la_SOURCES += \
    db.cc \
    cursor.cc
endif
_dballe_la_CPPFLAGS = $(PYTHON_CFLAGS)
_dballe_la_LDFLAGS = -module -avoid-version -export-symbols-regex init_dballe
_dballe_la_LIBADD = ../dballe/libdballe.la

doc_DATA = python-dballe.rst

if HAVE_RST2HTML
doc_DATA += python-dballe.html
endif

_dballe.so: _dballe.la
	ln -fs .libs/_dballe.so $@

dballe: _dballe.so
	ln -s . dballe

python-dballe.rst: doc-dballe $(pkgpython_PYTHON) dballe
	$(srcdir)/doc-dballe > $@ || (rm -f $@; false)

python-dballe.html: python-dballe.rst
	rst2html $< > $@ || (rm -f $@; false)

clean-local:
	rm -f *.pyc dballe.so dballe

EXTRA_DIST = \
    common.h \
    vartable.h \
    varinfo.h \
    var.h \
    record.h \
    db.h \
    cursor.h \
    test.py test-db.py test-volnd.py test-rconvert.py

INCLUDES = -I/usr/include/python$(PYTHON_VERSION) -I$(top_srcdir)
