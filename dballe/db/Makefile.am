## Process this file with automake to produce Makefile.in

SUBDIRS = .

dbincludedir = $(includedir)/dballe/db

# Include the .cc files that contain template definitions
dbinclude_HEADERS = \
		querybuf.h \
		internals.h \
		pseudoana.h \
		dballe.h \
		dba_import.h \
		dba_export.h

noinst_LTLIBRARIES = libdb.la
libdb_la_SOURCES = \
		querybuf.c \
		internals.c \
		pseudoana.c \
		dballe.c \
		dba_import.c \
		dba_export.c

INCLUDES = -DCONF_DIR=\"$(confdir)\"

EXTRA_DIST = db.dox
