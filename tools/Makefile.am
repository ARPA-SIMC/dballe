## Process this file with automake to produce Makefile.in

bin_PROGRAMS = dbatbl dbamsg dbadb

dbatbl_SOURCES = processor.c conversion.c dbatbl.c
dbatbl_LDFLAGS = ../dballe/libdballe.la -lpopt
dbatbl_DEPENDENCIES = ../dballe/libdballe.la

dbamsg_SOURCES = processor.c conversion.c dbamsg.c
dbamsg_LDFLAGS = ../dballe/libdballe.la -lpopt
dbamsg_DEPENDENCIES = ../dballe/libdballe.la

dbadb_SOURCES = processor.c conversion.c dbadb.c
dbadb_LDFLAGS = ../dballe/libdballe.la -lpopt
dbadb_DEPENDENCIES = ../dballe/libdballe.la

EXTRA_DIST = processor.h conversion.h
