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
libdballef_la_LDFLAGS = -version-info @LIBDBALLEF_VERSION_INFO@


#
# Unit testing
#

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

check_PROGRAMS = check_fdballe check_attrs check_missing check_missing_msg check_real0 check_range tut_test
TESTS = $(check_PROGRAMS)
testlib = dbtest.f90

CXXFLAGS += -O0

tut_test_SOURCES = \
	msgapi-tut.cc \
	../dballe/core/test-utils-core.cc \
	../dballe/core/tests/tut-main.cpp
tut_test_LDADD = \
	libdballef.la \
	../dballe/libdballe.la \
	$(WREPORT_LIBS) \
	$(WIBBLE_LIBS)

check_fdballe_SOURCES = $(testlib) check_fdballe.f90
check_fdballe_LDADD = ./libdballef.la -lcnf -lpopt
check_fdballe_FCFLAGS = -g

check_attrs_SOURCES = $(testlib) check_attrs.f90
check_attrs_LDADD = ./libdballef.la -lcnf -lpopt
check_attrs_FCFLAGS = -g

check_missing_SOURCES = $(testlib) check_missing.f90
check_missing_LDADD = ./libdballef.la -lcnf -lpopt
check_missing_FCFLAGS = -g

check_missing_msg_SOURCES = $(testlib) check_missing_msg.f90
check_missing_msg_LDADD = ./libdballef.la -lcnf -lpopt
check_missing_msg_FCFLAGS = -g

check_real0_SOURCES = $(testlib) check_real0.f90
check_real0_LDADD = ./libdballef.la -lcnf -lpopt
check_real0_FCFLAGS = -g

check_range_SOURCES = $(testlib) check_range.f90
check_range_LDADD = ./libdballef.la -lcnf -lpopt
check_range_FCFLAGS = -g

noinst_PROGRAMS = dump_dballe dumpmsg_dballe mkmsg mkmsg-sca genattr

dump_dballe_SOURCES = dump_dballe.f90
dump_dballe_LDADD = ../fortran/libdballef.la -lcnf -lpopt
dump_dballe_FCFLAGS = -g 

dumpmsg_dballe_SOURCES = dumpmsg_dballe.f90
dumpmsg_dballe_LDADD = ../fortran/libdballef.la -lcnf -lpopt
dumpmsg_dballe_FCFLAGS = -g 

mkmsg_SOURCES = mkmsg.f90
mkmsg_LDADD = ../fortran/libdballef.la -lcnf -lpopt
mkmsg_FCFLAGS = -g 

mkmsg_sca_SOURCES = mkmsg-sca.f90
mkmsg_sca_LDADD = ../fortran/libdballef.la -lcnf -lpopt
mkmsg_sca_FCFLAGS = -g 

genattr_SOURCES = genattr.f90
genattr_LDADD = ../fortran/libdballef.la -lcnf -lpopt
genattr_FCFLAGS = -g 

# Normal bureaucracy :)

INCLUDES = -I$(top_srcdir)

EXTRA_DIST = fortran.dox check-utils.h error.h

endif
