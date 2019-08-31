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

dist_noinst_HEADERS = handles.h error.h

nodist_include_HEADERS = dballef.mod

lib_LTLIBRARIES = libdballef.la

libdballef_la_SOURCES = \
	error.cc \
	binding.cc \
	dballef.f90
libdballef_la_LIBADD = ../dballe/libdballe.la
libdballef_la_LDFLAGS = -version-info @LIBDBALLEF_VERSION_INFO@

dballef.mod: libdballef.la

#
# Unit testing
#

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

#TESTS = $(check_PROGRAMS)
dbtestlib = test.f90 dbtest.f90

check_PROGRAMS = check_real0 check_range check_fdballe check_fdballe_oldapi check_attrs check_set check_missing check_missing_msg check_segfault1 check_multiplehandler check_spiegab check_messages check_transactions1 check_connect_wipe

check_real0_SOURCES = $(dbtestlib) check_real0.f90
check_real0_DEPENDENCIES = dballef.mod
check_real0_LDADD = libdballef.la
check_real0_FCFLAGS = -g

check_range_SOURCES = $(dbtestlib) check_range.f90
check_range_DEPENDENCIES = dballef.mod
check_range_LDADD = libdballef.la
check_range_FCFLAGS = -g

check_fdballe_SOURCES = $(dbtestlib) check_fdballe.f90
check_fdballe_DEPENDENCIES = dballef.mod
check_fdballe_LDADD = libdballef.la
check_fdballe_FCFLAGS = -g

check_fdballe_oldapi_SOURCES = $(dbtestlib) check_fdballe_oldapi.f90
check_fdballe_oldapi_DEPENDENCIES = dballef.mod
check_fdballe_oldapi_LDADD = libdballef.la
check_fdballe_oldapi_FCFLAGS = -g

check_attrs_SOURCES = $(dbtestlib) check_attrs.f90
check_attrs_DEPENDENCIES = dballef.mod
check_attrs_LDADD = libdballef.la
check_attrs_FCFLAGS = -g

check_set_SOURCES = $(dbtestlib) check_set.f90
check_set_DEPENDENCIES = dballef.mod
check_set_LDADD = libdballef.la
check_set_FCFLAGS = -g

check_missing_SOURCES = $(dbtestlib) check_missing.f90
check_missing_DEPENDENCIES = dballef.mod
check_missing_LDADD = libdballef.la
check_missing_FCFLAGS = -g

check_missing_msg_SOURCES = test.f90 check_missing_msg.f90
check_missing_msg_DEPENDENCIES = dballef.mod
check_missing_msg_LDADD = libdballef.la
check_missing_msg_FCFLAGS = -g

check_segfault1_SOURCES = $(dbtestlib) check_segfault1.f90
check_segfault1_DEPENDENCIES = dballef.mod
check_segfault1_LDADD = libdballef.la
check_segfault1_FCFLAGS = -g

check_spiegab_SOURCES = $(dbtestlib) check_spiegab.f90
check_spiegab_DEPENDENCIES = dballef.mod
check_spiegab_LDADD = libdballef.la
check_spiegab_FCFLAGS = -g

check_messages_SOURCES = $(dbtestlib) check_messages.f90
check_messages_DEPENDENCIES = dballef.mod
check_messages_LDADD = libdballef.la
check_messages_FCFLAGS = -g

check_multiplehandler_SOURCES = $(dbtestlib) check_multiplehandler.f90
check_multiplehandler_DEPENDENCIES = dballef.mod
check_multiplehandler_LDADD = libdballef.la
check_multiplehandler_FCFLAGS = -g

check_transactions1_SOURCES = $(dbtestlib) check_transactions1.f90
check_transactions1_DEPENDENCIES = dballef.mod
check_transactions1_LDADD = libdballef.la
check_transactions1_FCFLAGS = -g

check_connect_wipe_SOURCES = $(dbtestlib) check_connect_wipe.f90
check_connect_wipe_DEPENDENCIES = dballef.mod
check_connect_wipe_LDADD = libdballef.la
check_connect_wipe_FCFLAGS = -g


EXTRA_DIST = fortran.dox check-utils.h

CLEANFILES = *.mod

check-local:
	$(TESTS_ENVIRONMENT) -v $(check_PROGRAMS)

endif
