## Process this file with automake to produce Makefile.in

DBALLELIBS =  ../dballe/libdballe.la

AM_CPPFLAGS = -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)
if FILE_OFFSET_BITS_64
AM_CPPFLAGS += -D_FILE_OFFSET_BITS=64
endif

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

