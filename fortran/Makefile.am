if DO_FORTRAN

# libdballef

lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	error.c \
	simple.c


libdballef_la_LIBADD = ../dballe/libdballe.la
libdballef_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@


# Tests

export DBA_REPINFO=../tables/repinfo.csv
export DBA_TABLES=../tables

check_PROGRAMS = check_fdballe
TESTS = $(check_PROGRAMS)

check_fdballe_SOURCES = check_fdballe.f
check_fdballe_LDADD = ./libdballef.la ../dballe/libdballe.la -lcnf -lpopt
check_fdballe_FFLAGS = -g -fno-second-underscore

noinst_PROGRAMS=dump_dballe

dump_dballe_SOURCES=\
dump   dump_dballe.f
dump_dballe_LDADD = ../fortran/libdballef.la ../dballe/libdballe.la -lcnf -lpopt
dump_dballe_FFLAGS = -g -fno-second-underscore

# Normal bureaucracy :)

INCLUDES = -I..

EXTRA_DIST = handles.h
 
endif
