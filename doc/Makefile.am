## Process this file with automake to produce Makefile.in

guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib db.dia db.eps
fapisources = fapi.tex ../dballe/core/record_keyword.tex ../dballe/msg/ltypes.tex ../dballe/msg/tranges.tex

cdoxdeps = libdballe.dox \
	  ../dballe/libdballe.la \
	  ../dballe/msg/ltypes.dox \
	  ../dballe/msg/tranges.dox

fortrandoxdeps = libdballef.dox \
	    ../fortran/libdballef.la

LATEX2HTML = latex2html -local_icons

doc_DATA =
noinst_DATA =

if DO_DOCS
doc_DATA += libdballe.doxytags guide.pdf
noinst_DATA += c_api/index.html
if DO_DBALLEF
doc_DATA += fapi.pdf libdballef.doxytags
noinst_DATA += fortran_api/index.html
endif
if HAVE_LATEX2HTML
noinst_DATA += \
	fapi_html/index.html \
	guide_html/index.html
endif
endif

install-data-local:
	for pkg in c fortran; do \
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
	for pkg in c fortran; do \
		rm -rf $(DESTDIR)$(docdir)/$${pkg}_api; \
	done || true

mainpages = ../dballe/main-core.dox
#mainpages += ../dballe/main-bufrex.dox
mainpages += ../dballe/main-msg.dox
mainpages += ../dballe/main-db.dox

TEXINPUTS="$(srcdir)"

libwreport.doxytags: /usr/share/doc/libwreport-dev/libwreport.doxytags.gz
	zcat $< > $@

libdballe.doxytags c_api/index.html xml/index.xml: $(cdoxdeps) $(mainpages)
	doxygen $<

libdballef.doxytags fortran_api/index.html: $(fortrandoxdeps)
	doxygen $<

fapi_html/index.html: $(fapisources)
	$(LATEX2HTML) -dir fapi_html -split 1 -mkdir $<

guide_html/index.html: $(guidesources)
	$(LATEX2HTML) -dir guide_html -split 1 -mkdir $<

#fapi-quick.html: fapi-quick.rst ../dballe/core/dba_record_keyword.rst
#fapi-quick.html: fapi-quick.rst
#	rst2html < $< > $@

%.ps: %.dvi
	TEXINPUTS="$(TEXINPUTS):" dvips $< -o

%.pdf: %.tex
	pdflatex $<

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

EXTRA_DIST = libdballe.dox $(guidesources) fapi.tex doxy2swig.py add_templates_to_manpage introduzione.odt

CLEANFILES = libdballe.doxytags libdballef.doxytags {guide,fapi}.{ps,pdf,dvi,aux,bbl,idx,log,toc,blg}

clean-local:
	rm -rf {c,fortran}_api
	rm -rf xml
	rm -rf guide_html
	rm -rf fapi_html
	rm -f libwreport.doxytags
