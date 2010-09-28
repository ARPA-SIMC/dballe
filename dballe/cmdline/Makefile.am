## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)

#
# Shared library
#

cmdlineincludedir = $(includedir)/dballe/cmdline

cmdlineinclude_HEADERS = \
	cmdline.h \
	conversion.h \
	processor.h

noinst_LTLIBRARIES = libdballe-cmdline.la
libdballe_cmdline_la_SOURCES = \
	cmdline.cc \
	processor.cc \
	conversion.cc
libdballe_cmdline_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(WREPORT_LIBS) $(LUA_LIBS)
