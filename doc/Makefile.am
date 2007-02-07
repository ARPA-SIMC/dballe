## Process this file with automake to produce Makefile.in

guidesources = guide.tex intro.tex install.tex maintainance.tex troubleshooting.tex local.bib db.dia db.eps
fapisources = fapi.tex ../dballe/core/aliases.tex ../dballe/core/record_keyword.tex ../dballe/msg/ltypes.tex ../dballe/msg/tranges.tex

coredoxdeps = libdballe-core.dox \
	      ../dballe/core/libdballe-core.la \
	      ../dballe/core/aliases.dox \
	      ../dballe/core/btable.dox \
	      ../dballe/core/record_keyword.dox

bufrexdoxdeps = libdballe-bufrex.dox \
	        ../dballe/bufrex/libdballe-bufrex.la \
	        $(coredoxdeps)

msgdoxdeps = libdballe-msg.dox \
	     ../dballe/msg/libdballe-msg.la \
	     ../dballe/msg/ltypes.dox \
	     ../dballe/msg/tranges.dox \
	     $(bufrexdoxdeps)

dbdoxdeps = libdballe-db.dox \
	    ../dballe/db/libdballe-db.la \
	    $(msgdoxdeps)

cppdoxdeps = libdballepp.dox \
	    ../dballe++/libdballepp.la

LATEX2HTML = latex2html -local_icons

doc_DATA =
noinst_DATA =

if DO_DOCS
doc_DATA += \
	libdballe-core.doxytags \
	libdballe-bufrex.doxytags \
	libdballe-msg.doxytags \
	libdballe-db.doxytags \
	libdballepp.doxytags \
	fapi.ps guide.ps
noinst_DATA += \
	core_api/index.html \
	bufrex_api/index.html \
	msg_api/index.html \
	db_api/index.html \
	c++_api/index.html \
	dballe-doc.i
if HAVE_LATEX2HTML
noinst_DATA += \
	fapi_html/index.html \
	guide_html/index.html
endif
endif

install-data-local:
	for pkg in core bufrex msg db c++; do \
		test -d $${pkg}_api && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/$${pkg}_api && \
		find $${pkg}_api -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/$${pkg}_api/ \; ; \
	done || true
	for pkg in fapi_html guide_html; do \
		test -d $$pkg && \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/$$pkg && \
		find $$pkg -type f -exec $(INSTALL_DATA) '{}' $(DESTDIR)$(docdir)/$$pkg/ \; ; \
	done || true


libdballe-core.doxytags core_api/index.html: $(coredoxdeps) ../dballe/main-core.dox
	doxygen $<

libdballe-bufrex.doxytags bufrex_api/index.html: $(bufrexdoxdeps) ../dballe/main-bufrex.dox
	doxygen $<

libdballe-msg.doxytags msg_api/index.html: $(msgdoxdeps) ../dballe/main-msg.dox
	doxygen $<

libdballe-db.doxytags db_api/index.html: $(dbdoxdeps) ../dballe/main-db.dox
	doxygen $<

libdballepp.doxytags c++_api/index.html c++_xml/index.xml: $(cppdoxdeps)
	doxygen $<

dballe-doc.i: c++_xml/index.xml doxy2swig.py
	./doxy2swig.py $< $@

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

EXTRA_DIST = libdballe-core.dox libdballe-bufrex.dox libdballe-msg.dox libdballe-db.dox $(guidesources) fapi.tex doxy2swig.py

CLEANFILES = libdballe-{core,bufrex,msg,db}.doxytags {guide,fapi}.{ps,pdf,dvi,aux,bbl,idx,log,toc,blg} dballe-doc.i

clean-local:
	rm -rf {core,bufrex,msg,db,c++}_api
	rm -rf c++_xml
	rm -rf guide_html
	rm -rf fapi_html
