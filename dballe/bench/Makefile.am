noinst_PROGRAMS=benchmark

benchmark_SOURCES= \
	benchmark.cc \
	bench/Benchmark.cc \
	bench/dba_var.cc \
	bench/dba_record.cc \
	bench/bufrex.cc \
	bench/db.cc \
	../tests/test-utils.cc
benchmark_LDADD = ../db/libdballe-db.la -lpopt

INCLUDES = -I..

EXTRA_DIST = bench/Benchmark.h
