## Process this file with automake to produce Makefile.in

SUBDIRS = dballe dballe++ fortran extra tools tables swig doc .
#examples 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libdballe-core.pc libdballe-bufrex.pc libdballe-msg.pc libdballe-db.pc libdballef.pc libdballepp.pc

m4dir = $(datadir)/aclocal
m4_DATA = libdballe-core.m4 libdballe-bufrex.m4 libdballe-msg.m4 libdballe-db.m4 libdballef.m4 libdballepp.m4

man_MANS = dbatbl.1 dbamsg.1 dbadb.1

%.1: tools/%
	( cd tools && ./`basename $<` help manpage ) > $@

EXTRA_DIST = autogen.sh \
	     libdballe-core.m4 libdballe-bufrex.m4 libdballe-msg.m4 libdballe-db.m4 \
	     libdballe-core.pc.in libdballe-bufrex.pc.in libdballe-msg.pc.in libdballe-db.pc.in \
	     libdballef.m4 libdballef.pc.in \
	     $(conf_DATA) dballe.spec

CLEANFILES = $(man_MANS)
