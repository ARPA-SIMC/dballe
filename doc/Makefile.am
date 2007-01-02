## Process this file with automake to produce Makefile.in

guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib db.dia db.eps
fapisources = fapi.tex

LATEX2HTML = latex2html -local_icons

noinst_DATA =

if DO_DOCS
noinst_DATA += \
	libdballe-core.doxytags \
	libdballe-bufrex.doxytags \
	libdballe-msg.doxytags \
	libdballe-db.doxytags \
	fapi.ps guide.ps
if DO_HTML
noinst_DATA += \
	coredox_html/index.html \
	bufrexdox_html/index.html \
	msgdox_html/index.html \
	dbdox_html/index.html \
	fapi_html/index.html \
	guide_html/index.html
endif
endif

libdballe-core.doxytags: libdballe-core.dox ../dballe/core/libdballe-core.la
	doxygen $<

coredox_html/index.html: libdballe-core.dox ../dballe/core/libdballe-core.la
	doxygen $<


libdballe-bufrex.doxytags: libdballe-bufrex.dox ../dballe/bufrex/libdballe-bufrex.la
	doxygen $<

bufrexdox_html/index.html: libdballe-bufrex.dox ../dballe/core/libdballe-bufrex.la
	doxygen $<


libdballe-msg.doxytags: libdballe-msg.dox ../dballe/msg/libdballe-msg.la
	doxygen $<

msgdox_html/index.html: libdballe-msg.dox ../dballe/core/libdballe-msg.la
	doxygen $<


libdballe-db.doxytags: libdballe-db.dox ../dballe/db/libdballe-db.la
	doxygen $<

dbdox_html/index.html: libdballe-db.dox ../dballe/core/libdballe-db.la
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

EXTRA_DIST = libdballe-core.dox libdballe-bufrex.dox libdballe-msg.dox libdballe-db.dox $(guidesources) $(fapisources)

CLEANFILES = libdballe-{core,bufrex,msg,db}.doxytags {guide,fapi}.{ps,pdf,dvi,aux,bbl,idx,log,toc,blg}

clean-local:
	rm -rf {core,bufrex,msg,db}dox_html
	rm -rf guide_html
	rm -rf fapi_html
