## Process this file with automake to produce Makefile.in

DBALLELIBS = ../dballe/core/libdballe-core.la ../dballe/bufrex/libdballe-bufrex.la ../dballe/msg/libdballe-msg.la ../dballe/db/libdballe-db.la

EXTRAS = $(top_srcdir)/extra/cmdline.c $(top_srcdir)/extra/processor.c $(top_srcdir)/extra/conversion.c

bin_PROGRAMS = dbatbl dbamsg dbadb

dbatbl_SOURCES = $(EXTRAS) dbatbl.c
dbatbl_LDFLAGS = $(DBALLELIBS) -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = $(EXTRAS) dbamsg.c
dbamsg_LDFLAGS = $(DBALLELIBS) -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS)

dbadb_SOURCES = $(EXTRAS) dbadb.c
dbadb_LDFLAGS = $(DBALLELIBS) -lpopt
dbadb_DEPENDENCIES = $(DBALLELIBS)
