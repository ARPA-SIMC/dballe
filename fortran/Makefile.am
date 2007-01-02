if DO_FORTRAN

# libdballef

dballeincludedir = $(includedir)/dballe

# Include the F90 interface file as well
dballeinclude_HEADERS = dballef.h

lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	error.c \
	simple.c
libdballef_la_LIBADD = ../dballe/db/libdballe-db.la -lcnf
libdballef_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@


# Tests

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

check_PROGRAMS = check_fdballe
TESTS =
if DB_TESTS
TESTS += $(check_PROGRAMS)
endif

check_fdballe_SOURCES = check_fdballe.f
check_fdballe_LDADD = ./libdballef.la -lcnf -lpopt
check_fdballe_FFLAGS = -g -fno-second-underscore

noinst_PROGRAMS = dump_dballe

dump_dballe_SOURCES = dump_dballe.f
dump_dballe_LDADD = ../fortran/libdballef.la -lcnf -lpopt
dump_dballe_FFLAGS = -g -fno-second-underscore

# Normal bureaucracy :)

INCLUDES = -I..

EXTRA_DIST = handles.h dballef.h fortran.dox
 
endif
