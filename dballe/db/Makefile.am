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
		db.h \
		import.h \
		export.h

lib_LTLIBRARIES = libdballe-db.la
libdballe_db_la_SOURCES = \
		querybuf.c \
		internals.c \
		repinfo.c \
		pseudoana.c \
		context.c \
		data.c \
		attr.c \
		cursor.c \
		db.c \
		import.c \
		export.c
libdballe_db_la_LIBADD = \
		../core/libdballe-core.la \
		../bufrex/libdballe-bufrex.la \
		../msg/libdballe-msg.la
libdballe_db_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

EXTRA_DIST = db.dox
