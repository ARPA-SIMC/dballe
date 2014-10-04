## Process this file with automake to produce Makefile.in

DBALLELIBS =  ../dballe/libdballe.la

AM_CPPFLAGS = -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)

bin_PROGRAMS = dbatbl dbamsg
dist_bin_SCRIPTS = dbatbl_makeb dbaexport

dbatbl_SOURCES = dbatbl.cc
dbatbl_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = dbamsg.cc
dbamsg_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS)

bin_PROGRAMS += dbadb
dbadb_SOURCES = dbadb.cc
dbadb_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt 
dbadb_DEPENDENCIES = $(DBALLELIBS)

if DO_BENCHMARKS
noinst_PROGRAMS = benchmark
benchmark_SOURCES = benchmark.cc bench/bench.cc
benchmark_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt
benchmark_DEPENDENCIES = $(DBALLELIBS)
benchmark_CPPFLAGS = -DBENCHDIR='"$(top_srcdir)/extra/bench/"'
endif

EXTRA_DIST = bench/bench.h \
             bench/3928-viaza-20130624-180728.csv \
             bench/3932-viaza-20130625-105958.csv

