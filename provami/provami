#!/usr/bin/python

import sys
import dballe
import string
import os
import re
from optparse import OptionParser
import wx

from wxDballe.Model import *
#from wxDballe.ModelSingleQuery import Model
from wxDballe.MapCanvas import MapCanvas
from wxDballe.MapChoice import MapChoice
#from wxDballe.MapChoiceSingleQuery import MapChoice
from wxDballe.LimitChoice import LimitChoice
from wxDballe.LevelsChoice import LevelsChoice
from wxDballe.ReportChoice import ReportChoice
from wxDballe.TimeRangeChoice import TimeRangeChoice
from wxDballe.VarNamesChoice import VarNamesChoice
from wxDballe.DateChoice import MinDateChoice, MaxDateChoice
#from wxDballe.DateCanvas import DateCanvas
from wxDballe.DataResults import DataPanel
from wxDballe.AttrDetails import AttrResults
from wxDballe.AnaDetails import AnaResults
from wxDballe.ResultGrid import ResultGrid
from wxDballe.FilterWindow import FilterWindow

VERSION="1.0"

###
### View
###


class Query(wx.Panel):
	def __init__(self, parent, model, id=-1):
		wx.Panel.__init__(self, parent, id)
		#wx.Panel.__init__(self, parent, id, wx.TAB_TRAVERSAL)
		self.model = model

		gbs = wx.GridBagSizer()
		gbs.Add(wx.StaticText(self, -1, "Variable: "), (0, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(VarNamesChoice(self, self.model), (0, 1), (1, 1), flag=wx.EXPAND)

		gbs.Add(wx.StaticText(self, -1, "Level: "), (1, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(LevelsChoice(self, self.model), (1, 1), (1, 1), flag=wx.EXPAND)

		gbs.Add(wx.StaticText(self, -1, "Time range: "), (2, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(TimeRangeChoice(self, self.model), (2, 1), (1, 1), flag=wx.EXPAND)

		gbs.Add(wx.StaticText(self, -1, "Report: "), (3, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(ReportChoice(self, self.model), (3, 1), (1, 1), flag=wx.EXPAND)

		timePanel = wx.Panel(self)
		box = wx.BoxSizer(wx.HORIZONTAL)
		box.Add(wx.StaticText(timePanel, -1, "min: "), 0, flag = wx.ALIGN_CENTER_VERTICAL)
		box.Add(MinDateChoice(timePanel, model), 1, flag=wx.EXPAND)
		box.Add(wx.StaticText(timePanel, -1, " max: "), 0, flag = wx.ALIGN_CENTER_VERTICAL)
		box.Add(MaxDateChoice(timePanel, model), 1, flag=wx.EXPAND)
		timePanel.SetSizer(box)

		gbs.Add(wx.StaticText(self, -1, "Time: "), (4, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(timePanel, (4, 1), (1, 1), flag=wx.EXPAND)

#		gbs.Add(wx.StaticText(self, -1, "Min time: "), (4, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
#		gbs.Add(MinDateChoice(self, self.model), (4, 1), (1, 1), flag=wx.EXPAND)
#
#		gbs.Add(wx.StaticText(self, -1, "Max time: "), (5, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
#		gbs.Add(MaxDateChoice(self, self.model), (5, 1), (1, 1), flag=wx.EXPAND)

#		gbs.Add(wx.StaticText(self, -1, "Timeline: "), (6, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
#		gbs.Add(DateCanvas(self, self.model), (6, 1), (1, 1), flag=wx.EXPAND)

		gbs.AddGrowableCol(1)

		self.SetSizerAndFit(gbs)

class ProgressDisplay(ProgressListener):
	def __init__(self, parent, model):
		self.parent = parent
		self.model = model
		self.model.registerProgressListener(self)
		self.dlg = None

	def progress(self, perc, text):
		print "%d%%: %s" % (perc, text)
		if perc == 100:
			if self.dlg != None:
				self.dlg.Destroy()
				self.dlg = None
		else:
			if self.dlg == None:
				self.dlg = wx.ProgressDialog("Updating data",
						text,
						maximum = 100,
						parent = self.parent,
						style = wx.PD_CAN_ABORT
							| wx.PD_APP_MODAL
							| wx.PD_ELAPSED_TIME
							| wx.PD_ESTIMATED_TIME
							| wx.PD_REMAINING_TIME)
			if not self.dlg.Update(perc, text):
				self.model.cancelUpdate()
		wx.Yield()


class Navigator(wx.Frame, ProgressListener, ModelListener):
	ACTION_MAP_WINDOW = wx.NewId()
	ACTION_ANA_WINDOW = wx.NewId()
	ACTION_ATTR_WINDOW = wx.NewId()
	ACTION_FILTERS_WINDOW = wx.NewId()
	ACTION_QUIT = wx.NewId()
	ACTION_DELETE_SELECTED = wx.NewId()
	ACTION_DELETE_ORPHANS = wx.NewId()
	ACTION_EXPORT = wx.NewId()
	ACTION_REFRESH = wx.NewId()

	ICON_PROVAMI = "provami_icon"
	ICON_MAP_WINDOW = "provami_map_window"
	ICON_ANA_WINDOW = "provami_ana_window"
	ICON_ATTR_WINDOW = "provami_attr_window"
	ICON_FILTERS_WINDOW = "provami_filters_window"

	def __init__(self, parent, model, title):
		wx.Frame.__init__(self, parent, -1, title, size = (600, 600),
				style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE)

		self.model = model
		self.model.registerProgressListener(self)
		self.model.registerUpdateListener(self)

		icon = wx.EmptyIcon()
		icon.CopyFromBitmap(wx.ArtProvider.GetBitmap(Navigator.ICON_PROVAMI, wx.ART_TOOLBAR, (16, 16)))
		self.SetIcon(icon)

		# Create progress display manager
		self.progressDisplay = ProgressDisplay(self, model)

		# Create toolbar
		self.tb = self.CreateToolBar(wx.TB_HORIZONTAL | wx.NO_BORDER | wx.TB_FLAT | wx.TB_TEXT)
		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_MAP_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_MAP_WINDOW, bmp, shortHelp="Open/close map", \
					longHelp="Open/close map window")

		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_ANA_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_ANA_WINDOW, bmp, shortHelp="Open/close pseudoana details", \
					longHelp="Open/close pseudoana details window")

		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_ATTR_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_ATTR_WINDOW, bmp, shortHelp="Open/close attribute details", \
					longHelp="Open/close attribute details window")
		self.Bind(wx.EVT_TOOL, self.onToolClick)

		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_FILTERS_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_FILTERS_WINDOW, bmp, shortHelp="Open/close extra filters", \
					longHelp="Open/close extra filters window")
		self.Bind(wx.EVT_TOOL, self.onToolClick)

		self.tb.ToggleTool(Navigator.ACTION_MAP_WINDOW, True)

		# Create menu bar
		self.menuBar = wx.MenuBar()

		mFile = wx.Menu()
		mFile.Append(Navigator.ACTION_EXPORT, "&Export...", "Export all selected data")
		mFile.AppendSeparator()
		mFile.Append(Navigator.ACTION_QUIT, "&Quit", "Quit wxdballe")
		self.menuBar.Append(mFile, "&File")

		mView = wx.Menu()
		mView.Append(Navigator.ACTION_MAP_WINDOW, "&Map", "Map window", wx.ITEM_CHECK)
		mView.Append(Navigator.ACTION_ANA_WINDOW, "&Pseudoana", "Pseudoana window", wx.ITEM_CHECK)
		mView.Append(Navigator.ACTION_ATTR_WINDOW, "&Attributes", "Attributes window", wx.ITEM_CHECK)
		mView.Append(Navigator.ACTION_FILTERS_WINDOW, "&Extra filters", "Extra filters window", wx.ITEM_CHECK)
		mView.AppendSeparator()
		mView.Append(Navigator.ACTION_REFRESH, "&Refresh", "Reload data from database")
		self.menuBar.Append(mView, "&View")
		self.menuBar.Check(Navigator.ACTION_MAP_WINDOW, True)

		mEdit = wx.Menu()
		mEdit.Append(Navigator.ACTION_DELETE_SELECTED, "&Delete displayed values", "Delete all values currently displayed")
		mEdit.Append(Navigator.ACTION_DELETE_ORPHANS, "&Delete empty stations", "Delete all stations with no data")
		self.menuBar.Append(mEdit, "&Edit")

		self.SetMenuBar(self.menuBar)
		self.Bind(wx.EVT_MENU, self.onToolClick)

		# Create status bar
		self.statusBar = self.CreateStatusBar()

		# Create extra windows
		self.mapWindow = MapChoice(self, model);
		self.mapWindow.SetIcon(icon)
		self.mapWindow.Show()
		self.mapWindow.Bind(MapCanvas.EVT_CURRENT_STATION_CHANGED, self.onChangedCurrentStation)

		self.query = Query(self, model)
		self.dataPanel = DataPanel(self, model, self.statusBar)
		self.dataPanel.Bind(ResultGrid.EVT_CHANGED_CURRENT_ROW, self.onChangedCurrentRow)

		box = wx.BoxSizer(wx.VERTICAL)
		box.Add(self.query, 0, flag=wx.EXPAND)
		box.Add(LimitChoice(self, model), 0, flag=wx.EXPAND)
		box.Add(self.dataPanel, 1, flag=wx.EXPAND)
		self.SetSizerAndFit(box)

		self.anaWindow = AnaResults(self, model)
		self.anaWindow.SetIcon(icon)
		self.anaWindow.Bind(ResultGrid.EVT_CHANGED_CURRENT_ROW, self.onChangedCurrentRow)
		#self.dataPanel.results.registerRecordListener(self.anaWindow)

		self.attrWindow = AttrResults(self, model)
		self.attrWindow.SetIcon(icon)
		#self.attrWindow.registerUpdatesWith(self.dataPanel.results)
		#self.attrWindow.registerUpdatesWith(self.anaWindow)

		self.filtersWindow = FilterWindow(self, model)
		self.filtersWindow.SetIcon(icon)

		self.last_context_id = None
		self.last_var = None
		self.updating = False

	def invalidate(self):
		self.updating = True

	def hasData(self, what):
#		"""
#		Try to reposition on the previously selected items after a data update
#		"""
		if what != "all": return
#			record = self.dataPanel.results.getFirstRow()
#
#		if self.last_context_id == None or self.last_var == None:
#			# Position on the first row
#			record = self.dataPanel.results.getFirstRow()
#		else:
#			# Try to get the previously selected item
#			record = self.model.recordByContextAndVar(self.last_context_id, self.last_var)
#			if record == None:
#				# If it's not in the results anymore, then go
#				# on the first row
#				record = self.dataPanel.results.getFirstRow()
#
#		if record != None:
#			context, var = record.enqi("context_id"), record.enqc("var")
#			print "HDR", context, var
#			row = self.dataPanel.results.GetTable().rowByContextAndVar(context, var)
#			self.anaWindow.displayRecord(record)
#			self.attrWindow.display(record)
#			self.last_context_id = record.enqi("context_id")
#			self.last_var = record.enqc("var")
#		else:
#			print "HDR none"
#			self.last_context_id = None
#			self.last_var = None
#
		self.updating = False

	def queryError(self, message):
		cnf = wx.MessageDialog(self,
			"The DB-ALLe query has failed.  The error is: " + message,
			'Query failed',
			wx.OK | wx.ICON_ERROR
			)
		cnf.ShowModal()
		cnf.Destroy()

	def onChangedCurrentStation(self, event):
		if self.updating: return
		"Handle change of currently selected station in the map"
		id = event.GetStation()
		self.anaWindow.displayID(id)

	def onChangedCurrentRow(self, event):
		if self.updating: return
		self.updating = True
		record = event.GetData()
		self.anaWindow.displayRecord(record)
		self.attrWindow.display(record)
		self.updating = False

	def mapHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_MAP_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_MAP_WINDOW, False)

	def anaHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_ANA_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_ANA_WINDOW, False)

	def attrHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_ATTR_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_ATTR_WINDOW, False)

	def filtersHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_FILTERS_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_FILTERS_WINDOW, False)

	def onToolClick(self, event):
		if event.GetId() == Navigator.ACTION_MAP_WINDOW:
			state = self.mapWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_MAP_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_MAP_WINDOW, not state)
			self.mapWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_ANA_WINDOW:
			state = self.anaWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_ANA_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_ANA_WINDOW, not state)
			self.anaWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_ATTR_WINDOW:
			state = self.attrWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_ATTR_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_ATTR_WINDOW, not state)
			self.attrWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_FILTERS_WINDOW:
			state = self.filtersWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_FILTERS_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_FILTERS_WINDOW, not state)
			self.filtersWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_REFRESH:
			self.model.update()
		elif event.GetId() == Navigator.ACTION_QUIT:
			self.Destroy()
		elif event.GetId() == Navigator.ACTION_DELETE_SELECTED:
			self.model.deleteCurrent()
		elif event.GetId() == Navigator.ACTION_DELETE_ORPHANS:
			self.model.deleteOrphans()
		elif event.GetId() == Navigator.ACTION_EXPORT:
#			dlg = wx.FileDialog(
#					self, message="Save file as...", defaultDir=os.getcwd(), 
#					defaultFile="", style=wx.SAVE
#					)
			dlg = wx.FileDialog(
					self, message="Save file as...", defaultDir=os.getcwd(), 
					#defaultFile="", wildcard="According to file extension|*|BUFR optimal template (*.bufr)|*.bufr|BUFR generic template (*.bufr)|*.bufr|CREX (*.crex)|*.crex|Comma Separated Values (*.csv)|*.csv|GNU R data file (*.Rdata)|*.Rdata|Pickled volNd Python objects (*.volnd)|*.volnd", style=wx.SAVE
					defaultFile="", wildcard="According to file extension|*|BUFR optimal template (*.bufr)|*.bufr|BUFR generic template (*.bufr)|*.bufr|CREX (*.crex)|*.crex|Comma Separated Values (*.csv)|*.csv|GNU R data file (*.Rdata)|*.Rdata", style=wx.SAVE
					)

			# Show the dialog and retrieve the user response. If it is the OK response, 
			# process the data.
			path = None
			eidx = 0
			if dlg.ShowModal() == wx.ID_OK:
				path = dlg.GetPath()
				eidx = dlg.GetFilterIndex()
#				# Check if it exists
#				if os.access(path, os.F_OK):
#					cnf = wx.MessageDialog(self,
#						path + " already exists.  Would you like to overwrite it?",
#						'File exists',
#						wx.OK | wx.ICON_INFORMATION | wx.YES_NO
#						)
#					if cnf.ShowModal() != wx.ID_OK:
#						path = None
#					cnf.Destroy()
			dlg.Destroy()

			if path != None:
				# Determine file type by its extension, defaulting to bufr
				encoding = "BUFR"
				# According to file extension
				if eidx == 0:
					crexmatch = re.compile('.crex$', re.IGNORECASE)
					csvmatch = re.compile('.csv$', re.IGNORECASE)
					if crexmatch.search(path):
						encoding = "CREX"
					elif csvmatch.search(path):
						encoding = "CSV"
				# BUFR optimal template (*.bufr)
				elif eidx == 1:
					encoding = "BUFR"
				# BUFR generic template (*.bufr)
				elif eidx == 2:
					encoding = "gBUFR"
				# CREX (*.crex)
				elif eidx == 3:
					encoding = "CREX"
				# Comma Separated Values (*.csv)
				elif eidx == 4:
					encoding = "CSV"
				elif eidx == 5:
					encoding = "R"
				#elif eidx == 6:
				#	encoding = "VOLND"

				try:
					self.model.exportToFile(str(path), encoding)
				except RuntimeError:
					error = ": ".join(map(str, sys.exc_info()[:2]))
					dlg = wx.MessageDialog(self, "Saving " + path + " failed: " + error,
								'Error saving ' + path,
								wx.OK | wx.ICON_ERROR | wx.ICON_INFORMATION
								)
					dlg.ShowModal()
					dlg.Destroy()
		else:
			event.Skip()


class ProvamiArtProvider(wx.ArtProvider):
	def __init__(self):
		wx.ArtProvider.__init__(self)

	def CreateBitmap(self, artid, client, size):
		bmp = wx.NullBitmap
		if artid == Navigator.ICON_PROVAMI:
			bmp = wx.Bitmap(filePath("icon-provami.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_MAP_WINDOW:
			bmp = wx.Bitmap(filePath("icon-map-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_ANA_WINDOW:
			bmp = wx.Bitmap(filePath("icon-ana-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_ATTR_WINDOW:
			bmp = wx.Bitmap(filePath("icon-attr-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_FILTERS_WINDOW:
			bmp = wx.Bitmap(filePath("icon-filters-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_MOVE:
			bmp = wx.Bitmap(filePath("icon-map-move.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_SELECT_AREA:
			bmp = wx.Bitmap(filePath("icon-map-select-area.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_SELECT_STATION:
			bmp = wx.Bitmap(filePath("icon-map-select-station.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_SELECT_IDENT:
			bmp = wx.Bitmap(filePath("icon-map-select-ident.png"), wx.BITMAP_TYPE_PNG)


		return bmp


#def decorate_treeview(title, view, controls):
#	scrolled = gtk.ScrolledWindow()
#	scrolled.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
#	scrolled.add(view)
#	vbox = gtk.VBox()
#	vbox.pack_start(scrolled)
#	if controls:
#		vbox.pack_start(controls, False, False)
#	frame = gtk.Frame(title)
#	frame.add(vbox)
#	return frame
#
#class AnaNav(gobject.GObject):
#	__gsignals__ = {
#		'selected': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
#                            (gobject.TYPE_INT,))
#	}
#
#	def delete_event(self, widget, data=None):
#		gtk.main_quit()
#		return False
#
#	def __init__(self, model):
#		gobject.GObject.__init__(self)
#		self.model = model
#
#		# Create the main window
#		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
#		self.window.set_title("DB-ALLe Navigator -- Pseudoana")
#		self.window.set_size_request(300, 200)
#		self.window.connect("delete_event", self.delete_event)
#
#		vbox = gtk.VBox()
#		self.window.add(vbox)
#
#		# Create the ana TreeView
#		self.anaview = gtk.TreeView(self.model.anaList)
#		frenderer = gtk.CellRendererText()
#		frenderer.set_property("xalign", 1.0)
#		trenderer = gtk.CellRendererText()
#		self.anaview.append_column(gtk.TreeViewColumn("Lat", frenderer, text=1))
#		self.anaview.append_column(gtk.TreeViewColumn("Lon", frenderer, text=2))
#		self.anaview.append_column(gtk.TreeViewColumn("Name", trenderer, text=3))
#		for i in range(1, 4):
#			self.anaview.get_column(i-1).set_sort_column_id(i)
#		selection = self.anaview.get_selection()
#		def emit_selected(selection):
#			(model, iter) = selection.get_selected()
#			self.emit('selected', model.get_value(iter, 0))
#		selection.connect("changed", emit_selected)
#
#		# Create the query controls
#		hbox = gtk.HBox()
#		self.resultCount = gtk.Label()
#		hbox.pack_start(self.resultCount, False, False)
#		hbox.pack_start(gtk.Label(), True, True)
#		hbox.pack_start(gtk.Label("Result limit:"), False, False)
#		hbox.pack_start(gtk.SpinButton(self.model.anaMax), False, False)
#
#		vbox.pack_start(decorate_treeview("Pseudoana", self.anaview, hbox))
#
#		# Add and display all
#		self.window.show_all()
#
#	def setResultCount(self, count, truncated):
#		if truncated:
#			self.resultCount.set_label(str(count) + "+ results")
#		else:
#			self.resultCount.set_label(str(count) + " results")
#
#class DataNav(gobject.GObject):
#	__gsignals__ = {
#		'selected': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
#                            (gobject.TYPE_INT, gobject.TYPE_STRING))
#	}
#
#	def delete_event(self, widget, data=None):
#		self.window.props.visible = False
#		return True
#
#	def __init__(self, model):
#		gobject.GObject.__init__(self)
#		self.model = model
#
#		# Create the main window
#		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
#		self.window.set_title("DB-ALLe Pseudoana Navigator -- Data")
#		self.window.set_size_request(300, 200)
#		self.window.connect("delete_event", self.delete_event)
#	
#		# Create the data TreeView
#		self.dataview = gtk.TreeView(self.model.dataList)
#		frenderer = gtk.CellRendererText()
#		frenderer.set_property("xalign", 1.0)
#		trenderer = gtk.CellRendererText()
#		self.dataview.append_column(gtk.TreeViewColumn("Type", trenderer, text=1))
#		self.dataview.append_column(gtk.TreeViewColumn("Value", frenderer, text=2))
#		self.dataview.append_column(gtk.TreeViewColumn("Unit", trenderer, text=3))
#		self.dataview.append_column(gtk.TreeViewColumn("Description", trenderer, text=4))
#		self.dataview.append_column(gtk.TreeViewColumn("Date", trenderer, text=5))
#		self.dataview.append_column(gtk.TreeViewColumn("Level", trenderer, text=6))
#		self.dataview.append_column(gtk.TreeViewColumn("Time range", trenderer, text=7))
#		for i in range(1, 8):
#			self.dataview.get_column(i-1).set_sort_column_id(i)
#		selection = self.dataview.get_selection()
#		def emit_selected(selection):
#			(model, iter) = selection.get_selected()
#			self.emit('selected', model.get_value(iter, 0), model.get_value(iter, 1))
#		selection.connect("changed", emit_selected)
#
#		# Create the query controls
#		hbox = gtk.HBox()
#		self.resultCount = gtk.Label()
#		hbox.pack_start(self.resultCount, False, False)
#		hbox.pack_start(gtk.Label(), True, True)
#		hbox.pack_start(gtk.Label("Result limit:"), False, False)
#		hbox.pack_start(gtk.SpinButton(self.model.dataMax), False, False)
#
#		self.window.add(decorate_treeview("Data", self.dataview, hbox))
#		self.window.show_all()
#
#	def setResultCount(self, count, truncated):
#		if truncated:
#			self.resultCount.set_label(str(count) + "+ results")
#		else:
#			self.resultCount.set_label(str(count) + " results")
#
#	def activate(self):
#		self.window.props.visible = True
#
#class AttrNav:
#	def delete_event(self, widget, data=None):
#		self.window.props.visible = False
#		return True
#
#	def __init__(self, model):
#		self.model = model
#
#		# Create the main window
#		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
#		self.window.set_title("DB-ALLe Pseudoana Navigator -- Attributes")
#		self.window.set_size_request(300, 200)
#		self.window.connect("delete_event", self.delete_event)
#
#		# Create the data TreeView
#		self.dataview = gtk.TreeView(self.model.attrList)
#		frenderer = gtk.CellRendererText()
#		frenderer.set_property("xalign", 1.0)
#		trenderer = gtk.CellRendererText()
#		self.dataview.append_column(gtk.TreeViewColumn("Type", trenderer, text=0))
#		self.dataview.append_column(gtk.TreeViewColumn("Value", frenderer, text=1))
#		self.dataview.append_column(gtk.TreeViewColumn("Unit", trenderer, text=2))
#		self.dataview.append_column(gtk.TreeViewColumn("Description", trenderer, text=3))
#		for i in range(1, 4):
#			self.dataview.get_column(i-1).set_sort_column_id(i)
#		self.window.add(decorate_treeview("Data", self.dataview, False))
#		self.window.show_all()
#
#	def activate(self):
#		self.window.props.visible = True
#
#
####
#### Controller
####
#
#class Controller:
#	def __init__(self, model):
#		self.model = model
#		self.model.anaMax.connect("value-changed", lambda x: self.refreshAna())
#		self.model.dataMax.connect("value-changed", lambda x: self.refreshData())
#		self.anaNav = AnaNav(self.model)
#		self.anaNav.connect("selected", lambda x,id: self.selectAna(id))
#		self.dataNav = False
#		self.attrNav = False
#		self.refreshAna()
#
#	# Set the current station to the one with the given ID
#	def selectAna(self, id):
#		# Refetch the data for this station
#		self.curPseudoanaID = id
#
#		# Display the data navigator
#		if self.dataNav == False:
#			self.dataNav = DataNav(self.model)
#			self.dataNav.connect("selected", lambda x,cid,vc: self.selectData(cid, vc))
#		else:
#			self.dataNav.activate()
#
#		# Fill in the data navigator
#		self.refreshData()
#
#	# Set the current datum to the one with the given context ID and vacode
#	def selectData(self, context_id, varcode):
#		self.curContextID = context_id
#		self.curVarcode = varcode
#		self.refreshAttr()
#
#		if self.attrNav == False:
#			self.attrNav = AttrNav(self.model)
#		else:
#			self.attrNav.activate()
#
#
#	def refreshAna(self):
#		self.model.refillAna()
#		self.anaNav.setResultCount(self.model.anaCount, self.model.anaTruncated)
#
#	def refreshData(self):
#		self.model.refillData(self.curPseudoanaID)
#		if self.dataNav:
#			self.dataNav.setResultCount(self.model.dataCount, self.model.dataTruncated)
#
#	def refreshAttr(self):
#		self.model.refillAttr(self.curContextID, self.curVarcode)
#		
#

###
### Main
###

class Parser(OptionParser):
    def __init__(self, *args, **kwargs):
        OptionParser.__init__(self, *args, **kwargs)

    def error(self, msg):
        sys.stderr.write("%s: error: %s\n\n" % (self.get_prog_name(), msg))
        self.print_help(sys.stderr)
        sys.exit(2)

if __name__ == "__main__":
	parser = Parser(usage="usage: %prog [options]",
			version="%prog "+ VERSION,
			description="navigate a DB-ALLe database")
	parser.add_option("--dsn", default="test", help="DSN to use to connect to the database (default: %default)")
	parser.add_option("--user", default=os.environ['USER'], help="User name to use to connect to the database (default: %default)")
	parser.add_option("--pass", default="", dest="password", help="Password to use to connect to the database (default: none)")

	(options, args) = parser.parse_args()

	app = wx.PySimpleApp()
        model = Model(options.dsn, options.user, options.password)

	for q in args:
		model.filter.setFromString(q)

	wx.ArtProvider.PushProvider(ProvamiArtProvider())

	navigator = Navigator(None, model, "Provami - DB-ALLe Navigator")
	navigator.Show()
	model.update()
	app.MainLoop()
