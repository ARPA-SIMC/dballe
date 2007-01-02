noinst_PROGRAMS=benchmark

benchmark_SOURCES= \
	benchmark.cc \
	bench/Benchmark.cc \
	bench/dba_var.cc \
	bench/dba_record.cc \
	bench/bufrex.cc \
	bench/db.cc \
	$(top_srcdir)/extra/test-utils-core.cc \
	$(top_srcdir)/extra/test-utils.cc
benchmark_LDADD = ../db/libdballe-db.la -lpopt

INCLUDES = -I$(top_srcdir)

EXTRA_DIST = bench/Benchmark.h
