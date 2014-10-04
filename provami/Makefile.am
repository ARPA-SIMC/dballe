## Process this file with automake to produce Makefile.in

dballedatadir = $(datadir)/dballe

dballedata_DATA = \
	icon-provami.png \
	icon-ana-window.png \
	icon-attr-window.png \
	icon-filters-window.png \
	icon-map-move.png \
	icon-map-select-area.png \
	icon-map-select-ident.png \
	icon-map-select-station.png \
	icon-map-window.png \
	world.dat

provamipythondir = $(pythondir)/provami

provamiModules = \
	provami/__init__.py \
	provami/AnaDetails.py \
	provami/AttrDetails.py \
	provami/DataMenu.py \
	provami/DataResults.py \
	provami/DateChoice.py \
	provami/FilterWindow.py \
	provami/IdentChoice.py \
	provami/LevelsChoice.py \
	provami/LimitChoice.py \
	provami/MapCanvas.py \
	provami/MapChoice.py \
	provami/MapChoiceSingleQuery.py \
	provami/Model.py \
	provami/Navigator.py \
	provami/ProgressDisplay.py \
	provami/ProvamiArtProvider.py \
	provami/QueryButton.py \
	provami/QueryChoice.py \
	provami/QueryPanel.py \
	provami/QueryTextCtrl.py \
	provami/ReportChoice.py \
	provami/ResultGrid.py \
	provami/TimeRangeChoice.py \
	provami/VarNamesChoice.py

provamipython_PYTHON = $(provamiModules)
nodist_provamipython_PYTHON = provami/Paths.py

noinst_DATA = provami/Paths.py

# mkdir is there for VPATH builds, as the provami directory at this point only
# exists in $(srcdir)
provami/Paths.py: $(srcdir)/provami/Paths.py.in
	mkdir -p provami
	sed -e 's|@DATADIR@|$(dballedatadir)|g' $< > $@

dist_bin_SCRIPTS = provami.py

man_MANS = provami.1

install-exec-hook:
	mv $(DESTDIR)$(bindir)/provami.py $(DESTDIR)$(bindir)/provami

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/provami

EXTRA_DIST = dballe-print-stations populate.py provami/Paths.py.in \
	     $(dballedata_DATA) $(provamiModules) \
	     provami.1 dballecsv notes regenerate-provami-manpage test-provami

CLEANFILES = provami/*.pyc provami/Paths.py
