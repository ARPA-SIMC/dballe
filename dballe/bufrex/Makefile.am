## Process this file with automake to produce Makefile.in

bufrexincludedir = $(includedir)/dballe/bufrex

# Include the .cc files that contain template definitions
bufrexinclude_HEADERS = \
		dtable.h \
		opcode.h \
		subset.h \
		msg.h

lib_LTLIBRARIES = libdballe-bufrex.la
libdballe_bufrex_la_SOURCES = \
		dtable.c \
		opcode.c \
		subset.c \
		msg.c \
		bufr_decoder.c \
		bufr_encoder.c \
		crex_decoder.c \
		crex_encoder.c
libdballe_bufrex_la_LIBADD = \
		../core/libdballe-core.la
libdballe_bufrex_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

EXTRA_DIST = bufrex.dox
