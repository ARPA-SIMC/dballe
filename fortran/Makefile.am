if DO_FORTRAN
lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	fdba_error.c \
	fdba_simple.c


libdballef_la_INCLUDES = @CHECK_CFLAGS@
libdballef_la_LIBADD = @CHECK_LIBS@ ../dballe/libdballe.la

INCLUDES = -I..

EXTRA_DIST = fdba_handles.h
 
endif
