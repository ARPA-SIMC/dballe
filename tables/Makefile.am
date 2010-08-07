## Process this file with automake to produce Makefile.in

crexbtables = B000101.txt B000103.txt B000203.txt B990203.txt
crexdtables = D000101.txt D000103.txt D000203.txt D990203.txt
bufrbtables = B0000000000000014000.txt B000000000980000.txt B000000000980201.txt B000000000980600.txt B000000000980601.txt B000000000981101.txt B000000000981200.txt B000000000981201.txt B000000002001200.txt B000000002541100.txt B0000000000098013102.txt B000000002551104.txt B000000000981301.txt

bufrdtables = D0000000000000014000.txt D000000000980000.txt D000000000980201.txt D000000000980600.txt D000000000980601.txt D000000000981101.txt D000000000981200.txt D000000000981201.txt D000000002001200.txt D000000002541100.txt D0000000000098013102.txt D000000002551104.txt D000000000981301.txt

table_DATA = repinfo.csv dballe.txt $(crexbtables) $(crexdtables) $(bufrbtables) $(bufrdtables)

noinst_DATA = dballe.tex

dballe.tex: dballe.txt mkbdoc
	$(srcdir)/mkbdoc tex < $< > $@ 

B000000002001200.txt: dballe.txt
	cut -b 1-119 < $< > $@

B990203.txt: dballe.txt
	cp $< $@

EXTRA_DIST = $(table_DATA) mkbdoc

distclean-local:
	rm -f dballe.tex B000000002001200.txt B990203.txt
