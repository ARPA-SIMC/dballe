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
	wxDballe/__init__.py \
	wxDballe/AnaDetails.py \
	wxDballe/AttrDetails.py \
	wxDballe/CSV.py \
	wxDballe/DataMenu.py \
	wxDballe/DataResults.py \
	wxDballe/DateCanvas.py \
	wxDballe/DateChoice.py \
	wxDballe/DateTimeCanvas.py \
	wxDballe/FilterWindow.py \
	wxDballe/IdentChoice.py \
	wxDballe/LevelsChoice.py \
	wxDballe/LimitChoice.py \
	wxDballe/MapCanvas.py \
	wxDballe/MapChoice.py \
	wxDballe/MapChoiceSingleQuery.py \
	wxDballe/Model.py \
	wxDballe/ModelSingleQuery.py \
	wxDballe/QueryButton.py \
	wxDballe/QueryChoice.py \
	wxDballe/QueryTextCtrl.py \
	wxDballe/ReportChoice.py \
	wxDballe/ResultGrid.py \
	wxDballe/TimeRangeChoice.py \
	wxDballe/VarNamesChoice.py

provamipython_PYTHON = $(provamiModules)
nodist_provamipython_PYTHON = wxDballe/Paths.py

noinst_DATA = wxDballe/Paths.py

wxDballe/Paths.py: wxDballe/Paths.py.in
	sed -e 's|@DATADIR@|$(dballedatadir)|g' $< > $@

dist_bin_SCRIPTS = provami dballecsv dballecsv

EXTRA_DIST = dballe populate.py wxDballe/Paths.py.in \
	     $(dballedata_DATA) $(wxDballeModules)

CLEANFILES = wxDballe/*.pyc wxDballe/Paths.py
