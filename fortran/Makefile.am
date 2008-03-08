#if DO_FORTRAN
#
## libdballef
#
#dballeincludedir = $(includedir)/dballe
#
## Include the F90 interface file as well
#dballeinclude_HEADERS = dballef.h
#
#lib_LTLIBRARIES = libdballef.la
#
## missing.f90
#libdballef_la_SOURCES = \
#	error.c \
#	simple.c
#libdballef_la_LIBADD = ../dballe/db/libdballe-db.la -lcnf
#libdballef_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@
#
#
## Tests
#
#TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
#
#check_PROGRAMS = check_fdballe check_attrs check_missing check_real0
#TESTS =
#if DB_TESTS
#TESTS += $(check_PROGRAMS)
#endif
#
#check_fdballe_SOURCES = check_fdballe.f90
#check_fdballe_LDADD = ./libdballef.la -lcnf -lpopt
#check_fdballe_FCFLAGS = -g
#
#check_attrs_SOURCES = check_attrs.f90
#check_attrs_LDADD = ./libdballef.la -lcnf -lpopt
#check_attrs_FCFLAGS = -g
#
#check_missing_SOURCES = check_missing.f90
#check_missing_LDADD = ./libdballef.la -lcnf -lpopt
#check_missing_FCFLAGS = -g
#
#check_real0_SOURCES = check_real0.f90
#check_real0_LDADD = ./libdballef.la -lcnf -lpopt
#check_real0_FCFLAGS = -g
#
#noinst_PROGRAMS = dump_dballe
#
#dump_dballe_SOURCES = dump_dballe.f
#dump_dballe_LDADD = ../fortran/libdballef.la -lcnf -lpopt
#dump_dballe_FCFLAGS = -g 
#
## Normal bureaucracy :)
#
#INCLUDES = -I..
#
#EXTRA_DIST = handles.h dballef.h fortran.dox check-utils.h
# 
#endif
