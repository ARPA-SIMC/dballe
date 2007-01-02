## Process this file with automake to produce Makefile.in

SUBDIRS = core bufrex msg db . bench tests

CFLAGS += -DTABLE_DIR=\"$(tabledir)\"

dballeincludedir = $(includedir)/dballe

# Include the .cc files that contain template definitions
dballeinclude_HEADERS = \
		cmdline.h

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
		cmdline.c
libdballe_la_LIBADD = \
		core/libdballe-core.la \
		bufrex/libdballe-bufrex.la \
		msg/libdballe-msg.la \
		db/libdballe-db.la
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

EXTRA_DIST = mainpage.dox dballe.dox
