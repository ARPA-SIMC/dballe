## Process this file with automake to produce Makefile.in

SUBDIRS = err conv core msg io aof bufrex db tests

CFLAGS += -DCONF_DIR=\"$(confdir)\"

dballeincludedir = $(includedir)/dballe

# Include the .cc files that contain template definitions
dballeinclude_HEADERS = \
		init.h \
		marshal.h \
		dba_file.h \
		cmdline.h

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
		init.c \
		marshal.c \
		dba_file.c \
		cmdline.c
libdballe_la_LIBADD = \
		err/liberr.la \
		conv/libconv.la \
		core/libcore.la \
		io/libio.la \
		msg/libmsg.la \
		bufrex/libbufrex.la \
		aof/libaof.la \
		db/libdb.la
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

EXTRA_DIST = mainpage.dox dballe.dox
