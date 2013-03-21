## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

export PYTHONPATH=.

TESTS = test.py
#pkgpython_PYTHON = dbacsv.py rconvert.py __init__.py

pyexec_LTLIBRARIES = dballe.la

nodist_dballe_la_SOURCES = vartable.cc var.cc record.cc dballe.cc
dballe_la_CPPFLAGS = $(PYTHON_CFLAGS)
dballe_la_LDFLAGS = -module -avoid-version -export-symbols-regex initdballe
dballe_la_LIBADD = ../dballe/libdballe.la

#_dballe.so: _dballe.la
#	ln -fs .libs/_dballe.so $@

clean-local:
	rm -f *.pyc

EXTRA_DIST = vartable.h var.h record.h

INCLUDES = -I/usr/include/python$(PYTHON_VERSION) -I$(top_srcdir)
