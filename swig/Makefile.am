## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

TESTS = test.py test-csv.py
if DO_DBALLE_DB
TESTS += test-db.py test-volnd.py
endif

doc_DATA = python-dballe.rst

if HAVE_RST2HTML
doc_DATA += python-dballe.html
endif

pyexec_LTLIBRARIES = _dballe.la

nodist_pkgpython_PYTHON = dballe.py
pkgpython_PYTHON = dbacsv.py rconvert.py __init__.py
if DO_DBALLE_DB
pkgpython_PYTHON += volnd.py
endif

nodist__dballe_la_SOURCES = dballe_wrap_python.cc
_dballe_la_LDFLAGS = -module
_dballe_la_LIBADD = ../dballe/libdballe.la

# Workaround for swig 1.3 missing an include in its code
dballe_wrap_python.cc Dballe.py: dballe.i ../doc/dballe-doc.i
	swig -python -c++ -I$(top_srcdir) -I/usr/include -o $@ $<
	mv dballe_wrap_python.cc dballe_wrap_python.cc.tmp
	( echo "#include <cstddef>"; cat dballe_wrap_python.cc.tmp ) > dballe_wrap_python.cc
	rm dballe_wrap_python.cc.tmp

python-dballe.rst: doc-dballe $(pkgpython_PYTHON) dballe dballe.i _dballe.so
	$(srcdir)/doc-dballe > $@ || (rm -f $@; false)

python-dballe.html: python-dballe.rst
	rst2html $< > $@ || (rm -f $@; false)

_dballe.so: _dballe.la
	ln -fs .libs/_dballe.so $@

dballe:
	ln -s . $@
	for x in $(pkgpython_PYTHON); do test -f $$x || cp $(srcdir)/$$x .; done

clean-local:
	rm -f *.pyc

INCLUDES = -I/usr/include/python$(PYTHON_VERSION) -I$(top_srcdir)

EXTRA_DIST = README dballe.i test.py test-db.py test-volnd.py test-rconvert.py test-csv.py doc-dballe

CLEANFILES = dballe.py dballe.pyc dballe_wrap_python.cc dballe _dballe.so python-dballe.rst python-dballe.html
