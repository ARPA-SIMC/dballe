## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

dballe_python_tests = test-core.py test-record.py test-db.py test-volnd.py test-rconvert.py test-csv.py

# TESTS = $(dballe_python_tests)

pkgpython_PYTHON = dballe/dbacsv.py dballe/rconvert.py dballe/__init__.py dballe/volnd.py

pyexec_LTLIBRARIES = dballe/_dballe.la

dballe__dballe_la_SOURCES = \
    common.cc \
    vartable.cc \
    varinfo.cc \
    var.cc \
    record.cc \
    dballe.cc \
    db.cc \
    cursor.cc
dballe__dballe_la_CPPFLAGS = $(PYTHON_CFLAGS)
dballe__dballe_la_LDFLAGS = -module -avoid-version -export-symbols-regex init_dballe
dballe__dballe_la_LIBADD = ../dballe/libdballe.la

doc_DATA = python-dballe.rst

if HAVE_RST2HTML
doc_DATA += python-dballe.html
endif

dballe/_dballe.so: dballe/_dballe.la
	ln -fs .libs/_dballe.so $@

python-dballe.rst: doc-dballe $(pkgpython_PYTHON) dballe/_dballe.so
	$(PYTHON) $(srcdir)/doc-dballe > $@ || (rm -f $@; false)

python-dballe.html: python-dballe.rst
	rst2html $< > $@ || (rm -f $@; false)

clean-local:
	rm -f *.pyc dballe/*.pyc dballe/_dballe.so

EXTRA_DIST = \
    doc-dballe \
    common.h \
    vartable.h \
    varinfo.h \
    var.h \
    record.h \
    db.h \
    cursor.h \
    $(dballe_python_tests)

AM_CPPFLAGS = -I/usr/include/python$(PYTHON_VERSION) -I$(top_srcdir)

check-local: dballe/_dballe.so
	for test in $(dballe_python_tests); do \
		PYTHON="$(PYTHON)" $(TESTS_ENVIRONMENT) $$test ; \
	done

CLEANFILES = python-dballe.rst python-dballe.html
