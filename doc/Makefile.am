## Process this file with automake to produce Makefile.in

generic_mds = dballe_env.md

doc_DATA = $(generic_mds)

noinst_DATA =

EXTRA_DIST = mksnippet add_templates_to_manpage $(generic_mds)

CLEANFILES =

if DO_DOCS

# C++ API documentation sources

cppdeps = libdballe.dox \
	  aliases.dox \
	  ltypes.dox \
	  tranges.dox \
	  libwreport.doxytags


# Fortran API documentation sources

fapisources = fapi.md fapi_concepts.md fapi_walkthrough.md fapi_reference.md \
              fapi_parms.md fapi_aliases.md fapi_connect.md fapi_ltypes.md \
              fapi_tranges.md fapi_btable.md fapi_transactions.md

EXTRA_DIST += $(fapisources)


# Sphinx sources

sphinx_packaged_sources =  \
    src/conf.py \
    src/index.rst \
    src/parms.rst \
    src/parms/input.rst \
    src/parms/output.rst \
    src/parms/query.rst \
    src/parms/attrs.rst \
    src/env.rst \
    src/connect.rst \
    src/cpp/dballe.rst \
    src/cpp/messages.rst \
    src/cpp/cursors.rst \
    src/cpp/db.rst \
    src/fortran/dballe.rst \
    src/fortran/walkthrough.rst \
    src/fortran/concepts.rst \
    src/fortran/transactions.rst \
    src/fortran/reference.rst \
    src/fortran/faq.rst \
    src/python/dballe.rst \
    src/python/dballe_types.rst \
    src/python/dballe_messages.rst \
    src/python/dballe_cursors.rst \
    src/python/dballe_db.rst \
    src/python/dballe.volnd.rst

sphinx_generated_sources =  \
    src/aliases.rst \
    src/ltypes.rst \
    src/tranges.rst \
    src/btable.rst

sphinx_sources = $(sphinx_packaged_sources) $(sphinx_generated_sources)

EXTRA_DIST += $(sphinx_packaged_sources)

# See https://stackoverflow.com/questions/19822435/multiple-targets-from-one-recipe-and-parallel-execution#19822767
.INTERMEDIATE: build-docs

# Build doxygen + sphinx documentation
build-docs: $(cppdeps) $(sphinx_sources) ../dballe/libdballe.la ../python/_dballe.la
	rm -rf html
	mkdir html
	$(DOXYGEN) libdballe.dox
	$(top_srcdir)/run-local $(SPHINX_BUILD) -b html src html/

libdballe.doxytags: build-docs
html/index.html: build-docs

aliases.dox: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_srcdir)/doc/mksnippet alias dox < $< > $@ || rm $@

ltypes.dox: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_srcdir)/doc/mksnippet levels dox < $< > $@ || rm $@

tranges.dox: mksnippet
	$(top_srcdir)/doc/mksnippet tranges dox > $@ || rm $@

src/btable.rst: ../tables/dballe.txt mksnippet
	$(top_srcdir)/doc/mksnippet btable rst < $< > $@ || rm $@

src/ltypes.rst: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_srcdir)/doc/mksnippet levels rst < $< > $@ || rm $@

src/tranges.rst: mksnippet
	$(top_srcdir)/doc/mksnippet tranges rst > $@ || rm $@

src/aliases.rst: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_srcdir)/doc/mksnippet alias rst < $< > $@ || rm $@


libwreport.doxytags:
	if test x$(suffix @WREPORT_DOXYGEN_TAGFILE@) = x.gz ; then \
	        zcat @WREPORT_DOXYGEN_TAGFILE@ > $@ ; \
	else \
	        cat @WREPORT_DOXYGEN_TAGFILE@ > $@ ; \
	fi


doc_DATA += libdballe.doxytags

CLEANFILES += libdballe.doxytags libwreport.doxytags aliases.dox ltypes.dox tranges.dox $(sphinx_generated_sources)


# make install machinery

install-data-local:
	find html -type d -exec $(mkinstalldirs) '$(DESTDIR)$(docdir)/{}'  \; ;
	find html -type f -not -path '*/.*' -exec $(INSTALL_DATA) '{}' '$(DESTDIR)$(docdir)/{}' \; ;

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/html


EXTRA_DIST += introduzione.odt

CLEANFILES += doxygen_sqlite3.db

clean-local:
	rm -rf {c,fortran}_xml
	rm -rf html

endif
