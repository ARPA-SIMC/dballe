if DO_DBALLEF

# libdballef

dballeincludedir = $(includedir)/dballe

# Include the F90 interface file as well
dist_dballeinclude_HEADERS = dballef.h

dist_noinst_HEADERS = simple.h commonapi.h dbapi.h msgapi.h handles.h

lib_LTLIBRARIES = libdballef.la

# missing.f90
libdballef_la_SOURCES = \
	error.cc \
	simple.cc \
	commonapi.cc \
	dbapi.cc \
	msgapi.cc \
	binding.cc
libdballef_la_LIBADD = ../dballe/libdballe.la -lcnf
libdballef_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@


# Tests

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

check_PROGRAMS = check_fdballe check_attrs check_missing check_real0 check_range
TESTS = $(check_PROGRAMS)
testlib = dbtest.f90

check_fdballe_SOURCES = $(testlib) check_fdballe.f90
check_fdballe_LDADD = ./libdballef.la -lcnf -lpopt
check_fdballe_FCFLAGS = -g

check_attrs_SOURCES = $(testlib) check_attrs.f90
check_attrs_LDADD = ./libdballef.la -lcnf -lpopt
check_attrs_FCFLAGS = -g

check_missing_SOURCES = $(testlib) check_missing.f90
check_missing_LDADD = ./libdballef.la -lcnf -lpopt
check_missing_FCFLAGS = -g

check_real0_SOURCES = $(testlib) check_real0.f90
check_real0_LDADD = ./libdballef.la -lcnf -lpopt
check_real0_FCFLAGS = -g

check_range_SOURCES = $(testlib) check_range.f90
check_range_LDADD = ./libdballef.la -lcnf -lpopt
check_range_FCFLAGS = -g

noinst_PROGRAMS = dump_dballe dumpmsg_dballe mkmsg

dump_dballe_SOURCES = dump_dballe.f
dump_dballe_LDADD = ../fortran/libdballef.la -lcnf -lpopt
dump_dballe_FCFLAGS = -g 

dumpmsg_dballe_SOURCES = dumpmsg_dballe.f
dumpmsg_dballe_LDADD = ../fortran/libdballef.la -lcnf -lpopt
dumpmsg_dballe_FCFLAGS = -g 

mkmsg_SOURCES = mkmsg.f
mkmsg_LDADD = ../fortran/libdballef.la -lcnf -lpopt
mkmsg_FCFLAGS = -g 

# Normal bureaucracy :)

INCLUDES = -I$(top_srcdir)

EXTRA_DIST = fortran.dox check-utils.h

endif
