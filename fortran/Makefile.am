if DO_FORTRAN
lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	error.c \
	simple.c


libdballef_la_LIBADD = ../dballe/libdballe.la
libdballef_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

INCLUDES = -I..

EXTRA_DIST = handles.h
 
endif
