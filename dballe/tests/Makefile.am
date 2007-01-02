check_PROGRAMS = check_dballe_tut
TESTS = $(check_PROGRAMS)

noinst_PROGRAMS = test-query
#check_mem
#check_imports

#EXTRA_SCRIPTS = check_imports

export DBA_REPINFO=../../tables/repinfo.csv
export DBA_TABLES=../../tables

check_dballe_tut_SOURCES = \
	tests/dba_core_error.cc \
	tests/dba_core.cc \
	tests/dba_core_aliases.cc \
	tests/dba_core_btable.cc \
	tests/dba_core_var.cc \
	tests/dba_core_record.cc \
	tests/dba_msg.cc \
	tests/dba_io_rawmsg.cc \
	tests/dba_io_rawfile.cc \
	tests/dba_db_repinfo.cc \
	tests/dba_db_dballe.cc \
	tests/dba_bufrex_opcode.cc \
	tests/dba_bufrex_dtable.cc \
	tests/dba_bufrex_conv.cc \
	tests/dba_bufrex_generic.cc \
	tests/dba_bufrex_bufr_decoder.cc \
	tests/dba_bufrex_crex_decoder.cc \
	tests/dba_aof_decoder.cc \
	tests/dba_db_import.cc \
	tests/dba_formatter.cc \
	test-utils.cc \
	tut-main.cc
check_dballe_tut_LDADD = ../libdballe.la -lpopt

test_query_SOURCES = test-query.c
test_query_LDADD = ../libdballe.la -lpopt

#check_mem_SOURCES=\
#	check_mem.f
#check_mem_LDADD = ../fortran/libdballef.la ../dballe/libdballe.la -lcnf -lpopt
#check_mem_FFLAGS = -g -fno-second-underscore

INCLUDES = -I..

EXTRA_DIST = test-utils.h tut.h tut_reporter.h tut_restartable.h
