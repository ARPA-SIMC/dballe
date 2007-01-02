## Process this file with automake to produce Makefile.in

SUBDIRS = dballe fortran tools swig tables doc .
#examples 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libdballe.pc

m4dir = $(datadir)/aclocal
m4_DATA = libdballe.m4

man_MANS = dbatbl.1 dbamsg.1 dbadb.1

%.1: tools/%
	( cd tools && ./`basename $<` help manpage ) > $@

EXTRA_DIST = autogen.sh libdballe.m4 libdballe.pc.in $(conf_DATA) dballe.spec

CLEANFILES = $(man_MANS)
