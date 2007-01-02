## Process this file with automake to produce Makefile.in

guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib db.dia db.eps
fapisources = fapi.tex ../dballe/core/aliases.tex ../dballe/core/record_keyword.tex ../dballe/msg/ltypes.tex

coredoxdeps = libdballe-core.dox \
	      ../dballe/core/libdballe-core.la \
	      ../dballe/core/core.dox \
	      ../dballe/core/aliases.dox \
	      ../dballe/core/record_keyword.dox

bufrexdoxdeps = libdballe-bufrex.dox \
	        ../dballe/bufrex/libdballe-bufrex.la \
	        ../dballe/bufrex/bufrex.dox \
	        $(coredoxdeps)

msgdoxdeps = libdballe-msg.dox \
	     ../dballe/msg/libdballe-msg.la \
	     ../dballe/msg/msg.dox \
	     ../dballe/msg/ltypes.dox \
	     $(bufrexdoxdeps)

dbdoxdeps = libdballe-db.dox \
	    ../dballe/db/libdballe-db.la \
	    ../dballe/db/db.dox \
	    $(msgdoxdeps)


LATEX2HTML = latex2html -local_icons

noinst_DATA =

if DO_DOCS
noinst_DATA += \
	libdballe-core.doxytags \
	libdballe-bufrex.doxytags \
	libdballe-msg.doxytags \
	libdballe-db.doxytags \
	fapi.ps guide.ps \
	coredox_html/index.html \
	bufrexdox_html/index.html \
	msgdox_html/index.html \
	dbdox_html/index.html
if HAVE_LATEX2HTML
noinst_DATA += \
	fapi_html/index.html \
	guide_html/index.html
endif
endif

libdballe-core.doxytags: $(coredoxdeps)
	doxygen $<

coredox_html/index.html: $(coredoxdeps)
	doxygen $<


libdballe-bufrex.doxytags: $(bufrexdoxdeps)
	doxygen $<

bufrexdox_html/index.html: $(bufrexdoxdeps)
	doxygen $<


libdballe-msg.doxytags: $(msgdoxdeps)
	doxygen $<

msgdox_html/index.html: $(msgdoxdeps)
	doxygen $<


libdballe-db.doxytags: $(dbdoxdeps)
	doxygen $<

dbdox_html/index.html: $(dbdoxdeps)
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
