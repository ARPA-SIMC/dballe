if DO_FORTRAN
lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	error.c \
	simple.c


libdballef_la_INCLUDES = @CHECK_CFLAGS@
libdballef_la_LIBADD = @CHECK_LIBS@ ../dballe/libdballe.la
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

INCLUDES = -I..

EXTRA_DIST = handles.h
 
endif
