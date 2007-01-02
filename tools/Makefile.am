## Process this file with automake to produce Makefile.in

DBALLELIBS = ../dballe/core/libdballe-core.la ../dballe/bufrex/libdballe-bufrex.la ../dballe/msg/libdballe-msg.la ../dballe/db/libdballe-db.la

bin_PROGRAMS = dbatbl dbamsg dbadb

dbatbl_SOURCES = processor.c conversion.c dbatbl.c
dbatbl_LDFLAGS = $(DBALLELIBS) ../dballe/libdballe.la -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS) ../dballe/libdballe.la

dbamsg_SOURCES = processor.c conversion.c dbamsg.c
dbamsg_LDFLAGS = $(DBALLELIBS) ../dballe/libdballe.la -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS) ../dballe/libdballe.la

dbadb_SOURCES = processor.c conversion.c dbadb.c
dbadb_LDFLAGS = $(DBALLELIBS) ../dballe/libdballe.la -lpopt
dbadb_DEPENDENCIES = $(DBALLELIBS) ../dballe/libdballe.la

EXTRA_DIST = processor.h conversion.h
