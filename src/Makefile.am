## Process this file with automake to produce Makefile.in

DBALLELIBS =  ../dballe/libdballe.la

AM_CPPFLAGS = -I$(top_srcdir) $(WREPORT_CFLAGS) -Werror
if FILE_OFFSET_BITS_64
AM_CPPFLAGS += -D_FILE_OFFSET_BITS=64
endif

bin_PROGRAMS = dbatbl dbamsg
dist_bin_SCRIPTS = dbatbl_makeb dbaexport

dbatbl_SOURCES = dbatbl.cc
dbatbl_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) $(POPT_LIBS)
dbatbl_DEPENDENCIES = $(DBALLELIBS)

dbamsg_SOURCES = dbamsg.cc
dbamsg_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) $(POPT_LIBS)
dbamsg_DEPENDENCIES = $(DBALLELIBS)

bin_PROGRAMS += dbadb
dbadb_SOURCES = dbadb.cc
dbadb_LDFLAGS = $(DBALLELIBS) $(WREPORT_LIBS) $(POPT_LIBS)
dbadb_DEPENDENCIES = $(DBALLELIBS)

