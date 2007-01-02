## Process this file with automake to produce Makefile.in

DBALLELIBS = ../dballe/core/libdballe-core.la ../dballe/bufrex/libdballe-bufrex.la ../dballe/msg/libdballe-msg.la ../dballe/db/libdballe-db.la

bin_PROGRAMS = dbatbl dbamsg dbadb

dbatbl_SOURCES = cmdline.c processor.c conversion.c dbatbl.c
dbatbl_LDFLAGS = $(DBALLELIBS) -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = cmdline.c processor.c conversion.c dbamsg.c
dbamsg_LDFLAGS = $(DBALLELIBS) -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS)

dbadb_SOURCES = cmdline.c processor.c conversion.c dbadb.c
dbadb_LDFLAGS = $(DBALLELIBS) -lpopt
dbadb_DEPENDENCIES = $(DBALLELIBS)

EXTRA_DIST = cmdline.h processor.h conversion.h
