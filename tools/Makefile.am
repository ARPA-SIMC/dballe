## Process this file with automake to produce Makefile.in

DBALLELIBS =  ../dballe/libdballe.la

INCLUDES = -I$(top_srcdir)

bin_PROGRAMS = dbatbl dbamsg dbadb
dist_bin_SCRIPTS = dbatbl_makeb

dbatbl_SOURCES = dbatbl.cc
dbatbl_LDFLAGS = $(DBALLELIBS) -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = dbamsg.cc
dbamsg_LDFLAGS = $(DBALLELIBS) -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS)

dbadb_SOURCES = dbadb.cc
dbadb_LDFLAGS = $(DBALLELIBS) -lpopt
dbadb_DEPENDENCIES = $(DBALLELIBS)
