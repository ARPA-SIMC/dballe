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
	  record_keyword.dox \
	  aliases.dox \
	  ltypes.dox \
	  tranges.dox \
	  libwreport.doxytags

record_keyword.dox: $(top_builddir)/dballe/core/record_keyword.gperf mksnippet
	$(top_srcdir)/doc/mksnippet key dox < $< > $@ || rm $@

aliases.dox: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_srcdir)/doc/mksnippet alias dox < $< > $@ || rm $@

ltypes.dox: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_srcdir)/doc/mksnippet levels dox < $< > $@ || rm $@

tranges.dox: mksnippet
	$(top_srcdir)/doc/mksnippet tranges dox > $@ || rm $@

libdballe.doxytags: $(cppdeps)
	doxygen $<

libwreport.doxytags: $(wildcard /usr/share/doc/*wreport*/libwreport.doxytags*)
	if [ -z "$<" ]; then \
	        ( echo "/usr/share/doc/*wreport*/libwreport.doxytags* not found" >&2 ; exit 1 ); \
	elif [ `basename "$<"` = libwreport.doxytags.gz ]; then \
	        zcat $< > $@ ; \
	else \
	        cat $< > $@ ; \
	fi


doc_DATA += libdballe.doxytags

CLEANFILES += libdballe.doxytags libwreport.doxytags record_keyword.dox aliases.dox ltypes.dox tranges.dox


# Fortran API documentation

fapisources = fapi.md fapi_concepts.md fapi_walkthrough.md fapi_reference.md \
	      fapi_parms.md fapi_aliases.md fapi_connect.md fapi_ltypes.md \
	      fapi_tranges.md fapi_btable.md

fortrandeps = libdballef.dox

libdballef.doxytags: $(fortrandeps)
	doxygen $<

fapi_reference.md: $(top_srcdir)/doc/make_fortran_reference libdballef.doxytags
	$< > $@ fortran_xml || rm $@

fapi_parms.md: $(top_builddir)/dballe/core/record_keyword.gperf mksnippet
	$(top_srcdir)/doc/mksnippet key md < $< > $@ || rm $@

fapi_aliases.md: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_srcdir)/doc/mksnippet alias md < $< > $@ || rm $@

fapi_ltypes.md: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_srcdir)/doc/mksnippet levels md < $< > $@ || rm $@

fapi_tranges.md: mksnippet
	$(top_srcdir)/doc/mksnippet tranges md > $@ || rm $@

fapi_btable.md: ../tables/dballe.txt mksnippet
	$(top_srcdir)/doc/mksnippet btable md < $< > $@ || rm $@


doc_DATA += $(fapisources)

CLEANFILES += libdballef.doxytags

EXTRA_DIST += $(fapisources) make_fortran_reference


# make install machinery

install-data-local:
	test -d c_api && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/c_api && \
		find c_api -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/c_api/ \;

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/c_api


EXTRA_DIST += doxy2swig.py add_templates_to_manpage introduzione.odt

CLEANFILES += doxygen_sqlite3.db

clean-local:
	rm -rf {c,fortran}_api
	rm -rf {c,fortran}_xml
endif
