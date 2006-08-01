## Process this file with automake to produce Makefile.in

SUBDIRS = .

dbincludedir = $(includedir)/dballe/db

# Include the .cc files that contain template definitions
dbinclude_HEADERS = \
		querybuf.h \
		internals.h \
		repinfo.h \
		pseudoana.h \
		context.h \
		data.h \
		attr.h \
		cursor.h \
		dba_db.h \
		import.h \
		export.h

noinst_LTLIBRARIES = libdb.la
libdb_la_SOURCES = \
		querybuf.c \
		internals.c \
		repinfo.c \
		pseudoana.c \
		context.c \
		data.c \
		attr.c \
		cursor.c \
		dba_db.c \
		import.c \
		export.c

INCLUDES = -DCONF_DIR=\"$(confdir)\"

EXTRA_DIST = db.dox
