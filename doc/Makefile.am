## Process this file with automake to produce Makefile.in

doc_DATA =

noinst_DATA =

EXTRA_DIST =

CLEANFILES =

if DO_DOCS

# C++ API documentation

cppdeps = libdballe.dox \
	  ../dballe/libdballe.la \
	  ../dballe/msg/ltypes.dox \
	  ../dballe/msg/tranges.dox \
	  libwreport.doxytags

cppdeps += ../dballe/main-core.dox
#mainpages += ../dballe/main-bufrex.dox
cppdeps += ../dballe/main-msg.dox
cppdeps += ../dballe/main-db.dox

libdballe.doxytags: $(cppdeps)
	doxygen $<

libwreport.doxytags: /usr/share/doc/libwreport-dev/libwreport.doxytags.gz
	zcat $< > $@

doc_DATA += libdballe.doxytags

CLEANFILES += libdballe.doxytags libwreport.doxytags


# Fortran API documentation

fapisources = fapi.md fortran-reference.md

fortrandeps = libdballef.dox \
	    ../fortran/libdballef.la

libdballef.doxytags: $(fortrandeps)
	doxygen $<

fortran-reference.md: $(top_srcdir)/doc/make_fortran_reference libdballef.doxytags
	$< > $@ fortran_xml || rm $<

doc_DATA += $(fapisources)

CLEANFILES += libdballef.doxytags

EXTRA_DIST += $(fapisources) make_fortran_reference


# Install guide

#guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib


# make install machinery

install-data-local:
	test -d c_api && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/c_api && \
		find c_api -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/c_api/ \; ; \
	done || true

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/c_api


#guide_html/index.html: $(guidesources)
#	$(LATEX2HTML) -dir guide_html -split 1 -mkdir $<


EXTRA_DIST += doxy2swig.py add_templates_to_manpage introduzione.odt

CLEANFILES += doxygen_sqlite3.db

clean-local:
	rm -rf {c,fortran}_api
	rm -rf {c,fortran}_xml

endif
