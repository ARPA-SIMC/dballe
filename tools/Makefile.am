## Process this file with automake to produce Makefile.in

DBALLELIBS = ../extra/libextra.la  ../dballe/libdballe.la

INCLUDES = -I$(top_srcdir)

bin_PROGRAMS = dbatbl dbamsg dbadb

dbatbl_SOURCES = dbatbl.c
dbatbl_LDFLAGS = $(DBALLELIBS) -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = dbamsg.c
dbamsg_LDFLAGS = $(DBALLELIBS) -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS)

dbadb_SOURCES = dbadb.c
dbadb_LDFLAGS = $(DBALLELIBS) -lpopt
dbadb_DEPENDENCIES = $(DBALLELIBS)
