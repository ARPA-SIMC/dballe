## Process this file with automake to produce Makefile.in

SUBDIRS = core

if DO_DBALLE_BUFREX
SUBDIRS += bufrex
endif
if DO_DBALLE_MSG
SUBDIRS += msg
endif
if DO_DBALLE_DB
SUBDIRS += db
endif

SUBDIRS += . bench

EXTRA_DIST = mainpage.dox dballe.dox \
	main-bufrex.dox \
	main-core.dox \
	main-db.dox \
	main-msg.dox

INCLUDES = -I$(top_srcdir)

dballeincludedir = $(includedir)/dballe

dballeinclude_HEADERS = \
	init.h

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = init.c
libdballe_la_LIBADD = \
		core/libdballe-core.la \
		bufrex/libdballe-bufrex.la \
		msg/libdballe-msg.la \
		db/libdballe-db.la
libdballe_la_LDFLAGS = -lstdc++ -version-info @LIBDBALLE_VERSION_INFO@
