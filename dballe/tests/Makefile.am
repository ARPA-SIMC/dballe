DBALLELIBS = ../core/libdballe-core.la ../bufrex/libdballe-bufrex.la ../msg/libdballe-msg.la ../db/libdballe-db.la

check_PROGRAMS = check_dballe_tut
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

#noinst_PROGRAMS = test-query
#check_mem
#check_imports

#EXTRA_SCRIPTS = check_imports

check_dballe_tut_SOURCES = \
	tests/dba_db_repinfo.cc \
	tests/dba_db_dballe.cc \
	tests/dba_db_import.cc \
	$(top_srcdir)/extra/test-utils-core.cc \
	$(top_srcdir)/extra/test-utils-bufrex.cc \
	$(top_srcdir)/extra/test-utils.cc \
	$(top_srcdir)/extra/tut-main.cc
check_dballe_tut_LDADD = $(DBALLELIBS) ../db/libdballe-db.la -lpopt

#test_query_SOURCES = test-query.c
#test_query_LDADD = ../db/libdballe-db.la -lpopt

#check_mem_SOURCES=\
#	check_mem.f
#check_mem_LDADD = ../fortran/libdballef.la ../dballe/libdballe.la -lcnf -lpopt
#check_mem_FFLAGS = -g -fno-second-underscore

INCLUDES = -I..

EXTRA_DIST = test-utils.h tut.h tut_reporter.h tut_restartable.h
