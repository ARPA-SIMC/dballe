## Process this file with automake to produce Makefile.in

guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib db.dia db.eps
fapisources = fapi.tex

LATEX2HTML = latex2html -local_icons

noinst_DATA =

if DO_DOCS
noinst_DATA += libdballe.doxytags fapi.ps guide.ps
if DO_HTML
noinst_DATA += dox_html/index.html fapi_html/index.html guide_html/index.html
endif
endif

libdballe.doxytags: libdballe.dox ../dballe/libdballe.la
	doxygen $<

dox_html/index.html: libdballe.dox ../dballe/libdballe.la
	doxygen $<

fapi_html/index.html: $(fapisources)
	$(LATEX2HTML) -dir fapi_html -mkdir $<

guide_html/index.html: $(guidesources)
	$(LATEX2HTML) -dir guide_html -mkdir $<

#fapi-quick.html: fapi-quick.rst ../dballe/core/dba_record_keyword.rst
#fapi-quick.html: fapi-quick.rst
#	rst2html < $< > $@

%.ps: %.dvi
	dvips $< -o

%.pdf: %.ps
	ps2pdf $<

%.aux: %.tex
	latex $<

%.bbl: %.aux
	bibtex `basename $< .aux`

%.dvi: %.tex %.aux %.bbl
	latex $<
	bibtex `basename $< .tex`
	latex $<

# Additional dependencies for guide.dvi
guide.dvi: $(guidesources)

fapi.dvi: $(fapisources)

EXTRA_DIST = libdballe.dox $(guidesources) $(fapisources)

CLEANFILES = libdballe.doxytags {guide,fapi}.{ps,pdf,dvi,aux,bbl,idx,log,toc,blg}

clean-local:
	rm -rf dox_html
	rm -rf guide_html
	rm -rf fapi_html
