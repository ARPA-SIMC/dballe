## Process this file with automake to produce Makefile.in

SUBDIRS = core msg

if DO_DBALLE_DB
SUBDIRS += db
endif

SUBDIRS += cmdline

# SUBDIRS += . bench

EXTRA_DIST = mainpage.dox dballe.dox \
	main-core.dox \
	main-db.dox \
	main-msg.dox

INCLUDES = -I$(top_srcdir)

dballeincludedir = $(includedir)/dballe

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = 
libdballe_la_LIBADD = \
		core/libdballe-core.la \
		msg/libdballe-msg.la \
		db/libdballe-db.la \
		cmdline/libdballe-cmdline.la
libdballe_la_LDFLAGS = -lstdc++ -version-info @LIBDBALLE_VERSION_INFO@
