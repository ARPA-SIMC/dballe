## Process this file with automake to produce Makefile.in

SUBDIRS = . exporters

dbincludedir = $(includedir)/dballe/db

# Include the .cc files that contain template definitions
dbinclude_HEADERS = \
		dba_import.h \
		dba_export.h \
		dballe.h

noinst_LTLIBRARIES = libdb.la
libdb_la_SOURCES = \
		dba_import.c \
		dba_export.c \
		dballe.c

INCLUDES = -DCONF_DIR=\"$(confdir)\"

EXTRA_DIST = db.dox
