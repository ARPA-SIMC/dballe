## Process this file with automake to produce Makefile.in

doc_DATA =

noinst_DATA =

EXTRA_DIST = mksnippet

CLEANFILES =

if DO_DOCS

# Generic documentation

generic_mds = dballe_env.md

doc_DATA += $(generic_mds)
EXTRA_DIST += $(generic_mds)


# C++ API documentation

cppdeps = libdballe.dox \
	  ../dballe/libdballe.la \
	  record_keyword.dox \
	  aliases.dox \
	  ltypes.dox \
	  tranges.dox \
	  libwreport.doxytags

record_keyword.dox: $(top_builddir)/dballe/dump-core-info mksnippet
	$(top_builddir)/dballe/dump-core-info keywords | $(top_srcdir)/doc/mksnippet key dox > $@ || rm $@

aliases.dox: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_builddir)/doc/mksnippet alias dox < $< > $@ || rm $@

ltypes.dox: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_builddir)/doc/mksnippet levels dox < $< > $@ || rm $@

tranges.dox: mksnippet
	$(top_builddir)/doc/mksnippet tranges dox > $@ || rm $@

libdballe.doxytags: $(cppdeps)
	doxygen $<

libwreport.doxytags: /usr/share/doc/libwreport-dev/libwreport.doxytags.gz
	zcat $< > $@

doc_DATA += libdballe.doxytags

CLEANFILES += libdballe.doxytags libwreport.doxytags record_keyword.dox aliases.dox ltypes.dox tranges.dox


# Fortran API documentation

fapisources = fapi.md fapi_concepts.md fapi_walkthrough.md fapi_reference.md \
	      fapi_parms.md fapi_aliases.md fapi_connect.md fapi_ltypes.md \
	      fapi_tranges.md fapi_btable.md

fortrandeps = libdballef.dox \
	    ../fortran/libdballef.la

libdballef.doxytags: $(fortrandeps)
	doxygen $<

fapi_reference.md: $(top_srcdir)/doc/make_fortran_reference libdballef.doxytags
	$< > $@ fortran_xml || rm $@

fapi_parms.md: $(top_builddir)/dballe/dump-core-info mksnippet
	$(top_builddir)/dballe/dump-core-info keywords | $(top_srcdir)/doc/mksnippet key md > $@ || rm $@

fapi_aliases.md: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_builddir)/doc/mksnippet alias md < $< > $@ || rm $@

fapi_ltypes.md: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_builddir)/doc/mksnippet levels md < $< > $@ || rm $@

fapi_tranges.md: mksnippet
	$(top_builddir)/doc/mksnippet tranges md > $@ || rm $@

fapi_btable.md: ../tables/dballe.txt mksnippet
	$(top_builddir)/doc/mksnippet btable md < $< > $@ || rm $@


doc_DATA += $(fapisources)

CLEANFILES += libdballef.doxytags

EXTRA_DIST += $(fapisources) make_fortran_reference


# Install guide

#guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib


# make install machinery

install-data-local:
	test -d c_api && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/c_api && \
		find c_api -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/c_api/ \;

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
