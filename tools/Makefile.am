## Process this file with automake to produce Makefile.in

DBALLELIBS = ../extra/libextra.la  ../dballe/db/libdballe-db.la ../dballe/msg/libdballe-msg.la ../dballe/bufrex/libdballe-bufrex.la ../dballe/core/libdballe-core.la 

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
