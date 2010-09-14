## Process this file with automake to produce Makefile.in

guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib db.dia db.eps
fapisources = fapi.tex ../dballe/core/aliases.tex ../dballe/core/record_keyword.tex ../dballe/msg/ltypes.tex ../dballe/msg/tranges.tex

cdoxdeps = libdballe.dox \
	  ../dballe/libdballe.la \
	  ../dballe/core/aliases.dox \
	  ../dballe/core/btable.dox \
	  ../dballe/core/record_keyword.dox \
	  ../dballe/msg/ltypes.dox \
	  ../dballe/msg/tranges.dox

cppdoxdeps = libdballepp.dox \
	    ../dballe++/libdballepp.la

fortrandoxdeps = libdballef.dox \
	    ../fortran/libdballef.la

LATEX2HTML = latex2html -local_icons

doc_DATA =
noinst_DATA =

if DO_DOCS
doc_DATA += libdballe.doxytags guide.ps
noinst_DATA += c_api/index.html
if DO_DBALLEPP
doc_DATA += libdballepp.doxytags
noinst_DATA += c++_api/index.html
endif
if DO_DBALLEF
doc_DATA += fapi.ps libdballef.doxytags
noinst_DATA += fortran_api/index.html
endif
if DO_DBALLE_PYTHON
noinst_DATA += dballe-doc.i
endif
if HAVE_LATEX2HTML
noinst_DATA += \
	fapi_html/index.html \
	guide_html/index.html
endif
endif

install-data-local:
	for pkg in c c++ fortran; do \
		test -d $${pkg}_api && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/$${pkg}_api && \
		find $${pkg}_api -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/$${pkg}_api/ \; ; \
	done || true
	for pkg in fapi_html guide_html; do \
		test -d $$pkg && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/$$pkg && \
		find $$pkg -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/$$pkg/ \; ; \
	done || true

uninstall-local:
	for pkg in c c++; do \
		rm -rf $(DESTDIR)$(docdir)/$${pkg}_api; \
	done || true

mainpages = ../dballe/main-core.dox 
#mainpages += ../dballe/main-bufrex.dox 
mainpages += ../dballe/main-msg.dox 
if DO_DBALLE_DB
mainpages += ../dballe/main-db.dox 
endif

TEXINPUTS="$(srcdir)"

libdballe.doxytags c_api/index.html: $(cdoxdeps) $(mainpages)
	doxygen $<

libdballepp.doxytags c++_api/index.html c++_xml/index.xml: $(cppdoxdeps)
	doxygen $<

libdballef.doxytags fortran_api/index.html: $(fortrandoxdeps)
	doxygen $<

dballe-doc.i: c++_xml/index.xml doxy2swig.py
	$(srcdir)/doxy2swig.py $< $@

fapi_html/index.html: $(fapisources)
	$(LATEX2HTML) -dir fapi_html -mkdir $<

guide_html/index.html: $(guidesources)
	$(LATEX2HTML) -dir guide_html -mkdir $<

#fapi-quick.html: fapi-quick.rst ../dballe/core/dba_record_keyword.rst
#fapi-quick.html: fapi-quick.rst
#	rst2html < $< > $@

%.ps: %.dvi
	TEXINPUTS="$(TEXINPUTS):" dvips $< -o

%.pdf: %.ps
	ps2pdf $<

%.aux: %.tex
	TEXINPUTS="$(TEXINPUTS):" latex $<

%.bbl: %.aux
	BIBINPUTS="$(TEXINPUTS):" bibtex `basename $< .aux`

%.dvi: %.tex %.aux %.bbl
	TEXINPUTS="$(TEXINPUTS):" latex $<
	BIBINPUTS="$(TEXINPUTS):" bibtex `basename $< .tex`
	TEXINPUTS="$(TEXINPUTS):" latex $<

# Additional dependencies for guide.dvi
guide.dvi: $(guidesources)

fapi.dvi: $(fapisources)

EXTRA_DIST = libdballe.dox $(guidesources) fapi.tex doxy2swig.py integrate_dbadb_manpage introduzione.odt

CLEANFILES = libdballe.doxytags {guide,fapi}.{ps,pdf,dvi,aux,bbl,idx,log,toc,blg} dballe-doc.i

clean-local:
	rm -rf {c,c++,fortran}_api
	rm -rf c++_xml
	rm -rf guide_html
	rm -rf fapi_html
