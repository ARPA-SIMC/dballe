## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

dballe_python_tests = test-core.py test-types.py test-record.py test-db.py test-explorer.py test-volnd.py test-csv.py

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

python-dballe.rst: doc-dballe $(pkgpython_PYTHON) $(pyexec_LTLIBRARIES)
	PYTHONPATH=.libs libtool --mode=execute $(PYTHON) $(srcdir)/doc-dballe > $@ || (rm -f $@; false)

python-dballe.html: python-dballe.rst
	rst2html $< > $@ || (rm -f $@; false)

clean-local:
	rm -f *.pyc dballe/*.pyc
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

if HAVE_PYTHON3
check-local:
	for test in $(dballe_python_tests); do \
		test -n "$$ARGS" -a "$${ARGS#$${test%.py}}" == "$$ARGS" && echo "Skipping $$test" && continue; \
		PYTHON="$(PYTHON)" $(TESTS_ENVIRONMENT) $$test ; \
	done
endif

CLEANFILES = python-dballe.rst python-dballe.html
