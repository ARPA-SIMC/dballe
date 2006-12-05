## Process this file with automake to produce Makefile.in

crexbtables = B000101.txt B000103.txt B000203.txt B990203.txt
crexdtables = D000101.txt D000103.txt D000203.txt D990203.txt
bufrbtables = B000000000980000.txt B000000000980201.txt B000000000980600.txt B000000000980601.txt B000000000981101.txt B000000000981200.txt B000000000981201.txt B000000002001200.txt B000000002541100.txt
bufrdtables = D000000000980000.txt D000000000980201.txt D000000000980600.txt D000000000980601.txt D000000000981101.txt D000000000981200.txt D000000000981201.txt D000000002001200.txt tables/D000000002541100.txt

table_DATA = repinfo.csv dballe.txt $(crexbtables) $(crexdtables) $(bufrbtables) $(bufrdtables) levels.txt tranges.txt

noinst_DATA = dballe.tex

dballe.tex: dballe.txt mkbdoc
	./mkbdoc < $< > $@ 

B000000002001200.txt: dballe.txt
	cut -b 0-119 < $< > $@

EXTRA_DIST = $(table_DATA) $(conf_DATA) mkbdoc

clean-local:
	rm -f *.idx
