## Process this file with automake to produce Makefile.in

crexbtables = B990203.txt B990204.txt B000299.txt
crexdtables = D990203.txt D990204.txt D000299.txt
bufrbtables = B000000002001200.txt \
	      B000000002001400.txt \
	      B0000000000200014001.txt


bufrdtables = D000000002001200.txt \
	      D000000002001400.txt \
	      D0000000000200014001.txt

table_DATA = repinfo.csv dballe.txt $(crexbtables) $(crexdtables) $(bufrbtables) $(bufrdtables)

noinst_DATA = dballe.tex

dballe.tex: dballe.txt mkbdoc
	$(srcdir)/mkbdoc tex < $< > $@ 

B000000002001400.txt: dballe.txt
	cut -b 1-119 < $< > $@

B0000000000200014001.txt: dballe.txt
	cut -b 1-119 < $< > $@

B990204.txt: dballe.txt
	cp $< $@

B000299.txt: dballe.txt
	cp $< $@

EXTRA_DIST = $(table_DATA) mkbdoc

distclean-local:
	rm -f dballe.tex
