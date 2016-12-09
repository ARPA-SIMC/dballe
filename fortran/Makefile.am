if DO_DBALLEF

# Do not parallel-build this Makefile.
# Currently Automake does not mkae the .o sources built from .f90 sources
# depend on .mod files, so builds of other sources fail on parallel builds
# until libdballef.la is ready
.NOTPARALLEL:

AM_CPPFLAGS = -I$(top_srcdir) $(WREPORT_CFLAGS)
if FILE_OFFSET_BITS_64
AM_CPPFLAGS += -D_FILE_OFFSET_BITS=64
endif

# libdballef

dballeincludedir = $(includedir)/dballe

# Include the F90 interface file as well
dist_dballeinclude_HEADERS = dballef.h dballeff.h

dist_noinst_HEADERS = common.h handles.h error.h trace.h

nodist_include_HEADERS = dballef.mod

lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	common.cc \
	error.cc \
	trace.cc \
	binding.cc \
	dballef.f90
libdballef_la_LIBADD = ../dballe/libdballe.la
libdballef_la_LDFLAGS = -version-info @LIBDBALLEF_VERSION_INFO@

dballef.mod: libdballef.la

#
# Unit testing
#

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

check_PROGRAMS = check_missing_msg

#TESTS = $(check_PROGRAMS)
dbtestlib = test.f90 dbtest.f90 dballef.mod

CXXFLAGS += -O0

check_missing_msg_SOURCES = test.f90 check_missing_msg.f90
check_missing_msg_LDADD = libdballef.la -lpopt
check_missing_msg_FCFLAGS = -g

check_PROGRAMS += check_real0 check_range check_fdballe check_attrs check_missing check_segfault1 check_multiplehandler check_spiegab check_messages

check_real0_SOURCES = $(dbtestlib) check_real0.f90
check_real0_LDADD = libdballef.la -lpopt
check_real0_FCFLAGS = -g

check_range_SOURCES = $(dbtestlib) check_range.f90
check_range_LDADD = libdballef.la -lpopt
check_range_FCFLAGS = -g

check_fdballe_SOURCES = $(dbtestlib) check_fdballe.f90
check_fdballe_LDADD = libdballef.la -lpopt
check_fdballe_FCFLAGS = -g

check_attrs_SOURCES = $(dbtestlib) check_attrs.f90
check_attrs_LDADD = libdballef.la -lpopt
check_attrs_FCFLAGS = -g

check_missing_SOURCES = $(dbtestlib) check_missing.f90
check_missing_LDADD = libdballef.la -lpopt
check_missing_FCFLAGS = -g

check_segfault1_SOURCES = $(dbtestlib) check_segfault1.f90
check_segfault1_LDADD = libdballef.la -lpopt
check_segfault1_FCFLAGS = -g

check_spiegab_SOURCES = $(dbtestlib) check_spiegab.f90
check_spiegab_LDADD = libdballef.la -lpopt
check_spiegab_FCFLAGS = -g

check_messages_SOURCES = $(dbtestlib) check_messages.f90
check_messages_LDADD = libdballef.la -lpopt
check_messages_FCFLAGS = -g

check_messages_stdinstdout_SOURCES = $(dbtestlib) check_messages_stdinstdout.f90
check_messages_stdinstdout_LDADD = libdballef.la -lpopt
check_messages_stdinstdout_FCFLAGS = -g

check_multiplehandler_SOURCES = $(dbtestlib) check_multiplehandler.f90
check_multiplehandler_LDADD = libdballef.la -lpopt
check_multiplehandler_FCFLAGS = -g


noinst_PROGRAMS = dumpmsg_dballe mkmsg mkmsg-sca genattr

dumpmsg_dballe_SOURCES = dumpmsg_dballe.f90 dballef.mod
dumpmsg_dballe_LDADD = libdballef.la -lpopt
dumpmsg_dballe_FCFLAGS = -g 

mkmsg_SOURCES = mkmsg.f90 dballef.mod
mkmsg_LDADD = libdballef.la -lpopt
mkmsg_FCFLAGS = -g 

mkmsg_sca_SOURCES = mkmsg-sca.f90 dballef.mod
mkmsg_sca_LDADD = libdballef.la -lpopt
mkmsg_sca_FCFLAGS = -g 

genattr_SOURCES = genattr.f90 dballef.mod
genattr_LDADD = libdballef.la -lpopt
genattr_FCFLAGS = -g 

noinst_PROGRAMS += dump_dballe check_messages_stdinstdout

dump_dballe_SOURCES = dump_dballe.f90 dballef.mod
dump_dballe_LDADD = libdballef.la -lpopt
dump_dballe_FCFLAGS = -g 

EXTRA_DIST = fortran.dox check-utils.h

CLEANFILES = *.mod

check-local:
	for test in $(check_PROGRAMS); do \
		$(TESTS_ENVIRONMENT) $$test ; \
	done

endif
