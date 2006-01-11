check_PROGRAMS = check_dballe_tut check_fdballe check_mem
TESTS = $(check_PROGRAMS)
#check_imports

#EXTRA_SCRIPTS = check_imports

export DBA_REPINFO=../tables/repinfo.csv
export DBA_TABLES=../tables

noinst_PROGRAMS=stress benchmark

check_dballe_tut_SOURCES = \
	tests/dba_err_error.cc \
	tests/dba_core_btable.cc \
	tests/dba_core_var.cc \
	tests/dba_core_record.cc \
	tests/dba_msg.cc \
	tests/dba_io_rawmsg.cc \
	tests/dba_io_rawfile.cc \
	tests/dba_db_dballe.cc \
	tests/dba_bufrex_opcode.cc \
	tests/dba_bufrex_dtable.cc \
	tests/dba_bufrex_conv.cc \
	tests/dba_bufrex_bufr_decoder.cc \
	tests/dba_bufrex_crex_decoder.cc \
	tests/dba_aof_decoder.cc \
	tests/dba_db_import.cc \
	test-utils.cc \
	tut-main.cc
check_dballe_tut_LDADD = ../dballe/libdballe.la

check_fdballe_SOURCES=\
	check_fdballe.f
check_fdballe_LDADD = ../fortran/libdballef.la ../dballe/libdballe.la -lcnf
check_fdballe_FFLAGS = -g -fno-second-underscore

check_mem_SOURCES=\
	check_mem.f
check_mem_LDADD = ../fortran/libdballef.la ../dballe/libdballe.la -lcnf
check_mem_FFLAGS = -g -fno-second-underscore

stress_SOURCES=\
	stress.c
stress_LDADD = ../dballe/libdballe.la

benchmark_SOURCES= \
	benchmark.cc \
	bench/Benchmark.cc \
	bench/dba_var.cc \
	bench/dba_record.cc \
	bench/bufrex.cc
benchmark_LDADD = ../dballe/libdballe.la

INCLUDES = -I..

EXTRA_DIST = bench/Benchmark.h test-utils.h tut.h tut_reporter.h tut_restartable.h
