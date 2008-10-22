## Process this file with automake to produce Makefile.in

SUBDIRS = dballe
if DO_DBALLEPP
SUBDIRS += dballe++
endif
if DO_DBALLEF
SUBDIRS += fortran
endif
SUBDIRS += extra tools tables
if DO_DOCS
SUBDIRS += doc
endif
if DO_DBALLE_PYTHON
SUBDIRS += swig provami
endif
SUBDIRS += .
#examples 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libdballe.pc libdballef.pc libdballepp.pc

m4dir = $(datadir)/aclocal
m4_DATA = libdballe.m4 libdballef.m4 libdballepp.m4

man_MANS = dbatbl.1 dbamsg.1 dbadb.1

dbadb.1: tools/dbadb doc/integrate_dbadb_manpage
	( cd tools && ./`basename $<` help manpage ) | $(srcdir)/doc/integrate_dbadb_manpage > $@

%.1: tools/%
	( cd tools && ./`basename $<` help manpage ) > $@

EXTRA_DIST = autogen.sh \
	     libdballe.m4 libdballef.m4 libdballepp.m4 \
	     libdballe.pc.in libdballef.pc.in libdballepp.pc.in \
	     $(conf_DATA) dballe.spec

CLEANFILES = $(man_MANS)
