## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = tables dballe extra src
if DO_DBALLEF
SUBDIRS += fortran
endif
if DO_DOCS
SUBDIRS += doc
endif
if DO_DBALLE_PYTHON
SUBDIRS += python
endif
SUBDIRS += bench .

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libdballe.pc libdballef.pc

m4dir = $(datadir)/aclocal
m4_DATA = libdballe.m4 libdballef.m4

man_MANS = dbatbl.1 dbamsg.1 dbadb.1 dbaexport.1

dbadb.1: src/dbadb doc/add_templates_to_manpage
	( cd src && ./`basename $<` help manpage ) | ( cd $(srcdir) && $(srcdir)/doc/add_templates_to_manpage ) > $@

dbamsg.1: src/dbamsg doc/add_templates_to_manpage
	( cd src && ./`basename $<` help manpage ) | ( cd $(srcdir) && $(srcdir)/doc/add_templates_to_manpage ) > $@

dbaexport.1: src/dbaexport
	COLUMNS=200 help2man --name='export data from DB-All.e' --section=1 --no-info --version-string="$(PACKAGE_VERSION)" $< > $@

%.1: src/%
	( cd src && ./`basename $<` help manpage ) > $@

BUILT_SOURCES = dbaexport.1

EXTRA_DIST = README.md NEWS.md LICENSE BENCHMARKS autogen.sh \
	     libdballe.m4 libdballef.m4 \
	     libdballe.pc.in libdballef.pc.in \
	     $(conf_DATA) get_line_no run-check.conf.example \
	     run-check run-local run-bench show_code_notes \
	     fabfile.py \
	     UPGRADE-5.x \
	     dbaexport.1 \
             fedora/SPECS/dballe.spec

CLEANFILES = dbatbl.1 dbamsg.1 dbadb.1 fabfile.pyc
