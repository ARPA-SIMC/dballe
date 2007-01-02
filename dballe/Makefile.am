## Process this file with automake to produce Makefile.in

SUBDIRS = core msg bufrex db . bench tests

CFLAGS += -DTABLE_DIR=\"$(tabledir)\"

dballeincludedir = $(includedir)/dballe

# Include the .cc files that contain template definitions
dballeinclude_HEADERS = \
		init.h \
		formatter.h \
		cmdline.h

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
		init.c \
		formatter.c \
		cmdline.c
libdballe_la_LIBADD = \
		core/libdballe-core.la \
		bufrex/libbufrex.la \
		msg/libmsg.la \
		msg/bufrex/libbufrex.la \
		msg/aof/libaof.la \
		db/libdb.la
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

EXTRA_DIST = mainpage.dox dballe.dox
