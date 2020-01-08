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
    conf.py \
    index.rst \
    howtos.rst \
    reference.rst \
    general_ref/index.rst \
    general_ref/parms.rst \
    general_ref/parms/input.rst \
    general_ref/parms/output.rst \
    general_ref/parms/query.rst \
    general_ref/parms/attrs.rst \
    general_ref/env.rst \
    general_ref/connect.rst \
    cpp_ref/index.rst \
    cpp_ref/messages.rst \
    cpp_ref/cursors.rst \
    cpp_ref/db.rst \
    fortran_how/index.rst \
    fortran_how/errors.rst \
    fortran_how/start.rst \
    fortran_how/iorecords.rst \
    fortran_how/query.rst \
    fortran_how/insert.rst \
    fortran_how/delete.rst \
    fortran_how/attributes.rst \
    fortran_how/describe.rst \
    fortran_how/files.rst \
    fortran_how/faq.rst \
    fortran_ref/index.rst \
    fortran_ref/concepts.rst \
    fortran_ref/functions.rst \
    fortran_ref/transactions.rst \
    python_ref/index.rst \
    python_ref/types.rst \
    python_ref/messages.rst \
    python_ref/cursors.rst \
    python_ref/db.rst \
    python_ref/volnd.rst

sphinx_generated_sources =  \
    general_ref/aliases.rst \
    general_ref/ltypes.rst \
    general_ref/tranges.rst \
    general_ref/btable.rst \
    general_ref/msg_vars.rst

sphinx_sources = $(sphinx_packaged_sources) $(sphinx_generated_sources)

EXTRA_DIST += $(sphinx_packaged_sources)

# See https://stackoverflow.com/questions/19822435/multiple-targets-from-one-recipe-and-parallel-execution#19822767
.INTERMEDIATE: build-docs

# Build doxygen + sphinx documentation
build-docs: $(cppdeps) $(sphinx_sources) ../dballe/libdballe.la ../python/_dballe.la
	rm -rf html
	mkdir html
	$(DOXYGEN) libdballe.dox
	$(top_srcdir)/run-local $(SPHINX_BUILD) -b html . html/

libdballe.doxytags: build-docs
html/index.html: build-docs

aliases.dox: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_srcdir)/doc/mksnippet alias dox < $< > $@ || rm $@

ltypes.dox: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_srcdir)/doc/mksnippet levels dox < $< > $@ || rm $@

tranges.dox: mksnippet
	$(top_srcdir)/doc/mksnippet tranges dox > $@ || rm $@

general_ref/btable.rst: ../tables/dballe.txt mksnippet
	$(top_srcdir)/doc/mksnippet btable rst < $< > $@ || rm $@

general_ref/ltypes.rst: $(top_srcdir)/dballe/msg/ltypes.txt mksnippet
	$(top_srcdir)/doc/mksnippet levels rst < $< > $@ || rm $@

general_ref/tranges.rst: mksnippet
	$(top_srcdir)/doc/mksnippet tranges rst > $@ || rm $@

general_ref/aliases.rst: $(top_srcdir)/dballe/core/aliases.gperf mksnippet
	$(top_srcdir)/doc/mksnippet alias rst < $< > $@ || rm $@

general_ref/msg_vars.rst: $(top_srcdir)/dballe/vars.csv mksnippet
	$(top_srcdir)/doc/mksnippet msg_vars rst < $< > $@ || rm $@


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
