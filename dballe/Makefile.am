## Process this file with automake to produce Makefile.in

SUBDIRS = core msg io aof bufrex db . bench tests

CFLAGS += -DTABLE_DIR=\"$(tabledir)\"

dballeincludedir = $(includedir)/dballe

# Include the .cc files that contain template definitions
dballeinclude_HEADERS = \
		init.h \
		marshal.h \
		dba_file.h \
		formatter.h \
		cmdline.h

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
		init.c \
		marshal.c \
		dba_file.c \
		formatter.c \
		cmdline.c
libdballe_la_LIBADD = \
		core/libcore.la \
		io/libio.la \
		msg/libmsg.la \
		bufrex/libbufrex.la \
		aof/libaof.la \
		db/libdb.la
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

EXTRA_DIST = mainpage.dox dballe.dox
