## Process this file with automake to produce Makefile.in

DBALLELIBS =  ../dballe/libdballe.la

INCLUDES = -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)

bin_PROGRAMS = dbatbl dbamsg
dist_bin_SCRIPTS = dbatbl_makeb

dbatbl_SOURCES = dbatbl.cc
dbatbl_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = dbamsg.cc
dbamsg_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt
dbamsg_DEPENDENCIES = $(DBALLELIBS)

if DO_DBALLE_DB
bin_PROGRAMS += dbadb
dbadb_SOURCES = dbadb.cc
dbadb_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) -lpopt 
dbadb_DEPENDENCIES = $(DBALLELIBS)
endif
